////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiVideoPlaybackRead
//
//  File Name:	CstiVideoPlaybackRead.cpp
//
//  Owner:		Ting-Yu Yang
//
//	Abstract:
//		Video playback Read class definition
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stiOS.h"

#include "rtp.h"
#include "rtcp.h"
#include "payload.h"

#include "RtpPayloadAddon.h"
#include "CstiVideoPlaybackRead.h"
#include "stiTaskInfo.h"

#include <boost/optional.hpp>


// Note: FRAME_PACKET_ANALYSIS is defined in VideophoneEngine > ConfMgr > DataTasks > DataCommon > CstiPacket.h
//       The analysis needs to access some protected variables in the CstiPacket class.  See where the friend classes are set.
#ifdef FRAME_PACKET_ANALYSIS
#include "stdint.h"
/*
 
The frame packet analysis code associates packets into the appropriate frame.  The packets for 3 consecutive frames are are tracked.  When the first packet
of the 4th frame arrives, a simple analysis is performed on the 3rd frame and pertinent information is printed.  Then the frames are shifted
so that the 2nd frame is shifted into the 3rd frame, and so forth.

Every NUM_FRAME_INTERVALS frames a new frame average is calculated.  Intervals between frames that differ (+/-) more than DELTA_FRAME_ARRIVAL_TIME milliseconds from the average are printed out.
The frame interval between frame N and frame N+1 is defined as the time between the arrival of the first packet of frame N and the first packet of frame N+1

!!!! BUG:   Packet sequence number wrapping is not handled correctly and producess a false result.

tsc_get_clock() is implemented in the Acme sdk (tunnel code). If this library is not available then this analysis cannot be used unless an alternative clock is used.

Possible Improvements
(1) Figure how to tell if the first packet in a frame is REALLY the first packet in a frame.  Right now, it is the first packet with the frame's timestamp.
(2) Handle the wrapping of sequence numbers correctly
(3) The analysis is not started over at the beginning of each call.
(4) The information is printed out with printf's which is expensive on the PC.

*/

extern "C"
{
	uint32_t tsc_get_clock();
}
class FramePackets
{
public:
	FramePackets()  {m_timestamp = 0; m_nrtpParam = 0; m_firstPktClock = 0;}
	~FramePackets() {m_timestamp = 0; m_nrtpParam = 0; m_firstPktClock = 0;}

	enum {MAX_PACKETS = 32};

	uint32_t    m_firstPktClock;
	RvUint32    m_timestamp;
	int         m_nrtpParam;
	RvRtpParam  m_rtpParam[MAX_PACKETS];

	RvUint16    FirstSeq();
	RvUint16    LastSeq();

	void        Init(RvUint32 timestamp);
	void        AddRvRtpParam(const RvRtpParam &rtpParam);
	int         Analyze();
	bool        IsEndMarkerMissing();
};

void FramePackets::Init(RvUint32 timestamp)
{
	m_timestamp     = timestamp;
	m_nrtpParam     = 0;
	m_firstPktClock = tsc_get_clock();
}

void FramePackets::AddRvRtpParam(const RvRtpParam &rtpParam)
{
	if (m_nrtpParam < MAX_PACKETS)
	{
		m_rtpParam[m_nrtpParam++] = rtpParam;

		for(int i = m_nrtpParam - 1; (i > 0) && (m_rtpParam[i-1].sequenceNumber > m_rtpParam[i].sequenceNumber); i--)
		{
			RvRtpParam tmp  = m_rtpParam[i-1];
			m_rtpParam[i-1] = m_rtpParam[i];
			m_rtpParam[i]   = tmp;
		}
	}
	else
	{
		printf("m_nrtpParam %d > MAX_PACKETS %d\n", m_nrtpParam, MAX_PACKETS);
	}
}

RvUint16 FramePackets::FirstSeq()
{
	RvUint16 result = 0;
	if (m_nrtpParam > 0)
	{
		result = m_rtpParam[0].sequenceNumber;
	}
	return result;
}

RvUint16 FramePackets::LastSeq()
{
	RvUint16 result = 0;
	if (m_nrtpParam > 0)
	{
		result = m_rtpParam[m_nrtpParam-1].sequenceNumber;
	}
	return result;
}


int FramePackets::Analyze()
{
	int lostPkts = 0;
	for(int i=1; i<m_nrtpParam; i++)
	{
		lostPkts += (m_rtpParam[i].sequenceNumber - (m_rtpParam[i-1].sequenceNumber + 1));
	}

	int numEndMarkers = 0;
	for(int i=0; i<m_nrtpParam; i++)
	{
		if (m_rtpParam[i].marker)
		{
			numEndMarkers++;
		}
	}

	if ((lostPkts > 0) || (!m_rtpParam[m_nrtpParam-1].marker) || (numEndMarkers != 1))
	{
		printf("\nLost Pkt or no End Marker: ts %d lost %d [ ", m_timestamp, lostPkts);
		for(int i=0; i<m_nrtpParam; i++)
		{
			if (i == (m_nrtpParam - 1))
			{
				 printf("%d EndMarker %d ]\n", m_rtpParam[i].sequenceNumber, m_rtpParam[i].marker);
			}
			else if (m_rtpParam[i].marker)
			{
				printf("<%d EndMarker %d>, \n", m_rtpParam[i].sequenceNumber, m_rtpParam[i].marker);
			}
			else
			{
				printf("%d, ", m_rtpParam[i].sequenceNumber);
			}
		}
	}

	if ((m_nrtpParam > 0) && m_rtpParam[m_nrtpParam - 1].marker && (m_rtpParam[m_nrtpParam - 1].len == 0))
	{
		printf("********* End Packet has len == 0\n");
	}
	return lostPkts;
}

bool FramePackets::IsEndMarkerMissing()
{
	return (m_nrtpParam > 0) && (!m_rtpParam[m_nrtpParam - 1].marker);
}

class FramePacketsHistory
{
public:
	FramePacketsHistory() {Init();}
	~FramePacketsHistory() {}

	void Init();
	void AddPkt(CstiVideoPacket * poPacket);

	enum {MAX_FRAMES = 3, NUM_FRAME_INTERVALS = 100, DELTA_FRAME_ARRIVAL_TIME = 20};
	FramePackets   m_framePackets[MAX_FRAMES];

	int m_frame;
	int m_framePrev;
	int m_framePrevPrev;

