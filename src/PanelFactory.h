/**
 * ESP32 E-Paper display firmware
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 */

#pragma once

#include <vector>
#include <cstring>

#include "Panel.h"

// ***************************************************************************

class PanelFactory
{
public:
    PanelFactory()
    {
        _panelPtrs.push_back(new Panel43bw());
    }

    void init() { }

    Panel *createPanel(const char *name)
    {
        Panel *ret = nullptr;
        for (auto const& pPanel: _panelPtrs)
        {
            if (strcmp(name, pPanel->getName()) == 0)
            {
                ret = pPanel;
            }
        }
        return ret;
    }

private:
    std::vector<Panel*> _panelPtrs;
};

// ***************************************************************************
