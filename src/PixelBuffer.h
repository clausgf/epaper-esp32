/**
 * ESP32 E-Paper display firmware
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 */

#pragma once

#include <tuple>

#include <Adafruit_GFX.h>

#include "logger.h"


class PixelBuffer: Adafruit_GFX
{
public:
    PixelBuffer(int width, int height, int bitPerPixel,
        const Logger& parentLogger = rootLogger);
    virtual ~PixelBuffer();

    const uint8_t* getBufPtr() const { return _bufPtr; }
    bool writePngChannelToBuffer(std::tuple<uint8_t, uint8_t, uint8_t> color);
    bool prepareBufForPng(unsigned char *pngImagePtr, size_t pngImageSize);
    void deleteBuf();

    virtual void drawPixel(int16_t x, int16_t y, uint16_t color);
    void drawBattery(int16_t x, int16_t y, uint16_t color, int voltage_mV, int percentage);
    void drawWiFi(int16_t x, int16_t y, uint16_t color, int rssi);

private:
    const int _width;
    const int _height;
    const int _bitPerPixel;
    Logger _logger;

    unsigned char *_pngImagePtr;
    size_t _pngImageSize;
    uint8_t* _bufPtr;
    size_t _bufSize;
};
