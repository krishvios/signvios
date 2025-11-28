/*!
* \file CstiTextPlayback.cpp
* \brief The purpose of Text Playback is to manage the text processes from
*        the collecting the incoming data from the socket to sending it to
*        the text driver for playback
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/

#include "rtp.h"
#include "rtcp.h"
#include "payload.h"

#include "CstiTextPlayback.h"
#include "stiTaskInfo.h"
#include "stiPayloadTypes.h"
#include "rvrtpnatfw.h"
#include "utf8EncodeDecode.h"

//
// Constants
//
const int nCHECK_FOR_DELAYED_BLOCKS_INTERVAL = 250;  // Milliseconds.
const int nMAX_REDUNDANT_BLOCKS = 6;


/*!
 * \brief Constructor
 * \param hSipMidMgr
 */
CstiTextPlayback::CstiTextPlayback (
	std::shared_ptr<vpe::RtpSession> session,
	uint32_t maxRate,
	bool rtpKeepAliveEnable,
	RvSipMidMgrHandle sipMidMgr,
	int remoteSipVersion)
:
	MediaPlayback<CstiTextPlaybackRead, CstiTextPacket, EstiTextCodec, TTextPayloadMap>(
		stiTP_TASK_NAME,
        session,
        0,
        maxRate,
		rtpKeepAliveEnable,
		sipMidMgr,
		estiTEXT_NONE,
        estiT140_TEXT,
		remoteSipVersion
		),
    m_checkForDelayedBlocksTimer (nCHECK_FOR_DELAYED_BLOCKS_INTERVAL, this)
{

	// Set the extended sequence number to zero
	// TODO: why is this initial value different than video? (and why does audio store a short sequence number?)
	m_un32ExpectedPacket = 0;

	m_signalConnections.push_back (m_checkForDelayedBlocksTimer.timeoutSignal.Connect (
			std::bind(&CstiTextPlayback::EventCheckForDelayedBlocksTimerTimeout, this)));

	// TODO: consolidate the dataAvailableSignal between media playback implementations (shouldn't be too bad, but they have diverged just a little)
	m_signalConnections.push_back (m_playbackRead.dataAvailableSignal.Connect (
		[this]{
			PostEvent (
				std::bind(&CstiTextPlayback::EventDataAvailable, this));
		}));
}


