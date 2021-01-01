/**
 * ESP32 E-Paper display firmware
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 */

#include "logging.h"

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <Ticker.h>

#include <asyncHTTPrequest.h>
#include <ArduinoJson.h>

#include <GxEPD2_EPD.h>
// specific display drivers for 4.2" 400x300 bw and 7.5" 880x528 red/black/white
#include <epd/GxEPD2_420.h>
#include <epd3c/GxEPD2_750c_Z90.h>

#include "epd.h"
#include "settings.h"


// ***** logging *************************************************************
auto logHandler = SerialLogHandler(115200, true);
//auto logHandler = UdpLogHandler("mbp2015", 10000);
auto logFormatter = DefaultLogFormatter("epd");
Logger l = Logger(&logFormatter, &logHandler);

// ***** high-level epd ******************************************************
auto epd = EPD(&rawDisplay, /*SCK*/13, /*MISO*/16 /*not used*/, /*MOSI*/14 /*DIN*/, /*CS*/15);

// ***** Data stored in RTC memory is preserved during deep sleep ************
const int MAX_RTC_HTTP_ETAG_SIZE = 127;
RTC_DATA_ATTR char rtc_http_etag[MAX_RTC_HTTP_ETAG_SIZE] = {0};
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR unsigned long activeDuration_ms = 0;
RTC_DATA_ATTR unsigned long updateInterval_s = 0;
RTC_DATA_ATTR unsigned long sleepDuration_ms = 0;
RTC_DATA_ATTR int imageResponseCode = 0;
RTC_DATA_ATTR int batteryPercentage = 0;

// ***** WiFi management *****************************************************

void connect() {
    WiFi.disconnect();
    l.info("WiFi: establishing connection to SSID %s", wifi_ssid);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
    WiFi.begin(wifi_ssid, wifi_password);
    WiFi.onEvent( [] (WiFiEvent_t event) {
        l.debug("WiFi event %d status %d", event, WiFi.status());
    });
}

void disconnect() {
    WiFi.setAutoConnect(false);
    WiFi.setAutoReconnect(false);
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    l.info("WiFi: disconnected & switched off");
}

bool waitForConnectionOk(unsigned long timeout) {
    unsigned long startTime = millis();
    l.info("WiFi: waiting for connection");
    while ( millis()-startTime < timeout ) {
        wl_status_t status = WiFi.status();
        if (status == WL_CONNECTED) {
            String macAddress = WiFi.macAddress();
            String ipAddress = WiFi.localIP().toString();
            int8_t rssi = WiFi.RSSI();
            l.info("WiFi %s", WiFi.isConnected() ? "connected" : "failure");
            l.info("WiFi MAC address: %s", macAddress.c_str());
            l.info("WiFi IP address: %s", ipAddress.c_str());
            l.info("WiFi RSSI: %d", rssi);

            return true;
        } else if (status == WL_CONNECT_FAILED ) {
            l.error("WiFi: connection failure");
            return false;
        }
        delay(50);
    }
    l.error("WiFi: connection timeout after %d ms", timeout);
    return false;
}

String mac() {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    return mac;
}


// ***** Battery Voltage *****************************************************

class BatteryMonitor {
public:
    BatteryMonitor(int pin, int voltageDividerFactor_perMille = 1000, int noOfMeasurements = 10, 
        int minVoltage_mV = 3400, int maxVoltage_mV = 4200):
        _pin(pin),
        _noOfMeasurements(noOfMeasurements),
        _voltageDividerFactor_perMille(voltageDividerFactor_perMille), 
        _minVoltage_mV(minVoltage_mV), 
        _maxVoltage_mV(maxVoltage_mV) {
    };

    int getVoltage_mV() {
        analogSetWidth(11);  // alldgedly more linear than 12
        analogSetPinAttenuation(_pin, ADC_11db);

        int sum = 0;
        for (int i=0; i<_noOfMeasurements; i++) {
            sum += analogRead(_pin);
        }

        const int adc_counts = sum / _noOfMeasurements;
        const int adc_mV = adc_counts * 3900 / 2047;  // full scale 3.9 V @ 11 dB, 11 bit
        return adc_mV * _voltageDividerFactor_perMille / 1000;
    }

