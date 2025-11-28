//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved

// Includes
//
#include "stiTrace.h"
#include "CstiVideoInput.h"


// This was set to 3. (one to be encoding into, one to be sending, and one for good measure)
// When an empty frame data is not available we send a key frame.  And when a 1920x1080 key 
// frame is encoded we are more likely to run out of these buffers.  This can cause a deluge
// of key frames.  Increasing to 12 buffers helps this issue.
#define stiVideoINPUT_NUM_FRAME_BUFFERS 12

CstiVideoInput::CstiVideoInput()
:
CstiEventQueue("CstiVideoInput"),
m_un32Width(0),
m_un32Height(0),
m_bKeyFrame(false),
m_bPrivacy(false),
m_bRecording(false),
m_bShutdownMsgReceived(false),
m_eVideoCodec(estiVIDEO_NONE),
m_pVideoEncoder(nullptr),
m_unFrameRate(0),
m_unTargetBitrate(0),
m_unIntraFramerate(0),
m_unLevel(0),
m_unConstraints(0),
m_ePacketization(EstiPacketizationScheme::estiPktUnknown),
m_eProfile(EstiVideoProfile::estiPROFILE_NONE),
m_eTestVideoCodec (EstiVideoCodec::estiVIDEO_NONE)
{
#ifdef ACT_FRAME_RATE_METHOD
	for (int i = 0; i < 3; i++)
	{
		m_fActualFrameRateArray[i] = 30.0;
	}
#endif

	while (m_FrameDataEmpty.size () < stiVideoINPUT_NUM_FRAME_BUFFERS)
	{
		m_FrameDataEmpty.push (new FrameData ());
	}
}

CstiVideoInput::~CstiVideoInput()
{
	if (m_pVideoEncoder)
	{
		delete m_pVideoEncoder;
		m_pVideoEncoder = nullptr;
	}

	while (m_FrameData.size())
	{
		FrameData* pData = m_FrameData.front();
		m_FrameData.pop();
		delete pData;
	}

	while (m_FrameDataEmpty.size())
	{
		FrameData* pData = m_FrameDataEmpty.front();
		m_FrameDataEmpty.pop();
		delete pData;
	}
}


/*! \brief Event Handler for VideoRecordStart
*/
void CstiVideoInput::eventRecordStart()
{
	stiTrace("CstiVideoInput::EventRecordStart\n");
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> threadSafe(m_EncoderMutex);

#ifdef ACT_FRAME_RATE_METHOD
	m_un32FrameCount = 0;
	m_dStartFrameTime = 0.0;
#endif

	if (m_pVideoEncoder)
	{
		if (m_pVideoEncoder->AvEncoderCodecGet () != m_eVideoCodec)
		{
			m_pVideoEncoder->AvEncoderClose ();
			delete m_pVideoEncoder;
			m_pVideoEncoder = nullptr;
			hResult = IstiVideoEncoder::CreateVideoEncoder (&m_pVideoEncoder, m_eVideoCodec);
			stiTESTRESULT ();
		}
	}
	else
	{
		hResult = IstiVideoEncoder::CreateVideoEncoder (&m_pVideoEncoder, m_eVideoCodec);
		stiTESTRESULT ();
	}

	m_bRecording = true;
	
	UpdateEncoder ();

STI_BAIL:;
}

bool CstiVideoInput::IsRecording()
{
	return m_bRecording;
}

bool CstiVideoInput::AllowsFUPackets()
{
	return estiH264_NON_INTERLEAVED == m_ePacketization || estiH264_INTERLEAVED == m_ePacketization;
}

/*! \brief Event Handler for VideoRecordStop
*/
void CstiVideoInput::eventRecordStop()
{
	stiTrace("CstiVideoInput::eventRecordStop\n");
	std::lock_guard<std::recursive_mutex> threadSafe(m_EncoderMutex);
	m_bRecording = false;
	//--------Flush Frames----------
	while (m_FrameData.size())
	{
		FrameData *pData = m_FrameData.front();
		m_FrameData.pop();
		pData->Reset();
		m_FrameDataEmpty.push(pData);
	}

	if (m_pVideoEncoder)
	{
		m_pVideoEncoder->AvEncoderClose ();
		delete m_pVideoEncoder;
		m_pVideoEncoder = nullptr;
	}
}

