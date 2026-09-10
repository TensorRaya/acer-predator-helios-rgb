/*---------------------------------------------------------*\
| RGBController_AcerPredatorHeliosLampArray.h               |
|                                                           |
|   RGBController for the HID LampArray lid logo and rear   |
|   glass bar of the Acer Predator Helios 16                |
|                                                           |
|   Raya Elena Solano                           10 Sep 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-only                   |
\*---------------------------------------------------------*/

#pragma once

#include <memory>
#include "RGBController.h"
#include "AcerPredatorHeliosLampArrayController.h"

#define ACER_HELIOS_LAMPARRAY_MODE_DIRECT       0
#define ACER_HELIOS_LAMPARRAY_MODE_FIRMWARE     1

class RGBController_AcerPredatorHeliosLampArray : public RGBController
{
public:
    RGBController_AcerPredatorHeliosLampArray(std::shared_ptr<AcerPredatorHeliosLampArrayController> controller_ptr, const char* zone_name);
    ~RGBController_AcerPredatorHeliosLampArray();

    void        SetupZones();
    void        ResizeZone(int zone, int new_size);

    void        DeviceUpdateLEDs();
    void        UpdateZoneLEDs(int zone);
    void        UpdateSingleLED(int led);

    void        DeviceUpdateMode();

private:
    std::shared_ptr<AcerPredatorHeliosLampArrayController>  controller;
    std::string                                             zone_label;
};
