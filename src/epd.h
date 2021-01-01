/**
 * ESP32 E-Paper display firmware
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 */

#ifndef EPD_H
#define EPD_H

#include "GxEPD2_EPD.h"
#include "lodepng.h"

#include "logging.h"


class EPD {
public:

    EPD(GxEPD2_EPD *rawDisplay, int spiSck, int spiMiso, int spiMosi, int spiCs) {
        _rawDisplay = rawDisplay;
        _spiSck = spiSck;
        _spiMiso = spiMiso;
        _spiMosi = spiMosi;
        _spiCs = spiCs; 
        _bufPtr = nullptr;
    }

    void start() {
        l.info("EPD starting");
        _rawDisplay->init(0);
        SPI.end();
        SPI.begin(_spiSck, _spiMiso /*not used*/, _spiMosi, _spiCs);
    }

    void stop() {
        _rawDisplay->hibernate();
        SPI.end();
        l.info("EPD stopped: display hibernating");
    }

    bool createBufferFromPng(unsigned char *pngImage, size_t pngImageSize) {
        l.info("EPD createBufferFromPng: decoding image (%d bytes)", pngImageSize);

        // prepare decoding options which are called state in lodepng
        LodePNGState state;
        lodepng_state_init(&state);
        state.info_raw.colortype = LCT_PALETTE;
        state.info_raw.bitdepth = 1U; // 2u for two-color
        state.decoder.color_convert = 0;

        unsigned int width = 0;
        unsigned int height = 0;
        int error = lodepng_decode(&_bufPtr, &width, &height, &state, pngImage, pngImageSize);
        lodepng_state_cleanup(&state);

        // sanity checks
        if (error != 0)
        {
            deleteBuffer();
            l.error("EPD error %u while decoding PNG: %s", error, lodepng_error_text(error));
            return false;
        }
        if (width != _rawDisplay->WIDTH || height != _rawDisplay->HEIGHT)
        {
            deleteBuffer();
            l.error("EPD display size is %dx%d does not match image size %dx%d", _rawDisplay->WIDTH, _rawDisplay->HEIGHT, width, height);
            return false;
        }
        l.info("EPD createBufferFromPng: decoding ok!");
        return true;
    }

    void deleteBuffer() {
        if (_bufPtr != nullptr) {
            free(_bufPtr);  // yes, the lodepng memory management is c style
            _bufPtr = nullptr;
        }
    }

    void displayBuffer() {
        l.info("EPD displaying image");
        _rawDisplay->writeImage(_bufPtr, 0, 0, _rawDisplay->WIDTH, _rawDisplay->HEIGHT);
        _rawDisplay->refresh(false);
        l.info("EPD image display complete");
        //if (_rawDisplay->hasFastPartialUpdate)
        //{
        //    _rawDisplay->writeImageAgain(_bufPtr, 0, 0, _rawDisplay->WIDTH, _rawDisplay->HEIGHT);
        //}
        _rawDisplay->powerOff();
    }

    /*
    // from https://github.com/G6EJD/ESP32-e-Paper-Weather-Display
    void drawBattery(int x, int y)
    {
        uint8_t percentage = 100;
        float voltage = analogRead(35) / 4096.0 * 7.46;
        if (voltage > 1)
        { // Only display if there is a valid reading
            Serial.println("Voltage = " + String(voltage));
            percentage = 2836.9625 * pow(voltage, 4) - 43987.4889 * pow(voltage, 3) + 255233.8134 * pow(voltage, 2) - 656689.7123 * voltage + 632041.7303;
            if (voltage >= 4.20)
                percentage = 100;
            if (voltage <= 3.50)
                percentage = 0;
            display.drawRect(x + 15, y - 12, 19, 10, GxEPD_BLACK);
            display.fillRect(x + 34, y - 10, 2, 5, GxEPD_BLACK);
            display.fillRect(x + 17, y - 10, 15 * percentage / 100.0, 6, GxEPD_BLACK);
            drawString(x + 65, y - 11, String(percentage) + "%", RIGHT);
            //drawString(x + 13, y + 5,  String(voltage, 2) + "v", CENTER);
        }
    }
    */

private:
    GxEPD2_EPD * _rawDisplay;
    int _spiSck;
    int _spiMiso;
    int _spiMosi;
    int _spiCs;
    uint8_t * _bufPtr;
};

#endif
