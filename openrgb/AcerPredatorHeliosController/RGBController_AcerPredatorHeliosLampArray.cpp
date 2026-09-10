/*---------------------------------------------------------*\
| RGBController_AcerPredatorHeliosLampArray.cpp             |
|                                                           |
|   RGBController for the HID LampArrays of the Acer        |
|   Predator Helios 16: front light bar, lid logo, glass bar|
|                                                           |
|   Raya Elena Solano                           10 Sep 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-only                   |
\*---------------------------------------------------------*/

#include "RGBController_AcerPredatorHeliosLampArray.h"

/**------------------------------------------------------------------*\
    @name Acer Predator Helios 16 Light Bar / Lid Logo / Glass Bar
    @category Accessory
    @type USB
    @save :x:
    @direct :white_check_mark:
    @effects :x:
    @detectors DetectAcerPredatorHeliosLightBar,DetectAcerPredatorHeliosLidLogo,DetectAcerPredatorHeliosGlassBar
    @comment The Darfon "InfiniteRing" (0D62:A20A) and "Cover Logo"
        (0D62:A00A) expose standard HID LampArray interfaces: 33 lamps
        on the InfiniteRing (left edge 4, front edge 14, palm rest logo and
        stripes 15), 14 lamps behind the lid logo and 16 lamps in the rear
        glass bar.
        Colours are set per lamp via the LampRangeUpdate report;
        "Firmware" hands control back to the built-in autonomous effect.
\*-------------------------------------------------------------------*/

RGBController_AcerPredatorHeliosLampArray::RGBController_AcerPredatorHeliosLampArray(std::shared_ptr<AcerPredatorHeliosLampArrayController> controller_ptr, const lamp_segments& segment_list)
{
    controller  = controller_ptr;
    segments    = segment_list;

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
    unsigned int total     = controller->GetLampCount();
    unsigned int assigned  = 0;

    /*-----------------------------------------------------*\
    | One lamp array can span several physical areas (e.g.  |
    | left edge, front edge and palm rest logo); expose     |
    | them as separate zones so they can be coloured        |
    | independently. Whatever the device reports beyond the |
    | known segments goes into the last zone.               |
    \*-----------------------------------------------------*/
    for(unsigned int i = 0; i < segments.size(); i++)
    {
        unsigned int count = segments[i].second;

        if(i == segments.size() - 1)
        {
            count = total - assigned;
        }
        else if(assigned + count > total)
        {
            count = total - assigned;
        }

        if(count > 0)
        {
            AddLampZone(segments[i].first, count);
            assigned += count;
        }
    }

    SetupColors();
}

void RGBController_AcerPredatorHeliosLampArray::AddLampZone(const std::string& label, unsigned int count)
{
    zone new_zone;
    new_zone.name       = label;
    new_zone.type       = ZONE_TYPE_LINEAR;
    new_zone.leds_min   = count;
    new_zone.leds_max   = count;
    new_zone.leds_count = count;
    zones.push_back(new_zone);

    for(unsigned int i = 0; i < count; i++)
    {
        led new_led;
        new_led.name = label + " " + std::to_string(i);
        leds.push_back(new_led);
    }
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
