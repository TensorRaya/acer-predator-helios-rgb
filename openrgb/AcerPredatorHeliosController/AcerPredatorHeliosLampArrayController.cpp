/*---------------------------------------------------------*\
| AcerPredatorHeliosLampArrayController.cpp                 |
|                                                           |
|   Driver for the HID LampArray (usage page 0x59) lid      |
|   logo and rear glass bar of the Acer Predator Helios 16  |
|   (Darfon Cover Logo, 0D62:A00A)                          |
|                                                           |
|   Raya Elena Solano                           10 Sep 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-only                   |
\*---------------------------------------------------------*/

#include <cstring>
#include <chrono>
#include <thread>
#include "AcerPredatorHeliosLampArrayController.h"

AcerPredatorHeliosLampArrayController::AcerPredatorHeliosLampArrayController(hid_device* dev_handle, const char* path, const char* name)
{
    dev             = dev_handle;
    location        = path;
    device_name     = name;
    lamp_count      = 0;

    ReadAttributes();
}

AcerPredatorHeliosLampArrayController::~AcerPredatorHeliosLampArrayController()
{
    if(dev)
    {
        hid_close(dev);
    }
}

std::string AcerPredatorHeliosLampArrayController::GetDeviceName()
{
    return(device_name);
}

std::string AcerPredatorHeliosLampArrayController::GetLocation()
{
    return("HID: " + location);
}

unsigned int AcerPredatorHeliosLampArrayController::GetLampCount()
{
    return(lamp_count);
}

void AcerPredatorHeliosLampArrayController::ReadAttributes()
{
    unsigned char buf[LAMPARRAY_ATTRIBUTES_LEN];

    memset(buf, 0, sizeof(buf));
    buf[0] = LAMPARRAY_REPORT_ATTRIBUTES;

    /*-----------------------------------------------------*\
    | LampArrayAttributesReport: LampCount (u16) followed   |
    | by bounding box, kind and min update interval.        |
    | The Darfon firmware NAKs GET requests for a short     |
    | window after a write, so back off and retry.          |
    \*-----------------------------------------------------*/
    for(unsigned int attempt = 0; attempt < 8; attempt++)
    {
        if(hid_get_feature_report(dev, buf, sizeof(buf)) >= 3)
        {
            lamp_count = (unsigned int)buf[1] | ((unsigned int)buf[2] << 8);
            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50 * (attempt + 1)));
    }
}

void AcerPredatorHeliosLampArrayController::SetAutonomousMode(bool enabled)
{
    const std::lock_guard<std::mutex> lock(dev_mutex);

    unsigned char buf[LAMPARRAY_CONTROL_LEN];

    buf[0] = LAMPARRAY_REPORT_CONTROL;
    buf[1] = enabled ? 0x01 : 0x00;

    hid_send_feature_report(dev, buf, sizeof(buf));
}

void AcerPredatorHeliosLampArrayController::SetLampRange(unsigned int first, unsigned int last, RGBColor color, unsigned char intensity, bool complete)
{
    const std::lock_guard<std::mutex> lock(dev_mutex);

    unsigned char buf[LAMPARRAY_RANGE_UPDATE_LEN];

    /*-----------------------------------------------------*\
    | LampRangeUpdateReport: flags, LampIdStart (u16),      |
    | LampIdEnd (u16), R, G, B, Intensity                   |
    \*-----------------------------------------------------*/
    buf[0] = LAMPARRAY_REPORT_RANGE_UPDATE;
    buf[1] = complete ? LAMPARRAY_FLAG_UPDATE_COMPLETE : 0x00;
    buf[2] = (unsigned char)(first & 0xFF);
    buf[3] = (unsigned char)(first >> 8);
    buf[4] = (unsigned char)(last & 0xFF);
    buf[5] = (unsigned char)(last >> 8);
    buf[6] = RGBGetRValue(color);
    buf[7] = RGBGetGValue(color);
    buf[8] = RGBGetBValue(color);
    buf[9] = intensity;

    hid_send_feature_report(dev, buf, sizeof(buf));
}

void AcerPredatorHeliosLampArrayController::SetAllLamps(RGBColor color, unsigned char intensity)
{
    if(lamp_count == 0)
    {
        return;
    }

    SetLampRange(0, lamp_count - 1, color, intensity, true);
}
