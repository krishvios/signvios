#ifndef CSTIAUDIOINPUT_H
#define CSTIAUDIOINPUT_H

#include "IstiAudioInput.h"
#include "CstiCallbackList.h"

class CstiAudioOutput;

class CstiAudioInput : public IstiAudioInput
{
public:
	
	CstiAudioInput ( CstiAudioOutput * );
	
	stiHResult Initialize();
	stiHResult Uninitialize();
	
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

	virtual stiHResult CallbackRegister (
		PstiObjectCallback pfnCallback,
		size_t CallbackParam,
		size_t CallbackFromId);

	virtual stiHResult CallbackUnregister (
		PstiObjectCallback pfnCallback,
		size_t CallbackParam);

	virtual int GetDeviceFD () const;

private:
	
	EstiBool m_bPrivacy;
	CstiAudioOutput *m_pAudioOutput;
	CstiCallbackList m_oCallbacks;
	STI_OS_SIGNAL_TYPE m_fdSignal;
};

#endif // CSTIAUDIOINPUT_H

