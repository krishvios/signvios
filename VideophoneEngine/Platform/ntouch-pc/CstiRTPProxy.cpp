// CstiRTPHandler.cpp : Defines the entry point

#include "stiTrace.h"

#include "CstiRTPProxy.h"
#include "CstiVideoInput.h"
#include "CstiAudioInput.h"

#include "rtp.h"
#include "payload.h"

#ifdef RTP_PACKET_DEBUG
int CstiRTPProxy::gnVidRxCounter = 0;
int CstiRTPProxy::gnVidTxCounter = 0;
int CstiRTPProxy::gnAudRxCounter = 0;
int CstiRTPProxy::gnAudTxCounter = 0;
#endif
GipsVideoEngineWindows *CstiRTPProxy::gpVideoEngine = (GipsVideoEngineWindows*)0;
GIPSVENetwork *CstiRTPProxy::gpVeNetwork = (GIPSVENetwork*)0;
GIPSVEBase* CstiRTPProxy::gpVeBase = (GIPSVEBase*)0;
bool CstiRTPProxy::gzInCall = false;
bool CstiRTPProxy::gzH263_MicrosoftPacketization = false;

CstiRTPProxy::CstiRTPProxy()
{
#ifdef RTP_PACKET_DEBUG
	gnVidRxCounter = 0;
	gnVidTxCounter = 0;
	gnAudRxCounter = 0;
	gnAudTxCounter = 0;
#endif
}
CstiRTPProxy::~CstiRTPProxy()
{
}

void CstiRTPProxy::Initialize()
{
}

//==================================================================================//
//
// Use the GipsVideoEngineWindows to process a LOCAL VIDEO Packet using the
// RTPProxyVideoReceive() method to access the GIPS VideoEngine->GIPSVideo_ReceivedRTPPacket()
// method 
//
void CstiRTPProxy::SetGipsVideoEngineWindows(GipsVideoEngineWindows *ipVideoEngine)
{
	gpVideoEngine = ipVideoEngine;
}

void CstiRTPProxy::SetGipsVoiceEngineNetwork(GIPSVENetwork *ipVeNetwork)
{
	gpVeNetwork = ipVeNetwork;
}

void CstiRTPProxy::SetGipsVoiceEngineBase(GIPSVEBase *ipVeBase)
{
	gpVeBase = ipVeBase;
}

void CstiRTPProxy::SetInCall(bool izInCall)
{
#ifdef stiDEBUG_OUTPUT_RTP
	static char szMessage[256];
	sprintf(szMessage, "vpEngine::CstiRTPProxy::SetInCall() - %s", izInCall ? "TRUE" : "FALSE");
	DebugTools::instanceGet()->DebugOutput(szMessage); //...
#endif
	gzInCall = izInCall;
#ifdef RTP_PACKET_DEBUG
	if (izInCall)
	{
		DebugTools::instanceGet()->DebugOutput("%%%%%%%%%% RESETTING Packet Counters %%%%%%%%%%"); //...
		gnVidRxCounter = 0;
		gnVidTxCounter = 0;
		gnAudRxCounter = 0;
		gnAudTxCounter = 0;
	}
#endif
}

void CstiRTPProxy::SetH263_MicrosoftPacketization(bool izH263_MicrosoftPacketization)
{
	gzH263_MicrosoftPacketization = izH263_MicrosoftPacketization;
}

