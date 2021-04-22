/**
 * Configuration Manager for ESP32
 * Copyright (c) 2021 clausgf@github. See LICENSE.md for legal information.
 */

#pragma once

#include <string>
#include <ArduinoJson.h>
#include <logger.h>

// ***************************************************************************

/**
 * ConfigurationManager
 */
class ConfigurationManager
{
public:
    ConfigurationManager(const char *section = "config", const char *defaultJsonStr = "", 
        const int capacity = 2048,
        const Logger& parentLogger = rootLogger);
    virtual ~ConfigurationManager();
    void initNvs();

    std::string getJsonStr() const;
    bool setConfig(std::string version, std::string json);
    bool resetConfig();
    std::string getVersion() const;

    int getInt(std::string key) const;
    std::string getString(std::string key) const;

    void log() const;

private:
    bool _loadConfig();
    bool _saveConfig();

    const char *_VERSION_KEY = "version";
    const char *_DATA_KEY = "data";

    const char *_section;
    const char *_defaultJsonStr;
    const int _capacity;
    Logger _logger;

    std::string _version;
    DynamicJsonDocument _jsonDoc;
};

// ***************************************************************************

/**
 * The defaultConfig variable must be defined outside this library, 
 * e.g. in main.cpp.
 * 
 * It is defined here to enable all modules in the user application
 * to access the configuration.
 */
extern ConfigurationManager defaultConfig;
