#include "CstiConvertToH264.h"


CstiConvertToH264::CstiConvertToH264(uint32_t nFrameRate) :
m_nWidth(0),
m_nHeight(0),
m_pBytes(NULL)
{
	m_pDecodeCodec = avcodec_find_decoder(AV_CODEC_ID_H263);
	m_pVideoDecodeContext = avcodec_alloc_context3(m_pDecodeCodec);
	m_pVideoDecodeContext->err_recognition = true;
	m_pVideoDecodeContext->flags |= CODEC_FLAG_PSNR;
	m_pVideoDecodeContext->skip_frame = AVDISCARD_NONREF;
	AVDictionary *pOptions = NULL;
	while (avcodec_open2(m_pVideoDecodeContext, m_pDecodeCodec, &pOptions) < 0)
	{
		stiTrace("Failure to Initialize Decoder Convert H264");
		Sleep(10);
	}
	
	m_pFrame = av_frame_alloc();
	//---------------------------------------
	IstiVideoEncoder* Encoder;
	IstiVideoEncoder::CreateVideoEncoder(&Encoder, estiVIDEO_H264);
	m_pVideoEncoder = Encoder;
	m_pVideoEncoder->AvVideoBitrateSet(256 * 1024);
	m_pVideoEncoder->AvVideoFrameRateSet(nFrameRate);
	m_pVideoEncoder->AvVideoIFrameSet(120);
	m_pVideoEncoder->AvVideoFrameSizeSet(352, 288); // just a guess
	m_pVideoEncoder->AvVideoRequestKeyFrame();
	while (m_pVideoEncoder->AvInitializeEncoder() != stiRESULT_SUCCESS)
	{
		stiTrace("Failure to Initialize Encoder Convert H264");
		Sleep(10);
	}
	//---------------------------------------
	m_pFrameStorage = new FrameData();
}
CstiConvertToH264::~CstiConvertToH264()
{
	m_pVideoEncoder->AvEncoderClose();
	delete(m_pVideoEncoder);
	avcodec_close(m_pVideoDecodeContext);
	av_free(m_pDecodeCodec);
	av_free(m_pVideoDecodeContext);
	delete(m_pFrameStorage);
	delete(m_pBytes);

	//av_free(m_pFrame->data);
	av_free(m_pFrame);
}



SstiMSPacket* CstiConvertToH264::DecodeFrame(unsigned char* buffer, int length)
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
		m_pVideoEncoder->AvVideoRequestKeyFrame();
		return NULL;
	}

	if (result < 0 || !got_picture)
	{
		m_pVideoEncoder->AvVideoRequestKeyFrame();
		return NULL;
	}
	//----------------------------------------------
	AllocateFrame(m_pFrame->width, m_pFrame->height);
	//----------------------------------------------
	size_t ySize = m_nWidth * m_nHeight;
	size_t uvSize = ySize / 4;
	size_t frameSize = ySize + uvSize * 2;

	uint8_t * src[3] = { m_pFrame->data[0], m_pFrame->data[1], m_pFrame->data[2] };
	uint8_t * dst[3] = { m_pBytes, dst[0] + ySize, dst[1] + uvSize };
	size_t dstLineSize[3] = { m_pFrame->width, m_pFrame->width / 2, m_pFrame->width / 2 };

	AVPixelFormat format = (AVPixelFormat)m_pFrame->format;
	
	for (int y = 0; y < m_nHeight; y++)
	{
		for (int plane = 0; plane < 3; plane++) 
		{
			if ((y & 1) == 0 || plane == 0) 
			{
				memcpy(dst[plane], src[plane], dstLineSize[plane]);
				src[plane] += m_pFrame->linesize[plane];
				dst[plane] += dstLineSize[plane];
			}
		}
	}
	
	stiHResult hResult = m_pVideoEncoder->AvVideoCompressFrame(m_pBytes, m_pFrameStorage, estiCOLOR_SPACE_I420);
	if (stiIS_ERROR(hResult))
	{
		return NULL;
	}

	uint32_t packetlength = m_pFrameStorage->GetDataLen();

	uint8_t * pBuffer = new uint8_t[packetlength];

	m_pFrameStorage->GetFrameData(pBuffer);
	m_pFrameStorage->Reset();
	
	uint32_t un32SliceSize;
	uint32_t un8SliceType;
	bool bKeyFrame = false;
	for (int i = 0; i < packetlength;)
	{
		un32SliceSize = *(uint32_t*)&pBuffer[i];
		un8SliceType = (pBuffer[i + sizeof(uint32_t)] & 0x1F);
		if (un8SliceType == SLICE_TYPE_IDR)
		{
			bKeyFrame = true;
		}

		pBuffer[i + 0] = 0x0;
		pBuffer[i + 1] = 0x0;
		pBuffer[i + 2] = 0x0;
		pBuffer[i + 3] = 0x1;
		i += sizeof(uint32_t)+un32SliceSize;
	}


	//This packet will be deleted later.
	SstiMSPacket* packetOut = new SstiMSPacket();
	packetOut->buffer = pBuffer;
	packetOut->length = packetlength;
	packetOut->keyFrame = bKeyFrame;

	return packetOut;
}

void CstiConvertToH264::AllocateFrame(uint32_t Width, uint32_t Height)
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
		m_pVideoEncoder->AvVideoFrameSizeSet(m_nWidth, m_nHeight);
		m_pVideoEncoder->AvInitializeEncoder();
	}
}
