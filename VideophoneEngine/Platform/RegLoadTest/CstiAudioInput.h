// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2022 Sorenson Communications, Inc. -- All rights reserved

#ifndef CSTIAUDIOINPUT_H
#define CSTIAUDIOINPUT_H

#include "stiSVX.h"
#include "stiError.h"
#include "CstiOsTaskMQ.h"
#include "CstiEvent.h"
#include "stiEventMap.h"
#include "CstiSignal.h"

#include "BaseAudioInput.h"

#include <memory>

class CstiAudioInput : public CstiOsTaskMQ, public BaseAudioInput
{
public:
	
	CstiAudioInput ();
	
	virtual ~CstiAudioInput ();
	
	stiHResult Initialize ();

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
	
	stiWDOG_ID m_wdFrameTimer;
	
	FILE *m_pInputFile;

	STI_OS_SIGNAL_TYPE m_PacketReadySignal;

	SstiAudioRecordSettings m_AudioRecordSettings;

	int Task () override;

	stiDECLARE_EVENT_MAP (CstiAudioInput);
	stiDECLARE_EVENT_DO_NOW (CstiAudioInput);
	
	stiHResult TimerHandle (
		CstiEvent *poEvent); // Event

	stiHResult ShutdownHandle (
		CstiEvent* poEvent);  // The event to be handled
};

using CstiAudioInputSharedPtr = std::shared_ptr<CstiAudioInput>;

#endif // CSTIAUDIOINPUT_H

