////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:
//
//  File Name:	RtpPayloadAddon.cpp
//
//  Owner:		Ting-Yu Yang
//
//	Abstract:	This files contains the functions for supporting H264 &
//   generic dynamic payload (such as FECC) based on RadVision Rtp/Rtco package
//
////////////////////////////////////////////////////////////////////////////////

#include "rvtypes.h"
#include "RtpPayloadAddon.h"
#include <cstring>
#include <cstdio>
#include "bitfield.h"
#include "stiAssert.h"
#include "stiTrace.h"

static const int nCONTINUE_BLOCK_HEADER_SIZE = 4;
static const int nNON_CONTINUE_BLOCK_HEADER_SIZE = 1;

/* == H263-1998 == */

struct SstiH2631998PayloadHeader
{
	unsigned short RR:5;
	unsigned short P:1;
	unsigned short V:1;
	unsigned short PLEN:6;
	unsigned short X:1;
	unsigned short PEBIT:2;
};


struct SstiH2631998PayloadInfo
{
	unsigned long PSC:6;
	unsigned long TR:2;
	unsigned long SSI:1;
	unsigned long DCI:1;
	unsigned long FPFR:1;
	unsigned long SRC:3;
	unsigned long PCT:1;
	unsigned long UMV:1;
	unsigned long SAC:1;
	unsigned long AP:1;
	unsigned long PB:1;
	unsigned long QUANT:5;
	unsigned long CPM:1;
	unsigned long PEI:1;
};

RvInt32 RVCALLCONV rtpH2631998Pack(
	IN    void *buf,
	IN    RvInt32 len,
	IN    RvRtpParam *p,
	IN    RvRtpPayloadH263a *h263)
{
#if 1
	p->sByte -= 2;//sizeof (SstiH2631998PayloadHeader);
	auto hPtr=(RvUint16*)((RvUint8*)buf /*+ p->sByte*/);

	auto pPayloadHeader = (SstiH2631998PayloadHeader *)hPtr;
	memset (pPayloadHeader, 0, sizeof (SstiH2631998PayloadHeader));
	pPayloadHeader->PEBIT = h263->eBit;
	//pPayloadHeader->PLEN = h263->sBit;
	pPayloadHeader->P = 1;//h263->p;

	if (pPayloadHeader->P)
	{
		auto pInfo = (SstiH2631998PayloadInfo *)&hPtr[1];
		pPayloadHeader->PLEN = sizeof (SstiH2631998PayloadInfo);
		p->sByte -= sizeof (SstiH2631998PayloadInfo);

		memset (pInfo, 0, sizeof (SstiH2631998PayloadInfo));
		pInfo->QUANT = h263->quant;
		pInfo->SRC = h263->src;
	}
#else
	p->sByte -= 2;
	RvUint32 *hPtr=(RvUint32*)((RvUint8*)buf + p->sByte);
	bitfieldSet (hPtr[0], 1, 5, 1); // P
#endif

#if 0
	// Round-trip test

	//char buf2[len];
	RvRtpParam p2 = *p;
	H263param h2632;
	rtpH2631998Unpack (buf,len,&p2,&h2632);
	if (h263->f != h2632.f) { printf ("\tMismatch %s %d!=%d\n", "f", h263->f, h2632.f); }
	if (h263->p != h2632.p) { printf ("\tMismatch %s %d!=%d\n", "p", h263->p, h2632.p); }
	if (h263->sBit != h2632.sBit) { printf ("\tMismatch %s %d!=%d\n", "sBit", h263->sBit, h2632.sBit); }
	if (h263->eBit != h2632.eBit) { printf ("\tMismatch %s %d!=%d\n", "eBit", h263->eBit, h2632.eBit); }
	if (h263->src != h2632.src) { printf ("\tMismatch %s %d!=%d\n", "src", h263->src, h2632.src); }
	if (h263->i != h2632.i) { printf ("\tMismatch %s %d!=%d\n", "i", h263->i, h2632.i); }
	if (h263->a != h2632.a) { printf ("\tMismatch %s %d!=%d\n", "a", h263->a, h2632.a); }
	if (h263->s != h2632.s) { printf ("\tMismatch %s %d!=%d\n", "s", h263->s, h2632.s); }
	if (h263->dbq != h2632.dbq) { printf ("\tMismatch %s %d!=%d\n", "dbq", h263->dbq, h2632.dbq); }
	if (h263->trb != h2632.trb) { printf ("\tMismatch %s %d!=%d\n", "trb", h263->trb, h2632.trb); }
	if (h263->tr != h2632.tr) { printf ("\tMismatch %s %d!=%d\n", "tr", h263->tr, h2632.tr); }
	if (h263->gobN != h2632.gobN) { printf ("\tMismatch %s %d!=%d\n", "gobN", h263->gobN, h2632.gobN); }
	if (h263->mbaP != h2632.mbaP) { printf ("\tMismatch %s %d!=%d\n", "mbaP", h263->mbaP, h2632.mbaP); }
	if (h263->quant != h2632.quant) { printf ("\tMismatch %s %d!=%d\n", "quant", h263->quant, h2632.quant); }
	if (h263->hMv1 != h2632.hMv1) { printf ("\tMismatch %s %d!=%d\n", "hMv1", h263->hMv1, h2632.hMv1); }
	if (h263->vMv1 != h2632.vMv1) { printf ("\tMismatch %s %d!=%d\n", "vMv1", h263->vMv1, h2632.vMv1); }
	if (h263->hMv2 != h2632.hMv2) { printf ("\tMismatch %s %d!=%d\n", "hMv2", h263->hMv2, h2632.hMv2); }
	if (h263->vMv2 != h2632.vMv2) { printf ("\tMismatch %s %d!=%d\n", "vMv2", h263->vMv2, h2632.vMv2); }
#endif

	return RV_OK;
}


