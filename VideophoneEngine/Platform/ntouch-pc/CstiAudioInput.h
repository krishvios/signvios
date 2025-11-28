#pragma once

#include <sstream>


#include "IPlatformAudioInput.h"
#include "BaseAudioInput.h"
#include <queue>
#include "CstiNativeAudioLink.h"

class CstiAudioInput : public BaseAudioInput, public IPlatformAudioInput
{
public:
	
	CstiAudioInput ();

	static CstiAudioInput *GetInstance();
	static CstiAudioInput *gpCstiAudioInput;
	
	virtual stiHResult AudioRecordStart();

	virtual stiHResult AudioRecordStop();

	virtual stiHResult AudioRecordPacketGet(SsmdAudioFrame * pAudioFrame);

	virtual stiHResult AudioRecordCodecSet(EstiAudioCodec eAudioCodec);
		
	virtual stiHResult AudioRecordSettingsSet(const SstiAudioRecordSettings * pAudioRecordSettings);

	virtual stiHResult EchoModeSet(EsmdEchoMode eEchoMode);
		
	virtual stiHResult PrivacySet(EstiBool bEnable);
		
	virtual stiHResult PrivacyGet(EstiBool *pbEnabled) const;

	virtual int GetDeviceFD () const;

	void PacketDeliver (short *ipBuffer, unsigned int inLen) override;
	
private:
	std::recursive_mutex m_LockMutex;
	EstiBool m_bPrivacy;
	std::queue<short*> m_qAudioPackets;
	bool m_bRunning;


};

