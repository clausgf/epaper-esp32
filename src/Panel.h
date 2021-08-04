/**
 * ESP32 E-Paper display firmware
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 */

#pragma once

#include <tuple>
#include <vector>

#include "PanelInterface.h"

// ***************************************************************************

class Panel
{
public:
    typedef std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> RgbColors;
    Panel(): pIf(nullptr) {};

    virtual const char *getName() const = 0;
    virtual const int getChannels() const = 0;
    virtual const int getBitsPerChannel() const = 0;
    virtual const int getWidth() const = 0;
    virtual const int getHeight() const = 0;

    virtual const RgbColors& getChannelRgbColors() const = 0;
    virtual const int getDefaultColor(int channel) const = 0;

    virtual void init(PanelInterface *pIf) = 0;
    virtual void deep_sleep() = 0;

    virtual void writeChannel(int channel, const uint8_t *data) = 0;
    virtual void display() = 0;

protected:
    PanelInterface *pIf;
};

// ***************************************************************************

class Panel43bw: public Panel
{
public:
    Panel43bw(): Panel() {};

    virtual const char *getName() const { return "Waveshare-042bw"; };
    virtual const int getChannels() const { return 1; }
    virtual const int getBitsPerChannel() const { return 1; }
    virtual const int getWidth() const { return 400; }
    virtual const int getHeight() const { return 300; }

    virtual const RgbColors& getChannelRgbColors() const { return _rgbColors; }
    virtual const int getDefaultColor(int channel) const { return 0; }

    virtual void init(PanelInterface *pIf);
    virtual void deep_sleep();

    virtual void writeChannel(int channel, const uint8_t *data);
    virtual void display();

protected:
    const uint32_t _before_reset_ms = 200;
    const uint32_t _reset_duration_ms = 200;
    const uint32_t _after_reset_ms = 200;
    const uint32_t _power_off_timeout_ms = 200;
    const uint32_t _power_on_timeout_ms = 500;
    const uint32_t _refresh_timeout_ms = 200;
    static const RgbColors _rgbColors;
};

// ***************************************************************************