void CstiVideoInput::SendVideoFrame(UINT8* pBytes, INT32 nDataLength, UINT32 nWidth, UINT32 nHeight, EstiVideoCodec eCodec)
{
	if (nWidth != m_un32Width || nHeight != m_un32Height || !m_bRecording)
	{
		return;
	}

	FrameData *pData = nullptr;
	{
		// scoped threadsafe
		std::lock_guard<std::recursive_mutex> threadSafe(m_FrameDataMutex);
		if (m_FrameDataEmpty.empty())
		{
			pData = nullptr;
		}
		else
		{
			pData = m_FrameDataEmpty.front();
			pData->Reset();
			m_FrameDataEmpty.pop();
		}
	}

	if (pData)
	{
		int previousSize = 4;
		if (pBytes[2] == 0x01)
		{
			int previousSize = 3;
		}

		int StartNAL = 0;
		for (int i = previousSize; i < nDataLength - 4;)
		{
			if (pBytes[i] == 0x00)
			{
				if (pBytes[i + 1] == 0x00)
				{
					if (pBytes[i + 2] == 0x00)
					{
						if (pBytes[i + 3] == 0x01)
						{
							int32_t length = (i - StartNAL) - previousSize;
							uint32_t SliceType = (pBytes[StartNAL + previousSize] & 0x1F);
							stiTrace("Slice Type: %#x\n", SliceType);
							switch (SliceType)
							{
							case 0x5:
								pData->SetKeyFrame();
							case 0x7:
							case 0x8:
								pData->AddFrameData((uint8_t*)(&length), sizeof(uint32_t));
								pData->AddFrameData(&pBytes[StartNAL + previousSize], length);
								break;
							}
							StartNAL = i;
							//+---------------+
							//|0|1|2|3|4|5|6|7|
							//+-+-+-+-+-+-+-+-+
							//|F|NRI|   Type  |
							//+---------------+
							previousSize = sizeof(uint32_t);
							i += 5; //We are skipping 8 bits here because of the nal unit header

						}
						else
						{
							i += 4;
						}
					}
					else if (pBytes[i + 2] == 0x01)
					{
						int32_t length = (i - StartNAL) - previousSize;
						uint32_t SliceType = (pBytes[StartNAL + previousSize] & 0x1F);
						stiTrace("Slice Type: %#x\n", SliceType);
						switch (SliceType)
						{
						case 0x5:
							pData->SetKeyFrame();
						case 0x7:
						case 0x8:
							pData->AddFrameData((uint8_t*)(&length), sizeof(uint32_t));
							pData->AddFrameData(&pBytes[StartNAL + previousSize], length);
							break;
						}
						StartNAL = i;
						//+---------------+
						//|0|1|2|3|4|5|6|7|
						//+-+-+-+-+-+-+-+-+
						//|F|NRI|   Type  |
						//+---------------+
						previousSize = 3;
						i += 4; //We are skipping 8 bits here because of the nal unit header
					}
					else
					{
						i += 3;
					}
				}
				else
				{
					i += 2;
				}
			}
			else
			{
				i++;
			}
		}
		//Set the last nal unit
		{
			int32_t length = (nDataLength - StartNAL) - previousSize;
			uint32_t SliceType = (pBytes[StartNAL + previousSize] & 0x1F);
			stiTrace("Slice Type: %#x\n", SliceType);
			switch (SliceType)
			{
			case 0x5:
				pData->SetKeyFrame();
			case 0x7:
			case 0x8:
				pData->AddFrameData((uint8_t*)(&length), sizeof(uint32_t));
				pData->AddFrameData(&pBytes[StartNAL + previousSize], length);
				break;
			}
		}

		//pData->AddFrameData(pBytes, nDataLength);
		{
			//Scoped FrameDataMutex
			std::lock_guard<std::recursive_mutex> threadSafe(m_FrameDataMutex);
			m_FrameData.push(pData);
		}

		//Notify CstiVideoRecord that a packet is ready.
		PostEvent ([this]
			{
				packetAvailableSignal.Emit ();
			});
	}
}

