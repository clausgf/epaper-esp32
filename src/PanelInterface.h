/**
 * ESP32 E-Paper display firmware
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 */

#pragma once

#include <Arduino.h>
#include <SPI.h>

// ***************************************************************************

class PanelInterface
{
public:
    PanelInterface(int sck_pin, int miso_pin, int mosi_pin, int cs_pin, 
        int dc_pin, int rst_pin, int busy_pin):
        _sck_pin(sck_pin), _miso_pin(miso_pin), _mosi_pin(mosi_pin), _cs_pin(cs_pin), 
        _dc_pin(dc_pin), _rst_pin(rst_pin), _busy_pin(busy_pin), 
        _spi_settings(4000000, MSBFIRST, SPI_MODE0)
    {}

    void init();
    void reset(uint32_t before_reset_ms, uint32_t reset_duration_ms, uint32_t after_reset_ms);

    bool waitUntilNotBusy(int busy_level, uint32_t timeout_ms);

    void writeCommand(uint8_t command);
    void writeData(uint8_t data, int repetitions = 1);
    void writeData(const uint8_t* data, uint16_t numBytes);

    void startDataTransfer();
    void transferData(uint8_t value);
    void transferData(const uint8_t* data, uint16_t numBytes);
    void endDataTransfer();

private:
    int _sck_pin, _miso_pin, _mosi_pin, _cs_pin, _dc_pin, _rst_pin, _busy_pin;
    SPISettings _spi_settings;
};

// ***************************************************************************
