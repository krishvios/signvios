#include "CstiVideoOutputBase.h"
#include "stiTrace.h"
#include "CstiVideoInput.h"
#include "H264_BlackFrame.h"
#include "H263_BlackFrame.h"
#include "stiTools.h"
#include <cmath>

#define ENABLE_VIDEO_OUTPUT

#define DECODE_LATENCY_IN_BUFFERS 3

CstiVideoOutputBase::CstiVideoOutputBase (int playbackBufferSize, int numPaybackBuffers)
:
	CstiEventQueue ("CstiVideoOutput"),
	m_bufferReturnErrorTimer (BUFFER_RETURN_ERROR_TIMEOUT),
	m_videoFilePanTimer (VIDEO_FILE_PAN_SHIFT_TIMEOUT)
{
	//
	// Allocate the buffers we need for the bit streams
	// we need to decode.
	//
	for (int i = 0; i < numPaybackBuffers; i++)
	{
		CstiVideoPlaybackFrame *pVideoPlaybackFrame = nullptr;

		pVideoPlaybackFrame = new CstiVideoPlaybackFrame (playbackBufferSize);

		pVideoPlaybackFrame->VideoOutputSet (this);

		m_oBitstreamFifo.Put (pVideoPlaybackFrame);
		pVideoPlaybackFrame = nullptr;
	}

	m_signalConnections.push_back (m_videoFilePanTimer.timeoutEvent.Connect (
		[this]{PostEvent([this]{eventFilePanTimerTimeout();});
		}));
}


CstiVideoOutputBase::~CstiVideoOutputBase ()
{
	for (;;)
	{
		CstiVideoPlaybackFrame *pVideoPlaybackFrame = m_oBitstreamFifo.Get ();

		if (pVideoPlaybackFrame == nullptr)
		{
			break;
		}

		for (const auto &frame: m_FramesInApplication)
		{
			if (frame == pVideoPlaybackFrame)
			{
				stiTrace ("CstiVideoOutputBase::~CstiVideoOutput: Frame is also in application.\n");
				break;
			}
		}

		for (const auto &frame: m_FramesInGstreamer)
		{
			if (frame == pVideoPlaybackFrame)
			{
				stiTrace ("CstiVideoOutputBase::~CstiVideoOutput: Frame is also in GStreamer.\n");
				break;
			}
		}

		delete pVideoPlaybackFrame;
	}
}

/*!
* \brief Start up the task
* \retval None
*/
stiHResult CstiVideoOutputBase::Startup ()
{
	CstiEventQueue::StartEventLoop();

	return stiRESULT_SUCCESS;
}

void CstiVideoOutputBase::DisplaySettingsGet (
	SstiDisplaySettings * pDisplaySettings) const
{
	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("DisplaySettingsGet is not implemented\n");
	);
}


stiHResult CstiVideoOutputBase::DisplaySettingsSet (
	const SstiDisplaySettings * pDisplaySettings)
{
	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("DisplaySettingsSet is not implemented\n");
	);
	return stiRESULT_SUCCESS;
}

stiHResult CstiVideoOutputBase::OutputImageSettingsPrint (
	const IstiVideoOutput::SstiImageSettings *pImageSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	vpe::trace ("\tbVisible = ", pImageSettings->bVisible, " (", pImageSettings->unPosX, ", ", pImageSettings->unPosY, ", ", pImageSettings->unWidth, ", ", pImageSettings->unHeight, ")\n");

	return hResult;
}

bool CstiVideoOutputBase::ImageSettingsEqual (
	const IstiVideoOutput::SstiImageSettings *pImageSettings0,
	const IstiVideoOutput::SstiImageSettings *pImageSettings1)
{
	bool bEqual = false;

	if (pImageSettings0->bVisible == pImageSettings1->bVisible &&
		pImageSettings0->unWidth == pImageSettings1->unWidth &&
		pImageSettings0->unHeight == pImageSettings1->unHeight &&
		pImageSettings0->unPosX == pImageSettings1->unPosX &&
		pImageSettings0->unPosY == pImageSettings1->unPosY)
	{
		bEqual = true;
	}

	return bEqual;
}

stiHResult CstiVideoOutputBase::VideoRecordSizeGet (
	uint32_t *pun32Width,
	uint32_t *pun32Height) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_pVideoInput)
	{
		hResult = m_pVideoInput->VideoRecordSizeGet (pun32Width, pun32Height);
		stiTESTRESULT ();
	}

STI_BAIL:

	return(hResult);
}


stiHResult CstiVideoOutputBase::VideoPlaybackSizeGet (
	uint32_t *pun32Width,
	uint32_t *pun32Height) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*pun32Width = m_un32DecodeWidth;
	*pun32Height = m_un32DecodeHeight;

	return (hResult);
}