	int m_frameCount;
	int m_frameIntervalCount;
	int m_badFrameCount;
	int m_betweenFramesLostCount;
	int m_withinFramesLostCount;
	int m_missingEndMarkerCount;
	int m_averageFrameInterval;

	uint32_t    m_TotalFrameIntervalTime;
};

void FramePacketsHistory::Init()
{
	m_frame                 = 0;
	m_framePrev             = 1;
	m_framePrevPrev         = 2;

	m_frameCount            = 0;
	m_frameIntervalCount    = 0;
	m_badFrameCount         = 0;
	m_betweenFramesLostCount= 0;
	m_withinFramesLostCount = 0;
	m_missingEndMarkerCount = 0;
	m_TotalFrameIntervalTime= 0;
	m_averageFrameInterval  = 0;

	m_framePackets[m_frame].Init(0);
	m_framePackets[m_framePrev].Init(0);
	m_framePackets[m_framePrevPrev].Init(0);
}

void FramePacketsHistory::AddPkt(CstiVideoPacket * poPacket)
{
	RvUint32 timestamp = poPacket->m_stRTPParam.timestamp;

	if (m_framePackets[m_frame].m_timestamp == timestamp)
	{
		m_framePackets[m_frame].AddRvRtpParam(poPacket->m_stRTPParam);
	}
	else if (m_framePackets[m_framePrev].m_timestamp == timestamp)
	{
		 m_framePackets[m_framePrev].AddRvRtpParam(poPacket->m_stRTPParam);
	}
	else if (m_framePackets[m_framePrevPrev].m_timestamp == timestamp)
	{
		 m_framePackets[m_framePrevPrev].AddRvRtpParam(poPacket->m_stRTPParam);
	}
	else
	{
		m_frameCount++;
		bool goodFrame = true;

		uint32_t firstPkt2firstPktIntervalInMilliseconds = m_framePackets[m_framePrev].m_firstPktClock - m_framePackets[m_framePrevPrev].m_firstPktClock;

		if (m_averageFrameInterval > 0)
		{
			int frameIntervalDelta = abs((int)(m_averageFrameInterval - firstPkt2firstPktIntervalInMilliseconds));
			if (DELTA_FRAME_ARRIVAL_TIME <= frameIntervalDelta)
			{
				printf("Interval between 1st pkt of frame %6d to 1st pkt of frame %6d: %3d ms\n", m_frameCount-3,  m_frameCount-2, firstPkt2firstPktIntervalInMilliseconds);
			}
		}

		m_frameIntervalCount++;
		m_TotalFrameIntervalTime += firstPkt2firstPktIntervalInMilliseconds;
		if (m_frameIntervalCount > NUM_FRAME_INTERVALS)
		{
			m_averageFrameInterval      = m_TotalFrameIntervalTime/m_frameIntervalCount;
			m_TotalFrameIntervalTime    = firstPkt2firstPktIntervalInMilliseconds;
			m_frameIntervalCount        = 1;

			printf("Average Frame Interval %d\n", m_averageFrameInterval);
		}

		if (m_framePackets[m_framePrevPrev].m_nrtpParam > 0)
		{
			RvUint16 lastSeq  = m_framePackets[m_framePrevPrev].LastSeq();
			RvUint16 firstSeq = m_framePackets[m_framePrev].FirstSeq();

			int lostBetweenFrames = firstSeq - (lastSeq + 1);
			if (lostBetweenFrames > 0)
			{
				goodFrame = false;
				m_betweenFramesLostCount++;
				printf("\nLost %d packets between [... %d] ... [%d ...]\n", lostBetweenFrames, lastSeq, firstSeq);
			}
			if (m_framePackets[m_framePrevPrev].Analyze() > 0)
			{
				goodFrame = false;
				m_withinFramesLostCount++;
			}
			if (m_framePackets[m_framePrevPrev].IsEndMarkerMissing())
			{
				goodFrame = false;
				m_missingEndMarkerCount++;
			}
		}

		if (goodFrame)
		{
			//printf("Good %d\n", frameCount);
		}
		else
		{
			m_badFrameCount++;
			printf("Frames Total %d, Bad %d, Missing [pkts between %d, pkts within %d, end markers %d], percentage %10.4f\n\n",
				   m_frameCount,
				   m_badFrameCount,
				   m_betweenFramesLostCount,
				   m_withinFramesLostCount,
				   m_missingEndMarkerCount,
				   (double)m_badFrameCount*100.0/(double)m_frameCount);
		}

		int tmp         = m_framePrevPrev;
		m_framePrevPrev = m_framePrev;
		m_framePrev     = m_frame;
		m_frame         = tmp;

		m_framePackets[m_frame].Init(timestamp);
		m_framePackets[m_frame].AddRvRtpParam(poPacket->m_stRTPParam);
	}
}
#endif

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//
// #define stiTEST_FOR_LOST_PACKETS
#ifdef stiTEST_FOR_LOST_PACKETS
bool g_abPacketsReceived[65535];
int g_anPacketsReceived[65535];
int g_nPacketArrayIndex;
int g_nFirstPacketReceived;
#endif // #ifdef stiTEST_FOR_LOST_PACKETS

#ifdef sciRTP_FROM_FILE
const char g_szRTP_FILE[] = "/data/ntouch/VideoIn.rtp";

typedef struct
{
	struct timeval start;  /* start of recording (GMT) */
	uint32_t source;        /* network source (multicast address) */
	uint16_t port;          /* UDP port */
} RD_hdr_t;

typedef struct
{
	uint16_t length;    /* length of packet, including this header (may
							be smaller than plen if not whole packet recorded) */
	uint16_t plen;      /* actual header+payload length for RTP, 0 for RTCP */
	uint32_t offset;    /* milliseconds since the start of recording */
} RD_packet_t;

FILE *fp;
int g_nPkts;
#endif

//
// Locals
//
#ifdef ercDebugPacketQueue
static int g_nTotalCount = 0;
#endif //#ifdef ercDebugPacketQueue


