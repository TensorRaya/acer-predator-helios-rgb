/*---------------------------------------------------------*\
| AcerPredatorHeliosControllerDetect.cpp                    |
|                                                           |
|   Detector for Acer Predator Helios 16 (PH16-7x) RGB      |
|   devices: keyboard MCU and the Darfon HID LampArrays     |
|   (front light bar, lid logo, rear glass bar)             |
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
| Darfon devices: "InfiniteRing" 0D62:A20A exposes the      |
| left/front edge + palm rest logo (33 lamps) as a HID      |
| LampArray on interface 1; "Cover Logo" 0D62:A00A exposes  |
| the lid logo                                              |
| (14 lamps, interface 1) and the rear glass bar (16 lamps, |
| interface 2) as HID LampArrays                            |
\*---------------------------------------------------------*/

DetectedControllers DetectAcerPredatorHeliosKeyboard(hid_device_info* info, const std::string& name)
{
    DetectedControllers detected_controllers;
    hid_device*         dev = hid_open_path(info->path);

    if(dev)
    {
        std::shared_ptr<AcerPredatorHeliosController> controller = std::make_shared<AcerPredatorHeliosController>(dev, info->path, name.c_str());
        detected_controllers.push_back(new RGBController_AcerPredatorHelios(controller));
    }

    return(detected_controllers);
}

static DetectedControllers DetectLampArray(hid_device_info* info, const std::string& name, const RGBController_AcerPredatorHeliosLampArray::lamp_segments& segments)
{
    DetectedControllers detected_controllers;
    hid_device*         dev = hid_open_path(info->path);

    if(dev)
    {
        std::shared_ptr<AcerPredatorHeliosLampArrayController> controller = std::make_shared<AcerPredatorHeliosLampArrayController>(dev, info->path, name.c_str());

        if(controller->GetLampCount() > 0)
        {
            detected_controllers.push_back(new RGBController_AcerPredatorHeliosLampArray(controller, segments));
        }
    }

    return(detected_controllers);
}

DetectedControllers DetectAcerPredatorHeliosLightBar(hid_device_info* info, const std::string& name)
{
    /*-----------------------------------------------------*\
    | Lamps 0-3 sit in the left edge, 4-17 run along the    |
    | front edge left to right, 18-32 light the Predator    |
    | logo and stripes on the right palm rest               |
    \*-----------------------------------------------------*/
    RGBController_AcerPredatorHeliosLampArray::lamp_segments segments;
    segments.push_back(std::make_pair("Left Edge",      4));
    segments.push_back(std::make_pair("Front Edge",     14));
    segments.push_back(std::make_pair("Palm Rest Logo", 15));

    return(DetectLampArray(info, name, segments));
}

DetectedControllers DetectAcerPredatorHeliosLidLogo(hid_device_info* info, const std::string& name)
{
    RGBController_AcerPredatorHeliosLampArray::lamp_segments segments;
    segments.push_back(std::make_pair("Lid Logo", 0));

    return(DetectLampArray(info, name, segments));
}

DetectedControllers DetectAcerPredatorHeliosGlassBar(hid_device_info* info, const std::string& name)
{
    RGBController_AcerPredatorHeliosLampArray::lamp_segments segments;
    segments.push_back(std::make_pair("Glass Bar", 0));

    return(DetectLampArray(info, name, segments));
}

REGISTER_HID_DETECTOR_I  ("Acer Predator Helios 16 Keyboard",       DetectAcerPredatorHeliosKeyboard, 0x05AF, 0x667B, 3);
REGISTER_HID_DETECTOR_I  ("Acer Predator Helios 16 Keyboard",       DetectAcerPredatorHeliosKeyboard, 0x04F2, 0x0117, 3);
REGISTER_HID_DETECTOR_IPU("Acer Predator Helios 16 Light Bar",      DetectAcerPredatorHeliosLightBar, 0x0D62, 0xA20A, 1, 0x0059, 0x0001);
REGISTER_HID_DETECTOR_IPU("Acer Predator Helios 16 Lid Logo",       DetectAcerPredatorHeliosLidLogo,  0x0D62, 0xA00A, 1, 0x0059, 0x0001);
REGISTER_HID_DETECTOR_IPU("Acer Predator Helios 16 Glass Bar",      DetectAcerPredatorHeliosGlassBar, 0x0D62, 0xA00A, 2, 0x0059, 0x0001);
