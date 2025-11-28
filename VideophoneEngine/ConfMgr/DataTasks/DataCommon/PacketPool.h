/*!
 * \file PacketPool.h
 * \brief contains PacketPool class
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2009 - 2018 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

//
// Includes
//
#include "CstiPacket.h"

#include <mutex>
#include <vector>

template <typename PacketType>
class PacketPool
{
public:

	PacketPool () = default;

	virtual	~PacketPool ()
	{
		stiLOG_ENTRY_NAME (PacketPool::~PacketPool);
		destroy ();
	}
	
	
	/*!
	 * \brief Creates the packets to be used in the pool
	 *
	 * \param size, number of packets to be created
	 *
	 * \return void
	 */
	void create (size_t size)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		// Validate that this pool has not been previously created
		stiASSERT (m_packets == nullptr);
		
		m_packets = new PacketType[size];
		m_poolSize = size;
	}
	
	
	/*!
	 * \brief Destroys the packets created for the pool
	 *
	 * \return void
	 */
	void destroy ()
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		if (m_packets)
		{
			m_packetPool.resize (0);
			
			delete[] m_packets;
			m_packets = nullptr;
			
			m_poolSize = 0;
			m_packetsAddedToPool = 0;
		}
	}
	
	
	/*!
	 * \brief Adds the provided data to a packet and adds the packet to the pool
	 *
	 * Adds the provided buffer, buffer size, and frame data to a packet and adds the packet to the pool
	 *
	 * \param buffer, a pointer to the uint8_t buffer to be used for the packet data
	 * \param maxBufferSize: NOTE: not used for all media types
	 * \param frame, a struct representing a specific media type frame
     *
	 * \return stiRESULT_SUCCESS if buffer was added to packet, stiRESULT_ERROR otherwise
	 */
	stiHResult bufferAdd (uint8_t *buffer, uint32_t maxBufferSize, typename PacketType::FrameType frame)
	{
		stiLOG_ENTRY_NAME (PacketPool::bufferAdd);
		stiHResult hResult = stiRESULT_SUCCESS;
		PacketType *packet = nullptr;

		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		stiTESTCONDMSG (m_packetsAddedToPool < m_poolSize, stiRESULT_ERROR, "Attempted to add buffer to packet of pool of insufficient size.");
		
		packet = &m_packets[m_packetsAddedToPool];
		
		// Copy in the new packet information
		packet->m_pun8Buffer = buffer;
		packet->m_frame = frame;
		packet->m_unBufferMaxSize = maxBufferSize;
		
		m_packetPool.push_back (packet);
		m_packetsAddedToPool++;
		
	STI_BAIL:

		return hResult;
	}
	
	
	/*!
	 * \brief Retrieves a media packet from the packet pool
	 *
	 * \return a pointer to a media packet
	 */
	std::shared_ptr<PacketType> packetGet ()
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		if (!m_packetPool.empty ())
		{
			PacketType* packet = m_packetPool.back ();
			m_packetPool.pop_back ();
			return std::shared_ptr<PacketType>(packet, [this](PacketType *packet){deleter (packet);});
		}
		
		return nullptr;
	}


	/*!
	 * \brief Returns the count of packets in the pool
	 *
	 * \return size_t - number of packets in the pool
	 */
	size_t count ()
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		return m_packetPool.size ();
	}
	
	
	/*!
	 * \brief Checks if the packet pool is empty
	 *
	 * \return true - packet pool is empty, false - packet pool is not empty
	 */
	bool empty ()
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		return m_packetPool.empty ();
	}
	
private:
	
	/*!
	 * \brief Custom deleter for shared pointers to media packets
	 *
	 *	This custom deleter returns the raw pointers to the pool
	 *
	 * \return void
	 */
	void deleter (PacketType *packet)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		
		packet->m_un32ExtSequenceNumber = 0;
		
		m_packetPool.push_back (packet);
	}


private:
	PacketType *m_packets = nullptr;
	std::vector<PacketType*> m_packetPool;
	size_t m_poolSize = 0;
	size_t m_packetsAddedToPool = 0;

	std::recursive_mutex m_mutex;

};

