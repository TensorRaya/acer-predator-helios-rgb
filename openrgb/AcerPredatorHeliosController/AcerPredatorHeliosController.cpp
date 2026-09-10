/*---------------------------------------------------------*\
| AcerPredatorHeliosController.cpp                          |
|                                                           |
|   Driver for Acer Predator Helios 16 (PH16-7x) keyboard   |
|   MCU and Darfon InfiniteRing light bar (vendor HID)      |
|                                                           |
|   Raya Elena Solano                           10 Sep 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-only                   |
\*---------------------------------------------------------*/

#include <cstring>
#include <chrono>
#include <thread>
#include "AcerPredatorHeliosController.h"
#include "StringUtils.h"

AcerPredatorHeliosController::AcerPredatorHeliosController(hid_device* dev_handle, const char* path, const char* name)
{
    dev             = dev_handle;
    location        = path;
    device_name     = name;
}

AcerPredatorHeliosController::~AcerPredatorHeliosController()
{
    if(dev)
    {
        hid_close(dev);
    }
}

std::string AcerPredatorHeliosController::GetDeviceName()
{
    return(device_name);
}

std::string AcerPredatorHeliosController::GetLocation()
{
    return("HID: " + location);
}

void AcerPredatorHeliosController::SendCmd(unsigned char op, unsigned char p1, unsigned char p2, unsigned char p3, unsigned char p4, unsigned char p5, unsigned char p6)
{
    unsigned char   buf[ACER_HELIOS_CMD_LEN];
    unsigned int    sum;

    /*-----------------------------------------------------*\
    | Byte 0 is the (unused) report ID, the command itself  |
    | is 8 bytes with an inverted checksum in the last one  |
    \*-----------------------------------------------------*/
    buf[0]  = 0x00;
    buf[1]  = op;
    buf[2]  = p1;
    buf[3]  = p2;
    buf[4]  = p3;
    buf[5]  = p4;
    buf[6]  = p5;
    buf[7]  = p6;

    sum     = 0;

    for(unsigned int i = 1; i < 8; i++)
    {
        sum += buf[i];
    }

    buf[8]  = (unsigned char)(0xFF - (sum & 0xFF));

    hid_send_feature_report(dev, buf, ACER_HELIOS_CMD_LEN);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

void AcerPredatorHeliosController::SetEffect(unsigned char effect, RGBColor color, unsigned char brightness, unsigned char speed)
{
    const std::lock_guard<std::mutex> lock(dev_mutex);

    /*-----------------------------------------------------*\
    | Firmware speed byte: 1 = fast .. 9 = slow, OpenRGB    |
    | speed slider is 1 = slow .. 9 = fast, so invert it    |
    \*-----------------------------------------------------*/
    unsigned char fw_speed = (unsigned char)(10 - speed);

    SendCmd(ACER_HELIOS_OP_BEGIN,       0, 0, 0, 0, 0, 0);
    SendCmd(ACER_HELIOS_OP_MODE_ZONE,   0, 0, 0, 0, 0, 0);

    if(effect == ACER_HELIOS_EFF_OFF)
    {
        SendCmd(ACER_HELIOS_OP_APPLY, 0x02, ACER_HELIOS_EFF_STATIC, fw_speed, 0, ACER_HELIOS_SCOPE_ZONE, 0x05);
        return;
    }

    SendCmd(ACER_HELIOS_OP_SET_COLOR, 0, 0, RGBGetRValue(color), RGBGetGValue(color), RGBGetBValue(color), 0);
    SendCmd(ACER_HELIOS_OP_APPLY, 0x02, effect, fw_speed, brightness, ACER_HELIOS_SCOPE_ZONE, 0x05);
}

void AcerPredatorHeliosController::SetPerKey(const RGBColor* colors, unsigned int count, unsigned char brightness)
{
    const std::lock_guard<std::mutex> lock(dev_mutex);

    unsigned char frame[ACER_HELIOS_FRAME_LEN];
    unsigned char chunk[ACER_HELIOS_FRAME_CHUNK + 1];

    memset(frame, 0, sizeof(frame));

    for(unsigned int i = 0; i < ACER_HELIOS_NUM_CELLS && i < count; i++)
    {
        frame[i * 4 + 1] = RGBGetRValue(colors[i]);
        frame[i * 4 + 2] = RGBGetGValue(colors[i]);
        frame[i * 4 + 3] = RGBGetBValue(colors[i]);
    }

    SendCmd(ACER_HELIOS_OP_BEGIN,       0, 0, 0, 0, 0, 0);
    SendCmd(ACER_HELIOS_OP_MODE_PERKEY, 0, 0, ACER_HELIOS_SCOPE_PERKEY, 0, 0, 0);

    /*-----------------------------------------------------*\
    | 512 byte frame goes out as 8 interrupt OUT packets    |
    \*-----------------------------------------------------*/
    for(unsigned int off = 0; off < ACER_HELIOS_FRAME_LEN; off += ACER_HELIOS_FRAME_CHUNK)
    {
        chunk[0] = 0x00;
        memcpy(&chunk[1], &frame[off], ACER_HELIOS_FRAME_CHUNK);
        hid_write(dev, chunk, sizeof(chunk));
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    SendCmd(ACER_HELIOS_OP_APPLY, 0x02, ACER_HELIOS_EFF_PERKEY, 0x05, brightness, ACER_HELIOS_SCOPE_PERKEY, 0x01);
}