RVAPI RvInt32 RVCALLCONV rtpH2631998Unpack(
	OUT void *buf,
	IN  RvInt32 len,
	OUT RvRtpParam *p,
	OUT RvRtpPayloadH263 *h263)
{
#if 1
	auto hPtr=(RvUint16*)((RvUint8*)buf + p->sByte);
	auto pPayloadHeader = (SstiH2631998PayloadHeader *)hPtr;
	memset (h263, 0, sizeof (RvRtpPayloadH263));
	h263->f = 1;
	//h263->p = 1;
	h263->src = 3;
	h263->quant = 2;
	h263->eBit = pPayloadHeader->PEBIT;
	p->sByte += sizeof (SstiH2631998PayloadHeader);
	p->sByte += pPayloadHeader->PLEN;
	/*if (h263->p) // I think this is accounted for inside PLEN
	{
		p->sByte += 2;
	}*/
#else
	#if 0
	// tool to find bits in stream
	RvUint32 *hPtr=(RvUint32*)((RvUint8*)buf + p->sByte);
	*hPtr = 0xaaaa;
	#else
	RvUint32 *hPtr=(RvUint32*)((RvUint8*)buf + p->sByte);
	memset (h263, 0, sizeof (H263param));
	h263->f = 1;
	h263->src = 3;
	h263->quant = 2;
	//h263->eBit =
	p->sByte += 2;
	p->sByte += bitfieldGet (hPtr[0], 7, 6);
	#endif
#endif
	return RV_OK;
}


RvInt32 RVCALLCONV rtpH2631998GetHeaderLength()
{
	return RvRtpGetHeaderLength () + sizeof (SstiH2631998PayloadInfo) + 2;//sizeof (SstiH2631998PayloadHeader);
}


/* == for dynamic payload type (such as FECC) == */


RvInt32 RVCALLCONV rtpDynamicPack(
	IN    void*buf,
	IN    RvInt32 len,
	IN    RvRtpParam*p,
	IN    void*param)
{
	RV_UNUSED_ARG(buf);

	RV_UNUSED_ARG(len);

	RV_UNUSED_ARG(param);

	return RV_OK;
}

RVAPI
RvInt32 RVCALLCONV rtpDynamicUnpack(
	OUT void*buf,
	IN  RvInt32 len,
	OUT RvRtpParam*p,
	OUT void*param)
{
	RV_UNUSED_ARG(buf);

	RV_UNUSED_ARG(len);

	RV_UNUSED_ARG(param);

	RV_UNUSED_ARG(p);



	return RV_OK;
}

