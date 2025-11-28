/*!
 * \file CstiPacketQueue.h
 * \brief contains CstiPacketQueue class
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2009 - 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

//
// Includes
//
#include <mutex>
#include <deque>

template <typename PacketType>
class CstiPacketQueue
{
public:
	// This value is always higher than any sequence number, since this number
	// is 32 bits, and sequence numbers contain only 16 bits.
	static const uint32_t un32INVALID_SEQ = 0xFFFFFFFF;

	CstiPacketQueue () = default;

	virtual	~CstiPacketQueue ()
	{
		stiLOG_ENTRY_NAME (CstiPacketQueue::~CstiPacketQueue);

		empty ();
	}

	virtual void dump ()
	{
		stiLOG_ENTRY_NAME (CstiPacketQueue::dump);

		// TODO: test with this define to make sure it compiles
#ifdef stiDEBUGGING_TOOLS
		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		for (auto &packet : m_packetQueue)
		{
			stiTrace ("Packet %p\n", packet.get ());
			stiTrace ("\tSN = %u\n", packet->m_un32ExtSequenceNumber);
			stiTrace ("\tPacket = %s\n", packet->str().c_str());
		}
#endif // stiDEBUGGING_TOOLS

	}


	/*!
	 * \brief Empties a packet queue
	 *
	 * \return void
	 */
	void clear	 ()
	{
		stiLOG_ENTRY_NAME (CstiPacketQueue::empty);

		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		m_packetQueue.clear ();
	}

	/*!
	 *
	 * \brief Checks if a packet queue is empty
	 *
	 * \return true - packet queue is empty, false - packet queue is not empty
	 */
	bool empty ()
	{
		stiLOG_ENTRY_NAME (CstiPacketQueue::isEmpty);

		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		return m_packetQueue.empty ();
	}


	/*!
	 * \brief Searches the packet queue for a video packet
	 *
	 * \return shared pointer to the packet found, NULL otherwise
	 *
	 * TODO: currently unused...
	 */
	std::shared_ptr<PacketType> packetSearch (typename PacketType::FrameType frame)
	{
		stiLOG_ENTRY_NAME (VideoPacketQueue::packetSearch);

		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		return packetFind ([frame](const std::shared_ptr<PacketType> &queuedPacket)
						   {
							   return queuedPacket->m_frame == frame;
						   });
	}

	
	/*!
	 * \brief Returns a packet that matches the passed in time stamp (if found).
	 *
	 * Searches the packet queue for the first packet that has the
	 * same timestamp as that passed in.  If found, returns a pointer
	 * to that packet.
	 *
	 * \return
	 *     a shared pointer to a packet whose timestamp matches the one passed
	 *     in, NULL otherwise.
	 */
	std::shared_ptr<PacketType> ByTimeStampGet (uint32_t timestamp)
	{
		stiLOG_ENTRY_NAME (CstiVideoPacketQueue::ByTimeStampGet);

		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		return packetFind ([timestamp](const std::shared_ptr<PacketType> &queuedPacket)
						   {
							   return queuedPacket->RTPParamGet ()->timestamp == timestamp;
						   });
	}

	/*!
	 * \brief Get a pkt whose SQ# is older than the one passed in (if found).
	 *
	 * Searches the packet queue for the first packet that has an
	 * older SQ# than that passed in.  If found, returns a pointer to
	 * that packet.
	 *
	 * \return
	 *     a shared pointer to a packet whose SQ# is smaller than the one
	 *     passed in, NULL otherwise.
	 */
	std::shared_ptr<PacketType> ByOlderSQNbrGet (uint32_t extSequenceNumber)
	{
		stiLOG_ENTRY_NAME (CstiVideoPacketQueue::ByOlderSQNbrGet);

		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		return packetFind ([extSequenceNumber](const std::shared_ptr<PacketType> &queuedPacket)
						   {
							   return queuedPacket->m_un32ExtSequenceNumber < extSequenceNumber;
						   });
	}


	/*!
	 *
	 * \brief Get the number of pkts that don't match the passed in time stamp.
	 *
	 * Searches the queue for packets whose time stamps don't match
	 * the time stamp passed in.  Calculates and returns this number.
	 *
	 * \return Returns: the calculated number
	 *
	 */
	int DifferentTimeStampCountGet (uint32_t timestamp)
	{
		stiLOG_ENTRY_NAME (CstiVideoPacketQueue::DifferentTimeStampCountGet);
		
#ifdef ercDebugPacketQueue
		stiTrace ("<CstiVideoPacketQueue::DiffTimeStampCountGet> Time = %u\n", timeStamp);
#endif

		int returnValue = 0;
		
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		for (auto &packet : m_packetQueue)
		{
#ifdef ercDebugPacketQueue
			stiTrace ("\tPacket %p, Seq = %d, Time = %d\n", packet.get (), packet->m_un32ExtSequenceNumber, packet->RTPParamGet ()->timestamp);
#endif
			if (packet->RTPParamGet ()->timestamp != timestamp)
			{
				returnValue++;
			}
		}
		
		return returnValue;
	}

	
	/*!
	 * \brief Adds a packet to the queue
	 *
	 *	This is used by the read task packet queues. Packets are pushed into the
	 *	queue from first received to last received. No other ordering is used.
	 *
	 * \return void
	 */
	void Add (std::shared_ptr<PacketType> packet)	// Packet to add to queue
	{
		stiLOG_ENTRY_NAME (CstiPacketQueue::Add);
		
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		m_packetQueue.push_back (packet);
	}


	/*!
	 * \brief Inserts a packet to the queue in order by sequence number
	 *
	 * \return
	 *	stiRESULT_SUCCESS on success
	 *	stiRESULT_DUPLICATE is set in the hResult returned if a duplicate is found
	 *	stiRESULT_PACKET_QUEUE_ERROR is set if the extended SN# of the packet is not set
	 */
	stiHResult Insert (std::shared_ptr<PacketType> packet)	// Packet to add to queue
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		stiLOG_ENTRY_NAME (CstiVideoPacketQueue::Insert);

		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		stiTESTCONDMSG (packet->m_un32ExtSequenceNumber != 0, stiRESULT_PACKET_QUEUE_ERROR,
						"Trying to insert a packet into a queue with sequence number set to 0.");

		// Is this the first packet in an empty queue?
		if (m_packetQueue.empty ())
		{
			m_markerReceived = false;
			m_numMissingPackets = 0;
			
			m_packetQueue.push_back (packet);
		}
		else
		{
			auto packetIter = std::find_if (m_packetQueue.begin (), m_packetQueue.end (),
											[packet](const std::shared_ptr<PacketType> &queuedPacket)
											{
												return queuedPacket->m_un32ExtSequenceNumber >= packet->m_un32ExtSequenceNumber;
											});
			
			if (packetIter == m_packetQueue.end ())
			{
				m_numMissingPackets += packet->m_un32ExtSequenceNumber - (m_packetQueue.back ()->m_un32ExtSequenceNumber + 1);
				m_packetQueue.push_back (packet);
			}
			else
			{
				// Check if this packet is a duplicate
				if ((*packetIter)->m_un32ExtSequenceNumber != packet->m_un32ExtSequenceNumber)
				{
					if (packetIter == m_packetQueue.begin ())
					{
						m_numMissingPackets += m_packetQueue.front ()->m_un32ExtSequenceNumber - (packet->m_un32ExtSequenceNumber + 1);
						
						m_packetQueue.push_front (packet);
					}
					else
					{
						if (m_numMissingPackets)
						{
							m_numMissingPackets--;
						}
						
						m_packetQueue.insert (packetIter, packet);
					}
				}
				else
				{
					// Trying to add a duplicate
					stiTESTCOND_NOLOG (false, stiRESULT_DUPLICATE);
				}
			}
		}
		
		if (packet->RTPParamGet ()->marker)
		{
			m_markerReceived = true;
		}

	STI_BAIL:

		return hResult;
	}


	/*!
	 * \brief Is the frame complete?
	 *
	 *	It is said to be complete if all the sequence numbers are sequential
	 *	and we have received the marker bit.
	 *
	 * \return true if the frame is deemed complete, false otherwise
	 */
	bool FrameComplete ()
	{
		bool bFrameComplete = false;

		if (m_markerReceived && m_numMissingPackets == 0)
		{
			bFrameComplete = true;
		}

		return bFrameComplete;
	}


	/*!
	 * \brief Get the sequence numbers of the first and last packet within the queue
	 *
	 * \param firstPacketSN - a pointer for where to store the first packet sequence number
	 * \param lastPacketSN - a pointer for where to store the last packet sequence number
	 *
	 * \return stiHResult
	 */
	stiHResult FirstAndLastSequenceGet (uint32_t *firstPacketSN, uint32_t *lastPacketSN)
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		if (!m_packetQueue.empty ())
		{
			if (firstPacketSN)
			{
				*firstPacketSN = m_packetQueue.front ()->m_un32ExtSequenceNumber;
			}
			
			if (lastPacketSN)
			{
				*lastPacketSN = m_packetQueue.back ()->m_un32ExtSequenceNumber;
			}
		}

		return hResult;
	}


	/*!
	 * \brief Removes a packet from the packet queue
	 *
	 * \return void
	 */
	void Remove (const std::shared_ptr<PacketType> &packet)
	{
		stiLOG_ENTRY_NAME (CstiPacketQueue::Remove);

		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		auto packetIter = std::find_if (m_packetQueue.begin (), m_packetQueue.end (),
										[packet](const std::shared_ptr<PacketType> &queuedPacket)
										{
											return queuedPacket == packet;
										});
		
		if (packetIter != m_packetQueue.end ())
		{
			m_packetQueue.erase (packetIter);
		}
	}


	/*!
	 * \brief Searches for a sequence number in the packet queue
	 *
	 * \return a shared pointer to the packet if found, NULL otherwise
	 */
	std::shared_ptr<PacketType> SequenceNumberSearch (
		uint16_t sequenceNumber)
	{
		stiLOG_ENTRY_NAME (CstiPacketQueue::SequenceNumberSearch);

		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		return packetFind ([sequenceNumber](const std::shared_ptr<PacketType> &queuedPacket)
						   {
							   return queuedPacket->RTPParamGet ()->sequenceNumber == sequenceNumber;
						   });
	}



	/*!
	 * \brief Searches for a sequence number in the packet queue
	 *
	 * \return a shared pointer to the packet if found, NULL otherwise
	 */
	std::shared_ptr<PacketType> ExtSequenceNumberSearch (uint32_t extSequenceNumber)
	{
		stiLOG_ENTRY_NAME (CstiPacketQueue::SequenceNumberSearch);

		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		return packetFind ([extSequenceNumber](const std::shared_ptr<PacketType> &queuedPacket)
						   {
							   return queuedPacket->m_un32ExtSequenceNumber == extSequenceNumber;
						   });
	}


	/*!
	 * \brief Searches for a sequence number in the packet queue, with option
	 *
	 *	Searches for the specified sequence number in the queue. If not found, the 
	 *	next available sequence number is returned in the second parameter.
	 *
	 *	un32INVALID_SEQ is placed in the second parameter if the specified 
	 *	sequence number is found. If the specified sequence number is not found, 
	 *	the second parameter will be used to return the sequence number of the 
	 *	next avialable packet in the queue (accounting for number wrap). In either 
	 *	case, if a packet is found in the queue, the return value will contain a 
	 *	pointer to the packet.
	 *
	 * \return a shared pointer to the specified or next available packet (if
	 *	found), NULL otherwise
	 */
	std::shared_ptr<PacketType> ExtSequenceNumberSearchEx (
		uint32_t extSequenceNumber,	// Sequence number to search for
		uint32_t *nextAvailableSequence) // The next available sequence found
	{
		stiLOG_ENTRY_NAME (CstiPacketQueue::ExtSequenceNumberSearchEx);
		
		uint32_t nextGreaterPacketSN = un32INVALID_SEQ;
		uint32_t lowestPacketSNFound = un32INVALID_SEQ;
		std::shared_ptr<PacketType> pNextGreaterPacket = nullptr;
		std::shared_ptr<PacketType>	pLowestPacketFound = nullptr;
		
		// Initialize return parameters
		std::shared_ptr<PacketType> returnPacket = nullptr;
		*nextAvailableSequence = un32INVALID_SEQ;
		
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		for (auto &packet : m_packetQueue)
		{
			if (packet->m_un32ExtSequenceNumber == extSequenceNumber)
			{
				returnPacket = packet;
				break;
			}
			else if (packet->m_un32ExtSequenceNumber < extSequenceNumber)
			{
				// Is it lower than lowest packet found?
				if (packet->m_un32ExtSequenceNumber < lowestPacketSNFound)
				{
					// Yes! This is now the lowest packet found
					lowestPacketSNFound = packet->m_un32ExtSequenceNumber;
					pLowestPacketFound = packet;
				}
			}
			else
			{
				// The packet must be higher than the one being looked for.
				// Is it lower than un32NextPacketHigher
				if (packet->m_un32ExtSequenceNumber < nextGreaterPacketSN)
				{
					// Yes! This is now the next highest packet found
					nextGreaterPacketSN = packet->m_un32ExtSequenceNumber;
					pNextGreaterPacket = packet;
				}
			}
		}
		
		if (!returnPacket)
		{
			// No! Did we find a packet higher than this one?
			if (nextGreaterPacketSN != un32INVALID_SEQ)
			{
				// Yes! Return this packet instead
				returnPacket = pNextGreaterPacket;
				*nextAvailableSequence = nextGreaterPacketSN;
			}
			else if (lowestPacketSNFound != un32INVALID_SEQ)
			{
				// Yes! Return this packet instead
				returnPacket = pLowestPacketFound;
				*nextAvailableSequence = lowestPacketSNFound;
			}
		}
		
		return returnPacket;
	}


	/*!
	 * \brief Searches for a sequence number in the packet queue, with option
	 *
	 *	Searches for the specified sequence number in the queue. If
	 *	not found, the next available sequence number is returned in
	 *	the second parameter.
	 *
	 *	un32INVALID_SEQ is placed in the second parameter if the
	 *	specified sequence number is found. If the specified sequence
	 *	number is not found, the second parameter will be used to
	 *	return the sequence number of the next avialable packet in the
	 *	queue (accounting for number wrap). In either case, if a
	 *	packet is found in the queue, the return value will contain a
	 *	pointer to the packet.
	 *
	 * \return a shared pointer to the specified or next available packet (if
	 *	found), NULL otherwise
	 */
	std::shared_ptr<PacketType> SequenceNumberSearchEx (
		uint16_t sequenceNumber,	// Sequence number to search for
		uint32_t *nextAvailableSequence) // The next available sequence found
	{
		stiLOG_ENTRY_NAME (CstiPacketQueue::SequenceNumberSearchEx);

		uint32_t nextGreaterPacketSN = un32INVALID_SEQ;
		uint32_t lowestPacketSNFound = un32INVALID_SEQ;
		std::shared_ptr<PacketType> pNextGreaterPacket = nullptr;
		std::shared_ptr<PacketType>	pLowestPacketFound = nullptr;
		
		// Initialize return parameters
		std::shared_ptr<PacketType> returnPacket = nullptr;
		*nextAvailableSequence = un32INVALID_SEQ;
		
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		for (auto &packet : m_packetQueue)
		{
			auto queuedPacketSN = packet->RTPParamGet ()->sequenceNumber;
			if (queuedPacketSN == sequenceNumber)
			{
				returnPacket = packet;
				break;
			}
			else if (queuedPacketSN < sequenceNumber)
			{
				// Is it lower than lowest packet found?
				if (queuedPacketSN < lowestPacketSNFound)
				{
					// Yes! This is now the lowest packet found
					lowestPacketSNFound = queuedPacketSN;
					pLowestPacketFound = packet;
				}
			}
			else
			{
				// The packet must be higher than the one being looked for.
				// Is it lower than un32NextPacketHigher
				if (queuedPacketSN < nextGreaterPacketSN)
				{
					// Yes! This is now the next highest packet found
					nextGreaterPacketSN = queuedPacketSN;
					pNextGreaterPacket = packet;
				}
			}
		}
		
		if (!returnPacket)
		{
			// No! Did we find a packet higher than this one?
			if (nextGreaterPacketSN != un32INVALID_SEQ)
			{
				// Yes! Return this packet instead
				returnPacket = pNextGreaterPacket;
				*nextAvailableSequence = nextGreaterPacketSN;
			}
			else if (lowestPacketSNFound != un32INVALID_SEQ)
			{
				// Yes! Return this packet instead
				returnPacket = pLowestPacketFound;
				*nextAvailableSequence = lowestPacketSNFound;
			}
		}
		
		return returnPacket;
	}

	/*!
	 * \brief Returns the count of packets in the queue
	 *
	 * \return int - number of packets in the queue
	 */
	int CountGet ()
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		return m_packetQueue.size ();
	}

	size_t AddAndCountGet(std::shared_ptr<PacketType> packet)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		Add (packet);

		return m_packetQueue.size ();
	}

	/*!
	 * \brief neturns the newest packet in the queue
	 *
	 * \return a shared pointer to the newest packet in the queue, NULL otherwise
	 */
	std::shared_ptr<PacketType> NewestGet ()
	{
		stiLOG_ENTRY_NAME (CstiPacketQueue::NewestGet);

		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		if (!m_packetQueue.empty ())
		{
			return m_packetQueue.back ();
		}
		
		return nullptr;
	}


	/*!
	 * \brief
	 * Description: Returns the oldest packet in the queue
	 *
	 * \return a shared pointer to the oldest packet in the queue, NULL otherwise
	 */
	std::shared_ptr<PacketType> OldestGet ()
	{
		stiLOG_ENTRY_NAME (CstiPacketQueue::OldestGet);

		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		if (!m_packetQueue.empty ())
		{
			return m_packetQueue.front ();
		}
		
		return nullptr;
	}

	std::shared_ptr<PacketType> OldestGetAndRemove()
	{
		stiLOG_ENTRY_NAME (CstiPacketQueue::OldestGetAndRemove);
		
		std::shared_ptr<PacketType> returnPacket = nullptr;
		
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		if (!m_packetQueue.empty ())
		{
			returnPacket = m_packetQueue.front ();
			
			if (returnPacket)
			{
				m_packetQueue.pop_front ();
			}
		}

		return returnPacket;
	}


	/*!
	 * \brief Returns the nth newest packet int the queue
	 *
	 * \return a shared pointer to the nth newest packet in the queue, NULL otherwise
	 */
	std::shared_ptr<PacketType> PreviousGet (size_t numberBack)
	{
		stiLOG_ENTRY_NAME (CstiPacketQueue::PreviousGet);

		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		if (m_packetQueue.size () > numberBack)
		{
			return m_packetQueue[m_packetQueue.size () - numberBack - 1];
		}
		
		return nullptr;
	}

	bool PacketsMissing ()
	{
		return m_numMissingPackets ? true : false;
	}
	
private:
	std::shared_ptr<PacketType> packetFind (const std::function<bool(const std::shared_ptr<PacketType> &)> &compareFunction)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		auto packetIter = std::find_if (m_packetQueue.begin (), m_packetQueue.end (), compareFunction);
		
		if (packetIter != m_packetQueue.end ())
		{
			return *packetIter;
		}
		
		return nullptr;
	}

protected:
	std::deque<std::shared_ptr<PacketType>> m_packetQueue;

	std::recursive_mutex 		m_mutex;

	// m_numMissingPackets and m_markerReceived are used to identify if a frame is complete.
	// They are reset when inserting the first packet into the queue for each frame.
	uint32_t m_numMissingPackets = 0;
	bool m_markerReceived = false; // this is an rtp param

};

