// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2022 Sorenson Communications, Inc. -- All rights reserved

#include "CstiDisplay.h"
#include <stdio.h>

#define stiDISPLAY_MAX_MESSAGES_IN_QUEUE 24
#define stiDISPLAY_MAX_MSG_SIZE 512
#define stiDISPLAY_TASK_NAME "CstiDisplayTask"
#define stiDISPLAY_TASK_PRIORITY 151
#define stiDISPLAY_STACK_SIZE 4096

#define SLICE_TYPE_NON_IDR	1
#define SLICE_TYPE_IDR		5

#define	MAX_FRAME_BUFFERS	6

#define DEFAULT_DECODE_WIDTH 1920
#define DEFAULT_DECODE_HEIGHT 1080

//#define WRITE_BITSTREAM_TO_FILE

CstiDisplay::CstiDisplay ()
	:
	m_fdSignal (NULL),
	m_eVideoCodec (estiVIDEO_H264),
	m_unFileCounter(0),
	m_pOutputFile (NULL),
	m_bCECSupported (false),
	m_bTelevisionPowered (false)
{
}


CstiDisplay::~CstiDisplay ()
{
	//
	// Free the frame buffers
	//
	while (!m_FrameList.empty ())
	{
		CstiVideoPlaybackFrame *pFrame = m_FrameList.front ();
		m_FrameList.pop_front ();

		delete pFrame;
		pFrame = NULL;
	}

	// close Signal 
	if (m_fdSignal)
	{
		stiOSSignalClose (&m_fdSignal);
		m_fdSignal = NULL;
	}
	m_bTelevisionPowered = false;
	m_bCECSupported = false;
}


stiHResult CstiDisplay::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (NULL == m_fdSignal)
	{
		EstiResult eResult = stiOSSignalCreate (&m_fdSignal);
		stiTESTCOND (estiOK == eResult, stiRESULT_TASK_INIT_FAILED);
	}
	
	for (int i = 0; i < MAX_FRAME_BUFFERS; ++i)
	{
		CstiVideoPlaybackFrame *pFrame = new CstiVideoPlaybackFrame;

		m_FrameList.push_back (pFrame);

		//
		// For each frame set the signal.  This is cumulative.  A clear
		// will need to be used for each set to completely clear the signal
		//
		stiOSSignalSet (m_fdSignal);
	}
	CECStatusCheck();

STI_BAIL:

	return hResult;
}

stiHResult CstiDisplay::DisplayModeCapabilitiesGet (
	uint32_t *pun32CapBitMask)
{
	*pun32CapBitMask = 0;

	return stiRESULT_SUCCESS;
}


void CstiDisplay::DisplaySettingsGet (
	SstiDisplaySettings * pDisplaySettings) const
{
	memset (pDisplaySettings, 0, sizeof (SstiDisplaySettings));
}


stiHResult CstiDisplay::DisplaySettingsSet (
	const SstiDisplaySettings * pDisplaySettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


stiHResult CstiDisplay::RemoteViewPrivacySet (
	EstiBool bPrivacy)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


stiHResult CstiDisplay::RemoteViewHoldSet (
	EstiBool bHold)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


stiHResult CstiDisplay::VideoRecordSizeGet (
	uint32_t *pun32Width,
	uint32_t *pun32Height) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*pun32Width = 352;
	*pun32Height = 288;

    return hResult;
}


stiHResult CstiDisplay::VideoPlaybackSizeGet (
	uint32_t *pun32Width,
	uint32_t *pun32Height) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*pun32Width = 352;
	*pun32Height = 288;

	return hResult;
}


stiHResult CstiDisplay::VideoPlaybackCodecSet (
	EstiVideoCodec eVideoCodec)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_eVideoCodec = eVideoCodec;
	
	return hResult;
}


stiHResult CstiDisplay::VideoPlaybackStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

#ifdef WRITE_BITSTREAM_TO_FILE
	if (m_pOutputFile)
	{
		fclose (m_pOutputFile);
		m_pOutputFile = NULL;
	}

	time_t t = time (NULL);

	std::stringstream FileName;

	FileName << "OutputFile." << t << m_unFileCounter << ".264";

	m_FileName = FileName.str ();

	m_pOutputFile = fopen (m_FileName.c_str (), "wb");

	++m_unFileCounter;
#endif

	return hResult;
}


stiHResult CstiDisplay::VideoPlaybackStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_pOutputFile)
	{
		fclose (m_pOutputFile);
		m_pOutputFile = NULL;

		m_FileName.clear ();
	}

	return hResult;
}


stiHResult CstiDisplay::VideoPlaybackFrameGet (
	IstiVideoPlaybackFrame **ppVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (!m_FrameList.empty (), stiRESULT_ERROR);

	*ppVideoFrame = m_FrameList.front ();
	m_FrameList.pop_front ();

	//
	// Clear the signal.  The signal won't be completely cleared
	// until clear is called for each set called.
	//
	stiOSSignalClear (m_fdSignal);
	
STI_BAIL:

	return hResult;
}
	

