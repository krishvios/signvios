// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
#include "CstiSLICRinger.h"
#include "stiTrace.h"

#define RING_INTERVAL 6000 // Six seconds


CstiSLICRinger::CstiSLICRinger()
:
    m_ringTimer (RING_INTERVAL, this)
{
	crcInit();

	 m_signalConnections.push_back (m_ringTimer.timeoutSignal.Connect (
			[this]{
				ringSlicDevice ();
			}));
}

CstiSLICRinger::~CstiSLICRinger ()
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	eventRingerStop ();
}

void CstiSLICRinger::SocketClose()
{
	if (m_sockFd)
	{
		close(m_sockFd);
		m_sockFd = 0;
	}
}

stiHResult CstiSLICRinger::SocketOpen()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL(g_stiSlicRingerDebug,
		{
			struct timeval tv;
			gettimeofday(&tv, nullptr);
			stiTrace("tomg-> enter SLICRinger::SocketOpen - %d.%6.6d\n", tv.tv_sec, tv.tv_usec);
		}
	);

	struct sockaddr_in ServerAddr;
	struct hostent *pHost;

	m_sockFd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sockFd < 0)
	{
		stiTHROW (stiRESULT_ERROR);
	}
	memset(&ServerAddr, '\0', sizeof(ServerAddr));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(SERVER_PORT);
	ServerAddr.sin_addr.s_addr = inet_addr(SERVER_NAME);

	stiDEBUG_TOOL(g_stiSlicRingerDebug,
		{
			struct timeval tv;
			gettimeofday(&tv, nullptr);
			stiTrace("tomg-> SLICRinger::SocketOpen set addresses - %d.%6.6d\n", tv.tv_sec, tv.tv_usec);
		}
	);

	if (ServerAddr.sin_addr.s_addr == INADDR_NONE)
	{

		stiDEBUG_TOOL(g_stiSlicRingerDebug,
			{
				struct timeval tv;
				gettimeofday(&tv, nullptr);
				stiTrace("tomg-> SLICRinger::SocketOpen calling gethostbyname - %d.%6.6d\n", tv.tv_sec, tv.tv_usec);
			}
		);
		pHost = gethostbyname(SERVER_NAME);
		if (pHost == nullptr)
		{
			stiTHROW (stiRESULT_ERROR);
		}
		memcpy(&ServerAddr.sin_addr, pHost->h_addr, sizeof(ServerAddr.sin_addr));
	}

	stiDEBUG_TOOL(g_stiSlicRingerDebug,
		{
			struct timeval tv;
			gettimeofday(&tv, nullptr);
			stiTrace("tomg-> SLICRinger::SocketOpen calling connect = %d.%6.6d\n", tv.tv_sec, tv.tv_usec);
		}
	);

	if (connect(m_sockFd, (struct sockaddr *)&ServerAddr, sizeof(ServerAddr)) < 0)
	{
		stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
}


void CstiSLICRinger::eventRingerStart ()
{
	if (!m_bFlashing)
	{
		m_bFlashing = true;
		ringSlicDevice ();
	}
}


void CstiSLICRinger::eventRingerStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	unsigned char cSlicBuffer[20];

	if (m_bFlashing)
	{
		stiDEBUG_TOOL(g_stiSlicRingerDebug,
			{
				struct timeval tv;
				gettimeofday(&tv, nullptr);
				stiTrace("tomg-> SLICRingerStop:calling SocketOpen time = %d.%6.6d\n", tv.tv_sec, tv.tv_usec);
			}
		);
		hResult = SocketOpen();
		stiDEBUG_TOOL(g_stiSlicRingerDebug,
			{
				struct timeval tv;
				gettimeofday(&tv, nullptr);
				stiTrace("tomg-> SLICRingerStop:return from SocketOpen time = %d.%6.6d\n", tv.tv_sec, tv.tv_usec);
			}
		);
		stiTESTRESULT ();

		cSlicBuffer[0] = RING_STOP_CMD;
		cSlicBuffer[1] = 3;
		cSlicBuffer[2] = crcFast(cSlicBuffer, 2); 
		hResult = SLICPacketSend(cSlicBuffer, 3);
		stiDEBUG_TOOL(g_stiSlicRingerDebug,
			{
				struct timeval tv;
				gettimeofday(&tv, nullptr);
				stiTrace("tomg-> SLICRingerStop:calling from SocketOpen time = %d.%6.6d\n", tv.tv_sec, tv.tv_usec);
			}
		);
		SocketClose();
		stiDEBUG_TOOL(g_stiSlicRingerDebug,
			{
				struct timeval tv;
				gettimeofday(&tv, nullptr);
				stiTrace("tomg-> SLICRingerStop:return from SocketOpen time = %d.%6.6d\n", tv.tv_sec, tv.tv_usec);
			}
		);
		m_bFlashing = false;
	}

STI_BAIL:

	return;
}


void CstiSLICRinger::eventDeviceDetect ()
{
	stiDEBUG_TOOL(g_stiSlicRingerDebug,
	{
		stiTrace ("eventDeviceDetect is not implemented.\n");
	});
}