RvInt32 RVCALLCONV rtpDynamicGetHeaderLength()
{
	return RvRtpGetHeaderLength();
}


RvInt32 RVCALLCONV rtpT140Pack(
	IN  void *      buf,
	IN  RvInt32     len,
	IN  RvRtpParam *  p,
	IN  void     *  param)
{
	auto hPtr=(RvUint8*)buf;

	memset (hPtr, 0, sizeof (RvUint8));
	*hPtr = (RvUint8) (0x2 << 5);  // Put in the left 2 bits.

	return RV_OK;
}


RvInt32 RVCALLCONV rtpT140Unpack(
	OUT  void *      buf,
	IN   RvInt32     len,
	OUT  RvRtpParam *  p,
	OUT  void *      param)
{
	RV_UNUSED_ARG(buf);

	RV_UNUSED_ARG(len);

	RV_UNUSED_ARG(param);

	RV_UNUSED_ARG(p);

	return RV_OK;
}


RvInt32 RVCALLCONV rtpT140GetHeaderLength()
{
	return RvRtpGetHeaderLength();
}

RvInt32 RVCALLCONV rtpT140RedPack(
	void *      buf,			// This should be a pointer to the location in the payload area where this header should be placed
	RvBool      bCont,			// Will there be another header?
	RvInt32     n32PayloadType,	// The payload type of the redundant data
	RvUint16    un16TSOffset,	// The timestamp offset since the original data was sent
	RvUint16    un16Len,		// The length in bytes of this redundant data block
	RvRtpParam *p,
	void       *param)
{
	auto hPtr=(unsigned short*)((unsigned char*)buf);

	// Is a short or long header needed?
	if (bCont)
	{
		// Long
		auto pPayloadHeader = (RvUint8*)hPtr;
		RvUint8 aun8Buffer[4];
		aun8Buffer[0] = 1 << 7; // The most significant bit needs to be 1 to signify there are follow-on block headers.
		aun8Buffer[0] |= n32PayloadType; // While this is coming in as a 32-bit value, only the least-significant 7-bits should be set.
		aun8Buffer[1] = (RvUint8)(un16TSOffset >> 6);  // Grab the 8 most significant bits (of the 14 bits total)
		aun8Buffer[2] = ((RvUint8)(un16TSOffset & 0x3f)) << 2;  // Grab the remaining 6 bits and shift them left 2 bits.
		aun8Buffer[2] |= (RvUint8)((un16Len >> 8) & 0x03); // Grab the 2 most significant bits (of the 10 bits total)
		aun8Buffer[3] = (RvUint8)(un16Len & 0xff);  // Grab the remaining 8 bits.
		*pPayloadHeader = aun8Buffer[0];
		pPayloadHeader++;
		*pPayloadHeader = aun8Buffer[1];
		pPayloadHeader++;
		*pPayloadHeader = aun8Buffer[2];
		pPayloadHeader++;
		*pPayloadHeader = aun8Buffer[3];
	}

	else
	{
		// Short
		auto pPayloadHeader = (RvUint8*)hPtr;
		RvUint8 aun8Buffer[1];
		aun8Buffer[0] = 0; // The most significant bit needs to be 0.
		aun8Buffer[0] |= n32PayloadType; // While this is coming in as a 32-bit value, only the least-significant 7-bits should be set.
		*pPayloadHeader = aun8Buffer[0];
	}

	return RV_OK;
}


/*! brief Unpack the T.140 Redundant header and set the pointers to the proper location within the array of structures.
 *
 * \retval The index into the array where the new data is stored.  -1 on error.
 * \param pun8Buf - Pointer to the buffer containing the headers and associated data.
 * \param n32Len - The length of the buffer.
 * \param paUnpackData[] - an array of pointers to structures to load with the block headers and associated data.
 * \param nNbrStructures - the number of structures in the array.
 * \param un32ExtSequenceNumber - the extended sequence number of the received packet.
 * \param p - Pointer to an RTP Parameter structure containing info about the received packet.
 * \param param - not used.
 */
