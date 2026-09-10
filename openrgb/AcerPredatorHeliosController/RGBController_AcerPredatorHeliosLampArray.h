/*---------------------------------------------------------*\
| RGBController_AcerPredatorHeliosLampArray.h               |
|                                                           |
|   RGBController for the HID LampArrays of the Acer        |
|   Predator Helios 16: front light bar, lid logo, glass bar|
|                                                           |
|   Raya Elena Solano                           10 Sep 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-only                   |
\*---------------------------------------------------------*/

#pragma once

#include <memory>
#include <vector>
#include "RGBController.h"
#include "AcerPredatorHeliosLampArrayController.h"

#define ACER_HELIOS_LAMPARRAY_MODE_DIRECT       0
#define ACER_HELIOS_LAMPARRAY_MODE_FIRMWARE     1

class RGBController_AcerPredatorHeliosLampArray : public RGBController
{
public:
    typedef std::vector<std::pair<std::string, unsigned int> > lamp_segments;

    RGBController_AcerPredatorHeliosLampArray(std::shared_ptr<AcerPredatorHeliosLampArrayController> controller_ptr, const lamp_segments& segments);
    ~RGBController_AcerPredatorHeliosLampArray();

    void        SetupZones();
    void        AddLampZone(const std::string& label, unsigned int count);
    void        ResizeZone(int zone, int new_size);

    void        DeviceUpdateLEDs();
    void        UpdateZoneLEDs(int zone);
    void        UpdateSingleLED(int led);

    void        DeviceUpdateMode();

private:
    std::shared_ptr<AcerPredatorHeliosLampArrayController>  controller;
    lamp_segments                                           segments;
};
