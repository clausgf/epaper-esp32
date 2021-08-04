/**
 * ESP32 E-Paper display firmware
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 */

#ifndef EPD_H
#define EPD_H

#include <string>
#include <map>
#include <tuple>
#include <vector>

#include "GxEPD2.h"
#include "GxEPD2_EPD.h"

#include "logger.h"

struct PanelDescription
{
    std::string panelName;
    uint8_t background;
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> channelColors;
};

class EPD
{
public:
    typedef std::map<GxEPD2::Panel, PanelDescription> PanelDescriptionMap;

    EPD(int pinSpiSck, int pinSpiMiso, int pinSpiMosi, int pinSpiCs, 
        int pinDc, int pinRst, int pinBusy, 
        const Logger& parentLogger = rootLogger);
    virtual ~EPD();

    // get/set panel type
    void setPanel(GxEPD2::Panel panel);
    GxEPD2::Panel getPanelType() const { return _panelType; }
    const PanelDescription* getPanelDescriptionPtr() const;
    GxEPD2_EPD* getPanelPtr() const { return _rawPanelPtr; }
    const std::string& getPanelName() const { static const auto unknownPanelName = std::string(""); return _panelDescriptionPtr != nullptr ? _panelDescriptionPtr->panelName : unknownPanelName; };
    uint8_t getBackground() const       { return _panelDescriptionPtr != nullptr ? _panelDescriptionPtr->background : 0xff; };
    const std::vector<std::tuple<uint8_t, uint8_t, uint8_t>>& getChannelColors() const { return _panelDescriptionPtr->channelColors; };
    uint8_t getNumChannels() const      { return _panelDescriptionPtr != nullptr ? _panelDescriptionPtr->channelColors.size() : 1; };
    //uint8_t getBitsPerChannel() const;
    // getNumColors() const;

    void start();
    void writeChannelToDisplay(int channel, const uint8_t* _bufPtr);
    void display();
    void stop();

private:
    static PanelDescriptionMap _panelDescriptions;
    GxEPD2::Panel _panelType;
    PanelDescription* _panelDescriptionPtr;
    GxEPD2_EPD* _rawPanelPtr;
};

#endif