ISignal<>& CstiVideoOutputBase::videoFileStartFailedSignalGet ()
{
	return videoFileStartFailedSignal;
}

ISignal<>& CstiVideoOutputBase::videoFileCreatedSignalGet () 
{
	return videoFileCreatedSignal;
}

ISignal<>& CstiVideoOutputBase::videoFileReadyToPlaySignalGet ()
{
	return videoFileReadyToPlaySignal;
}

ISignal<>& CstiVideoOutputBase::videoFileEndSignalGet ()
{
	return videoFileEndSignal;
}

ISignal<const std::string>& CstiVideoOutputBase::videoFileClosedSignalGet ()
{
	return videoFileClosedSignal;
}

ISignal<uint64_t, uint64_t, uint64_t>& CstiVideoOutputBase::videoFilePlayProgressSignalGet ()
{
	return videoFilePlayProgressSignal;
}

ISignal<>& CstiVideoOutputBase::videoFileSeekReadySignalGet ()
{
	return videoFileSeekReadySignal;
}


stiHResult CstiVideoOutputBase::RemoteViewPrivacySet (
	EstiBool bPrivacy)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		stiTrace("CstiVideoOutputBase::RemoteViewPrivacySet: bPrivacy = %d, m_bHolding = %d\n", bPrivacy, m_bHolding);
	);

	//
	// If we are going on privacy we will need a new SPS/PPS pair sent to the decoder
	// before decoding frames when we come off of privacy.
	//
	if (!m_bPrivacy && bPrivacy)
	{
		m_bSPSAndPPSSentToDecoder = false;
	}

	m_bPrivacy = bPrivacy;

	PostEvent ([this]
	{
		VideoSinkVisibilityUpdate(true);
	});

	EstiRemoteVideoMode eMode;

	if (m_bPrivacy &&
		!m_bHolding)
	{
		eMode = eRVM_PRIVACY;
	}
	else if (m_bHolding)
	{
		eMode = eRVM_HOLD;
	}
	else
	{
		eMode = eRVM_VIDEO;
	}

	remoteVideoModeChangedSignal.Emit (eMode);

	Unlock ();

	return (hResult);
}


stiHResult CstiVideoOutputBase::RemoteViewHoldSet (
	EstiBool bHold)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		stiTrace("CstiVideoOutputBase::RemoteViewHoldSet: bHold = %d\n", bHold);
	);

	//
	// If we are going on hold we will need a new SPS/PPS pair sent to the decoder
	// before decoding frames when we come off of hold.
	//
	if (!m_bHolding && bHold)
	{
		m_bSPSAndPPSSentToDecoder = false;
	}

	m_bHolding = bHold;

	PostEvent ([this]
	{
		VideoSinkVisibilityUpdate(true);
	});

	EstiRemoteVideoMode eMode;

	if (m_bHolding)
	{
		eMode = eRVM_HOLD;
	}
	else
	{
		if (m_bPrivacy)
		{
			eMode = eRVM_PRIVACY;
		}
		else
		{
			eMode = eRVM_VIDEO;
		}
	}

	remoteVideoModeChangedSignal.Emit (eMode);

	Unlock ();

	return (hResult);
}


GstState ChangeStateWait (
	GStreamerElement element,
	GstClockTime timeout)
{
	GstState state;
	GstState pendingState;

	switch (gst_element_get_state(element.get(), &state, &pendingState, timeout))
	{
		case GST_STATE_CHANGE_SUCCESS:
			{
				auto elementName = element.nameGet();
				stiDEBUG_TOOL (g_stiVideoOutputDebug,
					stiTrace("ChangeStateWait: GST_STATE_CHANGE_SUCCESS: element = %s waited %" PRIu64 " for change. state = %s. pending state = %s\n",
								elementName.c_str (),
								timeout,
								gst_element_state_get_name (state),
								gst_element_state_get_name (pendingState));
				);
			}
			break;
		case GST_STATE_CHANGE_ASYNC:
			{
				auto elementName = element.nameGet();
				stiDEBUG_TOOL (g_stiVideoOutputDebug,
					stiTrace("ChangeStateWait: GST_STATE_CHANGE_ASYNC: element = %s waited %" PRIu64 " for change. state = %s. pending state = %s\n",
								elementName.c_str (),
								timeout,
								gst_element_state_get_name (state),
								gst_element_state_get_name (pendingState));
				);
			}
			break;
		case GST_STATE_CHANGE_FAILURE:
			{
				auto elementName = element.nameGet ();
				stiASSERTMSG (estiFALSE, "ChangeStateWait: GST_STATE_CHANGE_FAILURE:  element = %s, change: state = %s, pending state = %s\n",
							  elementName.c_str (),
							  gst_element_state_get_name (state),
							  gst_element_state_get_name (pendingState));
			}
			break;
		default:
			break;
	}

	return state;
}


