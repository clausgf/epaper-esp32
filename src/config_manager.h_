/**
 * Configuration Manager for ESP32
 * Copyright (c) 2021 clausgf@github. See LICENSE.md for legal information.
 */

#pragma once

#include <string>

#include <ArduinoJson.h>
#include <logger.h>


//#define USE_NVS // NVS usage is incomplete and untested!


// ***************************************************************************

/**
 * ConfigManager
 */
class ConfigManager
{
public:
    ConfigManager(
        const int capacity = 2048,
        const Logger& parentLogger = rootLogger);
    virtual ~ConfigManager();

    std::string getJsonStr() const;
    bool setConfig(const char *jsonBuf);

    int getInt(std::string key, int defaultValue = 0) const;
    std::string getString(std::string key, const std::string defaultValue = "") const;

    void log() const;

private:
    const int _capacity;
    Logger _logger;

    DynamicJsonDocument _jsonDoc;
};

// ***************************************************************************

/**
 * The configManager variable must be defined outside this library, 
 * e.g. in main.cpp.
 * 
 * It is defined here to enable all modules in the user application
 * to access the configuration.
 */
extern ConfigManager configManager;
