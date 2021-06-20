/**
 * ESP32 E-Paper display firmware
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 */

#include <Arduino.h>
#include <WiFi.h>
#include "logger.h"

#include "wifi_network.h"


// ***************************************************************************

WifiNetwork::WifiNetwork(const char *ssid, const char *password, const Logger& parentLogger):
    _ssid(ssid), _password(password), _logger("wifi_network", parentLogger)
{
    WiFi.setAutoConnect(false);
    WiFi.setAutoReconnect(false);
}

// ***************************************************************************

WifiNetwork& WifiNetwork::onConnect(OnConnectCallback callback)
{
    _onConnectCallbacks.push_back(callback);
    return *this;
}

WifiNetwork& WifiNetwork::onDisconnect(OnDisconnectCallback callback)
{
    _onDisconnectCallbacks.push_back(callback);
    return *this;
}

void WifiNetwork::handleEvents()
{
    if (_triggerOnConnect)
    {
        for (auto callback : _onConnectCallbacks) 
            callback();
        _triggerOnConnect = false;
    }

    if (_triggerOnDisconnect)
    {
        for (auto callback : _onDisconnectCallbacks) 
            callback();
        _triggerOnDisconnect = false;
    }
}

// ***************************************************************************

void WifiNetwork::connect()
{
    WiFi.disconnect();
    _logger.debug("Establishing connection to SSID %s", _ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);

    WiFi.onEvent( [this] (system_event_id_t event, system_event_info_t info) 
    {
        _logger.debug("Wifi-event %d -> WiFi.status=%d", event, WiFi.status());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        switch (event) 
        {
            case SYSTEM_EVENT_STA_GOT_IP:
            case SYSTEM_EVENT_GOT_IP6:
                _logger.debug("Connected, got IP %s", WiFi.localIP().toString().c_str());
                _triggerOnConnect = true;
                break;
            case SYSTEM_EVENT_STA_LOST_IP:
                _logger.debug("Lost IP.");
                _triggerOnDisconnect = true;
                break;
        }
#pragma GCC diagnostic pop
    });
}


void WifiNetwork::disconnect()
{
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    _logger.info("Disconnected & switched off");
}

// ***************************************************************************

bool WifiNetwork::isConnected()
{
    wl_status_t status = WiFi.status();
    return (status == WL_CONNECTED);
}


bool WifiNetwork::waitUntilConnected(unsigned long timeout)
{
    _logger.info("Waiting for connection");
    while ( millis() < timeout )
    {
        if ( isConnected() ) 
        {
            String deviceId = getDeviceId();
            String macAddress = getMac();
            String ipAddress = getIpAddress();
            int8_t rssi = WiFi.RSSI();
            _logger.debug("WiFi connected DeviceId=%s MAC=%s IP=%s RSSI=%d", 
                deviceId.c_str(), macAddress.c_str(), ipAddress.c_str(), rssi);
            handleEvents();
            return true;
        }
        handleEvents();
        delay(10);
    }
    _logger.error("Connection timeout after %lu ms", timeout);
    return false;
}

// ***************************************************************************

String WifiNetwork::getDeviceId()
{
#ifdef ESP32
    const int ID_MAXLEN = 20;
    char id_buf[ID_MAXLEN];
    snprintf(id_buf, ID_MAXLEN-1, "e32-%06llx", ESP.getEfuseMac());
    String id = String(id_buf);
#else
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    String id = String("e32-") + mac;
#endif
    return id;
}

String WifiNetwork::getMac()
{
    return WiFi.macAddress();
}

String WifiNetwork::getIpAddress()
{
    return WiFi.localIP().toString();
}

int WifiNetwork::getRSSI()
{
    return WiFi.RSSI();
}
