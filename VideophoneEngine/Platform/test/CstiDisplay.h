//
//  CstiDisplay.h
//  gktest
//
//  Created by Dennis Muhlestein on 12/6/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#ifndef __gktest__CstiDisplay__
#define __gktest__CstiDisplay__

#include "IstiVideoOutput.h"

#include "CstiCallbackList.h"

class CstiDisplay : public IstiVideoOutput
{
	public:
		CstiDisplay();
		
		stiHResult Initialize();
		void Uninitialize();

	/*!
	 * \brief Register a callback function
	 * 
	 * \param pfnVPCallback 
	 * \param CallbackParam 
	 * \param CallbackFromId 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult CallbackRegister (
		PstiObjectCallback pfnVPCallback,
		size_t CallbackParam,
		size_t CallbackFromId);

	/*!
	 * \brief Unregister a callback function
	 * 
	 * \param pfnVPCallback 
	 * \param CallbackParam 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult CallbackUnregister (
		PstiObjectCallback pfnVPCallback,
		size_t CallbackParam);


	/*!
	 * \brief Get the display settings
	 * 
	 * \param pDisplaySettings 
	 */
	virtual void DisplaySettingsGet (
		SstiDisplaySettings * pDisplaySettings) const {}
	
	/*!
	 * \brief Set display settings
	 * 
	 * \param pDisplaySettings 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult DisplaySettingsSet (
	   const SstiDisplaySettings * pDisplaySettings) { return stiRESULT_SUCCESS; }
	
	/*!
	 * \brief Set remote privacy view
	 * 
	 * \param bPrivacy 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult RemoteViewPrivacySet (
		EstiBool bPrivacy) { return stiRESULT_SUCCESS; }
		
	/*!
	 * \brief Set remove view hold
	 * 
	 * \param bHold 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult RemoteViewHoldSet (
		EstiBool bHold) { return stiRESULT_SUCCESS; }
		
	/*!
	 * \brief Get the video playback size
	 * 
	 * \param pun32Width 
	 * \param pun32Height 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult VideoPlaybackSizeGet (
		uint32_t *pun32Width,
		uint32_t *pun32Height) const { *pun32Width = 352; *pun32Height = 288; }

	/*!
	 * \brief Set video playback codec
	 * 
	 * \param eVideoCodec 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult VideoPlaybackCodecSet (
		EstiVideoCodec eVideoCodec) { return stiRESULT_SUCCESS; }
		
	/*!
	 * \brief Start Video playback
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult VideoPlaybackStart (){ return stiRESULT_SUCCESS; }

	/*!
	 * \brief Stop video playback
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult VideoPlaybackStop (){ return stiRESULT_SUCCESS; }

	/*!
	 * \brief Put a video frame
	 * 
	 * \param pVideoFrame 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult VideoPlaybackFramePut (
		SstiPlaybackVideoFrame* /* UNUSED */) { return stiRESULT_SUCCESS; };

	/*!
	 * \brief Get a video frame
	 * 
	 * \param ppVideoFrame 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult VideoPlaybackFrameGet (
		IstiVideoPlaybackFrame **ppVideoFrame);
		
	/*!
	 * \brief Put Video playback
	 * 
	 * \param pVideoFrame 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult VideoPlaybackFramePut (
		IstiVideoPlaybackFrame *pVideoFrame);

	/*!
	 * \brief Disgard a frame
	 * 
	 * \param pVideoFrame 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult VideoPlaybackFrameDiscard (
		IstiVideoPlaybackFrame *pVideoFrame);

	/*!
	 * \brief Get the device file descriptor
	 * 
	 * \return int 
	 */
	virtual int GetDeviceFD () const ;

	/*!
	 * \brief Get video codec 
	 * 
	 * \param pCodecs 
	 * \param ePreferredCodec 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult VideoCodecsGet (
		std::list<int> *pCodecs,
		EstiVideoCodec ePreferredCodec);

	/*!
	 * \brief Get Codec Capability 
	 * 
	 * \param eCodec 
	 * \param pstCaps 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult CodecCapabilitiesGet (
		EstiVideoCodec eCodec,
		SstiVideoCapabilities *pstCaps);
	
	private:
		CstiCallbackList m_oCallbacks;
		STI_OS_SIGNAL_TYPE m_fdSignal;
	
};

#endif /* defined(__gktest__CstiDisplay__) */
