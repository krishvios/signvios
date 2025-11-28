#ifndef CSTIDISPLAY_H
#define CSTIDISPLAY_H

#include "BaseVideoOutput.h"
#include "VideoFrame.h"


class CstiDisplay : public BaseVideoOutput
{
public:

	stiHResult Initialize ();
	
	virtual stiHResult CallbackRegister (
		PstiObjectCallback pfnVPCallback,
		size_t CallbackParam,
		size_t CallbackFromId);

	virtual stiHResult CallbackUnregister (
		PstiObjectCallback pfnVPCallback,
		size_t CallbackParam);

	virtual void DisplaySettingsGet (
		SstiDisplaySettings * pDisplaySettings) const;
	
	virtual stiHResult DisplaySettingsSet (
	   const SstiDisplaySettings * pDisplaySettings);
	
	virtual stiHResult RemoteViewPrivacySet (
		EstiBool bPrivacy);
		
	virtual stiHResult RemoteViewHoldSet (
		EstiBool bHold);
		
	virtual stiHResult VideoPlaybackSizeGet (
		uint32_t *punWidth,
		uint32_t *punHeight) const;

	virtual stiHResult VideoPlaybackCodecSet (
		EstiVideoCodec eVideoCodec);
		
	virtual stiHResult VideoPlaybackStart ();

	virtual stiHResult VideoPlaybackStop ();

	virtual stiHResult VideoPlaybackFramePut (
		vpe::VideoFrame * pVideoFrame);
		
	virtual stiHResult VideoPlaybackFrameGet (
		IstiVideoPlaybackFrame **ppVideoFrame);
		
	virtual stiHResult VideoPlaybackFramePut (
		IstiVideoPlaybackFrame *pVideoFrame);

	virtual stiHResult VideoPlaybackFrameDiscard (
		IstiVideoPlaybackFrame *pVideoFrame);

	virtual stiHResult VideoCodecsGet(
		std::list<EstiVideoCodec> *pCodecs);

	virtual void PreferredDisplaySizeGet(
		unsigned int *displayWidth,
		unsigned int *displayHeight) const;
		
	virtual int GetDeviceFD () const;

	virtual stiHResult CodecCapabilitiesGet (
		EstiVideoCodec eCodec,
		SstiVideoCapabilities *pstCaps);
    
#if DEVICE == DEV_X86
	virtual void ScreenshotTake (EstiScreenshotType eType = eScreenshotFull);
#endif
};

#endif // CSTIDISPLAY_H
