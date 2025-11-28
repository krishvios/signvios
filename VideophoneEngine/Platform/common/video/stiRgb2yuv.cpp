//
//  stiRgb2yuv.cpp
//  ntouchMac
//
//  Created by Dennis Muhlestein on 3/21/13.
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//

#include "stiRgb2yuv.h"


uint32_t rgb2yuv[65536];
static uint8_t dealpha[256][256];

void rgb2ycbcr(uint8_t r, uint8_t g,  uint8_t b,  uint8_t *y, uint8_t *cb, uint8_t *cr)
{
	// Y  =  0.257*R + 0.504*G + 0.098*B + 16
	// CB = -0.148*R - 0.291*G + 0.439*B + 128
	// CR =  0.439*R - 0.368*G - 0.071*B + 128
	*y  = (uint8_t)((8432*(unsigned long)r + 16425*(unsigned long)g + 3176*(unsigned long)b + 16*32768)>>15);
	*cb = (uint8_t)((128*32768 + 14345*(unsigned long)b - 4818*(unsigned long)r -9527*(unsigned long)g)>>15);
	*cr = (uint8_t)((128*32768 + 14345*(unsigned long)r - 12045*(unsigned long)g-2300*(unsigned long)b)>>15);
}

void init_rgb2yuv()
{
	unsigned long i;
	uint8_t y, cb, cr;
	
	for (i = 0; i < 65536; i++) {
		//  Get colour components
		uint8_t r = (i >> 11) & 0x1F;
		uint8_t g = (i >>  5) & 0x3F;
		uint8_t b = i & 0x1F;
		
		//  Scale colours to 8 bits
		
		r <<= 3;
		g <<= 2;
		b <<= 3;
		
		//  Convert to YCbCr
		rgb2ycbcr(r, g, b, &y, &cb, &cr);
		//  Store in mapping table
		rgb2yuv[i] = ((uint32_t)y << 16) | ((uint32_t)cb << 8) | (uint32_t)cr;
	}
	
	for (int a = 0; a < 256; a++) {
		for (int c = 0; c < 256; c++) {
			dealpha[a][c] = (uint8_t)(((uint16_t)c << 8) / (a + 1));
		}
	}    
}