/*!
 * \brief Video playback frame put
 *
 * \return stiHResult
 */
stiHResult CstiVideoOutputBase::VideoPlaybackFramePut (
	IstiVideoPlaybackFrame *pVideoFrame)
{
	PostEvent ([this, pVideoFrame]{ eventVideoPlaybackFramePut (pVideoFrame); });

	return stiRESULT_SUCCESS;
}


void CstiVideoOutputBase::eventVideoPlaybackFramePut (
	IstiVideoPlaybackFrame *pVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoOutputDebug > 3,
		stiTrace("CstiVideoOutputBase::eventVideoPlaybackFramePut\n");
	);

	bool keyFrameNeeded = true;

	stiTESTCOND(pVideoFrame, stiRESULT_ERROR);

	m_FramesInApplication.remove (pVideoFrame);

#ifdef ENABLE_VIDEO_OUTPUT
	if (m_bPlaying)
	{
		stiDEBUG_TOOL (g_stiVideoOutputDebug > 3,
			stiTrace ("CstiVideoOutputBase::eventVideoPlaybackFramePut m_bPlaying = true\n");
		);

		if (estiVIDEO_H264 == m_eCodec)
		{
			if (!pVideoFrame->FrameIsCompleteGet ())
			{
				// todo - Since we now have knowledge of this being a keyframe in the pVideoFrame structure,
				// we can optimize the following so that we only go into the for loop for keyframes to get the
				// SPS and PPS headers.  Any that aren't keyframes should just get passed along to the decoder.

				stiDEBUG_TOOL (g_stiVideoOutputDebug,
					stiTrace ("CstiVideoOutputBase::eventVideoPlaybackFramePut: H264 Frame is not complete\n");
				);
				
				//
				// The H.264 frame is not a complete frame.
				// If it is an IDR frame then we want to extract the SPS and PPS and
				// send only those to the decoder.
				//
				// Loop through the NAL Units to look for IDR NAL Units.
				//
				bool bIsLastNALUnit = false;
				uint8_t *pun8Buffer = pVideoFrame->BufferGet();
				uint32_t un32FrameSize = pVideoFrame->DataSizeGet ();
				uint8_t *pun8NALUnit = nullptr;
				uint8_t *pun8NextNALUnit = nullptr;
				uint32_t un32HeaderLength = 0;
				uint32_t un32NextHeaderLength = 0;
				uint32_t un32NextNALUnitOffset = 0;

				for (;;)
				{
					if (pun8NextNALUnit == nullptr)
					{
						//
						// Look for the next byte stream header.
						//
						ByteStreamNALUnitFind (pun8Buffer, un32FrameSize, &pun8NALUnit, &un32HeaderLength);

						stiTESTCOND (pun8NALUnit, stiRESULT_ERROR);
					}
					else
					{
						pun8NALUnit = pun8NextNALUnit;
						un32HeaderLength = un32NextHeaderLength;
					}

					//
					// If we found a byte stream header look for the one following so that
					// we can compute size of the current NAL Unit and determine if this is the
					// last NAL Unit in the frame.
					//
					ByteStreamNALUnitFind (pun8NALUnit + un32HeaderLength, un32FrameSize - un32HeaderLength,
										   &pun8NextNALUnit, &un32NextHeaderLength);

					if (pun8NextNALUnit == nullptr)
					{
						//
						// We didn't find another byte stream header so this must be the last NAL Unit in the frame.
						//
						bIsLastNALUnit = true;
						pun8NALUnit += un32HeaderLength;
					}
					else
					{
						//
						// We found another NAL Unit header.  Determine the size of the current NAL Unit
						// and compute the offset to the next NAL Unit.
						//
						un32NextNALUnitOffset = pun8NextNALUnit - pun8Buffer;
						bIsLastNALUnit = false;
						pun8NALUnit += un32HeaderLength;
					}

					// Look at the NAL unit now to determine what to do
					//
					// We don't care about these:
					//     estiH264PACKET_NAL_SEI
					//     estiH264PACKET_NAL_SPS
					//     estiH264PACKET_NAL_PPS
					if ((pun8NALUnit[0] & 0x1F) == estiH264PACKET_NAL_CODED_SLICE_OF_IDR)
					{
						// If this is the first keyframe for this session, send it on.
						if (!m_bFirstKeyframeRecvd)
						{
							// Just let the whole thing be sent down.
							m_bFirstKeyframeRecvd = true;
						}
						else
						{
							// Truncate at this point and send it on to the platform to be handled.
							pVideoFrame->DataSizeSet (pun8NALUnit - pVideoFrame->BufferGet() - un32HeaderLength);
						}

						bIsLastNALUnit = true;
					}
					else if ((pun8NALUnit[0] & 0x1F) == estiH264PACKET_NAL_CODED_SLICE_OF_NON_IDR ||
							(pun8NALUnit[0] & 0x1F) == estiH264PACKET_NAL_CODED_SLICE_PARTITION_A ||
							(pun8NALUnit[0] & 0x1F) == estiH264PACKET_NAL_CODED_SLICE_PARTITION_B ||
							(pun8NALUnit[0] & 0x1F) == estiH264PACKET_NAL_CODED_SLICE_PARTITION_C)
					{
						// Break out and send it as is ...
						bIsLastNALUnit = true;
					}

					// Break when it's end of a frame or when we have determined that we don't need to look further.
					if (bIsLastNALUnit)
					{
						break;
					}

					pun8Buffer += un32NextNALUnitOffset;
					un32FrameSize -= un32NextNALUnitOffset;

				} // end for
			}
			else if (pVideoFrame->FrameIsKeyframeGet () && !m_bFirstKeyframeRecvd)
			{
				stiDEBUG_TOOL (g_stiVideoOutputDebug,
					stiTrace ("CstiVideoOutputBase::EventVideoPlaybackFramePut: H264 Recieved first keyframe\n");
				);
				
				m_bFirstKeyframeRecvd = true;
			}
		}
		else if (estiVIDEO_H265 == m_eCodec)
		{
			if (!pVideoFrame->FrameIsCompleteGet ())
			{
				
				stiDEBUG_TOOL (g_stiVideoOutputDebug,
							stiTrace ("CstiVideoOutputBase::EventVideoPlaybackFramePut: H265 Frame is not complete\n");
						);
				
			}
			else if (pVideoFrame->FrameIsKeyframeGet () && !m_bFirstKeyframeRecvd)
			{
				stiDEBUG_TOOL (g_stiVideoOutputDebug,
					stiTrace ("CstiVideoOutputBase::EventVideoPlaybackFramePut: H265 Recieved first keyframe\n");
				);
				
				m_bFirstKeyframeRecvd = true;
			}
		}

		//
		// If we still have data to send to GStreamer then do so.
		//
		if (0 < pVideoFrame->DataSizeGet ())
		{
			bool bVPSFound = false;
			bool bSPSFound = false;
			bool bPPSFound = false;
			
			if (estiVIDEO_H264 == m_eCodec)
			{
				//
				// Loop through the NAL Units to look for IDR NAL Units.
				//
				bool bIsLastNALUnit = false;
				uint8_t *pun8Buffer = pVideoFrame->BufferGet ();
				uint32_t un32FrameSize = pVideoFrame->DataSizeGet ();
				uint8_t *pun8NALUnit = nullptr;
				uint8_t *pun8NextNALUnit = nullptr;
				uint32_t un32HeaderLength = 0;
				uint32_t un32NextHeaderLength = 0;
				uint32_t un32NextNALUnitOffset = 0;

				for (;;)
				{
					if (pun8NextNALUnit == nullptr)
					{
						//
						// Look for the next byte stream header.
						//
						ByteStreamNALUnitFind (pun8Buffer, un32FrameSize, &pun8NALUnit, &un32HeaderLength);
						stiTESTCOND (pun8NALUnit, stiRESULT_ERROR);
					}
					else
					{
						pun8NALUnit = pun8NextNALUnit;
						un32HeaderLength = un32NextHeaderLength;
					}

					//
					// If we found a byte stream header look for the one following so that
					// we can compute size of the current NAL Unit and determine if this is the
					// last NAL Unit in the frame.
					//
					ByteStreamNALUnitFind (pun8NALUnit + un32HeaderLength, un32FrameSize - un32HeaderLength,
											&pun8NextNALUnit, &un32NextHeaderLength);

					if (pun8NextNALUnit == nullptr)
					{
						//
						// We didn't find another byte stream header so this must be the last NAL Unit in the frame.
						//
						bIsLastNALUnit = true;
						pun8NALUnit += un32HeaderLength;
					}
					else
					{
						//
						// We found another NAL Unit header.  Determine the size of the current NAL Unit
						// and compute the offset to the next NAL Unit.
						//
						un32NextNALUnitOffset = pun8NextNALUnit - pun8Buffer;
						bIsLastNALUnit = false;
						pun8NALUnit += un32HeaderLength;
					}
				
					if ((pun8NALUnit[0] & 0x1F) == estiH264PACKET_NAL_SPS)
					{
						stiDEBUG_TOOL (g_stiVideoOutputDebug,
							stiTrace ("CstiVideoOutputBase::eventVideoPlaybackFramePut: found H.264 SPS\n");
						);
						
						bSPSFound = true;
						
						applySPSFixups (pun8NALUnit);
						
					}
					else if ((pun8NALUnit[0] & 0x1F) == estiH264PACKET_NAL_PPS)
					{
						stiDEBUG_TOOL (g_stiVideoOutputDebug,
							stiTrace ("CstiVideoOutputBase::eventVideoPlaybackFramePut: found H.264 PPS\n");
						);
						
						bPPSFound = true;
					}

					// Break when it's end of a frame or when we have determined that we don't need to look further.
					if (bIsLastNALUnit)
					{
						break;
					}

					pun8Buffer += un32NextNALUnitOffset;
					un32FrameSize -= un32NextNALUnitOffset;
				} // end for

				//
				// If this frame does not contain an SPS and a PPS NAL Unit and
				// we have not yet sent a previous SPS/PPS pair to the deocder then
				// throw an error (but don't log it since this an expected and acceptable error).
				//
				if (!m_bSPSAndPPSSentToDecoder && !bSPSFound && !bPPSFound)
				{
					stiTHROW_NOLOG (stiRESULT_ERROR);
				}
			}
			else if (estiVIDEO_H265 == m_eCodec)
			{
				//
				// Loop through the NAL Units to look for IDR NAL Units.
				//
				bool bIsLastNALUnit = false;
				uint8_t *pun8Buffer = pVideoFrame->BufferGet ();
				uint32_t un32FrameSize = pVideoFrame->DataSizeGet ();
				uint8_t *pun8NALUnit = nullptr;
				uint8_t *pun8NextNALUnit = nullptr;
				uint32_t un32HeaderLength = 0;
				uint32_t un32NextHeaderLength = 0;
				uint32_t un32NextNALUnitOffset = 0;
				uint8_t nalType = 0;

				for (;;)
				{
					if (pun8NextNALUnit == nullptr)
					{
						//
						// Look for the next byte stream header.
						//
						ByteStreamNALUnitFind (pun8Buffer, un32FrameSize, &pun8NALUnit, &un32HeaderLength);
						stiTESTCOND (pun8NALUnit, stiRESULT_ERROR);
					}
					else
					{
						pun8NALUnit = pun8NextNALUnit;
						un32HeaderLength = un32NextHeaderLength;
					}

					//
					// If we found a byte stream header look for the one following so that
					// we can compute size of the current NAL Unit and determine if this is the
					// last NAL Unit in the frame.
					//
					ByteStreamNALUnitFind (pun8NALUnit + un32HeaderLength, un32FrameSize - un32HeaderLength,
											&pun8NextNALUnit, &un32NextHeaderLength);

					if (pun8NextNALUnit == nullptr)
					{
						//
						// We didn't find another byte stream header so this must be the last NAL Unit in the frame.
						//
						bIsLastNALUnit = true;
						pun8NALUnit += un32HeaderLength;
					}
					else
					{
						//
						// We found another NAL Unit header.  Determine the size of the current NAL Unit
						// and compute the offset to the next NAL Unit.
						//
						un32NextNALUnitOffset = pun8NextNALUnit - pun8Buffer;
						bIsLastNALUnit = false;
						pun8NALUnit += un32HeaderLength;
					}

					nalType = ((pun8NALUnit[0] >> 1) & 0x3F);
					
					switch (nalType)
					{
						case estiH265PACKET_NAL_UNIT_VPS:
							stiDEBUG_TOOL (g_stiVideoOutputDebug,
								stiTrace ("CstiVideoOutputBase::eventVideoPlaybackFramePut: found H.265 VPS\n");
							);
							bVPSFound = true;
							break;
						case estiH265PACKET_NAL_UNIT_SPS:
							stiDEBUG_TOOL (g_stiVideoOutputDebug,
								stiTrace ("CstiVideoOutputBase::eventVideoPlaybackFramePut: found H.265 SPS\n");
							);
							bSPSFound = true;
							break;
						case estiH265PACKET_NAL_UNIT_PPS:
							stiDEBUG_TOOL (g_stiVideoOutputDebug,
								stiTrace ("CstiVideoOutputBase::eventVideoPlaybackFramePut: found H.265 PPS\n");
							);
							bPPSFound = true;
							break;
						case estiH265PACKET_NAL_UNIT_CODED_SLICE_BLA_W_LP:
						case estiH265PACKET_NAL_UNIT_CODED_SLICE_BLA_W_RADL:
						case estiH265PACKET_NAL_UNIT_CODED_SLICE_BLA_N_LP:
						case estiH265PACKET_NAL_UNIT_CODED_SLICE_IDR_W_RADL:
						case estiH265PACKET_NAL_UNIT_CODED_SLICE_IDR_N_LP:
						case estiH265PACKET_NAL_UNIT_CODED_SLICE_CRA:
							stiDEBUG_TOOL (g_stiVideoOutputDebug,
								stiTrace ("CstiVideoOutputBase::eventVideoPlaybackFramePut:  H.265 NAL is keyframe\n");
							);
							
							
							if (!pVideoFrame->FrameIsKeyframeGet ())
							{
								stiDEBUG_TOOL (g_stiVideoOutputDebug,
									stiTrace ("CstiVideoOutputBase::eventVideoPlaybackFramePut:  H.265 NAL is keyframe but it was no marked as keyframe\n");
								);
								
								pVideoFrame->FrameIsKeyframeSet (true);
								
								m_bFirstKeyframeRecvd = true;
							}
														
							break;
					}
					
					// Break when it's end of a frame or when we have determined that we don't need to look further.
					if (bIsLastNALUnit)
					{
						break;
					}

					pun8Buffer += un32NextNALUnitOffset;
					un32FrameSize -= un32NextNALUnitOffset;
				} // end for

				//
				// If this frame does not contain an SPS and a PPS NAL Unit and
				// we have not yet sent a previous SPS/PPS pair to the decoder then
				// throw an error (but don't log it since this an expected and acceptable error).
				//
				if (!m_bSPSAndPPSSentToDecoder && !bVPSFound && !bSPSFound && !bPPSFound)
				{
					stiTHROW_NOLOG (stiRESULT_ERROR);
				}
			}

			GStreamerBuffer outputBuffer (pVideoFrame->BufferGet (), pVideoFrame->DataSizeGet (),
					[this, pVideoFrame] { GstreamerVideoFrameDiscardCallback(pVideoFrame); });
			stiTESTCOND (outputBuffer.get () != nullptr, stiRESULT_ERROR);

			stiDEBUG_TOOL (g_stiVideoOutputDebug > 3,
				stiTrace ("CstiVideoOutputBase::eventVideoPlaybackFramePut: GStreamerBuffer called with pVideoFrame = %p\n", pVideoFrame);
			);

			//
			// Set the pointer to the frame to NULL since it is now owned
			// by the GStreamerBuffer.  When the GStreamerBuffer is freed then the
			// free func will be called which will return the video frame buffer.
			//
			m_FramesInGstreamer.push_back (pVideoFrame);
			pVideoFrame = nullptr;

			{
				auto ret = pushBuffer (outputBuffer);
				stiTESTCOND (ret == GST_FLOW_OK, stiRESULT_ERROR);
				keyFrameNeeded = false;
			}

			
			if (estiVIDEO_H264 == m_eCodec)
			{
				// We have a PPS/SPS and we haven't passed one down to the
				// decoder before so set the boolean to true.
				if (!m_bSPSAndPPSSentToDecoder && bSPSFound && bPPSFound)
				{
					m_bSPSAndPPSSentToDecoder = true;
				}
			}
			else if (estiVIDEO_H265 == m_eCodec)
			{
				// We have a VPS/PPS/SPS and we haven't passed one down to the
				// decoder before so set the boolean to true.
				if (!m_bSPSAndPPSSentToDecoder && bVPSFound && bSPSFound && bPPSFound)
				{
					m_bSPSAndPPSSentToDecoder = true;
				}
			}

			if (m_FramesInGstreamer.size () > m_maxFramesInGstreamer)
			{
				m_maxFramesInGstreamer = m_FramesInGstreamer.size ();

				stiDEBUG_TOOL (g_stiVideoOutputDebug,
					stiTrace ("CstiVideoOutputBase::eventVideoPlaybackFramePut: m_maxFramesInGstreamer = %d\n", m_maxFramesInGstreamer);
					stiTrace ("CstiVideoOutputBase::eventVideoPlaybackFramePut: m_FramesInApplication.size = %d\n", m_FramesInApplication.size ());
				);
			}

			if (m_FramesInGstreamer.size () >= NUM_DEC_BITSTREAM_BUFS)
			{
				stiDEBUG_TOOL (g_stiVideoOutputDebug,
					stiTrace ("CstiVideoOutputBase::eventVideoPlaybackFramePut: starting m_bufferReturnTimer: m_maxFramesInGstreamer = %d\n", m_maxFramesInGstreamer);
				);

				stiDEBUG_TOOL (g_stiVideoOutputDebug,
					stiTrace ("CstiVideoOutputBase::eventVideoPlaybackFramePut: starting m_bufferReturnErrorTimer: m_maxFramesInGstreamer = %d, FramesInGstreamer = %d, FramesInApplication = %d\n",
						m_maxFramesInGstreamer,
						m_FramesInGstreamer.size (),
						m_FramesInApplication.size ());
				);

				m_bufferReturnErrorTimer.Start ();
			}
		}
	}
	else
	{
		stiASSERT (false);
	}