void CstiRTPProxy::SendKeyframeToRemote()
{
	if ((gpVideoEngine != NULL) && (gzInCall))
	{
		gpVideoEngine->GIPSVideo_SendKeyFrame(0);
	}
}
//
////==================================================================================//
////
//// Process a REMOTE VIDEO Packet from the Platform\ntouch-pc CstiVideoOutput for rendering
////  (Uses the ConferenceManager\DataTask\CstiVideoPlayback task to RECEIVE the
////   Packet using the Radvision RTP stack and then call this method)
////
//// Deliver Packet using:
////   nRet = _videoEngine->GIPSVideo_ReceivedRTPPacket(channel, (const char *)pRecvBuffer,nRet);
////
//int CstiRTPProxy::RTPProxyVideoReceive (void *iaPacket, int inLength)
//{
//	int vnResult = -1;
//	if ((gpVideoEngine != NULL) && (gzInCall))
//	{
//#ifdef RTP_PACKET_DEBUG
//		if ((gnVidRxCounter < 10) || (gnVidRxCounter % 10000 == 0))
//		{
//			static char szMessage[256];
//			sprintf(szMessage, "vpEngine::CstiRTPProxy::RTPProxyVideoReceive() - Process a REMOTE VIDEO Packet from the Platform for RENDERING: %d   Length: %d", gnVidRxCounter, inLength);
//			DebugTools::instanceGet()->DebugOutput(szMessage); //...
//		}
//		gnVidRxCounter++;
//#endif
//		char *pRecvBuffer = (char *)iaPacket;
//		int payloadType = pRecvBuffer[1]&0x7f;
//		if((payloadType == 34)&&(gzH263_MicrosoftPacketization))
//		{
//			//GIPS doesn't support Microsoft H263 format so convert 
//			//packet to RFC2190 before passing to GIPS
//#ifdef RTP_PACKET_DEBUG
//			if ((gnVidRxCounter < 10) || (gnVidRxCounter % 10000 == 0))
//			{
//				DebugTools::instanceGet()->DebugOutput("estiH263_Microsoft Packet Received");
//			}
//#endif
//			H263param stH263Info;
//			memset((void*)&stH263Info, 0, sizeof (H263param));
//			rtpParam stRTPParam;
//			memset((void*)&stRTPParam, 0, sizeof (rtpParam));
//			stRTPParam.sByte = 12; //RTP_HEADER_LENGTH = 12;
//			stRTPParam.len = inLength;
//			
//			//Unpack H263a header
//			if(RV_OK == rtpH263Unpack (pRecvBuffer,	inLength, &stRTPParam, &stH263Info))
//			{
//#ifdef RTP_PACKET_DEBUG
//				if ((gnVidRxCounter < 10) || (gnVidRxCounter % 10000 == 0))
//				{
//					DebugTools::instanceGet()->DebugOutput("estiH263_Microsoft Packet Unpacked OK");
//				}
//#endif
//				H263aparam stH263aInfo;
//				memset((void*)&stH263aInfo, 0, sizeof (H263aparam));
//				stH263aInfo.f = stH263Info.f;
//				stH263aInfo.p = stH263Info.p;
//				stH263aInfo.sBit = stH263Info.sBit;
//				stH263aInfo.eBit = stH263Info.eBit;
//				stH263aInfo.src = stH263Info.src;
//				stH263aInfo.i = stH263Info.i;
//				stH263aInfo.u = 0;
//				stH263aInfo.a = stH263Info.a;
//				stH263aInfo.s = stH263Info.s;
//				stH263aInfo.dbq = stH263Info.dbq;
//				stH263aInfo.trb = stH263Info.trb;
//				stH263aInfo.tr = stH263Info.tr;
//				stH263aInfo.gobN = stH263Info.gobN;
//				stH263aInfo.mbaP = stH263Info.mbaP;
//				stH263aInfo.quant = stH263Info.quant;
//				stH263aInfo.hMv1 = stH263Info.hMv1;
//				stH263aInfo.vMv1 = stH263Info.vMv1;
//				stH263aInfo.hMv2 = stH263Info.hMv2;
//				stH263aInfo.vMv2 = stH263Info.vMv2;
//				if(RV_OK != rtpH263aPack(pRecvBuffer, inLength, &stRTPParam, &stH263aInfo))
//				{
//#ifdef RTP_PACKET_DEBUG
//					if ((gnVidRxCounter < 10) || (gnVidRxCounter % 10000 == 0))
//					{
//						DebugTools::instanceGet()->DebugOutput("###ERROR### - Error repacking H263 packet");
//					}
//#endif
//				}
//			}
//		} // if((payloadType == 34)&&(gzH263_MicrosoftPacketization))
//		vnResult = gpVideoEngine->GIPSVideo_ReceivedRTPPacket(0, (const char *)iaPacket, inLength);
//#ifdef stiDEBUG_OUTPUT_RTP
//		if (vnResult != 0)
//		{
//			static char msg[256];
//			int vnError = gpVideoEngine->GIPSVideo_GetLastError();
//			sprintf(msg, "*** ERROR ***    gpVideoEngine->GIPSVideo_ReceivedRTPPacket(): %d", vnError);
//			DebugTools::instanceGet()->DebugOutput(msg);
//		}
//#endif
//	}
//	return vnResult;
//}

