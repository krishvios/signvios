#include "CstiConverterBase.h"

CstiConverterBase::CstiConverterBase()
{
	m_nWidth = (0);
	m_nHeight = (0);
	m_nEncodeWidth = (0);
	m_nEncodeHeight = (0);
	m_nFrameRate = (0);
	m_nBitrate = (0);
	m_pBytes = (NULL);
	m_pWaterMark = NULL;
	m_pEncodeBuffer = NULL;
	m_pContext = NULL;
	m_pDecodeCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
	m_pVideoDecodeContext = avcodec_alloc_context3(m_pDecodeCodec);
	m_pVideoDecodeContext->err_recognition = true;
	m_pVideoDecodeContext->flags |= CODEC_FLAG_PSNR;
	m_pVideoDecodeContext->skip_frame = AVDISCARD_NONREF;
	AVDictionary *pOptions = NULL;
	while(avcodec_open2(m_pVideoDecodeContext, m_pDecodeCodec, &pOptions) < 0)
	{
		//stiTrace("Failure to Initialize Decoder Converter Base\n");
		Sleep(10);
	}


	m_pFrame = av_frame_alloc ();
	m_pFrameStorage = new FrameData();

	m_UsageLock = stiOSMutexCreate();
}

void CstiConverterBase::RequestKeyFrame()
{
	if (m_pVideoEncoder)
	{
		m_pVideoEncoder->AvVideoRequestKeyFrame();
	}
}

stiHResult CstiConverterBase::ResetEncoder(uint32_t EncodeWidth, uint32_t EncodeHeight, uint32_t Bitrate, uint32_t Framerate)
{
	if (m_nEncodeWidth != EncodeWidth || m_nEncodeHeight != EncodeHeight)
	{
		m_nEncodeWidth = EncodeWidth;
		m_nEncodeHeight = EncodeHeight;
		m_nBitrate = Bitrate;
		m_nFrameRate = Framerate;
		if (m_pEncodeBuffer)
		{
			delete m_pEncodeBuffer;
		}
		m_pEncodeBuffer = new uint8_t[m_nEncodeWidth * m_nEncodeHeight * 2];

		m_pVideoEncoder->AvVideoFrameRateSet((float)m_nFrameRate);
		m_pVideoEncoder->AvVideoFrameSizeSet(m_nEncodeWidth, m_nEncodeHeight);
		m_pVideoEncoder->AvVideoBitrateSet(m_nBitrate);
		while (m_pVideoEncoder->AvInitializeEncoder() != stiRESULT_SUCCESS)
		{
			stiTrace("Failure to Initialize Encoder Reset");
			Sleep(10);
		}
	}
	//------------Set to Black--------------------
	int YSize = (m_nEncodeWidth * m_nEncodeHeight);
	int USize = (m_nEncodeWidth * m_nEncodeHeight / 4);
	ZeroMemory(m_pEncodeBuffer, sizeof(uint8_t)* YSize);
	for (int i = 0; i < USize; i++)
	{
		m_pEncodeBuffer[YSize + i] = 128;
		m_pEncodeBuffer[YSize + USize + i] = 128;
	}
	m_pVideoEncoder->AvVideoRequestKeyFrame();
	return stiRESULT_SUCCESS;
}


CstiConverterBase::~CstiConverterBase()
{
	m_pVideoEncoder->AvEncoderClose();
	delete(m_pVideoEncoder);
	avcodec_close(m_pVideoDecodeContext);
	av_free(m_pDecodeCodec);
	av_free(m_pVideoDecodeContext);
	m_pFrameStorage->Reset();
	delete(m_pFrameStorage);
	delete(m_pBytes);
	delete(m_pEncodeBuffer);
	//av_free(m_pFrame->data);
	av_free(m_pFrame);
	if (m_pContext != NULL)
	{
		sws_freeContext(m_pContext);
		m_pContext = NULL;
	}
	stiOSMutexDestroy(m_UsageLock);
}

