//////////////////////////////////////////////////////////////////////////////// 
//
//  ** Copyright (C) 2012 Sorenson Comm, Inc.  All rights reserved. **
//  		PROPRIETARY INFORMATION OF SORENSON COMMUNICATIONS, INC.
//							DO NOT DISTRIBUTE
//
//  File Name:  CstiRTPProxy.h
//
//	Abstract:
//		Common Unmanaged CstiRTPProxy
//
////////////////////////////////////////////////////////////////////////////////

//#pragma once
#ifndef CSTIRTPPROXY_H
#define CSTIRTPPROXY_H

#ifdef stiDEBUG_OUTPUT_RTP
#define RTP_PACKET_DEBUG
#endif

#include <WINSOCK2.H>
#include "GIPS_common_types.h"
#include "GipsVideoEngineWindows.h"
#include "GIPSVENetwork.h"
#include "GIPSVEBase.h"

#define DLLExport __declspec( dllexport )

class DLLExport CstiRTPProxy
{
public:
	CstiRTPProxy();
	~CstiRTPProxy();
private:
	static GipsVideoEngineWindows *gpVideoEngine;
	static GIPSVENetwork* gpVeNetwork;
	static GIPSVEBase* gpVeBase;
	static bool gzInCall;
	static bool gzH263_MicrosoftPacketization;

#ifdef RTP_PACKET_DEBUG
	static int gnVidRxCounter;
	static int gnVidTxCounter;
	static int gnAudRxCounter;
	static int gnAudTxCounter;
#endif
public:
	static void Initialize();
	static void SetGipsVideoEngineWindows(GipsVideoEngineWindows *ipVideoEngine);
	static void SetGipsVoiceEngineNetwork(GIPSVENetwork *ipVeNetwork);
	static void SetGipsVoiceEngineBase(GIPSVEBase *ipVeBase);
	static void SetInCall(bool izInCall);
	static void SetH263_MicrosoftPacketization(bool izH263_MicrosoftPacketization);
	static void SendKeyframeToRemote();

	static int RTPProxyVideoSend (void *iaPacket, int inLength); //'''
	static int RTPProxyAudioSend (void *iaPacket, int inLength);
	static int RTPProxyVideoReceive (void *iaPacket, int inLength);
	static int RTPProxyAudioReceive (void *iaPacket, int inLength);
};
#endif
