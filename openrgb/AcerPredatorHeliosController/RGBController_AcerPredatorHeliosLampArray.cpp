/*---------------------------------------------------------*\
| RGBController_AcerPredatorHeliosLampArray.cpp             |
|                                                           |
|   RGBController for the HID LampArray lid logo and rear   |
|   glass bar of the Acer Predator Helios 16                |
|                                                           |
|   Raya Elena Solano                           10 Sep 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-only                   |
\*---------------------------------------------------------*/

#include "RGBController_AcerPredatorHeliosLampArray.h"

/**------------------------------------------------------------------*\
    @name Acer Predator Helios 16 Lid Logo / Glass Bar
    @category Accessory
    @type USB
    @save :x:
    @direct :white_check_mark:
    @effects :x:
    @detectors DetectAcerPredatorHeliosLampArrays
    @comment Darfon "Cover Logo" (0D62:A00A) exposes two standard HID
        LampArray interfaces: 14 lamps behind the lid logo and 16 lamps
        in the rear glass bar. Colours are set per lamp via the
        LampRangeUpdate report; "Firmware" hands control back to the
        built-in autonomous effect.
\*-------------------------------------------------------------------*/

RGBController_AcerPredatorHeliosLampArray::RGBController_AcerPredatorHeliosLampArray(std::shared_ptr<AcerPredatorHeliosLampArrayController> controller_ptr, const char* zone_name)
{
    controller  = controller_ptr;
    zone_label  = zone_name;

    name        = controller->GetDeviceName();
    vendor      = "Acer";
    type        = DEVICE_TYPE_LEDSTRIP;
    description = "Acer Predator Helios 16 HID LampArray";
    location    = controller->GetLocation();

    mode Direct;
    Direct.name             = "Direct";
    Direct.value            = ACER_HELIOS_LAMPARRAY_MODE_DIRECT;
    Direct.flags            = MODE_FLAG_HAS_PER_LED_COLOR | MODE_FLAG_HAS_BRIGHTNESS;
    Direct.color_mode       = MODE_COLORS_PER_LED;
    Direct.brightness_min   = 0;
    Direct.brightness_max   = 255;
    Direct.brightness       = 255;
    modes.push_back(Direct);

    mode Firmware;
    Firmware.name           = "Firmware";
    Firmware.value          = ACER_HELIOS_LAMPARRAY_MODE_FIRMWARE;
    Firmware.flags          = 0;
    Firmware.color_mode     = MODE_COLORS_NONE;
    modes.push_back(Firmware);

    SetupZones();
}

RGBController_AcerPredatorHeliosLampArray::~RGBController_AcerPredatorHeliosLampArray()
{
    Shutdown();
}

void RGBController_AcerPredatorHeliosLampArray::SetupZones()
{
    zone new_zone;
    new_zone.name       = zone_label;
    new_zone.type       = ZONE_TYPE_LINEAR;
    new_zone.leds_min   = controller->GetLampCount();
    new_zone.leds_max   = controller->GetLampCount();
    new_zone.leds_count = controller->GetLampCount();
    zones.push_back(new_zone);

    for(unsigned int i = 0; i < new_zone.leds_count; i++)
    {
        led new_led;
        new_led.name = zone_label + " " + std::to_string(i);
        leds.push_back(new_led);
    }

    SetupColors();
}

void RGBController_AcerPredatorHeliosLampArray::ResizeZone(int /*zone*/, int /*new_size*/)
{
    /*-----------------------------------------------------*\
    | Lamp count is reported by the device                  |
    \*-----------------------------------------------------*/
}

void RGBController_AcerPredatorHeliosLampArray::DeviceUpdateLEDs()
{
    if(modes[active_mode].value != ACER_HELIOS_LAMPARRAY_MODE_DIRECT)
    {
        return;
    }

    unsigned char intensity = (unsigned char)modes[active_mode].brightness;

    /*-----------------------------------------------------*\
    | Coalesce runs of identical colour into range updates, |
    | the last one carries the UpdateComplete flag          |
    \*-----------------------------------------------------*/
    unsigned int start = 0;

    for(unsigned int i = 1; i <= colors.size(); i++)
    {
        if(i == colors.size() || colors[i] != colors[start])
        {
            controller->SetLampRange(start, i - 1, colors[start], intensity, i == colors.size());
            start = i;
        }
    }
}

void RGBController_AcerPredatorHeliosLampArray::UpdateZoneLEDs(int /*zone*/)
{
    DeviceUpdateLEDs();
}

void RGBController_AcerPredatorHeliosLampArray::UpdateSingleLED(int /*led*/)
{
    DeviceUpdateLEDs();
}

void RGBController_AcerPredatorHeliosLampArray::DeviceUpdateMode()
{
    if(modes[active_mode].value == ACER_HELIOS_LAMPARRAY_MODE_FIRMWARE)
    {
        controller->SetAutonomousMode(true);
    }
    else
    {
        controller->SetAutonomousMode(false);
        DeviceUpdateLEDs();
    }
}