void CstiConverterBase::ApplyWaterMark(uint8_t* pBuffer)
{
	if (m_pWaterMark)
	{
		for (int j = 0; j < m_nEncodeHeight; j++)
		{
			for (int i = 0; i < m_nEncodeWidth; i++)
			{
				double R = m_pWaterMark[(j * m_nEncodeWidth * 4) + (i * 4) + 0] / 255.0;
				double G = m_pWaterMark[(j * m_nEncodeWidth * 4) + (i * 4) + 1] / 255.0;
				double B = m_pWaterMark[(j * m_nEncodeWidth * 4) + (i * 4) + 2] / 255.0;
				double A = m_pWaterMark[(j * m_nEncodeWidth * 4) + (i * 4) + 3] / 255.0;
				if (A != 0)
				{
					int YIndex = (j * m_nEncodeWidth) + i;
					int YLength = (m_nEncodeHeight * m_nEncodeWidth);
					int Offset = ((j / 2) * (m_nEncodeWidth / 2) + (i / 2));
					int UIndex = YLength + Offset;
					int ULength = (m_nEncodeHeight / 2 * m_nEncodeWidth / 2);
					int VIndex = YLength + ULength + Offset;
					pBuffer[YIndex] = (1.0 - A) * pBuffer[YIndex] + (A * 255 * R);
					pBuffer[UIndex] = (1.0 - A) * pBuffer[UIndex] + (A * 128);
					pBuffer[VIndex] = (1.0 - A) * pBuffer[VIndex] + (A * 128);
				}
			}
		}
	}
}

int CstiConverterBase::Clamp(int Value, int Max, int Min)
{
	if (Value > Max)
	{
		return Max;
	}
	else if (Value < Min)
	{
		return Min;
	}
	else
	{
		return Value;
	}
}

void CstiConverterBase::AddWaterMark(uint8_t* pWaterMark)
{
	m_pWaterMark = pWaterMark;
}

VideoPacket* CstiConverterBase::EncodeWaterMark()
{
	m_pFrameStorage->Reset(); //We are reseting it so that we only have this frame's information.
	ApplyWaterMark(m_pEncodeBuffer);
	stiHResult hResult = m_pVideoEncoder->AvVideoCompressFrame(m_pEncodeBuffer, m_pFrameStorage, estiCOLOR_SPACE_I420);
	uint32_t packetlength = m_pFrameStorage->GetDataLen();

	uint8_t* pBuffer = new uint8_t[packetlength];

	m_pFrameStorage->GetFrameData(pBuffer);

	VideoPacket* packetOut = new VideoPacket();
	packetOut->Data = pBuffer;
	packetOut->Length = packetlength;
	packetOut->IsKeyFrame = m_pFrameStorage->IsKeyFrame();
	packetOut->EndianFormat = m_eVideoFormat;
	return packetOut;
}

