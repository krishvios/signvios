// Sorenson Communications Inc. Confidential. -- Do not distribute
// Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
//
// The softphone needs this file to compile Helios classes
#pragma once

#include <string>


struct __attribute__((packed)) lr_led_rgb
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};
