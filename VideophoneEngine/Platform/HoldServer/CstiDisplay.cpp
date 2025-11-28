#include "CstiDisplay.h"
//#include "../IstiVideoOutput.h"



stiHResult CstiDisplay::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


stiHResult CstiDisplay::CallbackRegister (
	PstiObjectCallback pfnCallback,
	size_t CallbackParam,
	size_t CallbackFromId)
{

	stiHResult hResult = stiRESULT_SUCCESS;


	return hResult;
}


stiHResult CstiDisplay::CallbackUnregister (
	PstiObjectCallback pfnCallback,
	size_t CallbackParam)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


void CstiDisplay::DisplaySettingsGet (
	SstiDisplaySettings * pDisplaySettings) const
{
	memset (pDisplaySettings, 0, sizeof (SstiDisplaySettings));
}


stiHResult CstiDisplay::DisplaySettingsSet (
	const SstiDisplaySettings * pDisplaySettings)
{
	return stiRESULT_SUCCESS;
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


stiHResult CstiDisplay::VideoPlaybackSizeGet (
	uint32_t *punWidth,
	uint32_t *punHeight) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*punWidth = 0;
	*punHeight = 0;

	return hResult;
}


stiHResult CstiDisplay::VideoPlaybackCodecSet (
	EstiVideoCodec eVideoCodec)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


stiHResult CstiDisplay::VideoPlaybackStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


stiHResult CstiDisplay::VideoPlaybackStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}


stiHResult CstiDisplay::VideoPlaybackFramePut (
	vpe::VideoFrame * pVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	return hResult;
}

void CstiDisplay::PreferredDisplaySizeGet(
	unsigned int *displayWidth,
	unsigned int *displayHeight) const
{
    //Not defined as this function is not called by Hold Server.
}

class CstiVideoPlaybackFrame : public IstiVideoPlaybackFrame
{
public:
	
	CstiVideoPlaybackFrame ()
	{
		m_pBuffer = new uint8_t[720 * 480];
		
		m_un32DataSize = 0;
	}
	
	~CstiVideoPlaybackFrame ()
	{
		if (m_pBuffer)
		{
			delete [] m_pBuffer;
			m_pBuffer = NULL;
		}
	}
	
	
	virtual uint8_t *BufferGet () 		// pointer to the video packet data
	{
		return m_pBuffer;
	}
	
	
	virtual uint32_t BufferSizeGet () const	// size of this buffer in bytes
	{
		return 720 * 480;
	}
	
	virtual stiHResult BufferSizeSet (uint32_t size) 
	{
		// Frame size not currently settable on HS.
		return stiRESULT_ERROR;
	}
	
	virtual uint32_t DataSizeGet () const		// Number of bytes in the buffer.
	{
		return m_un32DataSize;
	}
	
	
	virtual stiHResult DataSizeSet (
		uint32_t un32DataSize)
	{
		stiHResult hResult = stiRESULT_SUCCESS;
		
		m_un32DataSize = un32DataSize;
		
		return hResult;
	}

	virtual bool FrameIsCompleteGet()
	{
		return true;
	}

	virtual void FrameIsCompleteSet(bool bFrameIsComplete)
	{
		//do nothing
	}

	virtual void FrameIsKeyframeSet(bool bFrameIsKeyframe)
	{
		//do nothing
	}

	virtual bool FrameIsKeyframeGet()
	{
		return true;
	}
	
private:
	
	uint8_t *m_pBuffer;
	uint32_t m_un32DataSize;
};


stiHResult CstiDisplay::VideoPlaybackFrameGet (
	IstiVideoPlaybackFrame **ppVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	static CstiVideoPlaybackFrame *pFrame = new CstiVideoPlaybackFrame;
	
	*ppVideoFrame = pFrame;
	
	return hResult;
}
	

stiHResult CstiDisplay::VideoPlaybackFramePut (
	IstiVideoPlaybackFrame *pVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	return hResult;
}


stiHResult CstiDisplay::VideoPlaybackFrameDiscard (
	IstiVideoPlaybackFrame *pVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	return hResult;
}


int CstiDisplay::GetDeviceFD () const
{
	return 0;
}


///\brief Reports to the calling object what codecs this device is capable of.
///\brief Add codecs in the order that you would like preference to be given during channel
///\brief negotiations.
stiHResult CstiDisplay::VideoCodecsGet (
	std::list<EstiVideoCodec> *pCodecs)
{
	// Add the supported video codecs to the list
#ifdef stiENABLE_H264
	pCodecs->push_back (estiVIDEO_H264);
#endif
	
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
			pstCaps->Profiles.push_back(stProfileAndConstraint);
			break;

		case estiVIDEO_H264:
		{
			SstiH264Capabilities *pstH264Caps = (SstiH264Capabilities*)pstCaps;

			stProfileAndConstraint.eProfile = estiH264_BASELINE;
			stProfileAndConstraint.un8Constraints = 0xe0;
			pstH264Caps->Profiles.push_back(stProfileAndConstraint);
			pstH264Caps->eLevel = estiH264_LEVEL_4_1;
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