stiHResult CstiSLICRinger::SLICPacketSend (unsigned char *pBuff, int nPktSize)
{

	stiHResult hResult = stiRESULT_SUCCESS;

	char cReadBuffer[256];
	unsigned int nOffs;
	int nBytesRead;
	int nBytesToRead;

	if (write(m_sockFd, pBuff, nPktSize) < nPktSize)
	{
		stiTHROW (stiRESULT_ERROR);
	}
/*
 * the slic daemon should reply that it got our command.  if we ever
 * implement the get_status function, we would copy the result back
 * into the buffer we received the command in
 */
	nOffs = 0;
	nBytesToRead = sizeof(cReadBuffer);
	while (nOffs < sizeof(cReadBuffer))
	{
		nBytesRead = stiOSRead(m_sockFd, cReadBuffer + nOffs, nBytesToRead);
		if (nBytesRead < 0)
		{
			fprintf(stderr, "read failed with errno = %d\n", errno);
			stiTHROW (stiRESULT_ERROR);
		}

		if (nBytesRead == 0)
		{
			break;
		}

		nOffs += nBytesRead;
		if (nOffs > 2 && nOffs && nOffs == cReadBuffer[1])
		{
			break;
		}
		nBytesToRead -= nBytesRead;
	}

STI_BAIL:

	return hResult;
}


/*
 * these functions were lifted from the ICD file src/tools/slic/slicd/crc.c
 */
#define WIDTH    (8 * sizeof(slicRingerCrc))
#define TOPBIT   (1 << (WIDTH - 1))
#define POLYNOMIAL 0xD5
#define uint8_t unsigned char


void CstiSLICRinger::crcInit ()
{
	int dividend;
	uint8_t bit;
	slicRingerCrc remainder;

	/*
	 * Compute the remainder of each possible dividend.
	 */
	for (dividend = 0; dividend < 256; ++dividend)
	{
		/*
		 * Start with the dividend followed by zeros.
		 */
		remainder = dividend << (WIDTH - 8);

		/*
		 * Perform modulo-2 division, a bit at a time.
		 */
		for (bit = 8; bit > 0; --bit)
		{
			/*
			 * Try to divide the current data bit.
			 */
			if (remainder & TOPBIT)
			{
				remainder = (remainder << 1) ^ POLYNOMIAL;
			}
			else
			{
				remainder = (remainder << 1);
			}
		}

		crcTable[dividend] = remainder;
	}
}

slicRingerCrc CstiSLICRinger::crcFast (uint8_t const message[], int nBytes)
{
	uint8_t data;
	int byte;
	slicRingerCrc remainder = 0;
	/*
	 * Divide the message by the polynomial, a byte at a time.
	 */
	for (byte = 0; byte < nBytes; ++byte)
	{
		data = message[byte] ^ (remainder >> (WIDTH - 8));
		remainder = crcTable[data] ^ (remainder << 8);
	}
	/*
	 * The final remainder is the CRC.
	 */
	return remainder;
}


void CstiSLICRinger::ringSlicDevice ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	unsigned char cSlicBuff[20];
	if(m_bFlashing)
	{
		stiDEBUG_TOOL(g_stiSlicRingerDebug,
			{
				struct timeval tv;
				gettimeofday(&tv, nullptr);
				stiTrace("tomg-> SLICRingerTimerHandle:calling SocketOpen time = %d.%6.6d\n", tv.tv_sec, tv.tv_usec);
			}
		);
		hResult = SocketOpen();
		stiDEBUG_TOOL(g_stiSlicRingerDebug,
			{
				struct timeval tv;
				gettimeofday(&tv, nullptr);
				stiTrace("tomg-> SLICRingerTimerHandle:return from SocketOpen time = %d.%6.6d\n", tv.tv_sec, tv.tv_usec);
			}
		);
		stiTESTRESULT ();
		
		cSlicBuff[0] = RING_START_CMD;
		cSlicBuff[1] = RING_OPTS_LEN;
		cSlicBuff[2] = RING_OPTS_MSG;
		cSlicBuff[3] = 20;			// frequency
		cSlicBuff[4] = 60;			// cadence on
		cSlicBuff[5] = 60;			// cadence off
		cSlicBuff[6] = 2;			// flash/sound for two seconds
		cSlicBuff[7] = crcFast(cSlicBuff, 7);
		
		hResult = SLICPacketSend(cSlicBuff, RING_OPTS_LEN);
		stiDEBUG_TOOL(g_stiSlicRingerDebug,
			{
				struct timeval tv;
				gettimeofday(&tv, nullptr);
				stiTrace("tomg-> SLICRingerTimerHandle:calling SocketClose time = %d.%6.6d\n", tv.tv_sec, tv.tv_usec);
			}
		);
		SocketClose();
		stiDEBUG_TOOL(g_stiSlicRingerDebug,
			{
				struct timeval tv;
				gettimeofday(&tv, nullptr);
				stiTrace("tomg-> SLICRingerTimerHandle:calling SocketClose time = %d.%6.6d\n", tv.tv_sec, tv.tv_usec);
			}
		);	
		stiTESTRESULT ();
		m_ringTimer.restart ();
	}

STI_BAIL:

	return;	
}
