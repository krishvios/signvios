#include "H263Tools.h"
#include "IstiVideoInput.h"
#include "stiTrace.h"


stiHResult H263StartCodeFind (
	unsigned char *pSrc,
	uint32_t unSrcSize,
	uint32_t * punStartCodePos,
	bool * pbStartCodeFound)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Find the next code that matches 0000 0000 0000 0000 1xxxx xxxx
	uint32_t i;
	uint32_t unCurrentPos = 0;
	*pbStartCodeFound = false;

	for (i = 0; i < unSrcSize; i++)
	{
		if (pSrc[i] == 0 && pSrc[i + 1] == 0 && pSrc[i + 2] >= 128)
		{
			unCurrentPos = i;
			*pbStartCodeFound = true;
			break;
		}
	}

	*punStartCodePos = unCurrentPos;

//STI_BAIL:

	return hResult;
}

stiHResult H263PictureStartCodeFind (
	unsigned char *pSrc,
	uint32_t unSrcSize,
	uint32_t * punPictureStartCodePos,
	bool * pbPictureStartCodeFound)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Find the next code that matches 0000 0000 0000 0000 1000 00xx
	uint32_t unCurrentPos = 0;
	uint32_t unStartCodePos = 0;
	bool bStartCodeFound = false;
	*pbPictureStartCodeFound = false;

	for (;;)
	{
		hResult = H263StartCodeFind (&pSrc[unCurrentPos], unSrcSize - unCurrentPos, &unStartCodePos, &bStartCodeFound);
		stiTESTRESULT ();

		if (unCurrentPos == 0 && !bStartCodeFound)
		{
			break;
		}

		unCurrentPos += unStartCodePos;

		if (0x80 == (pSrc[unCurrentPos + 2] & 0xFC))
		{
			*pbPictureStartCodeFound = true;
			break;
		}
	}

	*punPictureStartCodePos = unCurrentPos;

STI_BAIL:

	return hResult;
}

stiHResult H263PacketHeaderAdd (
	unsigned char *pDst,
	uint32_t unDstSize,
	uint32_t unPacketSize,
	bool bIFrame,
	uint32_t *punCopySize)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	SsmdPacketInfo PacketInfo;
	uint32_t unCopySize = sizeof (SsmdPacketInfo);
/*
	uint32_t size;		// Size of packet including this header
	uint32_t f;			// 0 = GOB-fragmented, 1 = MB-fragmented
	uint32_t p;			// 0 = normal I or P, 1 = PB frame
	uint32_t sBit;		// Number of bits to ignore at the start of this packet
	uint32_t eBit;		// Number of bits to ignore at the start of this packet
	uint32_t src;		// Source format defined by H.263
	uint32_t i;			// 1 = Intra frame, 0 = other
	uint32_t a;			// 1 = Advanced prediction, 0 = off
	uint32_t s;			// 1 = Sytax based Arithmetic Code, 0 = off
	uint32_t dbq;		// Differential quant (DBQUANT), 0 if PB frame option is off.
	uint32_t trb;		// Temporal reference for B frame, 0 if PB frame option is off
	uint32_t tr;			// Temporal reference for P frame, 0 if PB frame option is off
	uint32_t gobN;		// GOB number in effect at the start of the packet
	uint32_t mbaP;		// Starting MB of this packet
	uint32_t quant;		// Quantization value at packet start
	uint32_t hMv1;		// Horizontal MV predictor
	uint32_t vMv1;		// Vertical MV predictor
	uint32_t hMv2;		// Horizontal 4MV predictor, 0 if advanced prediction is off
	uint32_t vMv2;		// Vertical 4MV predictor, 0 if advanced prediction is off
*/

	PacketInfo.size = unPacketSize + unCopySize;
	PacketInfo.f = 0;
	PacketInfo.p = 0;
	PacketInfo.sBit = 0;
	PacketInfo.eBit = 0;
	PacketInfo.src = 3; 	// TODO: Need to set
	PacketInfo.i = bIFrame;
	PacketInfo.a = 0;
	PacketInfo.s = 0;
	PacketInfo.dbq = 0;
	PacketInfo.trb = 0;
	PacketInfo.tr = 0;
	PacketInfo.gobN = 0; 	// TODO: Hopefully we dont have to set
	PacketInfo.mbaP = 0; 	// TODO: Hopefully we dont have to set
	PacketInfo.quant = 0;	// TODO: Hopefully we dont have to set
	PacketInfo.hMv1 = 0;	// TODO: Hopefully we dont have to set
	PacketInfo.vMv1 = 0;	// TODO: Hopefully we dont have to set
	PacketInfo.hMv2 = 0;	// TODO: Hopefully we dont have to set
	PacketInfo.vMv2 = 0;	// TODO: Hopefully we dont have to set


	if (unDstSize > unCopySize)
	{
		memcpy (pDst, &PacketInfo, unCopySize);
	}
	else
	{
		stiTHROWMSG (stiRESULT_ERROR, "unDstSize = %d is less than unCopySize = %d\n", unDstSize, unCopySize);
	}

	*punCopySize = unCopySize;

