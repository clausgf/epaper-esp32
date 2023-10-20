/**
 * ESP32 E-Paper display firmware
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 */

#include <Arduino.h>
#include <SPI.h>

#include "PanelInterface.h"


// ***************************************************************************

void PanelInterface::init()
{
    pinMode(_busy_pin, INPUT);
    pinMode(_cs_pin, OUTPUT);
    pinMode(_dc_pin, OUTPUT);
    pinMode(_rst_pin, OUTPUT);

    digitalWrite(_cs_pin, HIGH);
    digitalWrite(_dc_pin, HIGH);
    digitalWrite(_rst_pin, HIGH);
    SPI.end();
    SPI.begin(_sck_pin, /*unused*/_miso_pin, _mosi_pin, _cs_pin);
};

void PanelInterface::reset(uint32_t before_reset_ms, uint32_t reset_duration_ms, uint32_t after_reset_ms)
{
    digitalWrite(_rst_pin, HIGH);
    delay(before_reset_ms);
    digitalWrite(_rst_pin, LOW);
    delay(reset_duration_ms);
    digitalWrite(_rst_pin, HIGH);
    delay(after_reset_ms);
}

// ***************************************************************************

bool PanelInterface::waitUntilNotBusy(int busy_level, uint32_t timeout_ms)
{
    delay(1); // add some margin to become active
    unsigned long start_time_ms = micros();
    while (digitalRead(_busy_pin) == busy_level)
    {
        delay(1);
        if ((micros() - start_time_ms) > timeout_ms)
        {
            ESP_LOGE(__FILE__, "%s(%d): Busy Timeout after %ld ms (timeout %ld ms)", __FILE__, __LINE__, micros()-start_time_ms, timeout_ms);
            return false;
        }
    }
    ESP_LOGW(__FILE__, "%s(%d): Display was busy for %ld ms", __FILE__, __LINE__, micros()-start_time_ms);
    return true;
}

// ***************************************************************************

void PanelInterface::writeCommand(uint8_t command)
{
    SPI.beginTransaction(_spi_settings);
    digitalWrite(_dc_pin, LOW);
    digitalWrite(_cs_pin, LOW);
    SPI.transfer(command);
    digitalWrite(_cs_pin, HIGH);
    digitalWrite(_dc_pin, HIGH);
    SPI.endTransaction();
}

// ***************************************************************************

void PanelInterface::writeData(uint8_t data, int repetitions)
{
    SPI.beginTransaction(_spi_settings);
    digitalWrite(_cs_pin, LOW);
    for (int i=0; i<repetitions; i++)
    {
        SPI.transfer(data);
    }
    digitalWrite(_cs_pin, HIGH);
    SPI.endTransaction();
}

void PanelInterface::writeData(const uint8_t* data, uint16_t numBytes)
{
    SPI.beginTransaction(_spi_settings);
    digitalWrite(_cs_pin, LOW);
    for (uint16_t i = 0; i < numBytes; i++)
    {
        SPI.transfer(*data++);
    }
    digitalWrite(_cs_pin, HIGH);
    SPI.endTransaction();
}

// ***************************************************************************

void PanelInterface::startDataTransfer()
{
    SPI.beginTransaction(_spi_settings);
    digitalWrite(_cs_pin, LOW);
}

void PanelInterface::transferData(uint8_t value)
{
    SPI.transfer(value);
}

void PanelInterface::transferData(const uint8_t* data, uint16_t numBytes)
{
    for (uint16_t i = 0; i < numBytes; i++)
    {
        SPI.transfer(*data++);
    }
}

void PanelInterface::endDataTransfer()
{
    digitalWrite(_cs_pin, HIGH);
    SPI.endTransaction();
}

// ***************************************************************************
