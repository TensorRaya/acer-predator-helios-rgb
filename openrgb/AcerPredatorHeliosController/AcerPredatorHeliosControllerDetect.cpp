/*---------------------------------------------------------*\
| AcerPredatorHeliosControllerDetect.cpp                    |
|                                                           |
|   Detector for Acer Predator Helios 16 (PH16-7x) RGB      |
|   devices: keyboard MCU, InfiniteRing light bar and the   |
|   Cover Logo HID LampArrays                               |
|                                                           |
|   Raya Elena Solano                           10 Sep 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-only                   |
\*---------------------------------------------------------*/

#include <hidapi.h>
#include "DetectionManager.h"
#include "AcerPredatorHeliosController.h"
#include "AcerPredatorHeliosLampArrayController.h"
#include "RGBController_AcerPredatorHelios.h"
#include "RGBController_AcerPredatorHeliosLampArray.h"

/*---------------------------------------------------------*\
| Keyboard MCU: vendor interface 3 (usage page 0xFF02)      |
\*---------------------------------------------------------*/
#define ACER_HELIOS_KB_SUNREX_VID       0x05AF
#define ACER_HELIOS_KB_SUNREX_PID       0x667B
#define ACER_HELIOS_KB_CHICONY_VID      0x04F2
#define ACER_HELIOS_KB_CHICONY_PID      0x0117
#define ACER_HELIOS_KB_INTERFACE        3

/*---------------------------------------------------------*\
| Darfon devices                                            |
\*---------------------------------------------------------*/
#define DARFON_VID                      0x0D62
#define DARFON_INFINITERING_PID         0xA20A
#define DARFON_COVER_LOGO_PID           0xA00A
#define DARFON_VENDOR_INTERFACE         0
#define DARFON_LAMPARRAY_LOGO_INTERFACE 1
#define DARFON_LAMPARRAY_BAR_INTERFACE  2
#define HID_USAGE_PAGE_LAMPARRAY        0x0059
#define HID_USAGE_LAMPARRAY             0x0001

DetectedControllers DetectAcerPredatorHeliosKeyboard(hid_device_info* info, const std::string& name)
{
    DetectedControllers detected_controllers;
    hid_device*         dev = hid_open_path(info->path);

    if(dev)
    {
        std::shared_ptr<AcerPredatorHeliosController> controller = std::make_shared<AcerPredatorHeliosController>(dev, info->path, name.c_str());
        detected_controllers.push_back(new RGBController_AcerPredatorHelios(controller, true));
    }

    return(detected_controllers);
}

DetectedControllers DetectAcerPredatorHeliosLightBar(hid_device_info* info, const std::string& name)
{
    DetectedControllers detected_controllers;
    hid_device*         dev = hid_open_path(info->path);

    if(dev)
    {
        std::shared_ptr<AcerPredatorHeliosController> controller = std::make_shared<AcerPredatorHeliosController>(dev, info->path, name.c_str());
        detected_controllers.push_back(new RGBController_AcerPredatorHelios(controller, false));
    }

    return(detected_controllers);
}

static DetectedControllers DetectLampArray(hid_device_info* info, const std::string& name, const char* zone_name)
{
    DetectedControllers detected_controllers;
    hid_device*         dev = hid_open_path(info->path);

    if(dev)
    {
        std::shared_ptr<AcerPredatorHeliosLampArrayController> controller = std::make_shared<AcerPredatorHeliosLampArrayController>(dev, info->path, name.c_str());

        if(controller->GetLampCount() > 0)
        {
            detected_controllers.push_back(new RGBController_AcerPredatorHeliosLampArray(controller, zone_name));
        }
    }

    return(detected_controllers);
}

DetectedControllers DetectAcerPredatorHeliosLidLogo(hid_device_info* info, const std::string& name)
{
    return(DetectLampArray(info, name, "Lid Logo"));
}

DetectedControllers DetectAcerPredatorHeliosGlassBar(hid_device_info* info, const std::string& name)
{
    return(DetectLampArray(info, name, "Glass Bar"));
}

REGISTER_HID_DETECTOR_I  ("Acer Predator Helios 16 Keyboard",       DetectAcerPredatorHeliosKeyboard, 0x05AF, 0x667B, 3);
REGISTER_HID_DETECTOR_I  ("Acer Predator Helios 16 Keyboard",       DetectAcerPredatorHeliosKeyboard, 0x04F2, 0x0117, 3);
REGISTER_HID_DETECTOR_I  ("Acer Predator Helios 16 InfiniteRing",   DetectAcerPredatorHeliosLightBar, 0x0D62, 0xA20A, 0);
REGISTER_HID_DETECTOR_IPU("Acer Predator Helios 16 Lid Logo",       DetectAcerPredatorHeliosLidLogo,  0x0D62, 0xA00A, 1, 0x0059, 0x0001);
REGISTER_HID_DETECTOR_IPU("Acer Predator Helios 16 Glass Bar",      DetectAcerPredatorHeliosGlassBar, 0x0D62, 0xA00A, 2, 0x0059, 0x0001);
