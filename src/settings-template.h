/**
 * ESP32 E-Paper display firmware
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include <GxEPD2_EPD.h>
// specific display drivers for 4.2" 400x300 bw and 7.5" 880x528 red/black/white
#include <epd/GxEPD2_420.h>
#include <epd3c/GxEPD2_750c_Z90.h>

const char* WIFI_SSID = "My WiFi SSID";
const char* WIFI_PASSWORD = "My WiFi Password!";
const String base_url = "http://192.168.100.1:8080/";
const String display_id =  "Display ID (or alias) as configured on the server, e.g. epaper_43bw or hallway";

const unsigned long default_update_interval_s = 10 * 60;
const unsigned long min_update_interval_s = 30;

// raw display panel connected to Waveshare ESP32 e-Paper Driver Board
auto rawDisplay = GxEPD2_420(/*CS*/ 15, /*DC*/ 27, /*RST*/ 26, /*BUSY*/ 25);

#endif