int CstiTextPlayback::ClockRateGet() const
{
	return GetTextClockRate();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiTextPlayback::PacketQueueProcess
//
// Desc: Processes out-of-order packets previously received
//
// Abstract:
//	Looks for packets which were received before their time. If found, they are
//	processed.
//
// Parameters:
//	pbFoundEntry - Set to true upon return if an entry was found and played.  This is so that this method
//		can keep being called while it is finding and playing found/expired blocks of data.
//
// Returns:
//	stiRESULT_SUCCESS if successful, an error if not.
//
stiHResult CstiTextPlayback::PacketQueueProcess (bool *pbFoundEntry)
{

	stiLOG_ENTRY_NAME (CstiTextPlayback::PacketQueueProcess);

	stiHResult hResult = stiRESULT_SUCCESS;
	*pbFoundEntry = false;
	std::map<RvUint32, SstiT140Block*>::iterator it;
	RvUint32 un32Seq;

	stiDEBUG_TOOL ((2 & g_stiTPDebug),
		stiTrace ("<TP::PacketQueueProcess> Map contains %d entries\n", m_oooT140Blocks.size ());
	);

	// Remove any blocks that have a smaller sequence number than the expected one
	it = m_oooT140Blocks.begin ();
	while (it != m_oooT140Blocks.end () && it->second->un32Seq < m_un32ExpectedPacket)
	{
		// Free memory used by this map entry
		un32Seq = it->second->un32Seq;
		
		stiDEBUG_TOOL ((2 & g_stiTPDebug),
			stiTrace ("<TP::PacketQueueProcess> Deleting block with sequence # %d.  Expecting %d\n", un32Seq, m_un32ExpectedPacket);
		);
		
		delete [] it->second->pun8Buf;
		delete it->second;


		// Remove from the std::map
		m_oooT140Blocks.erase (un32Seq);

		// Advance to the next element
		it = m_oooT140Blocks.begin ();
	}
	
	// Is the packet we are looking for in the OOO map?
	while (m_oooT140Blocks.end () != (it = m_oooT140Blocks.find (m_un32ExpectedPacket)))
	{
		// Found the sequence we are looking for
		// Play it back
		stiDEBUG_TOOL ((2 & g_stiTPDebug),
			stiTrace ("<TP::PacketQueueProcess> Found sequence %d in the OOO queue.  Playing it\n", m_un32ExpectedPacket);
		);
		
		Playback (it->second, false);

		// Free memory used by this map entry
		un32Seq = it->second->un32Seq;
		delete [] it->second->pun8Buf;
		delete it->second;

		// Remove from the std::map
		m_oooT140Blocks.erase (un32Seq);

		*pbFoundEntry = true;
	}

	// Now look to see if we have held onto an ooo block long enough that we are ready to move on from what we expect.
	time_t now;
	time (&now);
	it = m_oooT140Blocks.begin ();
	double seconds = (it != m_oooT140Blocks.end ()) ? difftime (now, it->second->timeReceived) : 0;
	if (seconds >= 1)
	{
		// Found a block that has waited long enough
		// Play it back
		stiDEBUG_TOOL ((2 & g_stiTPDebug),
			stiTrace ("<TP::PacketQueueProcess> Sequence %d has waited %.1f seconds in the OOO queue.  Playing it\n",
						 it->second->un32Seq, seconds);
		);
		
		Playback (it->second, true);

		// Free memory used by this map entry
		un32Seq = it->second->un32Seq;
		delete [] it->second->pun8Buf;
		delete it->second;

		// Remove from the std::map
		m_oooT140Blocks.erase (un32Seq);

		*pbFoundEntry = true;
	}

	// If we still have blocks in the map, need to make sure that the system keeps waking up to see that they are handled.
	if (!m_oooT140Blocks.empty ())
	{
		m_checkForDelayedBlocksTimer.restart ();
	}


	return (hResult);
} // end CstiTextPlayback::PacketQueueProcess



////////////////////////////////////////////////////////////////////////////////
//; CstiTextPlayback::ReadPacketProcess
//
// Description: Processes the packet we just obtained from text playback read task
//
// Abstract:
//	Handles packet ordering problems, and passes packets to be decoded.
//
// Returns:
//	stiRESULT_SUCCESS if successful, an error if not.
//
stiHResult CstiTextPlayback::ReadPacketProcess (
	const std::shared_ptr<CstiTextPacket> &packet)
{
	stiLOG_ENTRY_NAME (CstiTextPlayback::ReadPacketProcess);

	stiHResult hResult = stiRESULT_SUCCESS;

	// Is this the sequence number we were expecting?
// 	stiTrace ("<TP::ReadPacketProcess> Have sq # %d, expecting %d\n", poPacket->m_un32ExtSequenceNumber, m_un32ExpectedPacket);
	
	if (packet->m_un32ExtSequenceNumber >= m_un32ExpectedPacket)
	{
		// Send the packet to be processed.
		PacketProcess (packet);

		// Reset the throw away count.
		m_nThrowAwayCount = 0;

	} // end if
	else
	{
		stiDEBUG_TOOL ((1 & g_stiTPDebug),
			stiTrace ("<TP::ReadPacketProcess> Too late.  returning to empty queue.\n");
		);
	} // end else

//STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiTextPlayback::PacketProcess
//
// Description: Disassemble packet into frames if necessary, then decode the frames.
//
// Abstract:
//	Disassemble packet into text frames, then send the frames to the decoder.
//
// Returns:
//	stiRESULT_SUCCESS if successful, an error if not.
//
stiHResult CstiTextPlayback::PacketProcess (
	std::shared_ptr<CstiTextPacket> packet)
{
	stiLOG_ENTRY_NAME (CstiTextPlayback::PacketProcess);

	stiHResult hResult = stiRESULT_SUCCESS;
	RvRtpParam * pstRTPParam = packet->RTPParamGet ();

	SsmdTextFrame * pstTextFrame = packet->m_frame;

	if (m_nCurrentRTPPayload != pstRTPParam->payload)
	{
		TTextPayloadMap::const_iterator i;

		i = m_PayloadMap.find (pstRTPParam->payload);

		if (i != m_PayloadMap.end ())
		{
			EstiTextCodec eNewCodecType = i->second.eCodec;

			// Change to it!
			m_nCurrentRTPPayload = pstRTPParam->payload;

			hResult = SetDeviceCodec (eNewCodecType);
			stiTESTRESULT ();
		} // end if
		else
		{
			// Tried to switch to an unknown codec.
			stiASSERT (estiFALSE);

			// It's not a valid text packet, ignore this one
			packet = nullptr;
		} // end else
	} // end if

	if (packet)
	{

		switch (m_currentCodec)
		{
			case estiTEXT_T140:
				// Is this the sequence number we expect?
				if (packet->m_un32ExtSequenceNumber == m_un32ExpectedPacket)
				{
					// Decode the received data
					SstiT140Block stT140Block{};
					memset (&stT140Block, 0, sizeof (SstiT140Block));

					stT140Block.un16Len = pstTextFrame->unFrameSizeInBytes;
					stT140Block.un32Seq = packet->m_un32ExtSequenceNumber;
					stT140Block.n32PayloadType = pstRTPParam->payload;
					stT140Block.un32TimeStamp = pstRTPParam->timestamp;
					stT140Block.pun8Buf = pstTextFrame->pun8Buffer;
					Playback (&stT140Block, false);
				}
				else
				{
					// Store this data into our map for later processing.
					std::map<RvUint32, SstiT140Block*>::iterator it;
					time_t now;
					time (&now);

					// Is there a block already in the map with a sequence number larger than this one?
					for (it = m_oooT140Blocks.begin (); it != m_oooT140Blocks.end (); it++)
					{
						if (it->second->un32Seq > packet->m_un32ExtSequenceNumber)
						{
							now = it->second->timeReceived;

							// Found one, we can break output.
							break;
						}
					}

					// only store those blocks that aren't already in the store.
					it = m_oooT140Blocks.find (packet->m_un32ExtSequenceNumber);

					if (m_oooT140Blocks.end () == it)
					{
						// Allocate memory for the buffer.
						auto pstT140Block = new SstiT140Block;
						memset (pstT140Block, 0, sizeof (SstiT140Block));

						pstT140Block->un16Len = pstTextFrame->unFrameSizeInBytes;
						pstT140Block->un32Seq = packet->m_un32ExtSequenceNumber;
						pstT140Block->n32PayloadType = pstRTPParam->payload;
						pstT140Block->un32TimeStamp = pstRTPParam->timestamp;
						pstT140Block->timeReceived = now;

						if ( pstT140Block->un16Len > 0)
						{
							pstT140Block->pun8Buf = new RvUint8[pstT140Block->un16Len];
							memcpy (pstT140Block->pun8Buf, pstTextFrame->pun8Buffer, pstT140Block->un16Len);
						}

						// Store it in the map now.
						m_oooT140Blocks[pstT140Block->un32Seq] = pstT140Block;

						stiDEBUG_TOOL ((2 & g_stiTPDebug),
								stiTrace ("<TP::PacketProcess> Stored sequence %d,\n", packet->m_un32ExtSequenceNumber);
						);
					}
					else
					{
						stiDEBUG_TOOL ((2 & g_stiTPDebug),
							stiTrace ("<TP::PacketProcess> Block with sequence %d is already in map\n", packet->m_un32ExtSequenceNumber);
						);
					}
				}
				break;

			case estiTEXT_T140_RED:
			{
				SstiT140Block astT140Blocks[nMAX_REDUNDANT_BLOCKS];
				RvInt32 n32IndexOfNew = rtpT140RedUnpack (
					pstTextFrame->pun8Buffer,
					pstTextFrame->unFrameSizeInBytes,
					astT140Blocks,
					nMAX_REDUNDANT_BLOCKS,
					packet->m_un32ExtSequenceNumber,
					pstRTPParam,
					nullptr);

				// Did the unpack work fine?
				if (-1 < n32IndexOfNew)
				{
					RvRtpParam *pRtpParam = packet->RTPParamGet ();

					// Is the marker bit set and the sequence number is greater than 3 less than we expect?
					if (pRtpParam->marker &&
						packet->m_un32ExtSequenceNumber > m_un32ExpectedPacket &&
						packet->m_un32ExtSequenceNumber <= (m_un32ExpectedPacket + 2))
					{
						// We lost a packet or two but they would have only been redundant packets so update
						// our expected packet number.
						m_un32ExpectedPacket = packet->m_un32ExtSequenceNumber;

						stiDEBUG_TOOL ((1 & g_stiTPDebug),
							stiTrace ("<TP::PacketProcess> Marker bit set ... lost packets were only redundant.  Updating expected sequence number\n");
						);
					}

					// Is this the sequence number we expect?
					if (packet->m_un32ExtSequenceNumber == m_un32ExpectedPacket)

					{

						if (astT140Blocks[n32IndexOfNew].un16Len)
						{
							// Play the received data
							Playback (&astT140Blocks[n32IndexOfNew], false);
						}
						else
						{
							stiDEBUG_TOOL ((1 & g_stiTPDebug),
								stiTrace ("<TP::PacketProcess> No new data in seq # %d.\n", m_un32ExpectedPacket);
							);

							m_un32ExpectedPacket++;
						}
					}

					// If we have redundant data, is the expected sequence within the redundant data?
					else if (n32IndexOfNew > 0 &&
								(astT140Blocks[n32IndexOfNew].un32Seq - n32IndexOfNew) <= m_un32ExpectedPacket)
					{
						// Find the redundant block and send it and everything thereafter to be processed.
						int i;
						for (i = 0; i <= n32IndexOfNew; i++)
						{
							if (astT140Blocks[i].un32Seq == m_un32ExpectedPacket)
							{
								// Is the expected sequence an empty block?
								if (astT140Blocks[i].un16Len)
								{
									stiDEBUG_TOOL ((1 & g_stiTPDebug),
										stiTrace ("<TP::PacketProcess> Found desired sequence [%d] in the redundant data of packet with sequence %d\n",
													astT140Blocks[i].un32Seq, astT140Blocks[n32IndexOfNew].un32Seq);
									);
									Playback (&astT140Blocks[i], false);
								}
								else
								{
									stiDEBUG_TOOL ((1 & g_stiTPDebug),
										stiTrace ("<TP::PacketProcess> No redundant data in seq # %d.\n", astT140Blocks[i].un32Seq);
									);

									m_un32ExpectedPacket++;
								}
							}
						}
					}

					else if (m_bReceivedPrintableText)
					{
						// Store this data into our map for later processing.
						std::map<RvUint32, SstiT140Block*>::iterator it;
						time_t now;
						time (&now);

						// Is there a block already in the map with a sequence number larger than this one?
						for (it = m_oooT140Blocks.begin (); it != m_oooT140Blocks.end (); it++)
						{
							if (it->second->un32Seq > astT140Blocks[0].un32Seq)
							{
								now = it->second->timeReceived;

								// Found one, we can break output.
								break;
							}
						}

						for (int i = 0; i <= n32IndexOfNew; i++)
						{
							// only store those blocks that aren't already in the store.
							it = m_oooT140Blocks.find (astT140Blocks[i].un32Seq);

							if (m_oooT140Blocks.end () == it)
							{
								// Allocate memory for the buffer.
								auto pstT140Block = new SstiT140Block;
								memset (pstT140Block, 0, sizeof (SstiT140Block));

								pstT140Block->un16Len = astT140Blocks[i].un16Len;
								pstT140Block->un32Seq = astT140Blocks[i].un32Seq;
								pstT140Block->n32PayloadType = astT140Blocks[i].n32PayloadType;
								pstT140Block->un32TimeStamp = astT140Blocks[i].un32TimeStamp;
								pstT140Block->timeReceived = now;

								if ( pstT140Block->un16Len > 0)
								{
									pstT140Block->pun8Buf = new RvUint8[pstT140Block->un16Len];
									memcpy (pstT140Block->pun8Buf, astT140Blocks[i].pun8Buf, pstT140Block->un16Len);
								}

								// Store it in the map now.
								m_oooT140Blocks[astT140Blocks[i].un32Seq] = pstT140Block;

								stiDEBUG_TOOL ((2 & g_stiTPDebug),
									stiTrace ("<TP::PacketProcess> Stored redundant sequence %d,\n", astT140Blocks[i].un32Seq);
								);
							}
							else
							{
								stiDEBUG_TOOL ((2 & g_stiTPDebug),
									stiTrace ("<TP::PacketProcess> Block with sequence %d is already in map\n", astT140Blocks[i].un32Seq);
								)
							}
						}
					}
					else
					{
						stiDEBUG_TOOL((1 & g_stiTPDebug),
							stiTrace("<TP::PacketProcess> Packet not processed %d\n", packet->m_un32ExtSequenceNumber);
						)
					}
				}
				else
				{
					// Unpack failed.  Treat this as a mal-formed packet and just throw away the received data.
					stiDEBUG_TOOL ((1 & g_stiTPDebug),
						stiTrace ("<!!!!!!!! Received a mal-formed packet.  Seq# %d, len = %d\n", packet->m_un32ExtSequenceNumber, pstTextFrame->unFrameSizeInBytes);
					);
				}
				
				break;
			}
				
			default:
				stiASSERT (estiFALSE);

				// This is not a valid frame to send to decoder,
				break;
		} // end switch
	}

STI_BAIL:

	return hResult;
}


/*!
 * \brief Event handler for when playback read notifies us that data is available
 */
void CstiTextPlayback::EventDataAvailable ()
{
	stiLOG_ENTRY_NAME (CstiTextPlayback::EventDataAvailable);

	uint16_t un16SequenceNumber;
	RvRtpParam * pstRTPParam;

	if(estiTRUE == m_bChannelOpen)
	{
		if(m_playbackRead.NumPacketAvailable() > 0)
		{
			// Get a packet from the read task
			auto packet = MediaPacketFullGet ();

			// Did we get a packet?
			if (packet)
			{
				if (!m_nMuted)
				{
					// Yes! process this packet
					pstRTPParam = packet->RTPParamGet ();
					un16SequenceNumber = pstRTPParam->sequenceNumber;

					if (pstRTPParam->payload == m_Settings.n8KeepAlivePayloadTypeNbr)
					{
						if (m_un32ExpectedPacket == packet->m_un32ExtSequenceNumber)
						{
							stiDEBUG_TOOL (1 & g_stiTPDebug,
								stiTrace ("TP: Received expected keep alive packet.  SQ#: %u\n", pstRTPParam->sequenceNumber);
							);
							m_un32ExpectedPacket++;
						}
						else
						{
							/*
							 * This case has not been handled.  If we are seeing false positives in shared text loss, this is
							 * likely the issue.
							 */ 
							stiTrace ("TP: ERROR Received out of order keep alive packet.  SQ#: %u\n", pstRTPParam->sequenceNumber);
						}
					}
					else
					{
						// Keep track of how many (payload) bytes were received
						// ACTION SLB - 2/18/02 Does payload bytes make more sense than
						// total RTP Bytes?
						// TYY - 2/26/04 should we count byte based on what we processed (here) or
						// based on what we received (CstiTextPlaybackRead::PacketRead()).
						// They are different in the case of system saturation.
						ByteCountAdd (pstRTPParam->len - pstRTPParam->sByte);
						
						if (m_bFirstPacket)
						{
							m_bFirstPacket = estiFALSE;
							
							m_un32ExpectedPacket = packet->m_un32ExtSequenceNumber;
						}
						
						// Keep track of this sequence number
						PacketsReceivedCountAdd (packet->m_un32ExtSequenceNumber);
						
						// Process the packet
						ReadPacketProcess (packet);
					}

					stiDEBUG_TOOL ((1 & g_stiTPDebug),
						stiTrace ("TP:: Getting SQ# %d (%u, %p)\n", un16SequenceNumber,
								packet->m_un32ExtSequenceNumber, packet.get ());
					);

					// Process any out-of-order packets that might be in the packet queue
					bool bContinue = true;

					while (bContinue)
					{
						PacketQueueProcess (&bContinue);
					}
				}
			}
		}
		else
		{
			// Process any out-of-order packets that might be in the packet queue
			bool bContinue = true;

			while (bContinue)
			{
				PacketQueueProcess (&bContinue);
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//; SpecialCharactersHandle
//
// Desc: This method search for special characters.  For some of them it
// just drops them, for others, it modifies them or carries out an action.
//
// Abstract:
// The following are looked for and may, at some point, be acted upon.  For now
// we just remove them from the stream.
// BEL - 0x0007
// ST  - 0x009C
// ESC - 0x001B
// Byte order mark - 0xFEFF
// 
// The following, if found, need to remove more than just the one character.
// INT - ESC 0061
// SOS - 0x0098
// SGR - 0x009B Ps 0x006D

// The following is modified from CR LF to NL (0x2028)
// CR LF - 0x000D 0x000A
//
// Returns:
//	stiRESULT_SUCCESS if successful, an error if not.
//
stiHResult SpecialCharactersHandle (uint16_t *pun16String)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nCurrent, nInsertion;

	for (nCurrent = nInsertion = 0; pun16String[nCurrent]; nCurrent++)
	{
		switch (pun16String[nCurrent])
		{
			case un16UNICODE_BEL:  // Bell
			case un16UNICODE_ST:  // ST
			case un16UNICODE_BYTE_ORDER_MARK:  // Byte order mark
			case un16UNICODE_LF:  // A LF character by itself is to simply be removed.

				// Remove the character (by not advancing the nInsertion point.
				stiDEBUG_TOOL ((4 & g_stiTPDebug),
					stiTrace ("<TP SpecialCharactersHandle> Removing character 0x%0x\n", pun16String[nCurrent]);
				);
				break;

			case un16UNICODE_ESC:  // ESC
				// Remove this character but also look to see if it was an INT (ESC 0x0061).
				if (un16UNICODE_INT == pun16String[nCurrent + 1])
				{
					stiDEBUG_TOOL ((4 & g_stiTPDebug),
						stiTrace ("<TP SpecialCharactersHandle> Removing INT sequence (0x1B 0x61)\n");
					);
					
					// It was an INT sequence, skip the 0x0061 too.
					nCurrent++;
				}
				else
				{
					stiDEBUG_TOOL ((4 & g_stiTPDebug),
						stiTrace ("<TP SpecialCharactersHandle> Removing ESC sequence (0x1B)\n");
					);
				}
				break;

			case un16UNICODE_SOS:  // SOS
			{
				// FOR SOS, coding is SOS, function code, paramater string, ST.  The parameter string should not be more than 255 characters.
				// Remove all characters between (and including) the SOS and the ST (or a maximum of 257 characters (includes SOS and one for
				// the function code.
				if (pun16String[nCurrent + 1])
				{
					int i;
					for (i = 0; pun16String[nCurrent + 1 + i] && un16UNICODE_ST != pun16String[nCurrent + 1 + i] && i < 255; i++)
					{
					}

					stiDEBUG_TOOL ((4 & g_stiTPDebug),
						stiTrace ("<TP SpecialCharactersHandle> Removed SOS sequence (%d characters)\n", i + 1);
					);

					 // Set nCurrent just past the SOS sequence
					nCurrent += (i + 1);
				}
				break;
			}

			case un16UNICODE_SGR:
			{
				// For SGR, coding is SGR Ps 0x6D.
				if (pun16String[nCurrent + 1])
				{
					int i;
					for (i = 0; pun16String[nCurrent + 1 + i] && un16UNICODE_SGR_TERMINATOR != pun16String[nCurrent + 1 + i]; i++)
					{
					}

					stiDEBUG_TOOL ((4 & g_stiTPDebug),
						stiTrace ("<TP SpecialCharactersHandle> Removed SGR sequence (%d characters)\n", i + 1);
					);

					 // Set nCurrent just past the SGR sequence
					nCurrent += (i + 1);
				}
				break;
			}

			case un16UNICODE_CR:
				// if the next character is a LF, replace the two with a NL, otherwise, just remove it.
				if (un16UNICODE_LF == pun16String[nCurrent + 1])
				{
					stiDEBUG_TOOL ((4 & g_stiTPDebug),
						stiTrace ("<TP SpecialCharactersHandle> Repalacing sequence <CR><LF> with <NL>\n");
					);
					pun16String[nInsertion++] = un16UNICODE_NL;
				}
				else
				{
					stiDEBUG_TOOL ((4 & g_stiTPDebug),
						stiTrace ("<TP SpecialCharactersHandle> Removing CR\n");
					);
				}
				break;

			// todo - erc - what should a UCS command look like that tells me of a specific subset of the ISO 10646-1 character set?

			default:
				// Move this character to the insertion point
				if (nInsertion != nCurrent)
				{
					stiDEBUG_TOOL ((4 & g_stiTPDebug),
						stiTrace ("<TP SpecialCharactersHandle> Compressing string ...\n");
					);
					pun16String[nInsertion] = pun16String[nCurrent];
				}
				
				// Update the nInsertion point.
				nInsertion++;
				break;
		}
	}

	// Terminate the string at the nInsertion point
	pun16String[nInsertion] = 0;

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiTextPlayback::Playback
//
// Desc: Looks for and processes a text packet which will be ready
//  to send to decoder from  packet queue
//
// Abstract:
//	The function will send text frames whose time become current to text
//  driver for playback
//
// Returns:
//	stiRESULT_SUCCESS if successful, an error if not.
//
stiHResult CstiTextPlayback::Playback (SstiT140Block *pstT140Block, bool bInformOfLoss)
{
	stiLOG_ENTRY_NAME (CstiTextPlayback::Playback);

	stiHResult hResult = stiRESULT_SUCCESS;
	int nRet = 0;
	int nOffset = 0;
	uint16_t *pun16Buffer = nullptr;

	// How many Unicode characters are encoded?
	int nLength = UTF8LengthGet (pstT140Block->pun8Buf, pstT140Block->un16Len);
	stiTESTCOND (nLength > 0, stiRESULT_ERROR);

	// If we have loss to report, add another
	if (bInformOfLoss)
	{
		nLength++;
	}

	// Allocate a buffer for the Unicode characters
	pun16Buffer = new uint16_t[nLength + 1];
	nOffset = 0;

	// If we have loss to report, put the lost character into the buffer and adjust the offset for where to put the rest.
	if (bInformOfLoss && m_bReceivedPrintableText)
	{
		pun16Buffer[0] = 0xfffd;
		nOffset = 1;
	}

	// Decode from UTF-8 back to Unicode then send it to the video output driver.
	nRet = UTF8ToUnicodeConvert (pun16Buffer + nOffset, pstT140Block->pun8Buf, pstT140Block->un16Len);

	if (0 < nRet)
	{
		// Add a null-terminator to the string.
		pun16Buffer[nLength] = 0;

		// Search for and deal with special characters
		hResult  = SpecialCharactersHandle (pun16Buffer);
		stiTESTRESULT ();

		// If we have a non-empty string, send it to UI
		if (*pun16Buffer)
		{
			if (pun16Buffer[0] != 0xfffd)
			{
				m_bReceivedPrintableText = estiTRUE;
			}

			stiDEBUG_TOOL ((4 & g_stiTPDebug),
				stiTrace ("<TP::Playback> Sent %d characters to the VideoOutput driver\n\t0x", nLength);
				for (int i = 0; i < nLength; i++)
				{
					stiTrace ("%04x ", pun16Buffer[i]);
				}
				stiTrace ("\n");
			);

			remoteTextReceivedSignal.Emit (pun16Buffer);

			// This will be freed higher up the stack
			// TODO:  Once we have vpe::Signals up in SipMgr, we can make this
			//        thing a vector and get rid of the hackish memory management
			pun16Buffer = nullptr;
		}
		else
		{
			stiDEBUG_TOOL ((1 & g_stiTPDebug),
				stiTrace ("<TP::Playback> Sent 0 characters to the VideoOutput driver ... must have been all Special Characters that were stripped.\n");
			);
		}
	}
	else
	{
		if (pstT140Block->un16Len == 0)
		{
			stiDEBUG_TOOL ((4 & g_stiTPDebug),
				stiTrace ("<TP::Playback> Block with seq# %d is empty.  Not sending to UI\n", pstT140Block->un32Seq);
			);
		}
		else
		{
			stiDEBUG_TOOL ((1 & g_stiTPDebug),
				stiTrace ("<TP::Playback> Block with seq# %d has %u bytes but failed to decode from UTF8 to Unicode.  Not sending to UI\n", pstT140Block->un32Seq, pstT140Block->un16Len);
			);
		}
	}

	// Update the expected packet number.
	m_un32ExpectedPacket = pstT140Block->un32Seq + 1;

STI_BAIL:

	delete[] pun16Buffer;

	return hResult;
}

/*!\brief Stops the processing of data received from the remote endpoint
 *
 */
stiHResult CstiTextPlayback::DataChannelHold ()
{
	stiLOG_ENTRY_NAME (CstiTextPlayback::DataChannelHold);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL ((1 & g_stiTPDebug),
		stiTrace ("CstiTextPlayback::DataChannelHold\n");
	);

	// Make sure the keep alive timer stops running
	m_keepAliveTimer.stop ();

	m_checkForDelayedBlocksTimer.stop ();

	MediaPlaybackType::DataChannelHold();

	// set the current text codec type to be NONE
	m_currentCodec = estiTEXT_NONE;

	// Empty out the ooo map
	std::map<RvUint32, SstiT140Block*>::iterator it;
	RvUint32 un32Seq;

	// Remove any blocks that have a smaller sequence number than the expected one
	it = m_oooT140Blocks.begin ();
	while (it != m_oooT140Blocks.end ())
	{
		// Free memory used by this map entry
		un32Seq = it->second->un32Seq;
		delete [] it->second->pun8Buf;
		delete it->second;

		// Remove from the std::map
		m_oooT140Blocks.erase (un32Seq);

		// Advance to the next element
		it = m_oooT140Blocks.begin ();
	}

	return hResult;
} // end CstiTextPlayback::DataChannelHold



/*!\brief Resumes the processing of data received from the remote endpoint
 *
 */
stiHResult CstiTextPlayback::DataChannelResume ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Set our first packet indicator and reset the rtp structure
	m_bFirstPacket = estiTRUE;

	hResult = m_playbackRead.RtpHaltedSet (false);
	stiTESTRESULT ();

	//
	// Set a data member indicating the channel is now open.
	//
	m_bChannelOpen = estiTRUE;

STI_BAIL:

	return hResult;
}

/*! brief Inform the data task if the remote has set video privacy or not
 *
 * \retval stiRESULT_SUCCESS or stiRESULT_ERROR
 */
stiHResult CstiTextPlayback::Muted (
	eMutedReason eReason) // Is privacy set to ON or OFF
{
	stiLOG_ENTRY_NAME (CstiTextPlayback::SetRemoteTextMute);

	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	switch (eReason)
	{
		case estiMUTED_REASON_PRIVACY:

			if ((m_nMuted & estiMUTED_REASON_PRIVACY) == 0)
			{
				m_nMuted |= estiMUTED_REASON_PRIVACY;
				m_bFirstPacket = estiTRUE;
			}

			break;

		case estiMUTED_REASON_HELD:

			if ((m_nMuted & estiMUTED_REASON_HELD) == 0)
			{
				m_playbackRead.RtpHaltedSet (true);
				
				m_nMuted |= estiMUTED_REASON_HELD;
				m_bFirstPacket = estiTRUE;
			}

			break;

		case estiMUTED_REASON_HOLD:

			if ((m_nMuted & estiMUTED_REASON_HOLD) == 0)
			{
				m_un32InitialRtcpTime = m_playbackRead.InitialRTCPTimeGet ();

				DataChannelHold ();

				m_nMuted |= estiMUTED_REASON_HOLD;
				m_bFirstPacket = estiTRUE;
			}

			break;
			
		case estiMUTED_REASON_DHV:
			break;
	}

	Unlock ();

	return (hResult);
}


/*! brief Inform the data task if the remote has set video privacy or not
 *
 * \retval stiRESULT_SUCCESS or stiRESULT_ERROR
 */
stiHResult CstiTextPlayback::Unmuted (
	eMutedReason eReason) // Is privacy set to ON or OFF
{
	stiLOG_ENTRY_NAME (CstiTextPlayback::SetRemoteTextMute);

	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	switch (eReason)
	{
		case estiMUTED_REASON_PRIVACY:

			if ((m_nMuted & estiMUTED_REASON_PRIVACY) != 0)
			{
				m_nMuted &= ~estiMUTED_REASON_PRIVACY;
			}

			break;

		case estiMUTED_REASON_HELD:

			if ((m_nMuted & estiMUTED_REASON_HELD) != 0)
			{
				m_nMuted &= ~estiMUTED_REASON_HELD;

				m_playbackRead.RtpHaltedSet (false);
			}

			break;

		case estiMUTED_REASON_HOLD:

			if ((m_nMuted & estiMUTED_REASON_HOLD) != 0)
			{
				m_nMuted &= ~estiMUTED_REASON_HOLD;

				DataChannelResume ();
			}

			break;
			
		case estiMUTED_REASON_DHV:
			break;
	}

	Unlock ();

	return (hResult);
}

/*!
 * \brief Event handler for when delayed blocks timer fires
 */
void CstiTextPlayback::EventCheckForDelayedBlocksTimerTimeout ()
{
	bool found = true;
	while (found)
	{
		PacketQueueProcess (&found);
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiTextPlayback::SetDeviceCodec
//
// Description: Changes the text mode in the driver
//
// Abstract:
//
//	This function changes the text mode value into text driver
//
// Returns:
//
stiHResult CstiTextPlayback::SetDeviceCodec (
	EstiTextCodec eMode)
{
	stiLOG_ENTRY_NAME (CstiTextPlayback::SetDeviceCodec);

	stiDEBUG_TOOL ((1 & g_stiTPDebug),
		stiTrace("TP:: SetDeviceCodec mode = %d\n", eMode);
	);

	stiHResult hResult = stiRESULT_SUCCESS;
	EstiTextCodec esmdTextCodec = estiTEXT_NONE;

	switch (eMode)
	{
		case estiTEXT_T140:
			esmdTextCodec = estiTEXT_T140;
			break;
		case estiTEXT_T140_RED:
			esmdTextCodec = estiTEXT_T140_RED;

			break;
		default:
			stiASSERT (estiFALSE);
			//eResult = estiERROR;
			break;
	}

	//if(estiERROR != eResult)
	if (estiTEXT_NONE != esmdTextCodec)
	{
		// If the mode is different from the current setting,
		// set the new mode into driver
		if(m_currentCodec != esmdTextCodec)
		{
			// save the current text codec type
			m_currentCodec = esmdTextCodec;
		}
	}

	return (hResult);
}

/*!
* \brief Load some initial settings
*/
stiHResult CstiTextPlayback::DataInfoStructLoad ()
{
	auto hResult = stiRESULT_SUCCESS;

	switch (NegotiatedCodecGet ())
	{
	case estiT140_TEXT:     // fall-through
	case estiT140_RED_TEXT:
		MaxChannelRateSet (unT140_RATE);
		break;
	default:
		stiTESTCOND (false, stiRESULT_ERROR);
		break;
	}

STI_BAIL:
	return hResult;
}