void CstiVideoInput::SendVideoFrame(UINT8* pBytes, UINT32 nWidth, UINT32 nHeight, EstiColorSpace eColorSpace)
{
	// go ahead and lock encoder mutex until the message is ready to send
	// otherwise it's possible to obtain an image from the input queue
	// that doesn't match the encoder settings (width/height or other)
	std::lock_guard<std::recursive_mutex> threadSafe(m_EncoderMutex);
	if (nWidth == m_un32Width && nHeight == m_un32Height && m_bRecording)
	{
		stiHResult hResult = stiRESULT_SUCCESS;
		FrameData *pData = nullptr;
		{// scoped threadsafe
			
			std::lock_guard<std::recursive_mutex> threadSafe(m_FrameDataMutex);

			if (m_FrameDataEmpty.empty())
			{
				pData = nullptr;
			}
			else
			{
				pData = m_FrameDataEmpty.front();
				pData->Reset();
				m_FrameDataEmpty.pop();
			}
		}

		if (pData)
		{
			// Compress the frame.
			hResult = m_pVideoEncoder->AvVideoCompressFrame(pBytes, pData, eColorSpace);
			if (stiIS_ERROR(hResult))
			{
				std::lock_guard<std::recursive_mutex> threadSafe(m_FrameDataMutex);
				stiTrace("FAILED: m_pVideoEncoder->AvVideoCompressFrame(m_pInputBuffer,	pData) Err %d\n", hResult);
				m_FrameDataEmpty.push(pData);
				pData = nullptr;
			}
			else
			{
				{// scoped threadsafe
					std::lock_guard<std::recursive_mutex> threadSafe(m_FrameDataMutex);
					m_FrameData.push(pData);
				}
				
				//Notify CstiVideoRecord that a packet is ready.
				PostEvent ([this]
					{
						packetAvailableSignal.Emit ();
					});


#ifdef ACT_FRAME_RATE_METHOD
				EncoderActualFrameRateCalcAndSet();
#endif
			}
		}
	}
}

stiHResult CstiVideoInput::KeyFrameRequest()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::lock_guard<std::recursive_mutex> threadSafe(m_EncoderMutex);
	if (m_pVideoEncoder)
	{
		m_pVideoEncoder->AvVideoRequestKeyFrame();
	}
	
	return hResult;
}

stiHResult CstiVideoInput::PrivacyGet(EstiBool *pbEnabled) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	*pbEnabled = m_bPrivacy ? estiTRUE:estiFALSE;
	return hResult;
}

stiHResult CstiVideoInput::PrivacySet(EstiBool bEnable)
{
	if ((bool)bEnable != m_bPrivacy)
	{
		m_bPrivacy = bEnable;
		ntouchPC::CstiNativeVideoLink::LocalVideoPrivacy (bEnable);
		videoPrivacySetSignal.Emit (bEnable);
	}

	return stiRESULT_SUCCESS;
}


#ifdef ACT_FRAME_RATE_METHOD
/*! \brief	Called each time a frame is ready, calculates the actual frame rate the camera is delivering and sets
*			the actual frame rate member variable in CstiVideoEncoder. This is used as a ratio with the target
*			frame rate to calculate bit rate in AvUpdateEncoderSettings() which is called from AvInitializeEncoder().
*/
void CstiVideoInput::EncoderActualFrameRateCalcAndSet()
{
	std::lock_guard<std::recursive_mutex> threadSafe(m_EncoderMutex);

	timeval ts;

	m_un32FrameCount++;
	if (m_un32FrameCount % 30 == 0)
	{
		gettimeofday(&ts, nullptr);
		double dCurrentTime = (ts.tv_sec & 0xFFFF) /* 16 bits only */ + (double)ts.tv_usec / 1e6;
		if (m_dStartFrameTime != 0.0)
		{
			float fFrameRateAverage = 0.0;
			for (int i = 2; i>0; i--)
			{
				m_fActualFrameRateArray[i] = m_fActualFrameRateArray[i - 1];
				fFrameRateAverage += m_fActualFrameRateArray[i];
			}
			m_fActualFrameRateArray[0] = 30.0f / static_cast<float>(dCurrentTime - m_dStartFrameTime);
			fFrameRateAverage += m_fActualFrameRateArray[0];
			fFrameRateAverage /= 3.0;

			float fRateDiff = (fFrameRateAverage > m_fCurrentEncoderRate) ?
				fFrameRateAverage - m_fCurrentEncoderRate : m_fCurrentEncoderRate - fFrameRateAverage;
			if (fRateDiff >= 1.5)
			{
				m_fCurrentEncoderRate = fFrameRateAverage;
				if (m_pVideoEncoder)
				{
					m_pVideoEncoder->AvVideoActualFrameRateSet(m_fCurrentEncoderRate);
					m_pVideoEncoder->AvInitializeEncoder();
				}
			}
		}

		m_dStartFrameTime = dCurrentTime;
	}
}
#endif //ACT_FRAME_RATE_METHOD