RvInt32 RVCALLCONV rtpT140RedUnpack(
	RvUint8 *pun8Buf,
	RvInt32 n32Len,
	SstiT140Block paUnpackData[],
	int nNbrStructures,
	RvUint32 un32ExtSequenceNumber,
	RvRtpParam *p,
	void *param)
{
	RvInt32 n32Ret = -1;
	int i;
	RvBool bCont;
	RvUint16 un16TSOffset;
	RvUint8 *pun8Current = pun8Buf;
	int nRedundantDataLength = 0;

	memset (paUnpackData, 0, sizeof (SstiT140Block) * nNbrStructures);

	// To be a redundant packet, we have to have at least one byte of data.
	if (n32Len > 0)
	{
		for (i = 0; i < nNbrStructures; i++, pun8Current += nCONTINUE_BLOCK_HEADER_SIZE)
		{
			bCont = *pun8Current >> 7;
			if (bCont)
			{
				if (pun8Current + nCONTINUE_BLOCK_HEADER_SIZE - pun8Buf > n32Len)
				{
					//
					// We have already exceeded the length of the buffer.
					//
					stiASSERT (estiFALSE);
					break;
				}
				// Since this is a continuation block, we have 4 bytes to look at for this block.

				// Payload type is in the lowest 7 bits of the first byte.
				paUnpackData[i].n32PayloadType = *pun8Current & 0x7f;

				// Timestamp offset is is the second byte and the highest six bits of the third byte.
				un16TSOffset = pun8Current[1];
				un16TSOffset <<= 6;
				un16TSOffset |= ((pun8Current[2] & 0xfc) >> 2);

				// Now apply the offset to come up with the original timestamp.
				paUnpackData[i].un32TimeStamp = p->timestamp - un16TSOffset;

				// Block length is the lowest 2 bits of the third byte and the fourth byte.
				paUnpackData[i].un16Len = (RvInt16)(pun8Current[2]) & 0x03;
				paUnpackData[i].un16Len <<= 8;
				paUnpackData[i].un16Len |= pun8Current[3];

				nRedundantDataLength += paUnpackData[i].un16Len;

			}
			else
			{
				if (pun8Current + nNON_CONTINUE_BLOCK_HEADER_SIZE - pun8Buf > n32Len)
				{
					//
					// We have already exceeded the length of the buffer.
					//
					stiASSERT (estiFALSE);
					break;
				}
				// This is the new data, therefore, there is only one byte of header info; the rest is within the RvRtpParam structure.
				// Payload type is in the lowest 7 bits of the first byte.
				paUnpackData[i].n32PayloadType = *pun8Current;
				paUnpackData[i].un32Seq = un32ExtSequenceNumber;
				paUnpackData[i].un16Len = p->len - p->sByte - nRedundantDataLength - (i * nCONTINUE_BLOCK_HEADER_SIZE) - nNON_CONTINUE_BLOCK_HEADER_SIZE;
				paUnpackData[i].un32TimeStamp = p->timestamp;

				// Set the location of this block of data (from within the buffer).  This is the new data.
				paUnpackData[i].pun8Buf = pun8Current + 1 + nRedundantDataLength;

				n32Ret = i;

				// Now, let's go back and update all the earlier blocks with their corresponding sequence number.
				int j;

				for (j = i - 1; j >= 0; j--)
				{
					paUnpackData[j].un32Seq = un32ExtSequenceNumber - (i - j);

					// Set the location of this block of data (from within the buffer).  This is the redundant data.
					paUnpackData[j].pun8Buf = paUnpackData[j + 1].pun8Buf - paUnpackData[j].un16Len;
				}

				// We are done now, break out of this loop.
				break;
			}
		}

		// Did we end because we ran out of structures?
		if (i >= nNbrStructures)
		{
			// We have more block headers than structures to put them in!!!
			stiASSERT(false);
		}
	}

	return n32Ret;
}


RvInt32 RVCALLCONV rtpT140RedGetHeaderLength()
{
	return RvRtpGetHeaderLength();
}


