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
}

// ***************************************************************************

void WifiNetwork::connect()
{
    WiFi.disconnect();
    _logger.debug("Establishing connection to SSID %s", _ssid);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
    WiFi.begin(_ssid, _password);

    WiFi.onEvent( [this] (system_event_id_t event, system_event_info_t info) {
        _logger.debug("Wifi-event %d -> WiFi.status=%d", event, WiFi.status());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        switch (event) {
            case SYSTEM_EVENT_STA_GOT_IP:
            case SYSTEM_EVENT_GOT_IP6:
                _logger.info("Connected, got IP %s", WiFi.localIP().toString().c_str());
                for (auto callback : _onConnectCallbacks) callback();
                break;
            case SYSTEM_EVENT_STA_LOST_IP:
                _logger.info("Lost IP.");
                for (auto callback : _onDisconnectCallbacks) callback();
                break;
        }
#pragma GCC diagnostic pop
    });
}


void WifiNetwork::disconnect()
{
    WiFi.setAutoConnect(false);
    WiFi.setAutoReconnect(false);
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    _logger.info("disconnected & switched off");
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
    while ( millis() < timeout ) {
        wl_status_t status = WiFi.status();
        if (status == WL_CONNECTED) {
            String macAddress = WiFi.macAddress();
            String ipAddress = WiFi.localIP().toString();
            int8_t rssi = WiFi.RSSI();
            _logger.info("WiFi %s MAC=%s IP=%s RSSI=%d", 
                WiFi.isConnected() ? "connected" : "failure", macAddress.c_str(), ipAddress.c_str(), rssi);
            return true;
        }
        delay(20);
    }
    _logger.error("Connection timeout after %lu ms", timeout);
    return false;
}


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

// ***************************************************************************

int WifiNetwork::getRSSI()
{
    return WiFi.RSSI();
}

String WifiNetwork::getId()
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