STI_BAIL:

	return hResult;
}

stiHResult H263PacketGet (
	unsigned char *pSrc,
	uint32_t unSrcSize,
	int nMaxPacketSize,
	uint32_t * punPacketLength)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	uint32_t unPacketLength = -1;
	uint32_t unFoundStartCode = 0;

	if (unSrcSize < (uint32_t)nMaxPacketSize)
	{
		stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug == 2,
			stiTrace ("H263PacketGet: nSrcSize = %d < nMaxPacketSize = %d\n", unSrcSize, nMaxPacketSize);
		);

		unPacketLength = unSrcSize;
	}
	else
	{
		//Start at the nMaxPacketSize and look backward for start code
		for (int i = nMaxPacketSize - 2; i >= 0; i--)
		{
			if (pSrc[i] == 0 && pSrc[i + 1] == 0 && pSrc[i + 2] >= 128)
			{
				unFoundStartCode = i;
				break;
			}
		}

		if (unFoundStartCode)
		{
			unPacketLength = unFoundStartCode;
		}
		else
		{
			stiTHROWMSG (stiRESULT_ERROR, "No start code found\n");
		}
	}

	*punPacketLength = unPacketLength;

STI_BAIL:

	return hResult;
}

stiHResult H263PacketAdd (
	unsigned char *pSrc,
	uint32_t unSrcSize,
	unsigned char *pDst,
	uint32_t unDstSize,
	bool bIFrame,
	uint32_t * punDstPos)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	uint32_t unDstPos = 0;

	hResult = H263PacketHeaderAdd (pDst, unDstSize, unSrcSize, bIFrame, &unDstPos);
	stiTESTRESULT ();

	unDstSize -= unDstPos;

	if (unSrcSize < unDstSize)
	{
		memcpy (&pDst[unDstPos], pSrc, unSrcSize);
		unDstPos += unSrcSize;
	}
	else
	{
		stiTHROWMSG (stiRESULT_ERROR, "Error: nSrcSize > nDstSize\n");
	}

	*punDstPos = unDstPos;

STI_BAIL:

	return hResult;
}

//
// This function takes a h263 bitsream in GOBs
// and breaks it into packets of max size nMaxPacketSize
// It puts a header at the start of every packet
// for internal use
//
stiHResult H263BitstreamPacketize (
	unsigned char *pSrc,
	uint32_t unSrcSize,
	unsigned char *pDst,
	uint32_t unDstSize,
	int nMaxPacketSize,
	bool bIFrame,
	uint32_t * punNewBitstreamSize)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t unSrcPos = 0;
	uint32_t unDstPos = 0;
	uint32_t unPacketSize = 0;
	uint32_t unAddedSize = 0;
	bool bPictureStartCodeFound = false;
	uint32_t unPictureStartCodePos = 0;

	// Find the H263 PSC
	hResult = H263PictureStartCodeFind (pSrc, unSrcSize, &unPictureStartCodePos, &bPictureStartCodeFound);
	stiTESTRESULT ();

	if (!bPictureStartCodeFound)
	{
		stiTHROWMSG (stiRESULT_ERROR, "No PSC found\n");
	}

	for (int i = 0;; i++)
	{
		hResult = H263PacketGet (&pSrc[unSrcPos], unSrcSize - unSrcPos, nMaxPacketSize, &unPacketSize);
		stiTESTRESULT ();

		if (0 == unPacketSize)
		{
			stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug == 2,
				stiTrace ("H263BitstreamPacketize: No more packets\n");
			);
			break;
		}

		stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug == 2,
			stiTrace ("H263BitstreamPacketize: Packet[%d] found nPacketSize = %d\n", i, unPacketSize);
		);

		stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug == 2,
			stiTrace ("H263BitstreamPacketize: Packet[%d] writing to unDstPos = %d\n", i, unDstPos);
		);

		hResult = H263PacketAdd (&pSrc[unSrcPos], unPacketSize, &pDst[unDstPos], unDstSize - unDstPos, bIFrame, &unAddedSize);
		stiTESTRESULT ();

		unDstPos += unAddedSize;

		// This is to keep it on a four byte boundary
		unDstPos = unDstPos + 3;

		unDstPos = unDstPos & ~3;

		unSrcPos += unPacketSize;

	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		*punNewBitstreamSize = 0;
	}
	else
	{
		*punNewBitstreamSize = unDstPos;
	}

	return hResult;
}