///////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlaybackRead::CstiVideoPlaybackRead
//
//  Description: Class constructor.
//
//  Abstract:
//
//
//  Returns: None.
//
CstiVideoPlaybackRead::CstiVideoPlaybackRead (const std::shared_ptr<vpe::RtpSession> &session)
:
	PlaybackReadType(stiVP_READ_TASK_NAME, session)
{
	stiLOG_ENTRY_NAME (CstiVideoPlaybackRead::CstiVideoPlaybackRead);

	StatsClear ();
	
#ifdef stiTEST_FOR_LOST_PACKETS
	memset (g_abPacketsReceived, 0, sizeof(g_abPacketsReceived));
	memset (g_anPacketsReceived, -1, sizeof (g_anPacketsReceived));
	stiTrace ("g_anPacketsReceived = %d, int = %d, 65535 * sizeof(int) = %d\n", sizeof (g_anPacketsReceived), sizeof (int), 65535 * sizeof(int));
	g_nPacketArrayIndex = 0;
	g_nFirstPacketReceived = -1;
#endif // #ifdef stiTEST_FOR_LOST_PACKETS
} // end CstiVideoPlaybackRead::CstiVideoPlaybackRead

////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlaybackRead::~CstiVideoPlaybackRead
//
// Description: Destructor
//
// Abstract:
//
// Returns:
//	None
//
CstiVideoPlaybackRead::~CstiVideoPlaybackRead ()
{
	stiLOG_ENTRY_NAME (CstiVideoPlaybackRead::~CstiVideoPlaybackRead);

#ifdef stiTEST_FOR_LOST_PACKETS
	if (-1 < g_nFirstPacketReceived)
	{
		int nStartWindow = -1;
		int nEndWindow = -1;
		int i, j;
		bool bFound = false;
		
		// This trace identifies packets that didn't show up.  Note that if allowed to run for very long,
		// (over 65535 packets worth) distorts the view since the array will be written over a second time.
		// Watch the traces during the run, it prints out every received sequence number that is evenly 
		// divided by 500 (e.g. 500, 1000, ... 43500, 44000, ...).
		stiTrace ("<VPR> Missing Packets:\n");
		for (i = 0; i < g_nPacketArrayIndex; i++)
		{
			if (!g_abPacketsReceived[i])
			{
				if (0 < nStartWindow)
				{
					nEndWindow = i + g_nFirstPacketReceived;
				}
				else
				{
					nStartWindow = nEndWindow = i + g_nFirstPacketReceived;
				}
			}
			else if (0 < nStartWindow)
			{
				stiTrace ("\tMissing packets %d-%d, %d packets\n", nStartWindow, nEndWindow, nEndWindow - nStartWindow + 1);
				nStartWindow = nEndWindow = -1;
				bFound = true;
			}
		}

		nEndWindow = 65535;
		if (0 < nStartWindow)
		{
			stiTrace ("\tMissing pkts %d-%d, %d pkts\n", nStartWindow, nEndWindow, nEndWindow - nStartWindow + 1);
			nStartWindow = nEndWindow = -1;
			bFound = true;
		}
		
		if (!bFound)
		{
			stiTrace ("\tNo missing packets found.  :-)\n");
		}

		// This trace gives perspective to out of order (ooo) packets.  Note that the first packets read
		// will most likely report OOO since we rarely start receiving at sequence number 0.
		int nNext = g_nFirstPacketReceived;
		for (i = 0; i < 65535 && g_anPacketsReceived[i] != -1; i++)
		{
			if (nNext != g_anPacketsReceived[i])
			{
				stiTrace ("Expected %d at entry %d.  Sequence found (and the following 9) are:\n\t", nNext, i);
				for (j = i; j < i + 10; j++)
				{
					if (j < 65535)
					{
						stiTrace ("%d, ", g_anPacketsReceived[j]);
					}
				}
				stiTrace ("\n");
				nNext = g_anPacketsReceived[i] + 1;
				if (nNext > 65535)
				{
					nNext = 0;
				}
			}
			else
			{
				nNext++;
			}
		}
	}
	else
	{
		stiTrace ("<VPR> Missing Packets:\n\tDidn't receive any packets to report on.\n");
	}
#endif // #ifdef stiTEST_FOR_LOST_PACKETS

	Shutdown ();

	// Make sure we didn't accidentally lose any buffers in the transistions between queues
	auto packetCount =
		m_emptyPacketPool.count () +
		m_oFullQueue.CountGet () +
		m_oRecvdQueue.CountGet ();

	stiASSERTMSG (packetCount == nMAX_VP_BUFFERS,
		"Not all buffers accounted for (%d+%d+%d != %d) - memory leak\n",
		m_emptyPacketPool.count (),
		m_oFullQueue.CountGet (),
		m_oRecvdQueue.CountGet (),
		nMAX_VP_BUFFERS);

	// Empty the queues
	m_oRecvdQueue.clear ();
	m_oFullQueue.clear ();
	
	// Destroy the packet pool
	m_emptyPacketPool.destroy ();

} // end CstiVideoPlaybackRead::~CstiVideoPlaybackRead


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlaybackRead::Initialize
//
// Description: Initializes the task
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on Success, otherwise estiERROR
//
stiHResult CstiVideoPlaybackRead::Initialize ()
{
	stiLOG_ENTRY_NAME (CstiVideoPlaybackRead::Initialize);

	stiDEBUG_TOOL ( g_stiVideoPlaybackReadDebug,
		stiTrace("CstiVideoPlaybackRead::Initialize\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;

	// Create the empty packet pool with the buffered video packets
	m_emptyPacketPool.create (nMAX_VP_BUFFERS);

	// Initialize the empty pools's video buffers
	for (int i = 0; i < nMAX_VP_BUFFERS; i++)
	{
		// Initialize the video driver structures.
		m_astVideoStruct[i].buffer = &m_videoBuffer[i][0];

		// Add all structures to the packets in the pool.
		m_emptyPacketPool.bufferAdd (&m_videoBuffer[i][0], nMAX_VIDEO_PLAYBACK_PACKET_BUFFER, &m_astVideoStruct[i]);
	}

#ifdef ercDebugPacketQueue
	VideoPacketQueuesLock();
	int nEQCount, nFQCount, nPQCount;
	nEQCount = m_emptyPacketPool.count ();
	nFQCount = m_oFullQueue.CountGet ();
	nPQCount = poPacketQueue->CountGet ();
	VideoPacketQueuesUnlock();
	g_nTotalCount  = nEQCount + nFQCount + nPQCount;
	stiTrace ("<VPR::Initialize>\n");
		stiTrace ("\tEmptyQueue:%d packets\n", nEQCount);
		stiTrace ("\tFullQueue::%d packets\n", nFQCount);
		stiTrace ("\tPacketQueue(ooo)::%d packets\n", nPQCount);
#endif //#ifdef ercDebugPacketQueue
		
	// Store a static copy of this class

//STI_BAIL:

	return (hResult);

} // end CstiVideoPlaybackRead::Initialize


#ifdef ercDebugPacketQueue
void CstiVideoPlaybackRead::VideoPacketQueuesLock ()
{
	m_oEmptyQueue.Lock ();
	m_oFullQueue.Lock ();
	poPacketQueue->Lock ();
}
void CstiVideoPlaybackRead::VideoPacketQueuesUnlock ()
{
	poPacketQueue->Unlock ();
	m_oFullQueue.Unlock ();
	m_oEmptyQueue.Unlock ();
}
#endif //#ifdef ercDebugPacketQueue


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlaybackRead::PacketProcessFromPort
//
// Description: Read and process a packet from the RTP Port
//
// Abstract:
//
// Returns:
//	ePACKET_OK - read successful
//	ePACKET_ERROR - read failed
//	ePACKET_CONTINUE - no data available to read
//
EstiPacketResult CstiVideoPlaybackRead::PacketProcessFromPort ()
{
	stiLOG_ENTRY_NAME (CstiVideoPlaybackRead::PacketProcessFromPort);

	EstiPacketResult ePacketResult = ePACKET_OK;

	// Get an empty packet from the queue
	auto packet = m_emptyPacketPool.packetGet();

	// Were we able to get a packet?
	if (!packet)
	{
		m_Stats.packetQueueEmptyErrors++;

		// No! This is a major problem!
		ePacketResult = ePACKET_ERROR;
#ifndef ercDebugPacketQueue
		stiASSERT (estiFALSE);
#else
		static bool bPrinted = false;
		if (!bPrinted)
		{
			int nCount;
			stiTrace ("<VPR::PacketProcessFromPort>\n");
			nCount = m_oEmptyQueue.CountGet ();
			stiTrace ("\tEmptyQueue:%d packets\n", nCount);
			nCount = m_oFullQueue.CountGet ();
			stiTrace ("\tFullQueue::%d packets\n", nCount);
			nCount = poPacketQueue->CountGet ();
			stiTrace ("\tPacketQueue(ooo)::%d packets\n", nCount);
		}
		bPrinted = true;
#endif //#ifdef ercDebugPacketQueue
	}
	else
	{
#ifdef ercDebugPacketQueue
		VideoPacketQueuesLock ();
		int nEQCount, nFQCount, nPQCount;
		nEQCount = m_emptyPacketPool.count ();
		nFQCount = m_oFullQueue.CountGet ();
		nPQCount = poPacketQueue->CountGet ();
		VideoPacketQueuesUnlock ();
		int nCurrentCount = nEQCount + nFQCount + nPQCount;// + nSQCount + nFRQCount;
		int nTolerance = nCurrentCount + 1;
		if (g_stiStunDebug)  // Set the amount of allowed tolerance in g_stiStunDebug tool.  Otherwise, it will be 1.
		{
			nTolerance = nCurrentCount + g_stiStunDebug;
		}
		if (g_nTotalCount > nTolerance)  // Allow for one for the read pkt in the VP task.
		{
			stiTrace ("<VPR::PacketProcessFromPort> Current Count = %d, Total Count = %d\n", nCurrentCount, g_nTotalCount);
			stiTrace ("\tEmptyQueue:%d packets\n", nEQCount);
			stiTrace ("\tFullQueue::%d packets\n", nFQCount);
			stiTrace ("\tPacketQueue(ooo)::%d packets\n", nPQCount);
			if (0 < (1 & g_stiUPnPDebug))
			{
				stiOSTaskDelay (3000000);
			}
		}
#endif //#ifdef ercDebugPacketQueue

		// Read a packet from the RTP layer and place it into this packet.
		ePacketResult = PacketRead (packet);

#if 0
	static int nCount = 0;
	nCount ++;
	if (nCount < 10 && nCount > 2)
	{
	  stiTrace("VPR:: dropping SQ# %d\n", (poPacket->RTPParamGet ())->sequenceNumber);
	  ePacketResult = ePACKET_CONTINUE;
	}
#endif

		// Was the packet read successful
		if (ePACKET_OK == ePacketResult)
		{
			// Yes, We got a packet, so process it now.
			ReadPacketProcess (packet);
		}
	} // end else

	return (ePacketResult);
} // end CstiVideoPlaybackRead::PacketProcessFromPort


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlaybackRead::ReadPacketProcess
//
// Description: Process the packet that was read from the port
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on Success, othwise estiERROR
//
EstiResult CstiVideoPlaybackRead::ReadPacketProcess (
	const std::shared_ptr<CstiVideoPacket> &packet)
{
	stiLOG_ENTRY_NAME (CstiVideoPlaybackRead::ReadPacketProcess);

	EstiResult eResult = estiOK;

	eResult = DataUnPack (packet);

#ifdef FRAME_PACKET_ANALYSIS

	static FramePacketsHistory s_framePacketsHistory;

	if (estiOK == eResult)
	{
		if (poPacket->m_stRTPParam.timestamp == 0)
		{
			printf("timestamp == 0, seq# %d, len %d\n", poPacket->m_stRTPParam.sequenceNumber, poPacket->m_stRTPParam.len);
		}

		s_framePacketsHistory.AddPkt(poPacket);

		static RvUint16 s_prevSeq = 0;
		if (((s_prevSeq+1) % 65536) != poPacket->m_stRTPParam.sequenceNumber)
		{
			printf("CstiVideoPlaybackRead::ReadPacketProcess OOO Seq exp: %d, got: %d\n",s_prevSeq+1, poPacket->m_stRTPParam.sequenceNumber);
		}
		s_prevSeq = poPacket->m_stRTPParam.sequenceNumber;
	}
#endif

	if (estiOK == eResult)
	{
		if (1 == m_oFullQueue.AddAndCountGet (packet))
		{
			dataAvailableSignal.Emit();
		}
		/*
			stiDEBUG_TOOL (g_stiVideoPlaybackReadDebug,
				static int nCnt = 0;
				if (++nCnt == 1)
				{
					stiTrace ("VPR:: Buffer Empty Left: %d\n", m_oEmptyQueue.CountGet ());
					nCnt = 0;
				}
			); // stiDEBUG_TOOLS
		 */
		stiDEBUG_TOOL (g_stiVideoPlaybackReadDebug,
				if (m_nDiscardedVideoPackets > 0)
				{
					stiTrace (
							"VPR:: Accepting packets again.  Discarded %d Video packets, empty = %d Full = %d\n\n",
							m_nDiscardedVideoPackets,
							m_emptyPacketPool.count (),
							m_oFullQueue.CountGet());
					m_nDiscardedVideoPackets = 0;
				} // end if
		); // stiDEBUG_TOOLS
	}

	return (eResult);
} // end CstiVideoPlaybackRead::ReadPacketProcess


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlaybackRead::PacketRead
//
// Description:
//
// Abstract:
//
// Returns:
//	ePACKET_OK - read successful

//	ePACKET_ERROR - read failed
//	ePACKET_CONTINUE - no data available to read
//
EstiPacketResult CstiVideoPlaybackRead::PacketRead (
	const std::shared_ptr<CstiVideoPacket> &packet)
{
	stiLOG_ENTRY_NAME (CstiVideoPlaybackRead::PacketRead);

	EstiPacketResult eReadResult = ePACKET_CONTINUE;
	int nBytesRead;
	RvRtpParam * pstRTPParam = packet->RTPParamGet();

	if (!m_bRtpHalted && nullptr != m_rtpSession->rtpGet() && nullptr == m_rtpSession->rtpTransportGet())
	{
		// Yes! Read the data from the socket. NOTE - the Radvision
		// documentation is incorrect. RvRtpRead does NOT return the number of
		// bytes read, it returns the result of rtpUnpack.  Was RvRtpRead
		// successful?
		//stiTrace("VPR:: %d Bytes in Socket\n",rtpGetAvailableBytes(m_poCall->VideoRtpSessionGet ()));

		RvInt32 eRvStatus = m_rtpSession->RtpRead (
			packet->BufferGet (),
			nMAX_VIDEO_PLAYBACK_PACKET_BUFFER,
			pstRTPParam);
		
		if (eRvStatus > RV_ERROR_UNKNOWN && pstRTPParam->len)
		{
#ifdef sciRTP_FROM_FILE
			RD_packet_t RDPacket;

			//
			// Read the rtp dump header.
			// The file being read is in rtpDump format (see http://www.cs.columbia.edu/irt/software/rtptools/)
			// This type of file can produced from Wireshark by navigating through the
			// Telephony/RTP/Show All Streams menu items.  Select the stream
			// you wish to dump and click on "Save as".
			//
			size_t nNumElements = fread (&RDPacket, sizeof (RDPacket), 1, fp);

			if (nNumElements == 0)
			{
				fseek (fp, 0L, SEEK_SET);
				g_nPkts = 1;
				//
				// Read the file header.  The header is terminated by a carriage return.
				//
				while (getc (fp) != '\n');

				RD_hdr_t RDHeader;

				fread (&RDHeader, sizeof (RDHeader), 1, fp);

				//
				// Read the packet again.
				//
				fread (&RDPacket, sizeof (RDPacket), 1, fp);
			}

			uint16_t un16PacketLength = stiWORD_BYTE_SWAP (RDPacket.length) - sizeof (RDPacket);

			//
			// Read the packet
			//
			fread (poPacket->BufferGet (), sizeof (uint8_t), un16PacketLength, fp);

			// Need to update pstRTPParam with the information from
			pstRTPParam->len = un16PacketLength;
			RvInt32 rvRet = rtpUnpack_ex (m_rtpSession->rtpGet(), poPacket->BufferGet (), nMAX_VIDEO_PLAYBACK_PACKET_BUFFER, pstRTPParam);
			if (rvRet != RV_OK)
			{
				stiDEBUG_TOOL (g_stiVideoPlaybackReadDebug,
					stiTrace ("<VPR::PacketRead> Error in rtpUnpack %d\n", rvRet);
				);
			}

			pstRTPParam->payload = 96; //m_unH264PayloadType;

			stiDEBUG_TOOL (4 & g_stiVideoPlaybackReadDebug,  // if 4 is set, print every read packet ... this is very very noisy.
					stiTrace ("<VPR::PacketRead> packet %d, SQ# %d\n", g_nPkts, pstRTPParam->sequenceNumber);
				);

			g_nPkts++;
#endif

//#if APPLICATION == APP_NTOUCH_PC
//			RvRtpParam * pstRTPParam = poPacket->RTPParamGet();
//			CstiRTPProxy::RTPProxyVideoReceive (poPacket->BufferGet(), pstRTPParam->len); //'''
//			//
//			// Finish packet processing (and then set packet to "empty")
//			//
//#endif
			//
			// Indicate that the rtp header is at the beginning of the buffer.
			//
			packet->RtpHeaderSet (packet->BufferGet ());
			
			//stiTrace("VPR:: %d Bytes in Socket\n",rtpGetAvailableBytes(m_poCall->VideoRtpSessionGet ()));
			// Yes! Find out how many bytes we actually read
			nBytesRead = pstRTPParam->len;
			
#ifdef stiTEST_FOR_LOST_PACKETS
			if (-1 == g_nFirstPacketReceived)
			{
				g_nFirstPacketReceived = pstRTPParam->sequenceNumber;
			}
			
			// Keep track of the received packet sequence number (record that we received it)
			g_abPacketsReceived[pstRTPParam->sequenceNumber - g_nFirstPacketReceived] = true;

			// Keep track of the order that we received packets
			g_anPacketsReceived[g_nPacketArrayIndex++] = pstRTPParam->sequenceNumber;

			// This is just printed to get an idea of how fast we are receiving packets AND to
			// know when we have just about received the 65535 packets (so that we can hangup
			// before we over-write the arrays again.
			if (0 == pstRTPParam->sequenceNumber % 500)
			{
				stiTrace ("<VPR::PacketRead> Received seq# %d\n", pstRTPParam->sequenceNumber);
			}
#endif //#ifdef stiTEST_FOR_LOST_PACKETS

			// Did we read any data?
			if (nBytesRead > 0)
			{
				// Do we have a handle to the RTCP Session?
				if (!m_bRtpHalted && nullptr != m_rtpSession->rtcpGet())
				{
					// Yes! Notify the rtcp session that the packet was received
					// (use 90kHz clock frequency).
					RvRtcpRTPPacketRecv (
						m_rtpSession->rtcpGet(),
						pstRTPParam->sSrc,
						(stiOSTickGet() - m_un32InitialRTCPTime) * m_mediaClockRate,
						pstRTPParam->timestamp,
						pstRTPParam->sequenceNumber);
				} // end if

				// We were successful with the read, set the return code
				// to reflect this.
				eReadResult = ePACKET_OK;
			} // end if
			else
			{
				// No! We failed to read any valid data.
				eReadResult = ePACKET_CONTINUE;
			} // end else
		}
		else
		{
// 			stiDEBUG_TOOL (g_stiVideoPlaybackReadDebug,
// 				stiTrace ("ERROR: VPR::PacketRead rtpReadWithRemoteAddress error %d\n", eRvStatus);
// 			);

			// No! We failed to read any valid data.
			eReadResult = ePACKET_CONTINUE;
		} // end else
	} // end if

	return (eReadResult);
} // end CstiVideoPlaybackRead::PacketRead


/*!
 * \brief Finds the codec that payload matches.
 *
 * \param payload - Playoad number of codec we are looking for.
 * \return tuple containing videocodec and packetizations scheme.
 */
std::tuple<EstiVideoCodec, EstiPacketizationScheme> CstiVideoPlaybackRead::CodecLookup(uint8_t payload)
{
	EstiVideoCodec videoCodec = estiVIDEO_NONE;
	EstiPacketizationScheme packetization = estiPktUnknown;
	
	auto i = m_PayloadMap.find (payload);
	
	if (i != m_PayloadMap.end ())
	{
		videoCodec = i->second.eCodec;
		packetization = i->second.ePacketizationScheme;
	}
	
	return std::make_tuple (videoCodec, packetization);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlaybackRead::DataUnPack
//
// Description: Calls the RadVision rtpXXXUnpack function
//
// Abstract:
//	This function determines the type of video and calls the appropriate rtp
//	unpack fuction. The endbit and startbit values are also saved away. Other
//	info is saved in the packet object at this time as well.
//
// Returns:
//	estiOK if successful, estiERROR otherwise.
//
EstiResult CstiVideoPlaybackRead::DataUnPack (
	const std::shared_ptr<CstiVideoPacket> &packet)
{
	stiLOG_ENTRY_NAME (CstiVideoPlaybackRead::DataUnPack);

	EstiResult 			eResult = estiOK;
	RvRtpPayloadH263			stH263Info;	// Information containing H.263 information.
	vpe::VideoFrame *			pstVideoPacket = packet->m_frame;
	RvRtpParam *			pstRTPParam = packet->RTPParamGet ();

#if 0
static uint32_t un32SQExpecting = 0;

if (0 == un32SQExpecting)
{
	un32SQExpecting = pstRTPParam->sequenceNumber;
}
else if (un32SQExpecting != pstRTPParam->sequenceNumber)
{
	stiTrace("VPR:: ******* lost packet SQ# %d, next SQ# %d Empty = %d FULL = %d *******\n",
	un32SQExpecting,
	pstRTPParam->sequenceNumber,
	m_oEmptyQueue.CountGet (),
	m_oFullQueue.CountGet ());

	un32SQExpecting =  pstRTPParam->sequenceNumber;
}

un32SQExpecting ++;
#endif

	//
	// Given the payload type, lookup the video codec
	// that it maps to.
	//
	EstiVideoCodec videoCodec;
	EstiPacketizationScheme packetizationScheme;
	
	std::tie(videoCodec, packetizationScheme) = CodecLookup (pstRTPParam->payload);

	packet->retransmit = false;
	
	if (videoCodec == estiVIDEO_RTX)
	{
		RtxPacketProcess (packet, m_ssrc);
		std::tie(videoCodec, packetizationScheme) = CodecLookup (pstRTPParam->payload);
	}
	
	extendedSequenceNumberDetermine (packet);
	
#ifdef sciRTP_FROM_FILE
	videoCodec = estiVIDEO_H264;
#endif

	// Perform any stream specific processing.
	switch (videoCodec)
	{
		case estiVIDEO_H263:
		{
			// Unpack the data and process the data stream header.
			int nUnpackResult = -1;

			if (estiH263_RFC_2190 == packetizationScheme)
			{
				RvRtpPayloadH263a stH263aInfo;
				nUnpackResult = RvRtpH263aUnpack (packet->RtpHeaderGet (),
					nMAX_VIDEO_PLAYBACK_PACKET_BUFFER,
					pstRTPParam,
					&stH263aInfo);
				stH263Info.f = stH263aInfo.f;
				stH263Info.p = stH263aInfo.p;
				stH263Info.sBit = stH263aInfo.sBit;
				stH263Info.eBit = stH263aInfo.eBit;
				stH263Info.src = stH263aInfo.src;
				stH263Info.i = !(stH263aInfo.i); // Radvision documentation is wrong.  It says this represents intra-frames
												// but it actually represents inter-coded frames.  Need to swap the logic.
				stH263Info.a = stH263aInfo.a;
				stH263Info.s = stH263aInfo.s;
				stH263Info.dbq = stH263aInfo.dbq;
				stH263Info.trb = stH263aInfo.trb;
				stH263Info.tr = stH263aInfo.tr;
				stH263Info.gobN = stH263aInfo.gobN;
				stH263Info.mbaP = stH263aInfo.mbaP;
				stH263Info.quant = stH263aInfo.quant;
				stH263Info.hMv1 = stH263aInfo.hMv1;
				stH263Info.vMv1 = stH263aInfo.vMv1;
				stH263Info.hMv2 = stH263aInfo.hMv2;
				stH263Info.vMv2 = stH263aInfo.vMv2;
			}
			else if (estiH263_RFC_2429 == packetizationScheme)
			{
				nUnpackResult = rtpH2631998Unpack (packet->RtpHeaderGet (),
					nMAX_VIDEO_PLAYBACK_PACKET_BUFFER,
					pstRTPParam,
					&stH263Info);
			}
			else
			{
				nUnpackResult = RvRtpH263Unpack (packet->RtpHeaderGet (),
					nMAX_VIDEO_PLAYBACK_PACKET_BUFFER,
					pstRTPParam,
					&stH263Info);
			}
			if (-1 < nUnpackResult)
			{
				// bits to ignore in current first byte
				packet->unZeroBitsAtStart = stH263Info.sBit;
				// bits to ignore in current last byte
				packet->unZeroBitsAtEnd = stH263Info.eBit;

				stiDEBUG_TOOL (g_stiVideoPlaybackReadDebug,
					stiTrace ("VPR:: H.263 SQ# %d Empty = %d Full = %d\n",
					pstRTPParam->sequenceNumber, m_emptyPacketPool.count (), m_oFullQueue.CountGet ());
				); // stiDEBUG_TOOL
			} // end if
			else
			{
				// rtp Unpack error occured
				stiASSERT (estiFALSE);
				eResult = estiERROR;
				++m_Stats.payloadHeaderErrors;
			} // end if

			break;
		}
		case estiVIDEO_H264:

			// bits to ignore in current first byte
			packet->unZeroBitsAtStart = 0;
			// bits to ignore in current last byte
			packet->unZeroBitsAtEnd = 0;

			stiDEBUG_TOOL (g_stiVideoPlaybackReadDebug,
				stiTrace ("VPR:: H.264 SQ# %d Empty = %d Full = %d\n\n",
				pstRTPParam->sequenceNumber, m_emptyPacketPool.count (), m_oFullQueue.CountGet());
			); // stiDEBUG_TOOL

			break;
		case estiVIDEO_H265:
			
			// bits to ignore in current first byte
			packet->unZeroBitsAtStart = 0;
			// bits to ignore in current last byte
			packet->unZeroBitsAtEnd = 0;
			
			stiDEBUG_TOOL (g_stiVideoPlaybackReadDebug,
						   stiTrace ("VPR:: H.265 SQ# %d Empty = %d Full = %d\n\n",
									 pstRTPParam->sequenceNumber, m_emptyPacketPool.count (), m_oFullQueue.CountGet());
						   ); // stiDEBUG_TOOL
			
			break;
		case estiVIDEO_RTX:
		default:
			eResult = estiERROR;
			if (pstRTPParam->len - pstRTPParam->sByte >= 1) // if there are no bytes, this is just a keepalive packet
			{
				stiTrace ("Payload type=%d\n", pstRTPParam->payload);

				++m_Stats.unknownPayloadTypeErrors;
			}
			else
			{
				// printf ("RTP keepalive (video) received.\n");
				++m_Stats.keepAlivePackets;
			}
			break;
	} // end switch

	// Did we succeed?
	if (estiOK == eResult)
	{
		packet->PayloadSet (packet->RtpHeaderGet () + pstRTPParam->sByte);
		
		// Yes! Calculate the offset into the actual video data and store into
		// the VideoPacket structure for the driver.
		pstVideoPacket->buffer = packet->PayloadGet ();

		// Save the number of bytes of video we received into the structure.
		packet->unPacketSizeInBytes = pstRTPParam->len - pstRTPParam->sByte;

		// Update the maximum buffer size
		packet->m_unBufferMaxSize = nMAX_VIDEO_PLAYBACK_PACKET_BUFFER - pstRTPParam->sByte;
	} // end if

	return (eResult);
} // end CstiVideoPlaybackRead::DataUnPack

//:-----------------------------------------------------------------------------
//:
//:	Public Methods
//:
//:-----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlaybackRead::MediaPacketFullGet
//
// Description: Retrieves a filled video packet from the read task
//
// Abstract:
//
// Returns:
//	CstiVideoPacket * - pointer to filled video packet
//
// TODO: merge this with the base PlaybackRead?  Not sure if it's appropriate for the other media types

//
std::shared_ptr<CstiVideoPacket> CstiVideoPlaybackRead::MediaPacketFullGet (uint16_t un16ExpectedPacket)
{
	stiLOG_ENTRY_NAME (CstiVideoPlaybackRead::MediaPacketFullGet);

	uint32_t un32NextPacket;
	
	// Get the oldest packet in the full queue
	auto packet = m_oFullQueue.SequenceNumberSearchEx (
						un16ExpectedPacket, &un32NextPacket);
#ifdef ercDebugPacketQueue
stiDEBUG_TOOL (g_stiVideoPlaybackPacketDebug,
	auto packetOldest = m_oFullQueue.OldestGet ();

	if (packet != packetOldest)
	{
		stiTrace ("\n\n\n<VPR::VideoPktFullGet> Returning %p with SQ# %d.  Would have returned %p with SQ# %d\n\n\n",
			packet, packet->RTPParamGet ()->sequenceNumber,
			packetOldest, packetOldest->RTPParamGet ()->sequenceNumber);
		stiTrace ("\tRequested SQ# %d\n", un16ExpectedPacket);
	}
);
#endif // #ifdef ercDebugPacketQueue

	if (packet)
	{
		// Now remove that packet from the queue
		m_oFullQueue.Remove (packet);

#if 0
		stiTrace ("Removed %p from full queue.  Empty = %d Full = %d\n", poPacket,
				  m_oEmptyQueue.CountGet (),
				  m_oFullQueue.CountGet());
#endif
	}

	return packet;
} // end CstiVideoPlaybackRead::MediaPacketFullGet

//:-----------------------------------------------------------------------------
//:
//: Data flow functions
//:
//:-----------------------------------------------------------------------------


/*!
 * \brief Post a message that the channel is closing.
 *
 * This method posts a message to the Video Playback task that the channel is
 * closing.
 *
 * \return estiOK when successful, otherwise estiERROR
 */
stiHResult CstiVideoPlaybackRead::DataChannelClose ()
{
	stiLOG_ENTRY_NAME (CstiVideoPlaybackRead::DataChannelClose);

	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	if (!m_dataChannelClosed)
	{
		MediaPlaybackRead::DataChannelClose();

#ifdef sciRTP_FROM_FILE
		if (fp)
		{
			fclose (fp);
			fp = NULL;
		}
#endif
	}

	return hResult;
}


/*!
 * \brief Processes a received packet (runs in the event loop thread)
 * \return stiHResult
 */
stiHResult CstiVideoPlaybackRead::ReceivedPacketsProcess()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	auto packet = m_oRecvdQueue.OldestGetAndRemove ();

	if (packet)
	{
		if (!m_bRtpHalted)
		{
			if (nullptr != m_rtpSession->rtcpGet())
			{
				RvRtpParam *pRtpParam = packet->RTPParamGet ();
				
				// Yes! Notify the rtcp session that the packet was received
				// (use 90kHz clock frequency).
				RvRtcpRTPPacketRecv (
					m_rtpSession->rtcpGet(),
					pRtpParam->sSrc,
					(stiOSTickGet() - m_un32InitialRTCPTime) * m_mediaClockRate,
					pRtpParam->timestamp,
					pRtpParam->sequenceNumber);
			} // end if
				
			ReadPacketProcess (packet);
		}
		else
		{
			m_Stats.packetsDiscardedMuted++;
		}
	}

	return hResult;
}


/*!
 * \brief When *not* using transports or tunneling, this event handler
 *        will be called when data is available to be read from the rtp socket
 */
void CstiVideoPlaybackRead::EventRtpSocketDataAvailable()
{
	PacketProcessFromPort ();
}


/*!
 * \brief When *using* transports or tunneling, this event handler
 *        will be called when data is available to be read from the rtp socket
 *
 * \note  TODO:  This is really strange that we have separate queues depending on
 *        how the data arrived...  We shouldn't care how the data arrive,
 *        just the fact that it did.  (unless both can be active at the
 *        same time, which I don't believe is the case)
 */
void CstiVideoPlaybackRead::EventRtpDataAvailable()
{
	ReceivedPacketsProcess ();

	// If there are more packets to process, schedule them
	if (m_oRecvdQueue.CountGet() > 0)
	{
		PostEvent (
			std::bind(&CstiVideoPlaybackRead::EventRtpDataAvailable, this));
	}
}


///\brief This is the callback method for the RTP event handler.
///
///\param hRTP The handle to the RTP session
///
void CstiVideoPlaybackRead::RTPOnReadEvent(RvRtpSession hRTP)
{
	CstiVideoPlaybackRead *pThis = this;
	
	// NOTE: Avoid accessing class data members in the list (except for the packet queue)
	// to avoid the need for locking the task's mutex.  This method is called by Radvision through
	// another thread.
	
#if 0
	static int nNoEmpty = 0;
#endif

	EstiBool	bDone	= estiFALSE;
	uint32_t 	un32Cnt	= 0;
	while(!bDone)
	{
		//
		// Lock the queue, grab the oldest available, remove it from the queue and then unlock the queue.
		//
		auto packet = pThis->m_emptyPacketPool.packetGet ();

		if (packet)
		{
			if (0 <= pThis->m_rtpSession->RtpRead (
					packet->BufferGet (),
					nMAX_VIDEO_PLAYBACK_PACKET_BUFFER,
					packet->RTPParamGet ()) &&
				packet->RTPParamGet()->len)
			{
#if 0
				if (nNoEmpty > 0)
				{
					stiTrace ("Number of times no packets available: %d\n", nNoEmpty);
					nNoEmpty = 0;
				}
#endif

#ifdef stiTEST_FOR_LOST_PACKETS
				RvRtpParam * pstRTPParam = packet->RTPParamGet();

				if (-1 == g_nFirstPacketReceived)
				{
					g_nFirstPacketReceived = pstRTPParam->sequenceNumber;
				}

				// Keep track of the received packet sequence number (record that we received it)
				g_abPacketsReceived[pstRTPParam->sequenceNumber - g_nFirstPacketReceived] = true;

				// Keep track of the order that we received packets
				g_anPacketsReceived[g_nPacketArrayIndex++] = pstRTPParam->sequenceNumber;

				// This is just printed to get an idea of how fast we are receiving packets AND to
				// know when we have just about received the 65535 packets (so that we can hangup
				// before we over-write the arrays again.
				if (0 == pstRTPParam->sequenceNumber % 500)
				{
					stiTrace ("<VPR::RTPOnReadEventCB> Received seq# %d\n", pstRTPParam->sequenceNumber);
				}
#endif //#ifdef stiTEST_FOR_LOST_PACKETS

				//stiTrace("APR:: %d Bytes read from Socket SQ# %d\n", pstRTPParam->len, pstRTPParam->sequenceNumber);
				//stiTrace("APR:: %d Bytes in Socket\n",rtpGetAvailableBytes(m_poCall->VideoRtpSessionGet ()));

				pThis->m_Stats.packetsRead++;

				//
				// Indicate that the payload is at the beginning of the buffer.
				//
				//#if APPLICATION == APP_NTOUCH_PC
				//			RvRtpParam * pstRTPParam = poPacket->RTPParamGet();
				//			CstiRTPProxy::RTPProxyVideoReceive (poPacket->BufferGet(), pstRTPParam->len); //'''
				//			//
				//			// Finish packet processing (and then set packet to "empty")
				//			//
				//#endif
				packet->RtpHeaderSet (packet->BufferGet ());

				// If the queue was empty prior to adding this packet, then post an event
				// to process all available packets in the queue
				if (1 == pThis->m_oRecvdQueue.AddAndCountGet (packet))
				{
					pThis->PostEvent (
						std::bind(&CstiVideoPlaybackRead::EventRtpDataAvailable, pThis));
				}

				un32Cnt++;

				// RvRtpGetAvailableBytes is not supported for the tunnel socket
				auto availableBytes = boost::make_optional (false, int());
				if (!m_rtpSession->tunneledGet ())
				{
					availableBytes = RvRtpGetAvailableBytes (m_rtpSession->rtpGet ());
				}

				if (un32Cnt > 10 || (availableBytes && *availableBytes <= 0))
				{
					bDone = estiTRUE;
				}
			}
			else
			{
				//
				// An error occurred so just drop the packet.
				//
				bDone = estiTRUE;
			}
		}
		else
		{
#if 0
			if (nNoEmpty == 0)
			{
				stiTrace ("Number of Recvd: %d, Number in Full: %d\n", pThis->m_oRecvdQueue.CountGet(), pThis->m_oFullQueue.CountGet());
			}

			nNoEmpty++;
#endif
			pThis->m_Stats.packetQueueEmptyErrors++;
			bDone = estiTRUE;
		}
	}
}

/*!
 * \brief specializations of initialization for video
 *
 * \return stiRESULT_SUCCESS when successful, otherwise stiRESULT_ERROR
 */
stiHResult CstiVideoPlaybackRead::DataChannelInitialize (
	uint32_t un32InitialRtcpTime,
	const TVideoPayloadMap *pPayloadMap)
{
	stiLOG_ENTRY_NAME (CstiVideoPlaybackRead::DataChannelInitialize);

	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	if (m_dataChannelClosed)
	{
		hResult = MediaPlaybackRead::DataChannelInitialize(un32InitialRtcpTime, pPayloadMap);

#ifdef sciRTP_FROM_FILE
		fp = fopen (g_szRTP_FILE, "rb");

		if (fp)
		{
			//
			// Read the file header.  The header is terminated by a carriage return.
			//
			while (getc (fp) != '\n');

			RD_hdr_t RDHeader;

			fread (&RDHeader, sizeof (RDHeader), 1, fp);
		}
		g_nPkts = 1;

		stiDEBUG_TOOL (g_stiVideoPlaybackReadDebug,
			stiTrace ("<VPR::DataChannelInit> %s %sopened successfully\n", g_szRTP_FILE, NULL == fp ? "NOT " : "");
		);
#endif

#ifdef stiDEBUGGING_TOOLS
		m_nDiscardedVideoPackets = 0;
#endif
	}

	Unlock ();

	return hResult;
}
