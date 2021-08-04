/**
 * ESP32 E-Paper display firmware
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 */

#include "Panel.h"

// ***************************************************************************

static const uint8_t EPD_4IN2_lut_vcom0[] = {
    0x00, 0x17, 0x00, 0x00, 0x00, 0x02,
    0x00, 0x17, 0x17, 0x00, 0x00, 0x02,
    0x00, 0x0A, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x0E, 0x0E, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00,
};
static const uint8_t EPD_4IN2_lut_ww[] = {
    0x40, 0x17, 0x00, 0x00, 0x00, 0x02,
    0x90, 0x17, 0x17, 0x00, 0x00, 0x02,
    0x40, 0x0A, 0x01, 0x00, 0x00, 0x01,
    0xA0, 0x0E, 0x0E, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const uint8_t EPD_4IN2_lut_bw[] = {
    0x40, 0x17, 0x00, 0x00, 0x00, 0x02,
    0x90, 0x17, 0x17, 0x00, 0x00, 0x02,
    0x40, 0x0A, 0x01, 0x00, 0x00, 0x01,
    0xA0, 0x0E, 0x0E, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const uint8_t EPD_4IN2_lut_wb[] = {
    0x80, 0x17, 0x00, 0x00, 0x00, 0x02,
    0x90, 0x17, 0x17, 0x00, 0x00, 0x02,
    0x80, 0x0A, 0x01, 0x00, 0x00, 0x01,
    0x50, 0x0E, 0x0E, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const uint8_t EPD_4IN2_lut_bb[] = {
    0x80, 0x17, 0x00, 0x00, 0x00, 0x02,
    0x90, 0x17, 0x17, 0x00, 0x00, 0x02,
    0x80, 0x0A, 0x01, 0x00, 0x00, 0x01,
    0x50, 0x0E, 0x0E, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};


const Panel::RgbColors Panel43bw::_rgbColors = { std::make_tuple(255,255,255) };

// ***************************************************************************

void Panel43bw::init(PanelInterface *pIf)
{
    this->pIf = pIf;
    pIf->reset(_before_reset_ms, _reset_duration_ms, _after_reset_ms);

    pIf->writeCommand(0x01); // POWER SETTING
    pIf->writeData(0x03);
    pIf->writeData(0x00);
    pIf->writeData(0x2b);
    pIf->writeData(0x2b);

    pIf->writeCommand(0x06); // boost soft start
    pIf->writeData(0x17);		//A
    pIf->writeData(0x17);		//B
    pIf->writeData(0x17);		//C

    pIf->writeCommand(0x04); // POWER_ON
    if (!pIf->waitUntilNotBusy(LOW, _power_on_timeout_ms))
    {
        ESP_LOGE(__FILE__, "%s(%d) Busy timeout expired on POWER_ON in init()!", __FILE__, __LINE__);
    }

    pIf->writeCommand(0x00); // panel setting
    pIf->writeData(0xbf); // KW-BF   KWR-AF	BWROTP 0f	BWOTP 1f
    pIf->writeData(0x0d);
/*
    pIf->writeCommand(0x00); // panel setting
    pIf->writeData(0x3f); // 300x400, B/W, LUT set by register
*/
    pIf->writeCommand(0x30); // PLL setting
    pIf->writeData(0x3C); // 3C 50Hz  3A 100HZ  29 150Hz  39 200Hz  31 171Hz

    pIf->writeCommand(0x61); // resolution setting
    pIf->writeData( getWidth() / 256 );
    pIf->writeData( getWidth() % 256 );
    pIf->writeData( getHeight() / 256 );
    pIf->writeData( getHeight() % 256 );

/*
    pIf->writeCommand(0x82); // vcom_DC setting
    pIf->writeData(0x12);
*/
    pIf->writeCommand(0x82); // vcom_DC setting
    pIf->writeData(0x28);

    pIf->writeCommand(0X50); // VCOM AND DATA INTERVAL SETTING
    pIf->writeData(0x97); // 97white border 77black border		VBDF 17|D7 VBDW 97 VBDB 57		VBDF F7 VBDW 77 VBDB 37  VBDR B7
    // ??? 0xd7 ???

    // set LUT
    pIf->writeCommand(0x20); pIf->writeData(EPD_4IN2_lut_vcom0, 44);
    pIf->writeCommand(0x21); pIf->writeData(EPD_4IN2_lut_ww, 42);
    pIf->writeCommand(0x22); pIf->writeData(EPD_4IN2_lut_bw, 42);
    pIf->writeCommand(0x23); pIf->writeData(EPD_4IN2_lut_wb, 42);
    pIf->writeCommand(0x24); pIf->writeData(EPD_4IN2_lut_bb, 42);

    // initialize RAM
    pIf->writeCommand(0x10); pIf->writeData(0xff, getWidth() * getHeight() / 8);
    pIf->writeCommand(0x13); pIf->writeData(0xff, getWidth() * getHeight() / 8);
}

// ***************************************************************************

void Panel43bw::deep_sleep()
{
    pIf->writeCommand(0x02); // power off
    if (!pIf->waitUntilNotBusy(LOW, _power_off_timeout_ms))
    {
        ESP_LOGW(__FILE__, "%s(%d) Busy timeout expired in hibernate()!", __FILE__, __LINE__);
    }
    pIf->writeCommand(0x07); // deep sleep
    pIf->writeData(0xA5);    // check code
}

// ***************************************************************************

void Panel43bw::writeChannel(int channel, const uint8_t *data)
{
    pIf->writeCommand(0x13);
    pIf->writeData(data, ( getWidth() + 7 ) / 8 * getHeight());
}

void Panel43bw::display()
{
    pIf->writeCommand(0x12); // display refresh
    if (!pIf->waitUntilNotBusy(LOW, _refresh_timeout_ms))
    {
        ESP_LOGW(__FILE__, "%s(%d) Busy timeout expired in display()!", __FILE__, __LINE__);
    }
}

// ***************************************************************************
