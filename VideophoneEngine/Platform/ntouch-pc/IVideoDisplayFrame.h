#pragma once
#include <stdint.h>
#include "stiSVX.h"

class IVideoDisplayFrame
{
public:
	//Buffer Data Stored in the YUV color space.  I420 Y, U, V.  NV12 Y, UV, nullptr
	virtual std::tuple<uint8_t *, uint8_t *, uint8_t*> data () = 0;
	//The stride length of the color being stored in data member.  I420 yStride, uStride, vStride.  NV12 yStride, uvStride, 0
	virtual std::tuple<int, int, int> stride () = 0;
	virtual int widthGet () const = 0;
	virtual int heightGet () const = 0;
	virtual EstiColorSpace colorspaceGet () = 0;
};