#endif

STI_BAIL:

	if (keyFrameNeeded)
	{
		keyframeNeededSignal.Emit ();
	}

	if (pVideoFrame)
	{
		hResult = VideoPlaybackFrameDiscard (pVideoFrame);
		stiASSERT (!stiIS_ERROR (hResult));

		pVideoFrame = nullptr;
	}
}


void CstiVideoOutputBase::applySPSFixups (
	uint8_t *spsNalUnit)
{
	// This method intentionally left blank
	// Derived classes may use this method to modify the SPS NAL Unit
}


/*!
 * \brief Discard a video frame
 *
 * \return stiHResult
 */
stiHResult CstiVideoOutputBase::VideoPlaybackFrameDiscard (
	IstiVideoPlaybackFrame *pVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	auto pVideoPlaybackFrame = static_cast<CstiVideoPlaybackFrame *>(pVideoFrame);

	stiTESTCOND(pVideoPlaybackFrame, stiRESULT_ERROR);

	m_FramesInApplication.remove (pVideoPlaybackFrame);

	hResult = VideoFrameDiscard (pVideoFrame);
	stiTESTRESULT ();

	pVideoFrame = nullptr;

STI_BAIL:

	Unlock ();

	return (hResult);
}


/*!
 * \brief Discard a video frame
 *
 * \return stiHResult
 */
