//
//  CstiTestEncoder.cpp
//  gktest
//
//  Created by Dennis Muhlestein on 12/5/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#include "CstiTestEncoder.h"

#include <fstream>

stiHResult CstiTestEncoder::CreateVideoEncoder(CstiTestEncoder **pTestEncoder)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*pTestEncoder = new CstiTestEncoder;
	stiTESTCOND(pTestEncoder != NULL, stiRESULT_MEMORY_ALLOC_ERROR);

STI_BAIL:

	return hResult;
}

stiHResult CstiTestEncoder::AvEncoderClose()
{
  stiHResult hResult=stiRESULT_SUCCESS;
	stiTESTCOND(m_encodedVideo, stiRESULT_ERROR);
	
	delete [] m_encodedVideo;
	m_encodedVideo=NULL;
	m_encodedVideoLen=0;
STI_BAIL:
	return hResult;
}

stiHResult CstiTestEncoder::AvInitializeEncoder()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::ifstream infile("mac.gkdat2", std::ifstream::binary);
	stiTESTCOND(infile.good(), stiRESULT_UNABLE_TO_OPEN_FILE);
	
	infile.seekg(0,std::ios::end);
	m_encodedVideoLen=infile.tellg();
	infile.seekg(0,std::ios::beg);
	
	if (m_encodedVideo)
	{
		delete [] m_encodedVideo;
	}
	m_encodedVideo = new uint8_t[m_encodedVideoLen];
	stiTESTCOND(m_encodedVideo, stiRESULT_MEMORY_ALLOC_ERROR);
	
	infile.read((char*)m_encodedVideo, m_encodedVideoLen);
	infile.close();
	
	m_encodedVideoOffset=0;
	
		
STI_BAIL:
	return hResult;
}



stiHResult CstiTestEncoder::AvVideoCompressFrame(uint8_t* /* pInputFrame UNUSED */,
												FrameData *pOutputFrame)
{

	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_bRequestKeyframe) {
		m_encodedVideoOffset=0;
		m_bRequestKeyframe=false;
	}
	
	if (m_encodedVideoOffset >= m_encodedVideoLen)
	{
		m_encodedVideoOffset=0;
	}
	stiTESTCOND(m_encodedVideoLen-m_encodedVideoOffset>=sizeof(int32_t), stiRESULT_DRIVER_ERROR);


	if (!m_encodedVideoOffset)
	{
		pOutputFrame->SetKeyFrame();
	}

	int32_t frameLen;
	memcpy(&frameLen, m_encodedVideo+m_encodedVideoOffset , sizeof(int32_t));
	m_encodedVideoOffset += sizeof(int32_t);
	stiTESTCOND(frameLen <= m_encodedVideoLen-m_encodedVideoOffset, stiRESULT_DRIVER_ERROR);

	//stiTrace ("Delivering Frame size: %d\n", frameLen);
	while (frameLen>=sizeof(int32_t))
	{
		uint32_t nalLen;
		memcpy(&nalLen, m_encodedVideo+m_encodedVideoOffset, sizeof(int32_t));
		m_encodedVideoOffset+=sizeof(int32_t);
		
		stiTESTCOND(m_encodedVideoLen-m_encodedVideoOffset >= nalLen, stiRESULT_DRIVER_ERROR);
		pOutputFrame->AddFrameData((uint8_t*)&nalLen, sizeof(uint32_t));
		pOutputFrame->AddFrameData(m_encodedVideo+m_encodedVideoOffset, nalLen);
		m_encodedVideoOffset += nalLen;
		frameLen -= sizeof(int32_t) + nalLen;
		//stiTrace ( "NAL: %d frameLen left: %d\n", nalLen, frameLen);
	}
STI_BAIL:
	return hResult;
}