int CstiRTPProxy::RTPProxyAudioReceive (void *iaPacket, int inLength)
{
	int vnResult = -1;
	if ((gpVeNetwork != NULL) && (gzInCall))
	{
#ifdef RTP_PACKET_DEBUG
		if ((gnAudRxCounter < 10) || (gnAudRxCounter % 2000 == 0))
		{
			static char szMessage[256];
			sprintf(szMessage, "vpEngine::CstiRTPProxy::RTPProxyAudioReceive()) - Process a REMOTE AUDIO Packet from the Platform for RENDERING: %d   Length: %d", gnAudRxCounter, inLength);
			DebugTools::instanceGet()->DebugOutput(szMessage); //...
		}
		gnAudRxCounter++;
#endif
		vnResult = gpVeNetwork->GIPSVE_ReceivedRTPPacket(0, (const char *)iaPacket, inLength);
#ifdef stiDEBUG_OUTPUT_RTP
		if (vnResult != 0)
		{
#ifdef RTP_PACKET_DEBUG
			if (gnAudRxCounter < 10)
			{
#endif
			static char msg[256];
			int error = gpVeBase->GIPSVE_LastError();
			sprintf(msg, "*** ERROR ***    gpVeNetwork->GIPSVE_ReceivedRTPPacket(): %d", error);
			DebugTools::instanceGet()->DebugOutput(msg);
#ifdef RTP_PACKET_DEBUG
		}
#endif
		}
#endif
	}
	return vnResult;
}

////==================================================================================//
////
//// Deliver a LOCAL VIDEO Packet to the Platform\ntouch-pc CstiVideoInput for sending
////  (Uses the ConferanceManager\DataTask\CstiVideoRecord task to SEND the
////   Packet using the Radvision RTP stack)
////
//// IMPLEMENTED for sending in: VpeExternalTransport::SendPacket()
////
//int CstiRTPProxy::RTPProxyVideoSend (void *iaPacket, int inLength) //'''
//{
//#ifdef RTP_PACKET_DEBUG
//	if ((gnVidTxCounter < 10) || (gnVidTxCounter % 10000 == 0))
//	{
//		static char szMessage[256];
//		sprintf(szMessage, "vpEngine::CstiRTPProxy::RTPProxyVideoSend() -  Deliver a LOCAL VIDEO Packet to the Platform for SENDING: %d   Length: %d", gnVidTxCounter, inLength);
//		DebugTools::instanceGet()->DebugOutput(szMessage); //...
//	}
//	gnVidTxCounter++;
//#endif
//	CstiVideoInput::GetInstance()->DeliverPacket(iaPacket, inLength);
//	return inLength;
//}

int CstiRTPProxy::RTPProxyAudioSend (void *iaPacket, int inLength)
{
#ifdef RTP_PACKET_DEBUG
	if ((gnAudTxCounter < 10) || (gnAudTxCounter % 2000 == 0))
	{
		static char szMessage[256];
		sprintf(szMessage, "vpEngine::CstiRTPProxy::RTPProxyAudioSend() - Deliver a LOCAL AUDIO Packet to the Platform for SENDING: %d   Length: %d", gnAudTxCounter, inLength);
		DebugTools::instanceGet()->DebugOutput(szMessage); //...
	}
	gnAudTxCounter++;
#endif
	CstiAudioInput::GetInstance()->DeliverPacket(iaPacket, inLength);
	return inLength;
}