VideoPacket* CstiConverterBase::DecodeFrame(unsigned char* buffer, int length)
{
	AVPacket packetIn = { 0 };
	av_init_packet(&packetIn);
	packetIn.data = (uint8_t*)buffer;
	packetIn.size = length;
	int got_picture;
	int result = avcodec_decode_video2(m_pVideoDecodeContext, m_pFrame, &got_picture, &packetIn);
	av_free_packet(&packetIn);
	bool IsKeyFrame = m_pFrame->key_frame == 1;
	if (m_nWidth == 0 && !IsKeyFrame) //We haven't initialized and we don't have a keyframe so don't return;
	{
		return NULL;
	}

	if (result <= 0 || !got_picture)
	{
		return NULL;
	}
	double RatioW = (m_nEncodeWidth / (double)m_pFrame->width);
	double RatioH = (m_nEncodeHeight / (double)m_pFrame->height);
	uint32_t nEncodeWidth;
	uint32_t nEncodeHeight;
	if (RatioW == RatioH)
	{
		nEncodeWidth = m_nEncodeWidth;
		nEncodeHeight = m_nEncodeHeight;
	}
	else if (RatioW > RatioH)
	{
		nEncodeWidth = m_pFrame->width * RatioH;
		nEncodeHeight = m_nEncodeHeight;
	}
	else
	{
		nEncodeWidth = m_nEncodeWidth;
		nEncodeHeight = m_pFrame->height * RatioW;
	}

	if (nEncodeWidth <= 0 || nEncodeHeight <= 0)
	{
		return NULL;
	}
	//----------------------------------------------
	AllocateFrame(nEncodeWidth, nEncodeHeight);
	//----------------------------------------------

	// Set up the pointers the way ffmpeg likes them for the scaling routine
	// First, the destination data pointers, one for Y, one for VUs
	UINT8* dst[3];
	dst[0] = m_pBytes;				   // Y plane
	dst[1] = dst[0] + (nEncodeWidth * nEncodeHeight);  // Us
	dst[2] = dst[1] + (nEncodeWidth * nEncodeHeight / 4);//m_pFrameBuffer + ((width * height) / 4);  // Vs


	// The destination 'stride' (width in bytes) of each line of data
	int dstStride[3];
	dstStride[0] = nEncodeWidth; // One Y for each pixel
	dstStride[1] = nEncodeWidth / 2; // One U for each two Ys
	dstStride[2] = nEncodeWidth / 2; // One V for each two Ys

	m_pContext = sws_getCachedContext(m_pContext, m_pFrame->width, m_pFrame->height, AVPixelFormat::AV_PIX_FMT_YUV420P, nEncodeWidth, nEncodeHeight, AVPixelFormat::AV_PIX_FMT_YUV420P, SWS_POINT, NULL, NULL, NULL);

	sws_scale(m_pContext, m_pFrame->data, m_pFrame->linesize, 0, m_pFrame->height, dst, dstStride);

	m_pFrameStorage->Reset(); //We are reseting it so that we only have this frame's information.
	stiHResult hResult = stiRESULT_SUCCESS;
	if (RatioW == RatioH)
	{
		ApplyWaterMark(m_pBytes);
		hResult = m_pVideoEncoder->AvVideoCompressFrame(m_pBytes, m_pFrameStorage, estiCOLOR_SPACE_I420);
	}
	else
	{
		//Calculate the Padding
		uint32_t PaddingV = (m_nEncodeHeight - nEncodeHeight) / 2;
		uint32_t PaddingH = (m_nEncodeWidth - nEncodeWidth) / 2;
		//Copy the Y Values
		int j = 0;
		for (int i = PaddingV; i < (m_nEncodeHeight - PaddingV); i++)
		{
			uint8_t* dstAddress = &m_pEncodeBuffer[i * m_nEncodeWidth];
			uint8_t* srcAddress = &m_pBytes[j * nEncodeWidth];
			memcpy(dstAddress, srcAddress, nEncodeWidth);
			j++;
		}
		//Copy the U and V values
		j = 0;
		for (int i = (PaddingV / 2); i < (m_nEncodeHeight - PaddingV) / 2; i++)
		{
			int YSrcSize = (nEncodeWidth * nEncodeHeight);
			int USrcSize = YSrcSize / 4;
			int YDstSize = (m_nEncodeWidth * m_nEncodeHeight);
			int UDstSize = YDstSize / 4;
			//------------------------
			uint8_t* srcAddress = &m_pBytes[YSrcSize + (j * nEncodeWidth / 2)];
			uint8_t* dstAddress = &m_pEncodeBuffer[YDstSize + (i * m_nEncodeWidth / 2)];
			memcpy(dstAddress, srcAddress, nEncodeWidth / 2);
			//------------------------
			srcAddress = &m_pBytes[YSrcSize + USrcSize + (j * nEncodeWidth / 2)];
			dstAddress = &m_pEncodeBuffer[YDstSize + UDstSize + (i * m_nEncodeWidth / 2)];
			memcpy(dstAddress, srcAddress, nEncodeWidth / 2);
			j++;
		}
		ApplyWaterMark(m_pEncodeBuffer);
		hResult = m_pVideoEncoder->AvVideoCompressFrame(m_pEncodeBuffer, m_pFrameStorage, estiCOLOR_SPACE_I420);
	}

	if (stiIS_ERROR(hResult))
	{
		return NULL;
	}
	else
	{
		uint32_t packetlength = m_pFrameStorage->GetDataLen();

		uint8_t* pBuffer = new uint8_t[packetlength];

		m_pFrameStorage->GetFrameData(pBuffer);

		VideoPacket* packetOut = new VideoPacket();
		packetOut->Data = pBuffer;
		packetOut->Length = packetlength;
		packetOut->IsKeyFrame = m_pFrameStorage->IsKeyFrame();
		packetOut->EndianFormat = m_eVideoFormat;
		return packetOut;
	}
}

void CstiConverterBase::AllocateFrame(uint32_t Width, uint32_t Height)
{
	if (m_nWidth != Width && m_nHeight != Height)
	{
		m_nWidth = Width;
		m_nHeight = Height;
		size_t ySize = m_nWidth * m_nHeight;
		size_t frameSize = ySize + (ySize / 2);
		if (m_pBytes)
		{
			delete(m_pBytes);
		}
		m_pBytes = new uint8_t[frameSize];
	}
}
