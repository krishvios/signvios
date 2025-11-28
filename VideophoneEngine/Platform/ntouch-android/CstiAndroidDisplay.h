/*!
* \file CstiSoftwareDisplay.h
* \brief Platform layer Software VideoOutput Abstract Class
*
*  Copyright (C) 2010-2012 by Sorenson Communications, Inc.  All Rights Reserved
*/

#ifndef CSTIANDROIDDISPLAY_H
#define CSTIANDROIDDISPLAY_H

#ifndef WIN32
#include <sys/time.h>
#endif

#include <queue>

#include "jni.h"

#include "CstiSoftwareDisplay.h"

#include "IstiVideoOutput.h"
#include "CstiFFMpegDecoder.h"
#include "CstiOsTaskMQ.h"
#include "stiEventMap.h"
#include "CstiEvent.h"
#include "stiOSSignal.h"


extern const int MIN_IFRAME_REQUEST_DELTA;
extern const int MAX_IFRAME_REQUEST_DELTA;

/**
 * 
 **/
class CstiAndroidDisplay : public CstiSoftwareDisplay
{
public:

	enum EEventType 
	{
		estiVP_DELIVER_FRAME = 512,
	};

	CstiAndroidDisplay ();
	~CstiAndroidDisplay();
    

	virtual stiHResult VideoPlaybackCodecSet (
		EstiVideoCodec eVideoCodec);
		
	virtual stiHResult VideoPlaybackStart ();

	virtual stiHResult VideoPlaybackStop ();
		
	virtual stiHResult VideoPlaybackFramePut (
			vpe::VideoFrame *pVideoFrame) { /* unused */
		return stiRESULT_SUCCESS; }

	virtual stiHResult VideoPlaybackFramePut (
			IstiVideoPlaybackFrame *pVideoFrame);

    virtual stiHResult VideoPlaybackFramePutJava (
        int outId,
        bool isKeyFrame,
        uint32_t dataSize);

	virtual stiHResult VideoPlaybackFrameDiscard (
		IstiVideoPlaybackFrame *pVideoFrame);
    
    virtual stiHResult VideoPlaybackFrameDiscard (
        int bufferId);

	virtual stiHResult VideoCodecsGet (
		std::list<EstiVideoCodec> *pCodecs);

#if DEVICE == DEV_X86
	virtual void ScreenshotTake (EstiScreenshotType eType = eScreenshotFull);
#endif
    
    virtual uint8_t* AllocateDirectByteBuffer(
        uint32_t bytes,
        int *outId);
    
    virtual uint8_t* AllocateBuffer(
        uint32_t bytes,
        int *outId);
    
    virtual uint8_t* ReallocateBuffer(
        uint32_t bytes,
        uint32_t inId,
        int *outId);
    
    virtual void FreeBuffer(
        uint8_t* buffer,
        int bufferId);
    
    virtual EstiBool HardwareCodecSupportedGet() = 0;
    
    virtual void HandleFlags(uint8_t un8Flags);

    virtual void HandleKeyframeRequest();

	virtual void PreferredDisplaySizeGet(unsigned int *displayWidth, unsigned int *displayHeight) const;
    
protected:
    virtual stiHResult InitDecoder();
    
    virtual IstiVideoPlaybackFrame *AllocateVideoPlaybackFrame();

	
private:

	stiDECLARE_EVENT_MAP (CstiAndroidDisplay);
	stiDECLARE_EVENT_DO_NOW (CstiAndroidDisplay);
};

#endif // CSTIDISPLAY_H
