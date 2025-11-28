#ifndef CSTIAUDIOINPUT_H
#define CSTIAUDIOINPUT_H

#include "BaseAudioInput.h"
#include "CAudioDeviceController.h"

class CstiAudioInput : public BaseAudioInput
{
public:
	
	CstiAudioInput ();
	
	virtual stiHResult AudioRecordStart ();

	virtual stiHResult AudioRecordStop ();

	virtual stiHResult AudioRecordPacketGet (
		SsmdAudioFrame * pAudioFrame);

	virtual stiHResult AudioRecordCodecSet (
		EstiAudioCodec eAudioCodec);
		
	virtual stiHResult AudioRecordSettingsSet (
		const SstiAudioRecordSettings * pAudioRecordSettings);

	virtual stiHResult EchoModeSet (
		EsmdEchoMode eEchoMode);
		
	virtual stiHResult PrivacySet (
		EstiBool bEnable);
		
	virtual stiHResult PrivacyGet (
		EstiBool *pbEnabled) const;

	virtual int GetDeviceFD () const;

private:
	
	EstiBool m_bPrivacy;
};

#endif // CSTIAUDIOINPUT_H