stiHResult CstiVideoInput::VideoRecordFrameGet(SstiRecordVideoFrame *pVideoPacket)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock(m_EncoderMutex, m_FrameDataMutex);
	std::lock_guard<std::recursive_mutex> encoderLock(m_EncoderMutex, std::adopt_lock);
	std::lock_guard<std::recursive_mutex> threadSafe(m_FrameDataMutex, std::adopt_lock);

	FrameData *pData = nullptr;
	stiTESTCOND(m_FrameData.size() > 0, stiRESULT_DRIVER_ERROR);
	pData = m_FrameData.front();
	stiTESTCOND(pVideoPacket != nullptr, stiRESULT_BUFFER_TOO_SMALL);
	stiTESTCOND(pVideoPacket->buffer != nullptr, stiRESULT_BUFFER_TOO_SMALL);
	stiTESTCOND(pVideoPacket->unBufferMaxSize >= pData->GetDataLen(), stiRESULT_BUFFER_TOO_SMALL);

	pData->GetFrameData(pVideoPacket->buffer);
	pVideoPacket->frameSize = pData->GetDataLen();
	pVideoPacket->keyFrame = pData->IsKeyFrame() ? estiTRUE : estiFALSE;

	if (m_pVideoEncoder)
	{
		pVideoPacket->eFormat = m_pVideoEncoder->AvFrameFormatGet();
	}
	else
	{
		pVideoPacket->eFormat = estiLITTLE_ENDIAN_PACKED;
	}
	

STI_BAIL:
	if (pData)
	{
		m_FrameData.pop();
		m_FrameDataEmpty.push(pData);
	}
	return hResult;
}

void CstiVideoInput::UpdateEncoder()
{
	if (m_pVideoEncoder)
	{
		m_pVideoEncoder->AvVideoFrameSizeSet(m_un32Width, m_un32Height);

		m_pVideoEncoder->AvVideoBitrateSet(m_unTargetBitrate);

		m_pVideoEncoder->AvVideoFrameRateSet((float)m_unFrameRate);

		m_pVideoEncoder->AvVideoIFrameSet(m_unIntraFramerate);

		m_pVideoEncoder->AvVideoProfileSet(m_eProfile, m_unLevel, m_unConstraints);

		m_pVideoEncoder->AvInitializeEncoder();
		
	}
}

stiHResult CstiVideoInput::VideoRecordSettingsSet(const SstiVideoRecordSettings *videoRecordSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTrace("VideoRecordSettingsSet\n");

	std::lock_guard<std::recursive_mutex> threadSafe(m_EncoderMutex);
	m_un32Width = videoRecordSettings->unColumns;
	m_un32Height = videoRecordSettings->unRows;

	//Inform NtouchPC of the new capture size
	ntouchPC::CstiNativeVideoLink::CaptureSize(m_un32Width, m_un32Height);
	m_ePacketization = videoRecordSettings->ePacketization;
	m_unFrameRate = videoRecordSettings->unTargetFrameRate;
	m_unTargetBitrate = videoRecordSettings->unTargetBitRate;
	m_unIntraFramerate = videoRecordSettings->unIntraFrameRate;
	m_eProfile = videoRecordSettings->eProfile;
	m_unLevel = videoRecordSettings->unLevel;
	m_unConstraints = videoRecordSettings->unConstraints;
	if (estiVIDEO_H264 == videoRecordSettings->codec ||
		estiVIDEO_H265 == videoRecordSettings->codec)
	{
		m_eVideoCodec = videoRecordSettings->codec;
	}
	else
	{
		m_eVideoCodec = estiVIDEO_NONE;
	}

	if (m_pVideoEncoder != nullptr)
	{
		if (m_pVideoEncoder->AvEncoderCodecGet () != m_eVideoCodec)
		{
			m_pVideoEncoder->AvEncoderClose ();
			delete m_pVideoEncoder;
			m_pVideoEncoder = nullptr;
			hResult = IstiVideoEncoder::CreateVideoEncoder (&m_pVideoEncoder, m_eVideoCodec);
			stiTESTRESULT ();
		}
		UpdateEncoder ();
	}



STI_BAIL:

	return hResult;
}

