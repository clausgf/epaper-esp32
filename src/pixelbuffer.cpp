/**
 * ESP32 E-Paper display firmware
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 */

#include <Arduino.h>
#include <esp_log.h>
#include <esp_heap_caps.h>

#include "lodepng.h"
#include "epd.h"

#include "pixelbuffer.h"


PixelBuffer::PixelBuffer(int width, int height, int bitPerPixel, const Logger& parentLogger): 
    Adafruit_GFX(width, height), 
    _width(width), 
    _height(height), 
    _bitPerPixel(bitPerPixel),
    _logger("pixelbuffer", parentLogger)
{
    _bufPtr = nullptr;
}

PixelBuffer::~PixelBuffer()
{
    deleteBuf();
}


// ***** Buffer Management ***************************************************

bool PixelBuffer::createBufFromPng(unsigned char *pngImagePtr, size_t pngImageSize)
{
    _logger.info("Decoding image (%d bytes)", pngImageSize);
    _logger.info("Memory report: largest %d B, total %d B free memory",
       heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
       heap_caps_get_free_size(MALLOC_CAP_8BIT) );
    _logger.error("Image size %dx%d @ %d bpp, %d B overall", 
        _width, _height, _bitPerPixel, _width*_height*_bitPerPixel / 8);

    // prepare decoding options which are called state in lodepng
    LodePNGState state;
    lodepng_state_init(&state);
    state.info_raw.colortype = LCT_PALETTE;
    state.info_raw.bitdepth = _bitPerPixel;
    state.decoder.color_convert = 0;

    unsigned int png_width = 0;
    unsigned int png_height = 0;
    int error = lodepng_decode(&_bufPtr, &png_width, &png_height, &state, pngImagePtr, pngImageSize);
    lodepng_state_cleanup(&state);

    // sanity checks
    if (error != 0)
    {
        _logger.error("Error %u while decoding PNG: %s", error, lodepng_error_text(error));
        deleteBuf();
        return false;
    }
    if (png_width != _width || png_height != _height)
    {
        _logger.error("PixelBuffer size is %dx%d does not match image size %dx%d", _width, _height, png_width, png_height);
        deleteBuf();
        return false;
    }
    _logger.info("PNG decoding ok!");
    _logger.info("Memory report: largest %d B, total %d B free memory",
       heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
       heap_caps_get_free_size(MALLOC_CAP_8BIT) );
    return true;
}

void PixelBuffer::deleteBuf()
{
    if (_bufPtr != nullptr)
    {
        free(_bufPtr);  // yes, the lodepng memory management is c style
        _bufPtr = nullptr;
    }
}

// ***** Drawing *************************************************************

void PixelBuffer::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (_bufPtr)
  {
    if ((x < 0) || (y < 0) || (x >= _width) || (y >= _height))
      return;

    int16_t t;
    switch (rotation) {
    case 1:
      t = x;
      x = _width - 1 - y;
      y = t;
      break;
    case 2:
      x = _width - 1 - x;
      y = _height - 1 - y;
      break;
    case 3:
      t = x;
      x = y;
      y = _height - 1 - t;
      break;
    }

    uint8_t *ptr = &_bufPtr[(x / 8) + y * ((WIDTH + 7) / 8)];
    if (color)
      *ptr |= 0x80 >> (x & 7);
    else
      *ptr &= ~(0x80 >> (x & 7));
  }
}

/**
 * Draws a battery symbol filled according to the percentage.
 * Size: 22x12
 */
void PixelBuffer::drawBattery(int x, int y, int voltage_mV, int percentage)
{
    // check invariants
    if (_bufPtr == nullptr) {
        _logger.error("_bufPtr not set. Call PixelBuffer::createBufFromPng first.");
        return;
    }

    drawRect(x, y, 20, 12, BLACK);
    fillRect(x + 20, y + 2, 2, 7, BLACK);
    fillRect(x + 2, y + 2, 16 * percentage / 100.0, 8, BLACK);

    //setTextColor(BLACK);
    //setTextSize(1);
    //setCursor(x+25, y); printf("%d", voltage_mV);
    //setCursor(x+25, y+8); printf("%d%%", percentage);
}

/**
 * Draws a signal strength indicator using up to 5 bars. 
 * Size: 14x12
 */
void PixelBuffer::drawWiFi(int x, int y, int rssi)
{
    // check invariants
    if (_bufPtr == nullptr) {
        _logger.error("_bufPtr not set. Call PixelBuffer::createBufFromPng first.");
        return;
    }

    int strength;
    if (rssi > -55) {
        strength = 5;
    } else if (rssi < -55 && rssi > -65) {
        strength = 4;
    } else if (rssi < -65 && rssi > -70) {
        strength = 3;
    } else if (rssi < -70 && rssi > -78) {
        strength = 2;
    } else if (rssi < -78 && rssi > -82) {
        strength = 1;
    } else {
        strength = 0;
    }

    for (int bar = 1; bar <= 5; bar++) {
        fillRect(x + (bar-1)*3, y+10, 2, 2, BLACK);
        if (bar <= strength) {
            fillRect(x + (bar-1)*3, y + 10 - 2*bar, 2, 2*bar, BLACK); 
        }
    }

    //setTextColor(BLACK);
    //setTextSize(1);
    //setCursor(x, y);
    //printf("%d", rssi);
}
