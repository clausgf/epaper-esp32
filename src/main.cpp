/**
 * ESP32 E-Paper display firmware
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_log.h>

#include <asyncHTTPrequest.h>
#include <ArduinoJson.h>
#include <logger.h>
#include <battery_monitor.h>
#include <configmanager.h>

#include "led.h"
#include "wifi_network.h"
#include "pixelbuffer.h"
#include "epd.h"
#include "settings.h"

#include "IPAddress.h"


// ***************************************************************************

auto logHandler = SerialLogHandler( true, 115200 );
Logger rootLogger = Logger( __FILE__, &logHandler );
Logger::LogLevel LOG_LEVEL = Logger::NOTSET;    // NOTSET  INFO

auto statusLed = LED(
    /*pin*/ 2 );

auto battery = BatteryMonitor(
    /*pin*/ 34,                     /* IO34 = ADC Channel 6 */
    /*voltageFactor_perMille*/ 2000,
    /*int voltageBias_mV*/ 0 );

auto net = WifiNetwork( WIFI_SSID, WIFI_PASSWORD );


// ***** high-level epd ******************************************************
auto epd = EPD(EPD_SCK, EPD_MISO, EPD_MOSI, EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);


// ***** Data stored in RTC memory is preserved during deep sleep ************
const int MAX_RTC_HTTP_ETAG_SIZE = 127;
RTC_DATA_ATTR char rtc_http_etag[MAX_RTC_HTTP_ETAG_SIZE] = {0};
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR unsigned long activeDuration_ms = 0;
RTC_DATA_ATTR unsigned long sleepDuration_ms = 0;
RTC_DATA_ATTR GxEPD2::Panel panelType = defaultPanelType;
RTC_DATA_ATTR int imageResponseCode = 0;

unsigned long updateInterval_s = 0;
unsigned long bootTimestamp;


// ***************************************************************************

void panic()
{
    while (1)
    {
        statusLed.toggle();
        rootLogger.error("user panic");
        delay(1000);
    }
}

// ***** HTTP ETag ***********************************************************

/**
 * Wrapper class for the ETag stored in RTC memory
 */
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

class HttpClient
{
public:

    HttpClient(bool debug=false): _responseText("")
    {
        _request.setDebug(debug);
    }

    void startRequest(String requestType, String url, String requestBody, String ifNoneMatch = "")
    {
        _requestType = requestType;
        _url = url;
        rootLogger.info("HttpClient request start: curl -i -X %s -H \"If-None-Match: %s\" %s", 
            _requestType.c_str(), ifNoneMatch.c_str(), _url.c_str());

        _request.onReadyStateChange([this](void *optParam, asyncHTTPrequest *request, int readyState)
        {
            if (readyState == 4) {
                // store the response
                _responseText = request->responseText();
            }
        });
        if (!_request.open(_requestType.c_str(), _url.c_str()))
        {
            rootLogger.error("HttpClient cannot open connection! %s %s", _requestType.c_str(), _url.c_str());
            return;
        }
        if (!ifNoneMatch.isEmpty())
        {
            _request.setReqHeader("If-None-Match", ifNoneMatch.c_str());
        }
        _request.send((const uint8_t*)requestBody.c_str(), requestBody.length());
    }

    bool isComplete()
    {
        return _request.readyState() == 4;
    }

    bool waitForCompletionUntil(unsigned long timeoutTime)
    {
        if ( timeoutTime - millis() < LONG_MAX ) // wrap-around aware timeoutTime > millis() for unsigned long
        { 
            rootLogger.info("HttpClient waiting for request completion: timeout=%lu", timeoutTime);
            while ( timeoutTime - millis() < LONG_MAX ) // wrap-around aware timeoutTime > millis for unsigned long
            { 
                if (isComplete())
                    break;
                delay(50);
            }
        }
        if (isComplete()) 
        {
            rootLogger.info("%s %s request complete with status=%d len=%d/%d", 
                _requestType.c_str(), _url.c_str(), _request.responseHTTPcode(), _responseText.length(), _request.responseLength());
            return true;
        } else {
            rootLogger.error("%s %s request timeout", _requestType.c_str(), _url.c_str());
            return false;
        }
    }

    void log()
    {
        rootLogger.debug("HttpClient %s %s:", _requestType.c_str(), _url.c_str());
        rootLogger.debug("HttpClient dumping all %d request headers:", _request.respHeaderCount());
        for (int i=0; i<_request.respHeaderCount(); i++) {
            rootLogger.debug("ResponseHeader[\"%s\"] = \"%s\"", _request.respHeaderName(i), _request.respHeaderValue(i));
        }
    }

    bool isResponseLengthOk()
    {
        return _responseText.length() == _request.responseLength();
    }

    int getResponseCode() { return _request.responseHTTPcode(); }
    String& getResponseText() { return _responseText; }

    String getResponseHeader(const char *name)
    {
        char *value = _request.respHeaderValue(name);
        return value == nullptr ? "" : value;
    }

    int getResponseMaxAgeSeconds(int defaultMaxAgeSeconds)
    {
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
        rootLogger.info("Max Age: %d s", ret);
        return ret;
    }

