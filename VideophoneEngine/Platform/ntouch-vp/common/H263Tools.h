#pragma once

#include "stiError.h"
#include "stiDefs.h"

stiHResult H263BitstreamPacketize (
	unsigned char *pSrc,
	uint32_t unSrcSize,
	unsigned char *pDst,
	uint32_t unDstSize,
	int nMaxPacketSize,
	bool bIFrame,
	uint32_t * punNewBitstreamSize);
