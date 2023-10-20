/**
 * ESP32 E-Paper display firmware
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 */

#ifndef EPD_H
#define EPD_H

#include <string>
#include <map>

#include "GxEPD2.h"
#include "GxEPD2_EPD.h"

#include "logger.h"


class EPD
{
public:
    typedef std::map<GxEPD2::Panel, std::string> PanelMap;

    EPD(int pinSpiSck, int pinSpiMiso, int pinSpiMosi, int pinSpiCs, 
        int pinDc, int pinRst, int pinBusy, 
        const Logger& parentLogger = rootLogger);
    virtual ~EPD();

    // get/set panel type
    void setPanel(GxEPD2::Panel panel);
    GxEPD2::Panel getPanelType() const { return _panelType; }
    GxEPD2_EPD* getPanelPtr() const { return _rawPanelPtr; }
    const std::string& getPanelName() const;
    const PanelMap& getSupportedPanels() const;

    void start();
    void displayPixelBuffer(const uint8_t* _bufPtr);
    void stop();

private:
    int _pinSpiSck;
    int _pinSpiMiso;
    int _pinSpiMosi;
    int _pinSpiCs;
    int _pinDc;
    int _pinRst;
    int _pinBusy;
    Logger _logger;
    static PanelMap _supportedPanels;
    GxEPD2::Panel _panelType;
    GxEPD2_EPD* _rawPanelPtr;
};

#endif
