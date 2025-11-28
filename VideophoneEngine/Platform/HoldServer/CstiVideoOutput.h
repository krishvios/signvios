#include "BaseVideoOutput.h"
#include "stiVideoDefs.h"

//class CVideoDeviceController;

class CstiVideoOutput : public BaseVideoOutput
{
public:
		
	stiHResult Initialize ();
	
	virtual stiHResult CallbackRegister (
		PstiAppGenericCallback pfnVPCallback,
		size_t CallbackParam);

	virtual stiHResult CallbackUnregister (
		PstiAppGenericCallback pfnVPCallback,
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
		uint32_t *pun32Width,
		uint32_t *pun32Height) const;

	virtual stiHResult VideoPlaybackCodecSet (
		EstiVideoCodec eVideoCodec);
		
	virtual stiHResult VideoPlaybackStart ();

	virtual stiHResult VideoPlaybackStop ();

	virtual stiHResult VideoPlaybackPacketPut (
		SsmdVideoPacket * pVideoPacket);

private:
		
	//CVideoDeviceController *m_poVideoDeviceController;
	
};
