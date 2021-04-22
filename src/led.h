/**
 * ESP32 E-Paper display firmware
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 */

#ifndef LED_H
#define LED_H

#include <Arduino.h>

/**
 * Status LED
 */
class LED {
public:
    LED(int pin): _pin(pin) { pinMode(_pin, OUTPUT); }

    void begin() {}

    void set(bool value) {
        digitalWrite(_pin, value ? HIGH : LOW);
    }

    void toggle() {
        digitalWrite(_pin, !digitalRead(_pin));
    }

private:
    int _pin;
};

#endif
