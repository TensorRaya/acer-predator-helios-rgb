/*---------------------------------------------------------*\
| RGBController_AcerPredatorHelios.h                        |
|                                                           |
|   RGBController for Acer Predator Helios 16 (PH16-7x)     |
|   keyboard (Jing-Mold/Sunrex and Chicony MCU)             |
|                                                           |
|   Raya Elena Solano                           10 Sep 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-only                   |
\*---------------------------------------------------------*/

#pragma once

#include <memory>
#include "RGBController.h"
#include "AcerPredatorHeliosController.h"

class RGBController_AcerPredatorHelios : public RGBController
{
public:
    RGBController_AcerPredatorHelios(std::shared_ptr<AcerPredatorHeliosController> controller_ptr);
    ~RGBController_AcerPredatorHelios();

    void        SetupZones();
    void        ResizeZone(int zone, int new_size);

    void        DeviceUpdateLEDs();
    void        UpdateZoneLEDs(int zone);
    void        UpdateSingleLED(int led);

    void        DeviceUpdateMode();

private:
    std::shared_ptr<AcerPredatorHeliosController>   controller;

    void        AddEffectMode(const char* mode_name, unsigned char value, bool has_color, bool has_speed);
};
