//
//  CstiDisplay.cpp
//  gktest
//
//  Created by Dennis Muhlestein on 12/6/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#include "CstiDisplay.h"

#include "CstiSoftwareDisplay.h" // for the VideoFrame

uint32_t CstiVideoPlaybackFrame::m_un32BufferCount;

CstiDisplay::CstiDisplay():m_fdSignal(NULL)
{
}

stiHResult CstiDisplay::Initialize ()
{
    stiHResult hResult = stiRESULT_SUCCESS;

    if (NULL == m_fdSignal)
    {
        EstiResult eResult = stiOSSignalCreate (&m_fdSignal);
        stiTESTCOND (estiOK == eResult, stiRESULT_TASK_INIT_FAILED);

        stiOSSignalSet (m_fdSignal);
    }

STI_BAIL:

    return hResult;
}


void CstiDisplay::Uninitialize ()
{
    // close Signal 
    if (m_fdSignal)
    {
        stiOSSignalClose (&m_fdSignal);
        m_fdSignal = NULL;
    }
}


stiHResult CstiDisplay::CallbackRegister (
    PstiObjectCallback pfnCallback,
    size_t CallbackParam,
    size_t CallbackFromId)
{
    stiHResult hResult = stiRESULT_SUCCESS;

    hResult = m_oCallbacks.Register (pfnCallback, CallbackParam, CallbackFromId);

    stiTESTRESULT ();

STI_BAIL:

    return hResult;
}


///\brief Unregisters a callback function from  the object.
///
stiHResult CstiDisplay::CallbackUnregister (
    PstiObjectCallback pfnCallback,
    size_t CallbackParam)
{
    //
    // Find and remove the callback in the list.
    //
    stiHResult hResult = stiRESULT_SUCCESS;

    hResult = m_oCallbacks.Unregister (pfnCallback, CallbackParam);

    stiTESTRESULT ();

STI_BAIL:

    return hResult;
}


stiHResult CstiDisplay::VideoCodecsGet (
    std::list<int> *pCodecs,
    EstiVideoCodec ePreferredCodec)
{
    // Add the supported video codecs to the list
    pCodecs->push_back (estiVIDEO_H264);

    return stiRESULT_SUCCESS;
}

stiHResult CstiDisplay::CodecCapabilitiesGet (
    EstiVideoCodec eCodec,  // The codec for which we are inquiring
    SstiVideoCapabilities *pstCaps)     // A structure to load with the capabilities of the given codec
{
    stiHResult hResult = stiRESULT_SUCCESS;

    switch (eCodec)
    {
        case estiVIDEO_H264:
            {
                SstiH264Capabilities *pstH264Caps = (SstiH264Capabilities*)pstCaps;
                pstH264Caps->nProfile = estiH264_BASELINE;
                pstH264Caps->eLevel = estiH264_LEVEL_1_3;
                pstH264Caps->unCustomMaxMBPS = 0;
                pstH264Caps->unCustomMaxFS = 0;
            }
            break;

        default:
            hResult = stiRESULT_ERROR;
    }

    return hResult;
}

int CstiDisplay::GetDeviceFD () const
{
	return stiOSSignalDescriptor(m_fdSignal);
}


stiHResult CstiDisplay::VideoPlaybackFrameGet (
		IstiVideoPlaybackFrame **ppVideoFrame)
{

  // TODO fix to be more effient at some point in the future
	*ppVideoFrame = new CstiVideoPlaybackFrame;
	return stiRESULT_SUCCESS;
}
		
	/*!
	 * \brief Put Video playback
	 * 
	 * \param pVideoFrame 
	 * 
	 * \return stiHResult 
	 */
stiHResult CstiDisplay::VideoPlaybackFramePut (
		IstiVideoPlaybackFrame *pVideoFrame)
{
		return VideoPlaybackFrameDiscard(pVideoFrame);
}

stiHResult CstiDisplay::VideoPlaybackFrameDiscard (
		IstiVideoPlaybackFrame *pVideoFrame)
{
	// TODO put in a queue or something
	delete (CstiVideoPlaybackFrame*)pVideoFrame;
	return stiRESULT_SUCCESS;
}