    int getPercentage() {
        int voltage_mV = getVoltage_mV();
        int percentage = -1;
        if (voltage_mV > _maxVoltage_mV)
            percentage = 100;
        else if (voltage_mV < _minVoltage_mV)
            percentage = 0;
        else
            percentage = (voltage_mV - _minVoltage_mV) * 100 / (_maxVoltage_mV - _minVoltage_mV);
        return percentage;
    }

private:
    int _pin;
    int _noOfMeasurements;
    int _voltageDividerFactor_perMille;
    int _minVoltage_mV;
    int _maxVoltage_mV;
};
auto batteryMonitor = BatteryMonitor(36, 500);


// ***** HTTP ETag ***********************************************************

class ETag {
public:

    void set(const String etag) {
        strncpy(rtc_http_etag, etag.c_str(), MAX_RTC_HTTP_ETAG_SIZE);
    }

    String get() {
        return rtc_http_etag;
    }
};
auto etag = ETag();


// ***** HttpClient **********************************************************

class HttpClient {
public:

    HttpClient(): _responseText("") {
    }

    void startRequest(String requestType, String url, String requestBody, String ifNoneMatch = "") {
        _requestType = requestType;
        _url = url;
        if (!WiFi.isConnected()) {
            l.error("HttpClient cannot start, network not connected! %s %s", _requestType.c_str(), _url.c_str());
            return;
        }
        l.info("HttpClient request: curl -i -X %s -H \"If-None-Match: %s\" %s", _requestType.c_str(), ifNoneMatch.c_str(), _url.c_str());

        //_request.setDebug(true);
        _request.onReadyStateChange([this](void *optParam, asyncHTTPrequest *request, int readyState) {
            if (readyState == 4) {
                // store the response
                _responseText = request->responseText();
            }
        });
        _request.open(_requestType.c_str(), _url.c_str());
        if (!ifNoneMatch.isEmpty()) {
            _request.setReqHeader("If-None-Match", ifNoneMatch.c_str());
        }
        _request.send((const uint8_t*)requestBody.c_str(), requestBody.length());
    }

    bool isComplete() {
        return _request.readyState() == 4;
    }

    bool waitForCompletionUntil(unsigned long timeoutTime) {
        unsigned long startTime = millis();
        if ( timeoutTime - startTime < LONG_MAX ) { // wrap-around aware timeoutTime > startTime for unsigned long
            l.debug("HttpClient waiting for request completion");
            while ( timeoutTime - millis() < LONG_MAX ) { // wrap-around aware timeoutTime > millis for unsigned long
                if (isComplete())
                    break;
                delay(50);
            }
        }
        if (isComplete()) {
            l.info("HttpClient request complete after %ld ms", millis()-startTime);
            return true;
        } else {
            l.error("HttpClient timeout after %ld ms", timeoutTime - startTime);
            return false;
        }
    }

    void log() {
        l.info("HttpClient %s %s:", _requestType.c_str(), _url.c_str());
        l.info("HttpClient dumping all %d request headers:", _request.respHeaderCount());
        for (int i=0; i<_request.respHeaderCount(); i++) {
            l.info("ResponseHeader[\"%s\"] = \"%s\"", _request.respHeaderName(i), _request.respHeaderValue(i));
        }
        l.info("HttpClient response_code=%d, length=%d (%d expected)", 
            _request.responseHTTPcode(), _responseText.length(), _request.responseLength());
    }

    bool isResponseLengthOk() {
        return _responseText.length() == _request.responseLength();
    }

    int getResponseCode() { return _request.responseHTTPcode(); }
    String getResponseText() { return _responseText; }

    String getResponseHeader(const char *name) { 
        char *value = _request.respHeaderValue(name);
        return value == nullptr ? "" : value;
    }

    int getResponseMaxAgeSeconds(int defaultMaxAgeSeconds) {
        int ret = defaultMaxAgeSeconds;
        char *value = _request.respHeaderValue("Cache-Control");
        if (value) {
            String dateStr = value;
            if (dateStr.startsWith("max-age=")) {
                int from = dateStr.indexOf("=")+1;
                int maxAge = dateStr.substring(from).toInt();
                ret = maxAge;
            }
        }
        return ret;
    }

