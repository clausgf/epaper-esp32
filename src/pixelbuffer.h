/**
 * ESP32 E-Paper display firmware
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 */

#ifndef PIXELBUFFER_H
#define PIXELBUFFER_H

#include <Adafruit_GFX.h>

#include "logger.h"


class PixelBuffer: Adafruit_GFX
{
public:
    PixelBuffer(int width, int height, int bitPerPixel,
        const Logger& parentLogger = rootLogger);
    virtual ~PixelBuffer();

    const uint8_t* getBufPtr() const { return _bufPtr; }
    bool writePngChannelToBuffer(std::tuple<uint8_t, uint8_t, uint8_t> colors);
    bool prepareBufFromPng(unsigned char *pngImagePtr, size_t pngImageSize);
    void deleteBuf();

    void drawPixel(int16_t x, int16_t y, uint16_t color);
    void drawBattery(int x, int y, int voltage_mV, int percentage);
    void drawWiFi(int x, int y, int rssi);

private:
    int _width;
    int _height;
    int _bitPerPixel;
    Logger _logger;

    unsigned char *_pngImagePtr;
    size_t _pngImageSize;
    uint8_t* _bufPtr;
    size_t _bufSize;

    const int BLACK = 0;
    const int WHITE = 1;
};

#endif
