////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:
//
//  File Name:	RtpPayloadAddon.h
//
//  Owner:		Ting-Yu Yang
//
//	Abstract:	This files contains the functions for supporting H264 &
//   generic dynamic payload (such as FECC) based on RadVision Rtp/Rtco package
//
////////////////////////////////////////////////////////////////////////////////

#ifndef RTPPAYLOADADDON_H
#define RTPPAYLOADADDON_H

#include "rtp.h"
#include "payload.h"

struct SstiT140Block
{
	RvUint8 *pun8Buf;			// The location of this block of data
	RvUint32 un32Seq;			// The would-be sequence number of this block
	RvInt32 n32PayloadType;		// The payload type of the redundant data
	RvUint32 un32TimeStamp;		// The timestamp
	RvUint16 un16Len;			// The length in bytes of this redundant data block
	time_t timeReceived;		// The time the block was received.
};


/* == H263-1998 == */
RvInt32 RVCALLCONV rtpH2631998Pack(
	IN    void *buf,
	IN    RvInt32 len,
	IN    RvRtpParam *p,
	IN    RvRtpPayloadH263a *h263);


RVAPI RvInt32 RVCALLCONV rtpH2631998Unpack(
	OUT void *buf,
	IN  RvInt32 len,
	OUT RvRtpParam*p,
	OUT RvRtpPayloadH263 *h263);


RvInt32 RVCALLCONV rtpH2631998GetHeaderLength();


/* == for dynamic payload type (such as FECC) == */
RVAPI
RvInt32 RVCALLCONV rtpDynamicPack(
		IN  void *      buf,
		IN  RvInt32     len,
		IN  RvRtpParam *  p,
		IN  void     *  param);

RVAPI
RvInt32 RVCALLCONV rtpDynamicUnpack(
		OUT  void *      buf,
		IN   RvInt32     len,
		OUT  RvRtpParam *  p,
		OUT  void *      param);

RVAPI
RvInt32 RVCALLCONV rtpDynamicGetHeaderLength();


/* == T.140 basic == */
RVAPI
RvInt32 RVCALLCONV rtpT140Pack(
		IN  void *      buf,
		IN  RvInt32     len,
		IN  RvRtpParam *  p,
		IN  void     *  param);

RVAPI
RvInt32 RVCALLCONV rtpT140Unpack(
		OUT  void *      buf,
		IN   RvInt32     len,
		OUT  RvRtpParam *  p,
		OUT  void *      param);

RVAPI
RvInt32 RVCALLCONV rtpT140GetHeaderLength();


/* == T.140 Red (redundant) == */
RVAPI
RvInt32 RVCALLCONV rtpT140RedPack(
	void *      buf,
	RvBool      bCont,
	RvInt32     n32PayloadType,
	RvUint16    unTSOffset,
	RvUint16    un16Len,
	RvRtpParam *p,
	void       *param);

RVAPI
RvInt32 RVCALLCONV rtpT140RedUnpack(
	RvUint8 *pun8Buf,
	RvInt32 n32Len,
	SstiT140Block paUnpackData[],
	int nNbrStructures,
	RvUint32 un32ExtSequenceNumber,
	RvRtpParam *p,
	void *param);

RVAPI
RvInt32 RVCALLCONV rtpT140RedGetHeaderLength();

#endif


