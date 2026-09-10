/*---------------------------------------------------------*\
| AcerPredatorHeliosController.h                            |
|                                                           |
|   Driver for Acer Predator Helios 16 (PH16-7x) keyboard   |
|   MCU and Darfon InfiniteRing light bar (vendor HID)      |
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
| Wire protocol (from PredatorSense USBPcap captures,       |
| OpenRGB issue #4158, verified on PH16-73):                |
|   8 byte feature report, byte 7 = 0xFF - sum(bytes 0..6)  |
|   0x88          begin                                     |
|   0xB1          zone (single colour) mode                 |
|   0x12 .. 08    per-key mode, frame follows on EP OUT     |
|   0x14 00 00 R G B 00   set colour                        |
|   0x08 02 EFF SPD BRT SCOPE 05   apply                    |
\*---------------------------------------------------------*/
#define ACER_HELIOS_CMD_LEN             9
#define ACER_HELIOS_NUM_CELLS           128
#define ACER_HELIOS_FRAME_LEN           (ACER_HELIOS_NUM_CELLS * 4)
#define ACER_HELIOS_FRAME_CHUNK         64

#define ACER_HELIOS_OP_BEGIN            0x88
#define ACER_HELIOS_OP_MODE_ZONE        0xB1
#define ACER_HELIOS_OP_MODE_PERKEY      0x12
#define ACER_HELIOS_OP_SET_COLOR        0x14
#define ACER_HELIOS_OP_APPLY            0x08

#define ACER_HELIOS_SCOPE_ZONE          0x01
#define ACER_HELIOS_SCOPE_PERKEY        0x08

enum
{
    ACER_HELIOS_EFF_STATIC          = 0x01,
    ACER_HELIOS_EFF_BREATHING       = 0x02,
    ACER_HELIOS_EFF_RAINBOW         = 0x03,
    ACER_HELIOS_EFF_SNAKE           = 0x05,
    ACER_HELIOS_EFF_RIPPLE          = 0x06,
    ACER_HELIOS_EFF_NEON            = 0x08,
    ACER_HELIOS_EFF_RAIN            = 0x0A,
    ACER_HELIOS_EFF_EXPLOSION       = 0x12,
    ACER_HELIOS_EFF_PULSE           = 0x25,
    ACER_HELIOS_EFF_STARS           = 0x26,
    ACER_HELIOS_EFF_METEOR          = 0x27,
    ACER_HELIOS_EFF_AURA            = 0x28,
    ACER_HELIOS_EFF_PERKEY          = 0x33,
    ACER_HELIOS_EFF_OFF             = 0xFE,   /* pseudo: static, brightness 0 */
};

class AcerPredatorHeliosController
{
public:
    AcerPredatorHeliosController(hid_device* dev_handle, const char* path, const char* name);
    ~AcerPredatorHeliosController();

    std::string GetDeviceName();
    std::string GetLocation();

    /*-----------------------------------------------------*\
    | brightness 0..100, speed 1 (slow) .. 9 (fast)         |
    \*-----------------------------------------------------*/
    void SetEffect(unsigned char effect, RGBColor color, unsigned char brightness, unsigned char speed);
    void SetPerKey(const RGBColor* colors, unsigned int count, unsigned char brightness);

private:
    hid_device*     dev;
    std::string     location;
    std::string     device_name;
    std::mutex      dev_mutex;

    void SendCmd(unsigned char op, unsigned char p1, unsigned char p2, unsigned char p3, unsigned char p4, unsigned char p5, unsigned char p6);
};
