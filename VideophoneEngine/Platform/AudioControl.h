/*!
* \file AudioControl.h
* \brief This file containes defines for controlling audio
*
* \date Mon Nov 29 2004
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*/

#ifndef AUDIO_CONTROL_H
#define AUDIO_CONTROL_H

#include "stiDefs.h"

/*! 
* \struct SsmdAudioFrame
* 
* \note Use of this file assumes that the size of an int is 32 bits 
*/ 
typedef struct SsmdAudioFrame
{	
	EstiAudioCodec esmdAudioMode;		//! audio format of this frame 
	uint8_t * pun8Buffer;        		//! pointer to the audio frame buffer 
	uint8_t un8NumFrames;				//! number of audio frames in the buffer
	uint32_t unBufferMaxSize;  		//! total size of the allocated buffer 
	uint32_t unFrameSizeInBytes;		//! size of one frame 
	uint32_t unFrameDuration;			//! time increase of one frame
	EstiBool bIsSIDFrame;				//! silence indicator frame
	uint32_t un32RtpTimestamp;			//! in the case that the AR task should be conscientious of merging
										//! frames or spacing the send, this should be set.  E.G. for audio
										//! bridging, if AudioPlayback detected loss, record side should wait
										//! to send at the appropriate time to account for the loss.
} SsmdAudioFrame;

#endif //#ifndef AUDIO_CONTROL_H