stiHResult CstiDisplay::VideoPlaybackFramePut (
	IstiVideoPlaybackFrame *pVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	CstiVideoPlaybackFrame *pFrame = (CstiVideoPlaybackFrame *)pVideoFrame;
	uint8_t *pBuffer = pFrame->BufferGet();
	uint32_t un32BufferLength = pFrame->DataSizeGet();
	bool bKeyFrame = false;
	
	switch (m_eVideoCodec)
	{
		case estiVIDEO_H264:
		{
			uint32_t un32SliceSize;
			uint32_t un8SliceType;

			while (un32BufferLength > 5)
			{
				un32SliceSize = *(uint32_t *)pBuffer;
				un32SliceSize = stiDWORD_BYTE_SWAP (un32SliceSize);
				
				un8SliceType = (pBuffer[sizeof (uint32_t)] & 0x1F);
				
				if (un8SliceType == SLICE_TYPE_IDR)
				{
					bKeyFrame = true;
				}
				
				if (m_pOutputFile)
				{
					unsigned char HeaderSyncCode[4] = {'\0', '\0', '\0', '\1'};

					fwrite (HeaderSyncCode, sizeof(HeaderSyncCode), 1, m_pOutputFile);
					fwrite (pBuffer + sizeof (uint32_t), un32SliceSize, sizeof (unsigned char ), m_pOutputFile);
				}

				if (sizeof (uint32_t) + un32SliceSize >= un32BufferLength)
				{
					break;
				}

				pBuffer += sizeof (uint32_t) + un32SliceSize;
				un32BufferLength -= sizeof (uint32_t) + un32SliceSize;
			}
			
			break;
		}
		
		case estiVIDEO_H263:
		{
			break;
		}
		
		default:
			
			stiASSERT (estiFALSE);
			
			break;
	}

	m_FrameList.push_back (pFrame);
	stiOSSignalSet (m_fdSignal);

	if (bKeyFrame)
	{
		// Send a signal to those registered to be informed of such.
		// todo: send the signal if/when we implement media in this tool.
	}
	
	return hResult;
}


stiHResult CstiDisplay::VideoPlaybackFrameDiscard (
	IstiVideoPlaybackFrame *pVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiVideoPlaybackFrame *pFrame = (CstiVideoPlaybackFrame *)pVideoFrame;
	
	m_FrameList.push_back (pFrame);

	//
	// Set the signal to indicate that there are frames available
	//
	stiOSSignalSet (m_fdSignal);

	return hResult;
}


int CstiDisplay::GetDeviceFD () const
{
	return stiOSSignalDescriptor (m_fdSignal);
}


///\brief Reports to the calling object what codecs this device is capable of.
///\brief Add codecs in the order that you would like preference to be given during channel
///\brief negotiations.
stiHResult CstiDisplay::VideoCodecsGet (
	std::list<EstiVideoCodec> *pCodecs)
{
	// Add the supported video codecs to the list
	pCodecs->push_back (estiVIDEO_H264);
	pCodecs->push_back (estiVIDEO_H263);
	
	return stiRESULT_SUCCESS;
}


///\brief Fills in the video capabilities structure with the specific capabilities of the given codec.
///\return stiRESULT_SUCCESS unless a request for an unsupported codec is made.
stiHResult CstiDisplay::CodecCapabilitiesGet (
	EstiVideoCodec eCodec,	// The codec for which we are inquiring
	SstiVideoCapabilities *pstCaps)		// A structure to load with the capabilities of the given codec
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiProfileAndConstraint stProfileAndConstraint;
	
	switch (eCodec)
	{
		case estiVIDEO_H263:
			stProfileAndConstraint.eProfile = estiH263_ZERO;
			pstCaps->Profiles.push_back (stProfileAndConstraint);
			break;

		case estiVIDEO_H264:
		{
			SstiH264Capabilities *pstH264Caps = (SstiH264Capabilities*)pstCaps;

			stProfileAndConstraint.eProfile = estiH264_BASELINE;
			stProfileAndConstraint.un8Constraints = 0x00;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);

			pstH264Caps->eLevel = estiH264_LEVEL_4;
			break;
		}

		default:
			hResult = stiRESULT_ERROR;
	}

	return hResult;
}


void CstiDisplay::ScreenshotTake (EstiScreenshotType eType)
{
}


CstiDisplay::EstiPowerStatus CstiDisplay::CECDisplayPowerGet ()
{
	EstiPowerStatus eReturnValue = ePowerStatusUnknown;
	char * result = nullptr;

	FILE *pPipe = popen ("echo 'pow 0' | cec-client -d 1 -s H2C", "r");
	if (pPipe)
	{
		char pszResult[128];
		while (!feof (pPipe))
		{
			result = fgets (pszResult, sizeof(pszResult), pPipe);
			if (0 == strncmp (pszResult, "power status: ",14) && result)
			{
				eReturnValue = ePowerStatusOff;
				if (0 == strcmp (&pszResult[14], "on\n"))
				{
					eReturnValue = ePowerStatusOn;
				}
				else if (0 == strcmp(&pszResult[14], "unknown\n"))
				{
					eReturnValue = ePowerStatusUnknown;
				}
				break;
			}
		}
		pclose (pPipe);
	}

	return(eReturnValue);
}

stiHResult CstiDisplay::CECStatusCheck()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	EstiPowerStatus eStatus = ePowerStatusUnknown; //CECDisplayPowerGet();  - For Registrar Load Test, we aren't interested in the connection to a TV
	if (eStatus != ePowerStatusUnknown)
	{
		m_bCECSupported = true;
		m_bTelevisionPowered = eStatus == ePowerStatusOn ? true : false;
	}

	return(hResult);
}