stiHResult CstiVideoOutputBase::VideoFrameDiscard (
	IstiVideoPlaybackFrame *pVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	auto pVideoPlaybackFrame = static_cast<CstiVideoPlaybackFrame *>(pVideoFrame);

	stiTESTCOND(pVideoPlaybackFrame, stiRESULT_ERROR);

	m_oBitstreamFifo.Put (pVideoPlaybackFrame);
	pVideoPlaybackFrame = nullptr;

	if (m_bPlaying)
	{
		stiOSSignalSet2 (m_Signal, 1);
	}

STI_BAIL:

	return (hResult);
}


/*!
 * \brief obtain a video frame
 *
 * \return stiHResult
 */
stiHResult CstiVideoOutputBase::VideoPlaybackFrameGet (
	IstiVideoPlaybackFrame **ppVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiVideoPlaybackFrame *pVideoPlaybackFrame = nullptr;

	Lock ();

	pVideoPlaybackFrame = m_oBitstreamFifo.Get ();

	if (pVideoPlaybackFrame)
	{
		m_lastBitsreamFifoGetSuccesful = true;

		m_FramesInApplication.push_back (pVideoPlaybackFrame);

		if (m_FramesInApplication.size () > m_maxFramesInApplication)
		{
			m_maxFramesInApplication = m_FramesInApplication.size ();

			stiDEBUG_TOOL (g_stiVideoOutputDebug,
				stiTrace ("CstiVideoOutputBase::VideoPlaybackFrameGet: m_maxFramesInApplication = %d\n", m_maxFramesInApplication);
				stiTrace ("CstiVideoOutputBase::VideoPlaybackFrameGet: m_FramesInGstreamer.size = %d\n", m_FramesInGstreamer.size ());
			);
		}

		//
		// If we just went to zero in the fifo then
		// signal to the engine that we are not ready.
		//
		if (m_oBitstreamFifo.CountGet () == 0)
		{
			stiOSSignalSet2 (m_Signal, 0);
		}
	}
	else
	{
		if (m_lastBitsreamFifoGetSuccesful)
		{
			stiASSERTMSG (estiFALSE, "BitstreamFifo.Get () returned NULL: FramesInGstreamer = %d, FramesInApplication = %d\n",
						  m_FramesInGstreamer.size (), m_FramesInApplication.size ());
		}

		m_lastBitsreamFifoGetSuccesful = false;

		//stiTrace ("No frames left in fifo.\n");
		stiDEBUG_TOOL (g_stiVideoOutputDebug,
			stiTrace ("CstiVideoOutputBase::VideoPlaybackFrameGet: m_oBitstreamFifo.Get () returned NULL\n");
			stiTrace ("CstiVideoOutputBase::VideoPlaybackFrameGet: Frames in Application: %d\n", m_FramesInApplication.size ());
			stiTrace ("CstiVideoOutputBase::VideoPlaybackFrameGet: Frames in GStreamer: %d\n", m_FramesInGstreamer.size ());
		);
	}

	Unlock ();

	// implicit upcast
	*ppVideoFrame = pVideoPlaybackFrame;

	return (hResult);
}


