#pragma once
#include "IstiVideoInput.h"

namespace Common
{
	struct VideoPacket
	{
		public:
			long Length;
			unsigned char* Data;
			bool IsKeyFrame;
			EstiVideoFrameFormat EndianFormat;
	};

}