/*!
 * \file CstiTextRecord.cpp
 * \brief The Text Record task has the responsibility of managing the flow of
 *        text data from the text driver on through to the SIP/RTP stack.
 *
 * It provides functions for an application to interface with the
 * conferencing engine and other services
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 – 2017 Sorenson Communications, Inc. -- All rights reserved
 */

//
// Includes
//
#include "rtp.h"
#include "rtcp.h"
#include "payload.h"
#include "rvrtpnatfw.h"

#include "CstiTextRecord.h"
#include "stiTaskInfo.h"
#include "RtpPayloadAddon.h"
#include "utf8EncodeDecode.h"

//
// Constants
//
#define SEND_TIMER_INTERVAL 275 // milliseconds  (less than 300 which is the maximum allowed without known reason for it to be longer)

//
// Typedefs
//

//
// Globals
//

//
// Locals
//

//
// Forward Declarations
//


///////////////////////////////////////////////////////////////////////////////
//; CstiTextRecord::CstiTextRecord
//
//  Description: Class constructor
//
//  Abstract:
//		This is the constructor for the Text Record class
//
//  Returns:
//		None.
//

// TODO: In audio, this junk is calculated from the platform layer
#undef nMAX_TEXT_RECORD_BYTES
#define nMAX_TEXT_RECORD_BYTES 255 
#undef nMAX_RECORD_TEXT_FRAME_MS
#define nMAX_RECORD_TEXT_FRAME_MS 10

CstiTextRecord::CstiTextRecord (
	std::shared_ptr<vpe::RtpSession> session,
	uint32_t maxRate,
	ECodec codec,
	RvSipMidMgrHandle sipMidMgr)
:
	vpe::MediaRecord<EstiTextCodec, SstiTextRecordSettings>(
		stiTR_TASK_NAME,
		session,
		0,
		maxRate,
		codec,
		estiTEXT_NONE,
		0,
		0),
	// TODO: better handling for this, but currently unused
	//m_nTimestampIncrement (nMAX_TEXT_RECORD_BYTES),
    m_sendTimer (SEND_TIMER_INTERVAL, this)
{
	stiLOG_ENTRY_NAME (CstiTextRecord::CstiTextRecord);
	
	m_TimeLastSent.tv_sec = 0;
	m_TimeLastSent.tv_usec = 0;

	if (sipMidMgr)
	{
		// Notify Radvision that we'll be using the eventloop thread
		m_signalConnections.push_back (startedSignal.Connect (
				[this, sipMidMgr]{ RvSipMidAttachThread (sipMidMgr, m_taskName.c_str()); }));

		// Notify Radvision that we're done using the eventloop thread
		m_signalConnections.push_back (shutdownSignal.Connect (
				[sipMidMgr]{ RvSipMidDetachThread (sipMidMgr); }));
	}

	m_signalConnections.push_back (m_sendTimer.timeoutSignal.Connect (
			std::bind(&CstiTextRecord::EventSendTimerTimeout, this)));
}


///////////////////////////////////////////////////////////////////////////////
//; CstiTextRecord::~CstiTextRecord
//
//  Description: Class destructor
//
//  Abstract:
//		This is the destructor for the Text Record class
//
//  Returns:
//
CstiTextRecord::~CstiTextRecord ()
{
	stiLOG_ENTRY_NAME (CstiTextRecord::~CstiTextRecord);

	Shutdown ();

	// Clean up the T.140 RED list now.
	SRedundant *pstRedBuf;
	while (!m_lRedBuffer.empty())
	{
		pstRedBuf = m_lRedBuffer.front ();
		
		if (pstRedBuf->pun8Buffer)
		{
			delete [] pstRedBuf->pun8Buffer;
			pstRedBuf->pun8Buffer = nullptr;
		}

		delete pstRedBuf;
		pstRedBuf = nullptr;

		m_lRedBuffer.pop_front ();
	}

	//
	// Delete any remaining text send buffers.
	//
	SSendBuffer *pstSendBuffer = nullptr;
	while (!m_lTextToSend.empty())
	{
		pstSendBuffer = m_lTextToSend.front ();

		if (pstSendBuffer->pun8Buffer)
		{
			delete [] pstSendBuffer->pun8Buffer;
			pstSendBuffer->pun8Buffer = nullptr;
		}

		delete pstSendBuffer;
		pstSendBuffer = nullptr;

		m_lTextToSend.pop_front ();
	}
} // end destructor