/*!
 * \brief Callback method so that GStreamer can tell us when it no longer needs a buffer.
 *
 * \return stiHResult
 */
void CstiVideoOutputBase::GstreamerVideoFrameDiscardCallback (
	IstiVideoPlaybackFrame *videoFrame)
{
	stiDEBUG_TOOL (g_stiVideoOutputDebug > 3,
		stiTrace ("CstiVideoOutputBase::GstreamerVideoFrameDiscardCallback mem = %p\n", videoFrame);
	);

	PostEvent ([this, videoFrame]{ EventGstreamerVideoFrameDiscard (videoFrame); });
}


/*!
 * \brief Gets the device file descriptor
 *
 * \return int
 */
int CstiVideoOutputBase::GetDeviceFD () const
{
	return (stiOSSignalDescriptor (m_Signal));
}


stiHResult CstiVideoOutputBase::DecodePipelineBlackFramePush ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (estiVIDEO_H265 == m_eCodec)
	{
	}
	else if (estiVIDEO_H264 == m_eCodec)
	{
		hResult = H264DecodePipelineBlackFramePush ();
		stiTESTRESULT ();
	}
	else if (estiVIDEO_H263 == m_eCodec)
	{
		hResult = H263DecodePipelineBlackFramePush ();
		stiTESTRESULT ();
	}
	else
	{
		stiASSERTMSG (estiFALSE, "CstiVideoOutput::DecodePipelineBlackFramePush: WARNING m_eCodec is unknown\n");
	}