    String getResponseDate() {
        char *value = _request.respHeaderValue("Date");
        return value == nullptr ? "" : value;
    }

private:
    String _requestType;
    String _url;
    String _responseText;
    asyncHTTPrequest _request;
};


// ***** Status Reporter *****************************************************

String getStatusAsJson() {
    StaticJsonDocument<1024> json;
    json["id"] = mac().c_str();
    json["bootCount"] = bootCount;
    json["lastDuration_ms"] = activeDuration_ms;
    json["lastSleepDuration_ms"] = sleepDuration_ms;
    json["lastResponseCode"] = imageResponseCode;
    json["lastVersion"] = rtc_http_etag;
    auto battery = json.createNestedObject("battery");
    battery["voltage"] = batteryMonitor.getVoltage_mV() / 1000.0;
    battery["percentage"] = batteryPercentage;
    auto wifi = json.createNestedObject("wifi");
    wifi["ssid"] = wifi_ssid;
    wifi["rssi"] = WiFi.RSSI();
    auto panel = json.createNestedObject("panel");
    panel["width"] = rawDisplay.WIDTH;
    panel["height"] = rawDisplay.HEIGHT;
    panel["color"] = rawDisplay.hasColor;
    panel["panel"] = rawDisplay.panel;
    char _body[1024];
    serializeJson(json, _body, sizeof(_body));
    return _body;
}


// ***** Setup ***************************************************************

void setup() {
    // startup
    unsigned long startTime = millis();
    updateInterval_s = default_update_interval_s;
    bootCount++;

    // check the battery, shutdown on empty battery
    batteryPercentage = batteryMonitor.getPercentage();
    if (batteryPercentage <= 0) {
        // emergency shutdown, battery exhausted
        l.error("Battery exhausted (%d mV, %d %%) - going to sleep indefinetely", 
            batteryMonitor.getVoltage_mV(), batteryPercentage);
        esp_deep_sleep_start();
    }

    // fully wake up
    pinMode(BUILTIN_LED, OUTPUT);
    digitalWrite(BUILTIN_LED, HIGH);
    connect();
    epd.start();
    waitForConnectionOk(5000);

    // send status & update display
    if (WiFi.isConnected()) {
        auto httpStatusReporter = HttpClient();
        httpStatusReporter.startRequest("put", base_url + "api/devices/" + mac(), getStatusAsJson());

        auto httpImageClient = HttpClient();
        httpImageClient.startRequest("get", base_url + "api/renderings/" + display_id + "/image", "", etag.get());
        httpImageClient.waitForCompletionUntil(startTime + 5000);
        updateInterval_s = httpImageClient.getResponseMaxAgeSeconds(updateInterval_s);
        httpImageClient.log();

        imageResponseCode = httpImageClient.getResponseCode();
        if (httpImageClient.isResponseLengthOk() && imageResponseCode == 200) {
            if (httpStatusReporter.isComplete()) {
                disconnect(); // stop networking now to save power
            }
            String png = httpImageClient.getResponseText();
            if (epd.createBufferFromPng((unsigned char*)png.c_str(), png.length())) {
                epd.displayBuffer();
                epd.deleteBuffer();
                etag.set(httpImageClient.getResponseHeader("ETag"));
            }
        } else {
            l.info("HTTP Response code != 200, nothing to display!");
        }
        httpStatusReporter.waitForCompletionUntil(startTime + 5000);
    }

    // shutdown
    disconnect();
    epd.stop();
    sleepDuration_ms = startTime + updateInterval_s * 1000l - millis();
    if (sleepDuration_ms < min_update_interval_s*1000l) {
        sleepDuration_ms = min_update_interval_s*1000l;
    }
    activeDuration_ms = millis() - startTime;
    l.info("System was awake for %.3f s", activeDuration_ms / 1000.0);
    l.info("System entering deep sleep state for %.3f s...", sleepDuration_ms / 1000.0);
    digitalWrite(BUILTIN_LED, LOW);
    esp_sleep_enable_timer_wakeup(sleepDuration_ms * 1000LL);
    esp_deep_sleep_start();
}


void loop() {
}