//:-----------------------------------------------------------------------------
//:
//:	Data flow functions
//:
//:-----------------------------------------------------------------------------

/*!
 * \brief Initializes the data channel
 */
stiHResult CstiTextRecord::DataChannelInitialize ()
{
	stiHResult hResult = MediaRecordType::DataChannelInitialize();

	//
	// Initialize text record driver
	//
	SetDeviceTextCodec (m_Settings.codec);

	EstiBool bPrivacy = estiFALSE;

	//m_pTextInput->PrivacyGet (&bPrivacy);

	if (bPrivacy)
	{
		Mute (estiMUTE_REASON_PRIVACY);
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiTextRecord::TextSend
//
// Description: Receives a Unicode character string to be sent
//
// Abstract:
//	This function posts an event to the Text Record task to send another string.
//
// Returns:
//	stiRESULT_SUCCESS if successful, an error if not.
//
stiHResult CstiTextRecord::TextSend (const uint16_t *pun16Text)
{
	stiLOG_ENTRY_NAME (CstiTextRecord::TextSend);

	stiHResult hResult = stiRESULT_SUCCESS;
	int nLength;

	// Did they pass in a valid pointer?
	stiTESTCOND (pun16Text && pun16Text[0], stiRESULT_ERROR);
	
	// Determine the length of the Unicode string passed in.
	for (nLength = 0; pun16Text[nLength]; nLength++)
	{
	}

	if (0 < nLength)
	{
		auto pun16TextCopy = new uint16_t[nLength + 1];

		// Copy the data (including the terminator (0x0000)).
		memcpy (pun16TextCopy, pun16Text, sizeof (uint16_t) * (nLength + 1));

		PostEvent (
			std::bind(&CstiTextRecord::EventTextSend, this, pun16TextCopy, nLength));
	}

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiTextRecord::EventTextSend
//
// Description: Put the text to be send into a list for sending
//
// Abstract:
//	This function encodes the Unicode text into a UTF-8 byte array, puts it into
//	a std::list structure and then makes sure that our timer is running that will
//	pick it up and send it.
//
// Returns:
//	stiRESULT_SUCCESS if successful, an error if not.
//
void CstiTextRecord::EventTextSend (const uint16_t *pun16Text, int nLength)
{
	stiLOG_ENTRY_NAME (CstiTextRecord::EventTextSend);

	stiHResult hResult = stiRESULT_SUCCESS;

	SSendBuffer *pstSendBuffer = nullptr;
	
	if (pun16Text)
	{
		// Encode the data and store it in a list of data to be sent.
		pstSendBuffer = new SSendBuffer;
		stiTESTCOND (pstSendBuffer, stiRESULT_ERROR);

		pstSendBuffer->pun8Buffer = new uint8_t[nLength * 3];		// At most, it will take 3 bytes to encode each
		stiTESTCOND (pstSendBuffer->pun8Buffer, stiRESULT_ERROR);	// character.  Make sure the buffer is large enough.

		// Convert the Unicode array into a UTF-8 encoded string.
		pstSendBuffer->nLength = UnicodeToUTF8Convert (pstSendBuffer->pun8Buffer, pun16Text);
		stiTESTCOND (0 < pstSendBuffer->nLength, stiRESULT_ERROR);

		// Add this buffer to our list of buffers.
		m_lTextToSend.push_back (pstSendBuffer);
		pstSendBuffer = nullptr;

		// Set the timer to send the queued text
		m_sendTimer.restart ();
	}

STI_BAIL:

	if (stiIS_ERROR (hResult) && pstSendBuffer)
	{
		// There was an error.  Need to free any memory that we have allocated to this point.
		if (pstSendBuffer->pun8Buffer)
		{
			delete [] pstSendBuffer->pun8Buffer;
			pstSendBuffer->pun8Buffer = nullptr;
		}

		delete pstSendBuffer;
		pstSendBuffer = nullptr;
	}

	// Need to free the memory allocated by TextSend
	delete [] pun16Text;
	pun16Text = nullptr;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiTextRecord::EventSendTimerTimeout
//
// Description: Called as a result of a timer firing.  Sends the data.
//
// Abstract:
//	This function loops through the std::list of data to be sent; combining it in
//	a new buffer.  The new buffer is then added to a redundant std::list and the
//	method PacketSend is called to send the data in the redundant list.  If T.140
//	is being used, there will be at most one element in the redundant list at any
//	time.  If T.140 RED is being used, there will be at most three elements in the
//	list.
//
//	Once sent, this method also cleans up the lists.
//
// Returns:
//	stiRESULT_SUCCESS if successful, an error if not.
//
void CstiTextRecord::EventSendTimerTimeout ()
{
	// Allocate an SRedundant structure for the new message
	stiHResult hResult = stiRESULT_SUCCESS;
	auto pstRedBuf = new SRedundant;
	pstRedBuf->n32Length = 0;
	pstRedBuf->un32TimeStamp = 0;
	pstRedBuf->pun8Buffer = nullptr;
	SSendBuffer *pstCurrent = nullptr;
	uint8_t *pun8Current = nullptr;

	std::list<SSendBuffer*>::iterator i;

	// how big does our buffer need to be?  It needs to contain enough space to accommodate the buffers in the m_lTextToSend list.
	for (i = m_lTextToSend.begin (); i != m_lTextToSend.end (); i++)
	{
		pstRedBuf->n32Length += (*i)->nLength;
	}

	// If there wasn't any data in the list, we still may need to send if we have redundant data to send.
	if (pstRedBuf->n32Length == 0)
	{
		// No new buffers to send.  Do we have buffers in the Redundant list that need to be sent again?
		if (m_lRedBuffer.empty())
		{
			// There isn't any redundant buffers to be sent.  Delete the newly allocated structure since we won't be sending it.
			delete pstRedBuf;
			pstRedBuf = nullptr;
		}
	}
	else
	{
		// There are new buffers to send.  Get them.
		pstRedBuf->pun8Buffer = new uint8_t[pstRedBuf->n32Length];
		stiTESTCOND (pstRedBuf->pun8Buffer, stiRESULT_ERROR);

		// Now that we have the buffer that we need, copy each of the UTF8 encodings that we have in the m_lTextToSend list into it.
		pun8Current = pstRedBuf->pun8Buffer;

		while (!m_lTextToSend.empty ())
		{
			pstCurrent = m_lTextToSend.front ();

			memcpy (pun8Current, pstCurrent->pun8Buffer, pstCurrent->nLength);

			// Move the current pointer where we will copy to next.
			pun8Current += pstCurrent->nLength;

			// Remove from the list.
			delete [] pstCurrent->pun8Buffer;
			delete pstCurrent;
			m_lTextToSend.pop_front ();
		}
	}

	if (pstRedBuf)
	{
		// Push this structure onto the Redundant list
		m_lRedBuffer.push_back (pstRedBuf);
		pstRedBuf = nullptr;
	}

	hResult = PacketSend ();

	// Did the packet get successfully sent?
	if (!stiIS_ERROR (hResult))
	{
		// We were successful in sending...
		// Delete buffers and remove from the Redundant list those that have been sent as many times as they were to be sent.

		// Are we using T.140 RED?
		if (estiTEXT_T140_RED == m_currentCodec)
		{
			// If we have three structures in the list, simply delete the front one
			if (m_lRedBuffer.size () == 3)
			{
				pstRedBuf = m_lRedBuffer.front ();

				if (pstRedBuf->pun8Buffer)
				{
					delete [] pstRedBuf->pun8Buffer;
					pstRedBuf->pun8Buffer = nullptr;
				}

				delete pstRedBuf;
				pstRedBuf = nullptr;

				m_lRedBuffer.pop_front ();
			}

			// If we have only two left and they are both empty, clear up the entire list now.
			if (m_lRedBuffer.size () == 2)
			{
				pstRedBuf = m_lRedBuffer.front ();
				if (pstRedBuf->n32Length == 0)
				{
					pstRedBuf = m_lRedBuffer.back ();
					if (pstRedBuf->n32Length == 0)
					{
						// Both are empty.  Deleting the back one.
						delete pstRedBuf;
						pstRedBuf = nullptr;
						m_lRedBuffer.pop_back ();

						// Deleting the front one.
						pstRedBuf = m_lRedBuffer.front ();
						delete pstRedBuf;
						pstRedBuf = nullptr;
						m_lRedBuffer.pop_front ();
					}
				}
			}
		}
		else
		{
			// For non-redundant, it is expected that we will never have more than one in the list at a time.
			// Clean up the list now.
			while (!m_lRedBuffer.empty())
			{
				pstRedBuf = m_lRedBuffer.front ();
				if (pstRedBuf->pun8Buffer)
				{
					delete [] pstRedBuf->pun8Buffer;
					pstRedBuf->pun8Buffer = nullptr;
				}

				delete pstRedBuf;
				pstRedBuf = nullptr;

				m_lRedBuffer.pop_front ();
			}
		}
	}
	else
	{
		// It was an error, remove it from the redundant list
		pstRedBuf = m_lRedBuffer.back ();

		if (pstRedBuf->pun8Buffer)
		{
			delete [] pstRedBuf->pun8Buffer;
			pstRedBuf->pun8Buffer = nullptr;
		}

		delete pstRedBuf;
		pstRedBuf = nullptr;

		m_lRedBuffer.pop_back ();

		stiTESTRESULT ();
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		if (pstRedBuf)
		{
			// An error has occurred, delete the allocated memory.
			if (pstRedBuf->pun8Buffer)
			{
				delete [] pstRedBuf->pun8Buffer;
				pstRedBuf->pun8Buffer = nullptr;
			}

			delete pstRedBuf;
			pstRedBuf = nullptr;
		}
	}

	// start the timer again if we have redundant data or more buffers to send
	// If we aren't using T.140 RED, we should never have more than 1 structure in the list.
	if (!m_lRedBuffer.empty())
	{
		m_sendTimer.restart ();
	}
}



/*!
 * \brief Start recording data
 */
void CstiTextRecord::EventDataStart ()
{
	stiLOG_ENTRY_NAME (CstiTextRecord::EventDataStart);
	stiDEBUG_TOOL ( g_stiTRDebug,
		stiTrace ("<CstiTextRecord::EventDataStart>\n");
	);

	// Is the text mute off? It must be for us to turn on the text.
	if (0 == m_nMuted)
	{
		// Indicate that we have started sending data
		m_bSendingMedia = estiTRUE;

		// Initially, there are old data in buffers which need to be flushed out
		m_nFlushPackets = 0; //nUNMUTE_FLUSH_TR_BUFFERS;

#if 0 // Currently no sending initialization string as part of defect 20705.
		// Send a ByteOrderMark to signify the start of Text
		if (!m_bSentByteOrderMark)
		{
			uint16_t aun16Buffer[2];
			aun16Buffer[0] = un16UNICODE_BYTE_ORDER_MARK;
			aun16Buffer[1] = 0;

			TextSend (aun16Buffer);
			m_bSentByteOrderMark = estiTRUE;
		}
#endif
	} // end if
}


////////////////////////////////////////////////////////////////////////////////
//; CstiTextRecord::PacketSend
//
// Description: Send a single packet's worth of data
//
// Abstract:
//
// Returns:
//	stiRESULT_SUCCESS if successful, an error if not.
//
stiHResult CstiTextRecord::PacketSend ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint8_t *pSendBuffer = nullptr;
	uint32_t un32NumBytesToSend = 0;
	uint32_t un32PayloadByte = 0;
	int nRTPRet = 0;
	RvRtpParam stRTPParam;

	stiTESTCOND (!m_lRedBuffer.empty(), stiRESULT_ERROR);

	if (m_bSendingMedia && 0 == m_nFlushPackets && m_nMuted == 0)
	{
		// initialize the rtp param structure
		RvRtpParamConstruct (&stRTPParam);

		// Offset of the first payload byte in the rtp packet buffer
		stRTPParam.sByte = m_RTPHeaderOffset.RtpHeaderOffsetLengthGet ();

		uint32_t un32RTPTimeStamp = (stiOSTickGet () - m_un32RTPInitTime) * GetTextClockRate ();

		// Set timestamp parameter
		stRTPParam.timestamp = un32RTPTimeStamp;

		// Set marker flag. - This parameter is used to indicate the first
		// frame following idle.
		struct timeval timeCurrent{};
		struct timeval timeDelta{};
		gettimeofday (&timeCurrent, nullptr);
		timersub (&timeCurrent, &m_TimeLastSent, &timeDelta);

		if (timeDelta.tv_sec > 0 || timeDelta.tv_usec > 300000)
		{
			stRTPParam.marker = 1;
		}
		else
		{
			stRTPParam.marker = 0;
		}
		m_TimeLastSent = timeCurrent;

#if 0
		//
		// TYY: Following rtp parameters are not applicable for RvRtpWrite()
		//

		// Sync Source flag - This param is obtained in RvRtpWrite() from the rtpSession.
		stRTPParam.sSrc = 0;

		// Sequence number - the stack generates the sequence number
		stRTPParam.sequenceNumber = 0;

		// The number of payload bytes in the rtp packet -
		// The parameter of len of RvRtpWrite() and not this parameter,
		// determines the length of the RTP packet
		stRTPParam.len = 0;

		// Payload type - This parameter is set by the rtpXXXPack function.
		stRTPParam.payload = 0;
#endif

		// Calculate the total bytes to send for a RTP packet
		un32NumBytesToSend = 0;
		uint32_t un32BytesInHeader = m_RTPHeaderOffset.RtpHeaderOffsetLengthGet ();;

		// Add the bytes for each of the entries in the Redundant list
		std::list<SRedundant*>::iterator j;
		for (j = m_lRedBuffer.begin (); j != m_lRedBuffer.end (); j++)
		{
			un32NumBytesToSend += (*j)->n32Length;

			// If the timestamp hasn't yet been set, do so now.
			if ((*j)->un32TimeStamp <= 0)
			{
				(*j)->un32TimeStamp = un32RTPTimeStamp;
			}
		}

		// Do we have anything to send?
		if(un32NumBytesToSend > 0)
		{
			// If we are sending using T.140 RED, we need to add additional bytes for the redundant headers
			if (estiTEXT_T140_RED == m_currentCodec)
			{
				//todo: Isn't this just un32BytesInheader += m_lRedBuffer.size () * 4 - 3;
				switch (m_lRedBuffer.size ())
				{
				case 3:
					un32BytesInHeader += 9;
					break;

				case 2:
					un32BytesInHeader += 5;
					break;

				case 1:
					un32BytesInHeader += 1;
					break;

				default:
					// Shouldn't ever have more than 2 redundant buffers!
					stiASSERT (false);
					break;
				}
			}

			un32NumBytesToSend += un32BytesInHeader;

			// Allocate a buffer large enough for the new data, the redundant data, and the headers.
			// Then copy the new data into it (just after the space for the headers).
			pSendBuffer = new uint8_t[un32NumBytesToSend];
			memset (pSendBuffer, 0, un32NumBytesToSend);

			// Load the send buffer with the data (after the space allotted for the headers)
			uint8_t *pun8LoadLocation = pSendBuffer + un32BytesInHeader;
			for (j = m_lRedBuffer.begin (); j != m_lRedBuffer.end (); j++)
			{
				memcpy (pun8LoadLocation, (*j)->pun8Buffer, (*j)->n32Length);

				pun8LoadLocation += (*j)->n32Length;
			}

			switch (m_currentCodec)
			{
			case estiTEXT_T140:
			{
				// Call the rtp function to place the RTP header at the
				// start of the data.
				nRTPRet = rtpT140Pack (
					pSendBuffer,
					un32NumBytesToSend,
					&stRTPParam,
					nullptr);
				stiTESTCOND(nRTPRet >= 0, stiRESULT_ERROR);

				break;
			}

			case estiTEXT_T140_RED:
			{
				// Call the rtp function to place the RTP header at the
				// start of the data.
				nRTPRet = rtpT140Pack (
					pSendBuffer,
					un32NumBytesToSend,
					&stRTPParam,
					nullptr);
				stiTESTCOND (nRTPRet >= 0, stiRESULT_ERROR);

				// Need to add the additional headers for the redundant data
				// Each header will look like this:
				// 1-bit - boolean signifying if another is to follow.
				// 7-bits - the T.140 payload type
				// 14-bits - the timestamp offset
				// 10-bits - the redundant block length

				// The header for the new data will look like this:
				// 1-bit - boolean signifying if another is to follow (will always be 0)
				// 7-bits - the T.140 payload type

				std::list<SRedundant*>::iterator it;
				int nBlocks = m_lRedBuffer.size ();
				int nIter = nBlocks;
				uint8_t *pun8BufferOffset = pSendBuffer
						+ m_RTPHeaderOffset.RtpHeaderOffsetLengthGet ();	// Move just past the rtp header (into the
				// payload aread reserved for T.140 headers);
				RvBool bMoreBlocks = RV_FALSE;
				RvUint32 un32TSOffset = 0;

				for (it = m_lRedBuffer.begin (); it != m_lRedBuffer.end (); it++, nIter--, pun8BufferOffset += 4)
				{
					if (nIter > 1)
					{
						bMoreBlocks = RV_TRUE;
					}
					else
					{
						bMoreBlocks = RV_FALSE;
					}

					un32TSOffset =  un32RTPTimeStamp - (*it)->un32TimeStamp;

					rtpT140RedPack (
						pun8BufferOffset,
						bMoreBlocks,
						m_Settings.redPayloadTypeNbr,
						un32TSOffset,
						(*it)->n32Length,
						nullptr,
						nullptr
						);
				}

				break;
			}

			default:
				stiTHROW (stiRESULT_ERROR);
				break;
			} // end of switch


			// Keep track of how many (payload) bytes will be sent
			ByteCountAdd (un32PayloadByte);

			// If we have an RTP session, send the packet
			if (nullptr != m_session->rtpGet())
			{
				stRTPParam.payload = m_Settings.payloadTypeNbr;

				// Send the data to the stack
				nRTPRet = RvRtpWrite (m_session->rtpGet(),
					pSendBuffer,
					un32NumBytesToSend,
					&stRTPParam);
				stiTESTCOND (nRTPRet >= 0, stiRESULT_ERROR);

				stiDEBUG_TOOL (g_stiTRDebug,
					static int nCnt = 0;
					if(++nCnt == 1)
					{
						stiTrace ("<CstiTextRecord::PacketSend> Write text SQ# %d timestamp = %d size = %d successful!!\n\n",
							stRTPParam.sequenceNumber, stRTPParam.timestamp, un32PayloadByte);
						nCnt = 0;
					}
				); // stiDEBUG_TOOL
			} // end if
			else
			{
				stiDEBUG_TOOL (g_stiTRDebug,
					stiTrace ("<CstiTextRecord::PacketSend> No hRTPSession handle\n");
				); // stiDEBUG_TOOL
			} // end else
		} // end if if(un32NumBytesToSend > 0)
	} // end if (m_bSendingData)

	// Updating the number of packets need to be flushed
	if(m_nFlushPackets > 0)
	{
		m_nFlushPackets --;
	}
	else
	{
		// Increment the BaseSendTime based on the number of samples/frame
		// 		m_un32RTPTimestamp += m_nTimestampIncrement;
	}

STI_BAIL:

	if (pSendBuffer)
	{
		delete [] pSendBuffer;
		pSendBuffer = nullptr;
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiTextRecord::ReadPacket
//
// Description: Read a packet from the text driver, and prepare the RTP
//
// Abstract:
//
// Returns:
//	ePACKET_OK - read successful
//	ePACKET_ERROR - read failed
//	ePACKET_CONTINUE - no data available to read
//
EstiPacketResult CstiTextRecord::ReadPacket(CstiTextPacket *poPacket)

{
	stiLOG_ENTRY_NAME (CstiTextRecord::PacketRead);

	EstiPacketResult ePacketResult = ePACKET_OK;
	/*	SsmdTextFrame * pstTextFrame = poPacket->TextFrameGet ();

	// Offset into the buffer to the start of the actual data
	pstTextFrame->pun8Buffer = poPacket->BufferGet () + m_RTPHeaderOffset.RtpHeaderOffsetLengthGet ();

	// NOTE: The buffer is allocated to the size of nMAX_TEXT_RECORD_BUFFER_BYTES,
	// but the other space are left for rtp header
	pstTextFrame->unBufferMaxSize = nMAX_TEXT_RECORD_BYTES;


	pstTextFrame->unFrameSizeInBytes = 0;

	// Retrieve an text packet from the text driver.
	stiHResult hResult = m_pTextInput->TextRecordPacketGet(pstTextFrame);

	// Did we get a frame?
	if (!stiIS_ERROR (hResult))
	{
		if(pstTextFrame->unFrameSizeInBytes <= 0)
		{
			// No! We failed to read any valid data.
			ePacketResult = ePACKET_CONTINUE;
		}
	}
	else
	{
		// No! We failed to read any valid data.
		ePacketResult = ePACKET_CONTINUE;
	}*/

	return (ePacketResult);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiTextRecord::SetDeviceTextCodec
//
// Description: Changes the text mode in the driver
//
// Abstract:
//
//	This function set the text codec type, and maximum packet size

//  into text driver
//
// Returns:
//
stiHResult CstiTextRecord::SetDeviceTextCodec (
	IN EstiTextCodec eCodec)
{
	stiLOG_ENTRY_NAME (CstiTextRecord::InitializeTextDriver);
	stiDEBUG_TOOL ( g_stiTRDebug,
		stiTrace("<CstiTextRecord::SetDeviceTextCodec> eCodec = %d\n", eCodec);
	);

	stiHResult hResult = stiRESULT_SUCCESS;
	EstiTextCodec esmdTextCodec = estiTEXT_NONE;
// 	unsigned short usSamplesPerPacket = 0;

	switch (eCodec)
	{
		case estiTEXT_T140:
			esmdTextCodec = estiTEXT_T140;
			m_RTPHeaderOffset.RtpHeaderLengthSet (rtpT140GetHeaderLength ());
			break;
			
		case estiTEXT_T140_RED:
			esmdTextCodec = estiTEXT_T140_RED;
			m_RTPHeaderOffset.RtpHeaderLengthSet (rtpT140RedGetHeaderLength ());
			break;

		default:
			esmdTextCodec = estiTEXT_T140;
			m_RTPHeaderOffset.RtpHeaderLengthSet (rtpT140GetHeaderLength ());

			stiASSERT (estiFALSE);
			break;
	}

	// Decide the maximum packet size according to codec type
	switch (eCodec)
	{
		case estiTEXT_T140:
		case estiTEXT_T140_RED:
		{
// 			if (m_Settings.nMaxTextFrames <= 0)
// 			{
// 				m_Settings.nMaxTextFrames = nMAX_TEXT_RECORD_BYTES;
// 			}
// 			else if (m_Settings.nMaxTextFrames > nMAX_TEXT_RECORD_BYTES)
// 			{
// 				m_Settings.nMaxTextFrames = nMAX_TEXT_RECORD_BYTES;
// 			}
			
// 			unsigned short nFrameSize = T140_MS_TO_SIZE (m_Settings.nMaxTextFrames);
// 			m_nTimestampIncrement = nFrameSize;  // todo - erc - What are we to base this on?
// 			usSamplesPerPacket = nFrameSize;
			
			// Set up a buffer to hold partial packets
//			m_nRemainderSize = 0;
//			if (m_pRemainderBuffer)
//			{
//				delete[] m_pRemainderBuffer;
//				m_pRemainderBuffer = NULL;
//			}
// 			m_pRemainderBuffer = new uint8_t[m_Settings.nMaxTextFrames];
			
			break;
		}

		default:
			stiASSERT (estiFALSE);
			break;
	} // end switch

	if (estiTEXT_NONE != esmdTextCodec)
	{
		// Save the current text codec type
		m_currentCodec = esmdTextCodec;
	}

	return (hResult);
} // end CstiTextRecord::SetDeviceTextCodec


bool CstiTextRecord::PrivacyGet() const
{
	// privacy not implemented...
	return false;
}

bool CstiTextRecord::payloadIdChanged(const SstiMediaPayload &offer, int8_t redPayloadTypeNbr) const
{
	return
		MediaRecordType::payloadIdChanged(offer, redPayloadTypeNbr) ||
		redPayloadTypeNbr != m_Settings.redPayloadTypeNbr
		;
}


/*!
* \brief Load some initial settings
*/
stiHResult CstiTextRecord::DataInfoStructLoad (
	const SstiSdpStream & /*sdpStream*/,
	const SstiMediaPayload &offer,
	int8_t redPayloadTypeNbr)
{
	auto hResult = stiRESULT_SUCCESS;
	auto negotiatedCodec = offer.eCodec;

	NegotiatedCodecSet (negotiatedCodec);

	m_Settings = {};  // reset settings
	m_Settings.payloadTypeNbr = offer.n8PayloadTypeNbr;
	m_Settings.redPayloadTypeNbr = redPayloadTypeNbr;

	PayloadTypeSet (offer.n8PayloadTypeNbr);

	switch (negotiatedCodec)
	{
	case estiT140_TEXT:
		m_Settings.codec = estiTEXT_T140;
		MaxChannelRateSet (unT140_RATE);
		break;

	case estiT140_RED_TEXT:
		m_Settings.codec = estiTEXT_T140_RED;
		MaxChannelRateSet (unT140_RATE);
		break;

	default:
		stiTHROW (stiRESULT_ERROR);
		break;
	}

STI_BAIL:
	return hResult;
}
