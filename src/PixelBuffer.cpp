/**
 * ESP32 E-Paper display firmware
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 */

#include <Arduino.h>
#include <esp_log.h>
#include <esp_heap_caps.h>

#include "lodepng.h"

#include "PixelBuffer.h"


PixelBuffer::PixelBuffer(int width, int height, int bitPerPixel, const Logger& parentLogger): 
    Adafruit_GFX(width, height), 
    _width(width), 
    _height(height), 
    _bitPerPixel(bitPerPixel),
    _logger(__FILE__, parentLogger)
{
    _pngImagePtr = nullptr;
    _pngImageSize = 0;
    _bufPtr = nullptr;
    _bufSize = 0;
}

PixelBuffer::~PixelBuffer()
{
    deleteBuf();
}


// ***** Buffer Management ***************************************************

#include "pngle.h"
static PixelBuffer* pixelBufferPtr;
uint8_t selected_r;
uint8_t selected_g;
uint8_t selected_b;

uint32_t pixels_set;
uint32_t pixels_unset;


void on_draw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
{
    uint8_t r = rgba[0]; // 0 - 255
    uint8_t g = rgba[1]; // 0 - 255
    uint8_t b = rgba[2]; // 0 - 255
    //uint8_t a = rgba[3]; // 0: fully transparent, 255: fully opaque

    if (r == selected_r && g == selected_g && b == selected_b)
    {
        pixelBufferPtr->drawPixel(x, y, 1);
        pixels_set++;
    } else {
        pixelBufferPtr->drawPixel(x, y, 0);
        pixels_unset++;
    }
}

bool PixelBuffer::writePngChannelToBuffer(std::tuple<uint8_t, uint8_t, uint8_t> color)
{
    // check invariants
    if (_pngImagePtr == nullptr) {
        _logger.error("_pngImagePtr not set.");
        return false;
    }

    // configure on_draw
    selected_r = std::get<0>(color);
    selected_g = std::get<1>(color);
    selected_b = std::get<2>(color);
    _logger.info("Decode PNG channel r=%d g=%d b=%d", selected_r, selected_g, selected_b);

    // create a white background
    memset(_bufPtr, 0, _bufSize);
    pixels_set = 0;
    pixels_unset = 0;

    // setup pngle to draw the channel
    pngle_t *pngle = pngle_new();

    pixelBufferPtr = this;
    pngle_set_draw_callback(pngle, on_draw);

    // Feed data to pngle
    int processed_bytes = 0;
    while (processed_bytes < _pngImageSize)
    {
        int fed = pngle_feed(pngle, &_pngImagePtr[processed_bytes], _pngImageSize - processed_bytes);
        if (fed < 0)
        {
            _logger.error("pngle error %s", pngle_error(pngle));
        }
        processed_bytes += fed;
        _logger.info("fed=%d processed =%d", fed, processed_bytes);
    }

    pngle_destroy(pngle);

    _logger.info("PNG decoding ok - set=%d unset=%d!", pixels_set, pixels_unset);
    return true;
}

bool PixelBuffer::prepareBufForPng(unsigned char *pngImagePtr, size_t pngImageSize)
{
    _logger.info("Decoding image (%d bytes)", pngImageSize);
    _logger.debug("Image size %dx%d @ %d bpp, %d B overall", 
        _width, _height, _bitPerPixel, _width*_height*_bitPerPixel / 8);

    _pngImagePtr  = pngImagePtr;
    _pngImageSize = pngImageSize;

    // allocate memory for channel
    _logger.info("Memory report: largest %d B, total %d B free memory",
       heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
       heap_caps_get_free_size(MALLOC_CAP_8BIT) );
    _bufSize = _height * ( ( _width + 7 ) / 8 );
    _bufPtr = (uint8_t *) malloc(_bufSize);

    /*
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
        _logger.info("info_png: compression_method=%u filter_method=%u interlace_method=%u bitdepth=%u colortype=%u palettesize=%u", 
            state.info_png.compression_method, state.info_png.filter_method, state.info_png.interlace_method,
            state.info_png.color.bitdepth, state.info_png.color.colortype, state.info_png.color.palettesize);
        deleteBuf();
        return false;
    }
    if (png_width != _width || png_height != _height)
    {
        _logger.error("PixelBuffer size is %dx%d does not match image size %dx%d", _width, _height, png_width, png_height);
        deleteBuf();
        return false;
    }
    */

    return true;
}

void PixelBuffer::deleteBuf()
{
    if (_bufPtr != nullptr)
    {
        free(_bufPtr);  // yes, the lodepng memory management is c style
        _bufPtr = nullptr;
        _bufSize = 0;
    }
    if (_pngImagePtr != nullptr)
    {
        //free(_pngImagePtr);  // yes, the lodepng memory management is c style
        _pngImagePtr = nullptr;
        _pngImageSize = 0;
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
void PixelBuffer::drawBattery(int16_t x, int16_t y, uint16_t color, int voltage_mV, int percentage)
{
    // check invariants
    if (_bufPtr == nullptr) {
        _logger.error("_bufPtr not set. Call PixelBuffer::createBufFromPng first.");
        return;
    }

    drawRect(x, y, 20, 12, color);
    fillRect(x + 20, y + 2, 2, 7, color);
    fillRect(x + 2, y + 2, 16 * percentage / 100.0, 8, color);

    //setTextColor(BLACK);
    //setTextSize(1);
    //setCursor(x+25, y); printf("%d", voltage_mV);
    //setCursor(x+25, y+8); printf("%d%%", percentage);
}

/**
 * Draws a signal strength indicator using up to 5 bars. 
 * Size: 14x12
 */
void PixelBuffer::drawWiFi(int16_t x, int16_t y, uint16_t color, int rssi)
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
        fillRect(x + (bar-1)*3, y+10, 2, 2, color);
        if (bar <= strength) {
            fillRect(x + (bar-1)*3, y + 10 - 2*bar, 2, 2*bar, color); 
        }
    }

    //setTextColor(BLACK);
    //setTextSize(1);
    //setCursor(x, y);
    //printf("%d", rssi);
}