STI_BAIL:

	return (hResult);
}


/*!\brief Pushes a black frame into the decode pipeline
 *
 */
stiHResult CstiVideoOutputBase::H264DecodePipelineBlackFramePush ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	GStreamerBuffer gstBuffer (cH264_BlackFrame, sizeof (cH264_BlackFrame), nullptr);

	int ret = pushBuffer(gstBuffer);
	stiTESTCOND (ret == GST_FLOW_OK, stiRESULT_ERROR);

STI_BAIL:

	return (hResult);
}


/*!\brief Pushes a black frame into the decode pipeline
 *
 */
stiHResult CstiVideoOutputBase::H263DecodePipelineBlackFramePush ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		stiTrace("CstiVideoOutput::H263DecodePipelineBlackFramePush\n");
	);

	GStreamerBuffer gstBuffer (cH263_BlackFrame, sizeof (cH263_BlackFrame), nullptr);

	int ret = pushBuffer(gstBuffer);
	stiTESTCOND (ret == GST_FLOW_OK, stiRESULT_ERROR);

STI_BAIL:

	return (hResult);
}

/*!
 * \brief EventFilePanTimerTimeout: Processes file pan timer events.
 */
void CstiVideoOutputBase::eventFilePanTimerTimeout ()
{
	stiLOG_ENTRY_NAME (CstiVideoOutput::EventFilePanTimerTimeout);

	if (m_eVideoFilePortalDirection == eMOVE_RIGHT)
	{
		m_nVideoFilePortalX++;
		if (m_nVideoFilePortalX + VIDEO_FILE_PAN_PORTAL_WIDTH == VIDEO_FILE_PAN_WIDTH - 1)
		{
			m_eVideoFilePortalDirection = eMOVE_DOWN;
		}
	}
	else if (m_eVideoFilePortalDirection == eMOVE_DOWN)
	{
		m_nVideoFilePortalY++;
		if (m_nVideoFilePortalY + VIDEO_FILE_PAN_PORTAL_HEIGHT == VIDEO_FILE_PAN_HEIGHT - 1)
		{
			m_eVideoFilePortalDirection = eMOVE_LEFT;
		}
	}
	else if (m_eVideoFilePortalDirection == eMOVE_LEFT)
	{
		m_nVideoFilePortalX--;
		if (m_nVideoFilePortalX == 0)
		{
			m_eVideoFilePortalDirection = eMOVE_UP;
		}
	}
	else
	{
		m_nVideoFilePortalY--;
		if (m_nVideoFilePortalY == 0)
		{
			m_eVideoFilePortalDirection = eMOVE_RIGHT;
		}
	}

	panCropRectUpdate();

	// Restart the pan timer
	m_videoFilePanTimer.Start ();
}