stiHResult CstiVideoInput::VideoRecordStart()
{	
	PostEvent ([this]
		{
			eventRecordStart ();
		});

	return stiRESULT_SUCCESS;
}

stiHResult CstiVideoInput::VideoRecordStop()
{
	PostEvent ([this]
		{
			eventRecordStop ();
		});

	return stiRESULT_SUCCESS;
}

/*!
* \brief Get video codec
*
* \param pCodecs - a std::list where the codecs will be stored.
*
* \return stiHResult
*/
stiHResult CstiVideoInput::VideoCodecsGet(std::list<EstiVideoCodec> *pCodecs)
{
	if (estiVIDEO_NONE == m_eTestVideoCodec)
	{
		// Add the supported video codecs to the list
#ifdef stiENABLE_H265_ENCODE
		WillowPM::PropertyManager *pm = WillowPM::PropertyManager::getInstance ();
		int nEnableH265 = NULL;
		pm->propertyGet ("EnableH265", &nEnableH265, WillowPM::PropertyManager::Persistent);
		if (nEnableH265 == 1)
		{
			pCodecs->push_back (estiVIDEO_H265);
		}
#endif
		pCodecs->push_back (estiVIDEO_H264);
	}
	else
	{
		pCodecs->push_back (m_eTestVideoCodec);
	}
	return stiRESULT_SUCCESS;
}


stiHResult CstiVideoInput::testCodecSet (EstiVideoCodec codec)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	m_eTestVideoCodec = codec;
	return hResult;
}

/*!
* \brief Get Codec Capability
*
* \param eCodec
* \param pstCaps
*
* \return stiHResult
*/
stiHResult CstiVideoInput::CodecCapabilitiesGet(EstiVideoCodec eCodec,	SstiVideoCapabilities *pstCaps)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiProfileAndConstraint stProfileAndConstraint;

	switch (eCodec)
	{
	case estiVIDEO_H264:
	{

		SstiH264Capabilities *pstH264Caps = (SstiH264Capabilities*)pstCaps;

		//Extended is not a profile in FFMPEG
		//stProfileAndConstraint.eProfile = estiH264_EXTENDED;
		//stProfileAndConstraint.un8Constraints = 0xf0;
		//pstH264Caps->Profiles.push_back(stProfileAndConstraint);

		stProfileAndConstraint.eProfile = estiH264_MAIN;
		stProfileAndConstraint.un8Constraints = 0xe0;
		pstH264Caps->Profiles.push_back(stProfileAndConstraint);

		stProfileAndConstraint.eProfile = estiH264_BASELINE;
		stProfileAndConstraint.un8Constraints = 0xa0;
		pstH264Caps->Profiles.push_back(stProfileAndConstraint);

		pstH264Caps->eLevel = estiH264_LEVEL_4_1;

		break;
	}

	case estiVIDEO_H265:
	{
		if (m_bH265Enabled)
		{
			SstiH265Capabilities *pstH265Caps = (SstiH265Capabilities*)pstCaps;

			stProfileAndConstraint.eProfile = estiH265_PROFILE_MAIN;
			stProfileAndConstraint.un8Constraints = 0x00;
			pstH265Caps->Profiles.push_back(stProfileAndConstraint);
			pstH265Caps->eLevel = estiH265_LEVEL_4;
			pstH265Caps->eTier = estiH265_TIER_MAIN;
		}
		break;
	}

	default:
		hResult = stiRESULT_ERROR;
	}

	return hResult;
}

stiHResult CstiVideoInput::CaptureCapabilitiesChanged ()
{
	captureCapabilitiesChangedSignal.Emit ();
	return stiRESULT_SUCCESS;
}