    String getResponseDate()
    {
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
    const String mac = net.getMac();
    const String ip = WiFi.localIP().toString();

    json["id"] = mac.c_str();
    json["boot_count"] = bootCount;
    json["last_duration_ms"] = activeDuration_ms;
    json["last_sleep_duration_ms"] = sleepDuration_ms;
    json["last_response_code"] = imageResponseCode;
    json["last_version"] = rtc_http_etag;
    auto jsonBattery = json.createNestedObject("battery");
    jsonBattery["voltage"] = battery.getVoltage_mV() / 1000.0;
    jsonBattery["percentage"] = battery.getPercentage();
    auto jsonWifi = json.createNestedObject("wifi");
    jsonWifi["ssid"] = WIFI_SSID;
    jsonWifi["rssi"] = net.getRSSI();
    jsonWifi["mac"] = mac.c_str();
    jsonWifi["ip"] = ip.c_str();
    auto jsonPanel = json.createNestedObject("panel");
    jsonPanel["width"] = epd.getPanelPtr()->WIDTH;
    jsonPanel["height"] = epd.getPanelPtr()->HEIGHT;
    jsonPanel["color"] = epd.getPanelPtr()->hasColor;
    jsonPanel["panel"] = epd.getPanelPtr()->panel;
    jsonPanel["panelName"] = epd.getPanelName().c_str();

    char _body[1024];
    serializeJson(json, _body, sizeof(_body));
    return String(_body);
}


void log_memory_usage()
{
    rootLogger.info("--------------------------------------------------------------\n");
    rootLogger.info("Memory report: largest %d B, total %d B free memory",
       heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
       heap_caps_get_free_size(MALLOC_CAP_8BIT) );
    heap_caps_print_heap_info(MALLOC_CAP_8BIT);
    rootLogger.info("--------------------------------------------------------------\n");
}


// ***************************************************************************
//             SETUP
// ***************************************************************************

void setup()
{
    // startup
    bootCount++;
    bootTimestamp = millis();
    rootLogger.setLevel(LOG_LEVEL);
    updateInterval_s = default_update_interval_s;

    // -----------------------------------------------------------------------
    // Check the battery, immediately shutdown on empty battery
    battery.measure();
    if (battery.getPercentage() <= 0)
    {
        // emergency shutdown, battery exhausted
        rootLogger.error("Battery exhausted (%d mV, %d%%)", battery.getVoltage_mV(), battery.getPercentage());
        // TODO shutdown all sensors and anything consuming energy
        #ifdef SLEEP_ON_BATTERY_EXHAUSTED
            esp_deep_sleep_start();
        #endif
    }

    // -----------------------------------------------------------------------
    // Wake up
#ifdef NICE_LOGGING
    delay(500);  // wait for serial interface to get up
#endif
    statusLed.set(true);
    rootLogger.info("Startup #%d  Battery=%d mV/%d%%", bootCount, battery.getVoltage_mV(), battery.getPercentage());

    net.connect();
    if ( net.waitUntilConnected(bootTimestamp + 5000) )
    {
        // set panel (before setting it into the status...)
        epd.setPanel(panelType);
        epd.start();

        // report status
        asyncHTTPrequest statusRequest;
        String statusUrl = base_url + "api/devices/" + net.getId();
        String statusBody = getStatusAsJson();
        //statusRequest.setDebug(true);
        if ( statusRequest.open("POST", statusUrl.c_str()) )
        {
            rootLogger.info("body: %s", statusBody.c_str());
            statusRequest.send((const uint8_t*)statusBody.c_str(), statusBody.length());
        } else {
            rootLogger.error("HttpClient cannot open connection! POST %s", statusUrl.c_str());
        }

        // get new image
        auto httpImageClient = HttpClient(/*debug*/ true);
        httpImageClient.startRequest("GET", base_url + "api/displays/" + net.getId() + "/image", "", etag.get());

        // Wait for image data...
        httpImageClient.waitForCompletionUntil(bootTimestamp + 5000);
        updateInterval_s = httpImageClient.getResponseMaxAgeSeconds(updateInterval_s);
        httpImageClient.log();

        // Display the image
        if ( httpImageClient.isResponseLengthOk() && httpImageClient.getResponseCode() == 200 )
        {
            // shut down networking if not longer needed - might destrpy response code
            if ( statusRequest.readyState() == 4 )
            {
                net.disconnect(); // stop networking now to save power
            }

            String png = httpImageClient.getResponseText();
            auto pb = PixelBuffer(epd.getPanelPtr()->WIDTH, epd.getPanelPtr()->HEIGHT, 1);
            if (pb.createBufFromPng((unsigned char*)httpImageClient.getResponseText().c_str(), png.length()))
            {
                pb.drawBattery(epd.getPanelPtr()->WIDTH - 22 - 5, 5, battery.getVoltage_mV(), battery.getPercentage());
                pb.drawWiFi(epd.getPanelPtr()->WIDTH - 22 - 5 - 14 - 5, 5, net.getRSSI());
                epd.displayPixelBuffer(pb.getBufPtr());
                etag.set(httpImageClient.getResponseHeader("ETag"));
            }
        } else {
            rootLogger.debug("HTTP Response code %d != 200, nothing to display!", httpImageClient.getResponseCode());
        }
        // TODO httpStatusReporter.waitForCompletionUntil(bootTimestamp + 5000);
    } else {
        // no network connection
    }

    // shutdown
    net.disconnect();
    epd.stop();
    sleepDuration_ms = bootTimestamp + updateInterval_s * 1000l - millis();
    if (sleepDuration_ms < min_update_interval_s*1000l)
    {
        sleepDuration_ms = min_update_interval_s*1000l;
    }
    activeDuration_ms = millis() - bootTimestamp;
    rootLogger.info("System was awake for %.3f s", activeDuration_ms / 1000.0);
    rootLogger.info("System entering deep sleep state for %.3f s...", sleepDuration_ms / 1000.0);
    digitalWrite(BUILTIN_LED, LOW);
    esp_sleep_enable_timer_wakeup(sleepDuration_ms * 1000LL);
    esp_deep_sleep_start();
}

// ***************************************************************************

void loop()
{
}
