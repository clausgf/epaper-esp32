/**
 * ESP32 E-Paper display firmware
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 */

#pragma once

#include <functional>
#include <vector>
#include <Arduino.h>

#include "logger.h"


class WifiNetwork 
{
public:
    typedef std::function<void()> OnConnectCallback;
    typedef std::function<void()> OnDisconnectCallback;

    WifiNetwork(const char *ssid, const char *password, const Logger& parentLogger = rootLogger);
    void connect();
    void disconnect();
    bool isConnected();
    bool waitUntilConnected(unsigned long timeout);
    WifiNetwork& onConnect(OnConnectCallback callback);
    WifiNetwork& onDisconnect(OnDisconnectCallback callback);

    int getRSSI();
    String getId();
    String getMac();
    String getIpAddress();

private:
    const char *_ssid;
    const char *_password;
    Logger _logger;
    std::vector<OnConnectCallback> _onConnectCallbacks;
    std::vector<OnDisconnectCallback> _onDisconnectCallbacks;
};
