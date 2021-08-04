/**
 * ESP32 E-Paper display firmware
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 */

#include <Arduino.h>

#include <SPI.h>
#include <Wire.h>
#include <esp_log.h>

// specific display drivers for 4.2" 400x300 bw and 7.5" 880x528 red/black/white
#include <epd/GxEPD2_420.h>
#include <epd3c/GxEPD2_750c_Z90.h>
#include <GxEPD2_EPD.h>

#include "Epd.h"


EPD::EPD(int pinSpiSck, int pinSpiMiso, int pinSpiMosi, int pinSpiCs, int pinDc, int pinRst, int pinBusy, const Logger& parentLogger):
    _pinSpiSck(pinSpiSck),
    _pinSpiMiso(pinSpiMiso),
    _pinSpiMosi(pinSpiMosi),
    _pinSpiCs(pinSpiCs),
    _pinDc(pinDc),
    _pinRst(pinRst),
    _pinBusy(pinBusy),
    _logger("epd", parentLogger)
{
    _panelType = GxEPD2::GDEW042T2;  // unfortunately, there's no value for "unknown"
    _panelDescriptionPtr = nullptr;
    _rawPanelPtr = nullptr;
}

EPD::~EPD()
{
    stop();
    if (_rawPanelPtr != nullptr)
    {
        // delete _rawPanelPtr;  // unfortunately, GxEPD2::... has no virtual destructor
    }
}


// ***** Panel ***************************************************************

EPD::PanelDescriptionMap EPD::_panelDescriptions = {
    // bw      panelName background channelColors
    { GxEPD2::GDEW042T2, { "GDEW042T2/Waveshare_4_2_bw", 0xff, { {255,255,255} } } },
    //{ GxEPD2::GDEW042M01, "GDEW042M01" },
    //{ GxEPD2::GDEW0583T7, "GDEW0583T7/Waveshare_5_83_bw" },
    //{ GxEPD2::GDEW0583T8, "GDEW0583T8" },
    //{ GxEPD2::GDEW075T8, "GDEW075T8/Waveshare_7_5_bw" },
    //{ GxEPD2::GDEW075T7, "GDEW075T7/Waveshare_7_5_bw_T7" },
    //{ GxEPD2::GDEW1248T3, "GDEW1248T3/Waveshare_12_24_bw" },
      // 3-color
    //{ GxEPD2::GDEW042Z15, "GDEW042Z15/Waveshare_4_2_bwr" },
    //{ GxEPD2::GDEW0583Z21, "GDEW0583Z21/Waveshare_5_83_bwr" },
    //{ GxEPD2::ACeP565, "ACeP565/Waveshare_5_65_7c" },
    //{ GxEPD2::GDEW075Z09, "GDEW075Z09/Waveshare_7_5_bwr" },
    //{ GxEPD2::GDEW075Z08, "GDEW075Z08/Waveshare_7_5_bwr_Z08" },
    { GxEPD2::GDEH075Z90, { "GDEH075Z90/Waveshare_7_5_bwr_Z90", 0xff, { {255,255,255}, {255,0,0} } } }
};

void EPD::setPanel(GxEPD2::Panel panel)
{
    _panelType = panel;

    // panel description
    auto it = _panelDescriptions.find(_panelType);
    _panelDescriptionPtr = (it != _panelDescriptions.end() ? &(it->second) : nullptr);

    // raw panel
    GxEPD2_EPD *rawPanelPtr = nullptr;
    switch (_panelType) {
        case GxEPD2::GDEW042T2: rawPanelPtr = new GxEPD2_420(_pinSpiCs, _pinDc, _pinRst, _pinBusy); break;
        case GxEPD2::GDEW042M01: break;
        case GxEPD2::GDEW0583T7: break;
        case GxEPD2::GDEW0583T8: break;
        case GxEPD2::GDEW075T8: break;
        case GxEPD2::GDEW075T7: break;
        case GxEPD2::GDEW1248T3: break;
        case GxEPD2::GDEW042Z15: break;
        case GxEPD2::GDEW0583Z21: break;
        case GxEPD2::ACeP565: break;
        case GxEPD2::GDEW075Z09: break;
        case GxEPD2::GDEW075Z08: break;
        case GxEPD2::GDEH075Z90: rawPanelPtr = new GxEPD2_750c_Z90(_pinSpiCs, _pinDc, _pinRst, _pinBusy); break;
        default: 
            _logger.error("Panel not supported. Modify EPD::setPanel()");
            rawPanelPtr = nullptr;
            break;
    }

    // save the result into the internal _rawPanelPtr
    if (_rawPanelPtr != nullptr)
    {
        // delete _rawPanelPtr;  // unfortunately, GxEPD2::... has no virtual destructor
    }
    _rawPanelPtr = rawPanelPtr;
}


// ***** EPD life cycle ******************************************************

void EPD::start()
{
    // check invariants
    if (_rawPanelPtr == nullptr) {
        _logger.error("_rawPanelPtr not set. Call EPD::setPanel() first.");
        return;
    }

    _logger.info("EPD starting");
    _rawPanelPtr->init(0);
    SPI.end();
    SPI.begin(_pinSpiSck, _pinSpiMiso /*not used*/, _pinSpiMosi, _pinSpiCs);
}

void EPD::writeChannelToDisplay(int channel, const uint8_t* _bufPtr)
{
    // check invariants
    if (_rawPanelPtr == nullptr) {
        _logger.error("_rawPanelPtr not set.");
        return;
    }
    if (_bufPtr == nullptr) {
        _logger.error("_bufPtr not set.");
        return;
    }

    _logger.info("Writing channel %d to display...", channel);
    switch (_panelType)
    {
        case GxEPD2::GDEH075Z90:
            ((GxEPD2_750c_Z90*)_rawPanelPtr)->writeImage(channel, _bufPtr, 0, 0, _rawPanelPtr->WIDTH, _rawPanelPtr->HEIGHT);
            break;
        default:
            _rawPanelPtr->writeImage(_bufPtr, 0, 0, _rawPanelPtr->WIDTH, _rawPanelPtr->HEIGHT);
            break;
    }
}

void EPD::display()
{
    _logger.info("Display/refreshing EPD...");
    // check invariants
    if (_rawPanelPtr == nullptr) {
        _logger.error("_rawPanelPtr not set. Call EPD::setPanel() first.");
        return;
    }
    _rawPanelPtr->refresh(false);
    _logger.info("Finished displaying/refreshing EPD...");
}

void EPD::stop()
{
    _logger.info("Stopping EPD...");
    // check invariants
    if (_rawPanelPtr == nullptr) {
        _logger.error("_rawPanelPtr not set. Call EPD::setPanel() first.");
        return;
    }
    _rawPanelPtr->hibernate(); // implies powerOff

    //SPI.end();
    _logger.info("EPD stopped: display hibernating");
}
