/**
 * ESP32 E-Paper display firmware
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 */

#pragma once

#include <Arduino.h>
#include <GxEPD2_EPD.h>
// specific display drivers for 4.2" 400x300 bw and 7.5" 880x528 red/black/white
#include <epd/GxEPD2_420.h>
#include <epd3c/GxEPD2_750c_Z90.h>

const char* WIFI_SSID = "My WiFi SSID";
const char* WIFI_PASSWORD = "My WiFi Password!";
const String base_url = "http://192.168.178.20:9830/";

const unsigned long default_update_interval_s = 30 * 60;
const unsigned long min_update_interval_s = 30;

const int EPD_SCK = 13;
const int EPD_MISO = 16;
const int EPD_MOSI = 14;
const int EPD_CS = 15;
const int EPD_DC = 27;
const int EPD_RST = 26;
const int EPD_BUSY = 25;
