/*---------------------------------------------------------*\
| AcerPredatorHeliosLampArrayController.h                   |
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

#pragma once

#include <string>
#include <mutex>
#include <hidapi.h>
#include "RGBController.h"

/*---------------------------------------------------------*\
| HID LampArray report IDs as declared by the device        |
\*---------------------------------------------------------*/
#define LAMPARRAY_REPORT_ATTRIBUTES         0x01
#define LAMPARRAY_REPORT_RANGE_UPDATE       0x05
#define LAMPARRAY_REPORT_CONTROL            0x06

#define LAMPARRAY_ATTRIBUTES_LEN            23
#define LAMPARRAY_RANGE_UPDATE_LEN          10
#define LAMPARRAY_CONTROL_LEN               2

#define LAMPARRAY_FLAG_UPDATE_COMPLETE      0x01

class AcerPredatorHeliosLampArrayController
{
public:
    AcerPredatorHeliosLampArrayController(hid_device* dev_handle, const char* path, const char* name);
    ~AcerPredatorHeliosLampArrayController();

    std::string     GetDeviceName();
    std::string     GetLocation();
    unsigned int    GetLampCount();

    void            SetAutonomousMode(bool enabled);
    void            SetAllLamps(RGBColor color, unsigned char intensity);
    void            SetLampRange(unsigned int first, unsigned int last, RGBColor color, unsigned char intensity, bool complete);

private:
    hid_device*     dev;
    std::string     location;
    std::string     device_name;
    unsigned int    lamp_count;
    std::mutex      dev_mutex;

    void            ReadAttributes();
};
