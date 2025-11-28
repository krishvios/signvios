//
//  stiRgb2yuv.h
//  ntouchMac
//
//  Created by Dennis Muhlestein on 3/21/13.
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//

#ifndef stiRgb2yuv_h
#define stiRgb2yuv_h

#include "stiDefs.h"

//class BGRAFormat
//{
//	void GetRGB(T& source, uint32_t col, uint32_t row, uint32_t &r, uint32_t &g, uint32_t &b);
//};

extern uint32_t rgb2yuv[];
void init_rgb2yuv();

/*
 * \brief RGB2NV12 nv12 is yuv format used by software encoder currently
 *
 * T is whatever source format of an RGB you need to convert to YUV.
 * RgbFormat is a function (or callable) object
 * rgbFormat ( T& source, uint32_t col, uint32_t row, uint8_t &r, uint8_t &g, uint8_t &b);
 * such that r, g, and b are populated with the correct values for the row and column of the source.
 * This allows conversion from RGBA, BGRA, ARGB or whatever custom format the RGB data is in.
 */
template <typename T, typename RgbFormat>
void RGB2NV12( T& source, uint32_t width, uint32_t height, uint8_t* yuvdest, RgbFormat rgbFormat )
{
		// do this once only
	static bool didInitTables = false;
	if (!didInitTables) {
		init_rgb2yuv();
		didInitTables = true;
	}
	
	uint8_t R, G, B, Y, U, V;
	uint32_t YUV;
	uint8_t *YoutputPtr = yuvdest;
	uint8_t *UVoutputPtr = yuvdest + (int)(width*height);

	size_t index;
	size_t superCol, superRow, subCol, subRow;
	size_t Usum, Vsum;
	
	for (superRow = 0; superRow < height / 2; superRow++) {
		for (superCol = 0; superCol < width / 2; superCol++) {
			// start a cluster of 4 pixels and sum their U & V
			Usum = 0;
			Vsum = 0;
			for (subRow = 0; subRow < 2; subRow++) {
				for (subCol = 0; subCol < 2; subCol++) {
					
					rgbFormat(source,superCol*2+subCol,superRow*2+subRow,R,G,B);
					R >>= 3;
					G >>= 2;
					B >>= 3;
					
					// do the lookup
					YUV = rgb2yuv[(R << 11) | (G << 5) | B];
					
					// write the Y value
					Y = YUV >> 16;
					
					YoutputPtr = yuvdest + ((superRow * 2 + subRow) * width) + (superCol * 2 + subCol);
					*YoutputPtr = Y;
					
					// sum the U & V values
					U = YUV >> 8;
					V = YUV;
					Usum += U;
					Vsum += V;
				}
			}
			// write the average U & V for this cluster
			index = (superRow * width) + superCol*2;
			UVoutputPtr[index] = Usum / 4;
			UVoutputPtr[index+1] = Vsum / 4;
		}
	}
}


#endif
