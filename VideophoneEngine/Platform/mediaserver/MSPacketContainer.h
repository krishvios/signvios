#pragma once
#include "SstiMSPacket.h"
#include "stiSVX.h"
#include <vector>

struct MSPacketContainer
{
public:
	uint32_t callIndex = 0;
	std::vector<std::shared_ptr<SstiMSPacket>> videoData;
	EstiVideoCodec codec = EstiVideoCodec::estiVIDEO_NONE; 
	uint32_t discardedFrames = 0;
	uint32_t maxFrameWidth = 0;
	uint32_t maxFrameHeight = 0;
};

