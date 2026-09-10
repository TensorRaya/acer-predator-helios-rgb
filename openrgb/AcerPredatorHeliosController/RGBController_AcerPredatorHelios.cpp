/*---------------------------------------------------------*\
| RGBController_AcerPredatorHelios.cpp                      |
|                                                           |
|   RGBController for Acer Predator Helios 16 (PH16-7x)     |
|   keyboard and Darfon InfiniteRing light bar              |
|                                                           |
|   Raya Elena Solano                           10 Sep 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-only                   |
\*---------------------------------------------------------*/

#include "RGBController_AcerPredatorHelios.h"

/**------------------------------------------------------------------*\
    @name Acer Predator Helios 16 Keyboard / InfiniteRing
    @category Keyboard
    @type USB
    @save :x:
    @direct :white_check_mark:
    @effects :white_check_mark:
    @detectors DetectAcerPredatorHeliosControllers
    @comment Keyboard MCU (Jing-Mold/Sunrex 05AF:667B, Chicony 04F2:0117)
        interface 3 and Darfon InfiniteRing (0D62:A20A) interface 0 of the
        Predator Helios 16 PH16-71/72/73. Per-key mode drives 128 cells;
        the physical key map is not yet known so the keyboard is exposed
        as a linear zone.
\*-------------------------------------------------------------------*/

RGBController_AcerPredatorHelios::RGBController_AcerPredatorHelios(std::shared_ptr<AcerPredatorHeliosController> controller_ptr, bool is_keyboard)
{
    controller  = controller_ptr;
    keyboard    = is_keyboard;

    name        = controller->GetDeviceName();
    vendor      = "Acer";
    type        = keyboard ? DEVICE_TYPE_KEYBOARD : DEVICE_TYPE_LEDSTRIP;
    description = keyboard ? "Acer Predator Helios 16 Keyboard" : "Acer Predator Helios 16 InfiniteRing Light Bar";
    location    = controller->GetLocation();

    if(keyboard)
    {
        mode Direct;
        Direct.name             = "Direct";
        Direct.value            = ACER_HELIOS_EFF_PERKEY;
        Direct.flags            = MODE_FLAG_HAS_PER_LED_COLOR | MODE_FLAG_HAS_BRIGHTNESS;
        Direct.color_mode       = MODE_COLORS_PER_LED;
        Direct.brightness_min   = 0;
        Direct.brightness_max   = 100;
        Direct.brightness       = 100;
        modes.push_back(Direct);
    }

    AddEffectMode("Static",     ACER_HELIOS_EFF_STATIC,     true,  false);
    AddEffectMode("Breathing",  ACER_HELIOS_EFF_BREATHING,  true,  true);
    AddEffectMode("Neon",       ACER_HELIOS_EFF_NEON,       false, true);
    AddEffectMode("Rainbow",    ACER_HELIOS_EFF_RAINBOW,    false, true);
    AddEffectMode("Snake",      ACER_HELIOS_EFF_SNAKE,      true,  true);
    AddEffectMode("Ripple",     ACER_HELIOS_EFF_RIPPLE,     true,  true);
    AddEffectMode("Rain",       ACER_HELIOS_EFF_RAIN,       true,  true);
    AddEffectMode("Explosion",  ACER_HELIOS_EFF_EXPLOSION,  true,  true);
    AddEffectMode("Pulse",      ACER_HELIOS_EFF_PULSE,      true,  true);
    AddEffectMode("Stars",      ACER_HELIOS_EFF_STARS,      true,  true);
    AddEffectMode("Meteor",     ACER_HELIOS_EFF_METEOR,     true,  true);
    AddEffectMode("Aura",       ACER_HELIOS_EFF_AURA,       true,  true);

    mode Off;
    Off.name                = "Off";
    Off.value               = ACER_HELIOS_EFF_OFF;
    Off.flags               = 0;
    Off.color_mode          = MODE_COLORS_NONE;
    modes.push_back(Off);

    SetupZones();
}

void RGBController_AcerPredatorHelios::AddEffectMode(const char* mode_name, unsigned char value, bool has_color, bool has_speed)
{
    mode m;
    m.name              = mode_name;
    m.value             = value;
    m.flags             = MODE_FLAG_HAS_BRIGHTNESS;
    m.brightness_min    = 0;
    m.brightness_max    = 100;
    m.brightness        = 100;
    m.color_mode        = MODE_COLORS_NONE;

    if(has_color)
    {
        m.flags        |= MODE_FLAG_HAS_MODE_SPECIFIC_COLOR;
        m.color_mode    = MODE_COLORS_MODE_SPECIFIC;
        m.colors_min    = 1;
        m.colors_max    = 1;
        m.colors.resize(1);
        m.colors[0]     = ToRGBColor(0x00, 0xAE, 0xC7);
    }

    if(has_speed)
    {
        m.flags        |= MODE_FLAG_HAS_SPEED;
        m.speed_min     = 1;
        m.speed_max     = 9;
        m.speed         = 5;
    }

    modes.push_back(m);
}

RGBController_AcerPredatorHelios::~RGBController_AcerPredatorHelios()
{
    Shutdown();
}

void RGBController_AcerPredatorHelios::SetupZones()
{
    zone new_zone;

    if(keyboard)
    {
        new_zone.name       = "Keyboard";
        new_zone.type       = ZONE_TYPE_LINEAR;
        new_zone.leds_min   = ACER_HELIOS_NUM_CELLS;
        new_zone.leds_max   = ACER_HELIOS_NUM_CELLS;
        new_zone.leds_count = ACER_HELIOS_NUM_CELLS;
    }
    else
    {
        new_zone.name       = "Light Bar";
        new_zone.type       = ZONE_TYPE_SINGLE;
        new_zone.leds_min   = 1;
        new_zone.leds_max   = 1;
        new_zone.leds_count = 1;
    }

    zones.push_back(new_zone);

    for(unsigned int i = 0; i < new_zone.leds_count; i++)
    {
        led new_led;
        new_led.name = keyboard ? ("Key " + std::to_string(i)) : "Light Bar";
        leds.push_back(new_led);
    }

    SetupColors();
}

void RGBController_AcerPredatorHelios::ResizeZone(int /*zone*/, int /*new_size*/)
{
    /*-----------------------------------------------------*\
    | Zones are fixed size                                  |
    \*-----------------------------------------------------*/
}

void RGBController_AcerPredatorHelios::DeviceUpdateLEDs()
{
    const mode& m = modes[active_mode];

    if(m.value == ACER_HELIOS_EFF_PERKEY)
    {
        controller->SetPerKey(&colors[0], (unsigned int)colors.size(), (unsigned char)m.brightness);
    }
    else
    {
        DeviceUpdateMode();
    }
}

void RGBController_AcerPredatorHelios::UpdateZoneLEDs(int /*zone*/)
{
    DeviceUpdateLEDs();
}

void RGBController_AcerPredatorHelios::UpdateSingleLED(int /*led*/)
{
    DeviceUpdateLEDs();
}

void RGBController_AcerPredatorHelios::DeviceUpdateMode()
{
    const mode& m = modes[active_mode];

    if(m.value == ACER_HELIOS_EFF_PERKEY)
    {
        controller->SetPerKey(&colors[0], (unsigned int)colors.size(), (unsigned char)m.brightness);
        return;
    }

    RGBColor color = (m.colors.size() > 0) ? m.colors[0] : ToRGBColor(0xFF, 0xFF, 0xFF);

    controller->SetEffect((unsigned char)m.value, color, (unsigned char)m.brightness, (unsigned char)m.speed);
}
