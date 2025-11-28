////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CVMCircularBuffer
//
//  File Name:	CVMCircularBuffer.cpp
//
//  Owner:
//
//	Abstract: This class contains the functions for video message circular buffer.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "CVMCircularBuffer.h"
#include "SorensonErrors.h"
#include "SMCommon.h"
#include "stiTrace.h"
#include "stiAssert.h"
#include <cstdio>

//
// Constants
//
#if APPLICATION == APP_NTOUCH_MAC
const uint32_t un32MAX_BUFFER_SIZE = 518400;  // 720*480*1.5 bytes which represents a non-compressed frame in YUV 4:2:0 format
const uint32_t un32MAX_CIRCULAR_BUFFER_SIZE = 20971520;  // 20 MB ;
#elif APPLICATION == APP_NTOUCH_IOS
const uint32_t un32MAX_BUFFER_SIZE = 518400;  // 720*480*1.5 bytes which represents a non-compressed frame in YUV 4:2:0 format
const uint32_t un32MAX_CIRCULAR_BUFFER_SIZE = 20971520;  // 20 MB ;
#else
const uint32_t un32MAX_BUFFER_SIZE = 518400;  // 720*480*1.5 bytes which represents a non-compressed frame in YUV 4:2:0 format
const uint32_t un32MAX_CIRCULAR_BUFFER_SIZE = 10485760;  // 10 MB ;
//const uint32_t un32MAX_CIRCULAR_BUFFER_SIZE = 5242880;  // 5 MB ;
#endif

//
// Typedefs
//

//#define USE_LOCK_TRACKING
#ifdef USE_LOCK_TRACKING
	int g_nLibLocks = 0;
	#define CVM_LOCK(A) \
		stiTrace ("<CVMCircularBuffer::Lock> Requesting lock from %s.  Lock = %p [%s]\n", A, m_muxBufferAccess, stiOSLinuxTaskName (stiOSLinuxTaskIDSelf ()));\
		Lock ();\
		stiTrace ("<CVMCircularBuffer::Lock> Locks = %d, Lock = %p.  Exiting from %s\n", ++g_nLibLocks, m_muxBufferAccess, A);
	#define CVM_UNLOCK(A) \
		stiTrace ("<CVMCircularBuffer::Unlock> Freeing lock from %s, Lock = %p [%s]\n", A, m_muxBufferAccess, stiOSLinuxTaskName (stiOSLinuxTaskIDSelf ()));\
		Unlock (); \
		stiTrace ("<CVMCircularBuffer::Unlock> Locks = %d, Lock = %p.  Exiting from %s\n", --g_nLibLocks, m_muxBufferAccess, A);
#else
	#define CVM_LOCK(A) Lock ();
	#define CVM_UNLOCK(A) Unlock ();
#endif

//
// Forward Declarations
//

//
// Globals
//

//
// Locals
//


///////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::CVMCircularBuffer
//
//  Description: Class constructor.
//
//  Abstract:
//
//  Returns: None.
//
CVMCircularBuffer::CVMCircularBuffer ()
{
	// Initialize the linked list to point to itself
	m_stCircularBufferLinkedList.pPrev = &m_stCircularBufferLinkedList;
	m_stCircularBufferLinkedList.pNext = &m_stCircularBufferLinkedList;
}


///////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::~CVMCircularBuffer
//
//  Description: Class destructor.
//
//  Abstract:
//
//  Returns: None.
//
CVMCircularBuffer::~CVMCircularBuffer ()
{
	CleanupCircularBuffer (estiTRUE);

	if (m_muxBufferAccess)
	{
		stiOSMutexDestroy (m_muxBufferAccess);
		m_muxBufferAccess = nullptr;
	}
}

////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::InitCircularBuffer
//
// Description: Initialize the circular buffer
//
// Abstract: Allocate the circular buffer and initialize the member 
//   variables for it.
//
//  Returns:
//	 stiRESULT_SUCCESS for success or stiRESULT_Error when an error occurs
//
stiHResult CVMCircularBuffer::InitCircularBuffer ()
{
	stiLOG_ENTRY_NAME (CVMCircularBuffer::InitCircularBuffer);
	
	stiDEBUG_TOOL (g_stiMP4CircBufDebug,
		stiTrace("<CVMCircularBuffer::InitCircularBuffer> Entry.\n");
	);

	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (nullptr != m_pCircularBuffer)
	{
		stiHEAP_FREE (m_pCircularBuffer);
		m_pCircularBuffer = nullptr;
	}

	if (nullptr == m_muxBufferAccess)
	{
		m_muxBufferAccess = stiOSMutexCreate ();
	}

	CVM_LOCK ("InitCircularBuffer");

	// Allocate the circular buffer and initialize the member variables for it
	m_pCircularBuffer = (uint8_t*)stiHEAP_ALLOC (un32MAX_CIRCULAR_BUFFER_SIZE	* sizeof (uint8_t));
	if (nullptr != m_pCircularBuffer)
	{
		m_un32CurrentFileOffset = 0;
		m_pCurrentWritePosition = m_pCircularBuffer;
		m_pReservedSpace = m_pCircularBuffer + (un32MAX_CIRCULAR_BUFFER_SIZE - un32MAX_BUFFER_SIZE);
		
		// Initialize the linked list for the circular buffer
		// Add a node to describe this buffer
		auto  pstNewNode = 
			(SCircularBufferNode*)stiHEAP_ALLOC (sizeof (SCircularBufferNode));
		if (nullptr != pstNewNode)
		{ 
			// Insert the new node into the circular buffer linked list
			InsertNodeIntoCircularBufferLinkedList (pstNewNode, &m_stCircularBufferLinkedList);
			m_stCircularBufferLinkedList.pBuffer = nullptr;
			m_stCircularBufferLinkedList.un32Bytes = 0;
			m_stCircularBufferLinkedList.un32Offset = 0;
			m_stCircularBufferLinkedList.eState = eCB_INVALID;
			
			pstNewNode->pBuffer = m_pCircularBuffer;
			pstNewNode->un32Bytes = m_pReservedSpace - m_pCurrentWritePosition;
			pstNewNode->un32Offset = 0;
			pstNewNode->eState = eCB_INVALID;
			
		} // end else
		else
		{
			stiDEBUG_TOOL (g_stiMP4CircBufDebug,
				stiTrace("<CVMCircularBuffer::InitCircularBuffer> memory allocation error!");
			); // stiDEBUG_TOOL
			
			stiTHROW(stiRESULT_ERROR);
		} // end if
	} // end else
	else
	{
		stiDEBUG_TOOL (g_stiMP4CircBufDebug,
			stiTrace("<CVMCircularBuffer::InitCircularBuffer> memory allocation error!");
		); // stiDEBUG_TOOL
		
		stiTHROW(stiRESULT_ERROR);
	}

STI_BAIL:	
		
	CVM_UNLOCK ("InitCircularBuffer");

	return hResult;	
} // end CVMCircularBuffer::InitCircularBuffer

////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::CleanupCircularBuffer
//
// Description: Clean up the circular buffer
//
// Abstract: Free the circular buffer and the circular buffer linked list.
//
// Returns: none
//
//
void CVMCircularBuffer::CleanupCircularBuffer (
	EstiBool bFreeBuffer,
	uint32_t un32Offset)
{
	stiLOG_ENTRY_NAME (CVMCircularBuffer::CleanupCircularBuffer);
	
	stiDEBUG_TOOL (g_stiMP4CircBufDebug,
		stiTrace("<CVMCircularBuffer::CleanupCircularBuffer> Entry - bFreeBuffer = %d.\n", bFreeBuffer);
	);

	CVM_LOCK ("CleanupCircularBuffer");

	// Free the Circular Buffer Linked List
	SCircularBufferNode* pCurNode;
	SCircularBufferNode* pNextNode = nullptr;
	if (bFreeBuffer)
	{
		pCurNode = m_stCircularBufferLinkedList.pNext;
	} // end if
	else
	{
		// When we are through, we want to have the member node and one more node describing the
		// allocated memory.  Start, therefore, with two nodes away from the member node.
		pCurNode = m_stCircularBufferLinkedList.pNext->pNext;
	} // end else

	// Loop through the linked list freeing nodes until we come back to the 
	// entry node.
	while (&m_stCircularBufferLinkedList != pCurNode)
	{
		pNextNode = pCurNode->pNext;
		stiHEAP_FREE (pCurNode);
		pCurNode = pNextNode;
	} // end while

	if (bFreeBuffer)
	{
		m_stCircularBufferLinkedList.pPrev = &m_stCircularBufferLinkedList;
		m_stCircularBufferLinkedList.pNext = &m_stCircularBufferLinkedList;

		// 
		// Free the Circular Buffer
		// 
		if (m_pCircularBuffer)
		{
			stiHEAP_FREE (m_pCircularBuffer);
			m_pCircularBuffer = nullptr;
			m_un32BeginningOfWindowOffset = 0;
			m_un32EndOfWindowOffset = 0;
			m_pCurrentWritePosition = nullptr;
			m_pReservedSpace = nullptr;
			m_un32CurrentFileOffset = 0;
		} // end if
	} // end if
	else
	{
		// Since we aren't freeing the circular buffer, we must reset the node that 
		// points to it.
		m_pCurrentWritePosition = m_pCircularBuffer;
		pNextNode = m_stCircularBufferLinkedList.pNext;
		pNextNode->pNext = &m_stCircularBufferLinkedList;
		m_stCircularBufferLinkedList.pPrev = pNextNode;
		pNextNode->pBuffer = m_pCircularBuffer;
		pNextNode->un32Bytes = m_pReservedSpace - m_pCurrentWritePosition;
		pNextNode->un32Offset = un32Offset;
		pNextNode->eState = eCB_INVALID;
		m_un32BeginningOfWindowOffset = un32Offset;
		m_un32EndOfWindowOffset = un32Offset;

		m_un32CurrentFileOffset = un32Offset;
	} // end else
	
	CVM_UNLOCK ("CleanupCircularBuffer");

} // end CVMCircularBuffer::CleanupCircularBuffer

////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::AdjustOffsets
//
// Description: Moves the offset of each mode and member variable used to track 
// 				an offset by the amount of specified by un32OffsetAdjustment.
//
// Abstract: 
//
// Returns: stiRESULT_SUCCESS
// 			stiRESULT_ERROR
//
stiHResult CVMCircularBuffer::AdjustOffsets(
	uint32_t un32OffsetAdjustment)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Update the node offsets.
	SCircularBufferNode* pstCurrent = nullptr;
	for (pstCurrent = m_stCircularBufferLinkedList.pNext; 
		 pstCurrent != &m_stCircularBufferLinkedList;
		 pstCurrent = pstCurrent->pNext)
	{
		if (pstCurrent->eState != eCB_INVALID)
		{
			pstCurrent->un32Offset += un32OffsetAdjustment;
		}
	}

	// Update the member offsets.
	m_un32BeginningOfWindowOffset += un32OffsetAdjustment;
	m_un32EndOfWindowOffset += un32OffsetAdjustment;
	m_un32CurrentFileOffset += un32OffsetAdjustment;

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::ConfigureBufferForUpload
//
// Description: Resets all internal members so data can be retrieved and upload.
// 				
// Abstract: 
//
// Returns: stiRESULT_SUCCESS
// 			stiRESULT_ERROR
//
stiHResult CVMCircularBuffer::ConfigureBufferForUpload()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32TotalData = 0;
	uint32_t un32Bytes = 0;  stiUNUSED_ARG (un32Bytes);
	SCircularBufferNode* pstCurrent = nullptr;

	CVM_LOCK ("ConfigureBufferForUpload");

	// Reset the nodes to not viewed so they are counted in the 
	// data to be uploaded.
	for (pstCurrent = m_stCircularBufferLinkedList.pNext; 
		 pstCurrent != &m_stCircularBufferLinkedList; 
		 pstCurrent = pstCurrent->pNext) 
	{
		if (pstCurrent->eState == eCB_VALID_VIEWED)
		{
			pstCurrent->eState = eCB_VALID_NOT_VIEWED;
		}
	}

	un32Bytes = GetDataSizeInBuffer(0, &un32TotalData);
	m_un32BeginningOfWindowOffset = 0;
	m_un32EndOfWindowOffset = un32TotalData;
	m_un64MaxFileOffset = un32TotalData;

	CVM_UNLOCK ("ConfigureBufferForUpload");

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::CreateNewNode
//
// Description: Creates a new node from the passed in node.  Based no the postion
// 				of the offset it will insert it before, after or in the middle of 
// 				the passed in node.
//
// Abstract: 
//
// Returns: stiRESULT_SUCCESS
// 			stiRESULT_ERROR
//
stiHResult CVMCircularBuffer::CreateNewNode(
	SCircularBufferNode *pstNodeToDivide,	  
	uint32_t un32BeginingByte,					  
	uint32_t un32NumberOfBytes,
	ECircularBufferNodeState newNodeState)				  
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Create a new node.
	auto  pstNewNode = 
		(SCircularBufferNode*)stiHEAP_ALLOC (sizeof (SCircularBufferNode));
	stiTESTCOND(nullptr != pstNewNode, stiRESULT_ERROR);

	// Determine where the new node resides in the existing node.
	// New node is at the begining
	if (un32BeginingByte == 0)
	{
		// Insert the new node before the divide node.
		InsertNodeIntoCircularBufferLinkedList (pstNewNode, pstNodeToDivide);

		// Fix up the new node and divided node.
		pstNewNode->pBuffer = pstNodeToDivide->pBuffer;
		pstNewNode->eState = newNodeState;
		pstNewNode->un32Bytes = un32NumberOfBytes;
		pstNewNode->un32Offset = 0;

		pstNodeToDivide->pBuffer += un32NumberOfBytes;
		pstNodeToDivide->un32Bytes -= un32NumberOfBytes;
		pstNodeToDivide->un32Offset += un32NumberOfBytes;
	}

	// New node is at the end.
	else if (un32BeginingByte + un32NumberOfBytes == pstNodeToDivide->un32Bytes)
	{
		// Insert the new node after the divide node
		InsertNodeIntoCircularBufferLinkedList (pstNewNode, pstNodeToDivide->pNext);

		// Fix up the new node and divided node.
		pstNewNode->pBuffer = pstNodeToDivide->pBuffer + un32BeginingByte;
		pstNewNode->eState = newNodeState;
		pstNewNode->un32Bytes = un32NumberOfBytes;
		pstNewNode->un32Offset = 0;

		pstNodeToDivide->un32Bytes -= un32NumberOfBytes;
	}

	// New node is in the middle.
	else
	{
		// Insert the new node after the divide node and then recusivly call 
		// this function to divide the new node.
		InsertNodeIntoCircularBufferLinkedList (pstNewNode, pstNodeToDivide->pNext);

		// Fix up the new node and divided node.
		pstNewNode->pBuffer = pstNodeToDivide->pBuffer + un32BeginingByte;
		pstNewNode->eState = newNodeState;
		pstNewNode->un32Bytes = pstNodeToDivide->un32Bytes - un32BeginingByte;
		pstNewNode->un32Offset = 0;

		pstNodeToDivide->un32Bytes = un32BeginingByte;

		hResult = CreateNewNode(pstNewNode, 
								un32NumberOfBytes, pstNewNode->un32Bytes - un32NumberOfBytes,
								pstNodeToDivide->eState);
	}

STI_BAIL:	

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::CreateCircularBuffer
//
// Description: Creates buffer space at the begining of the buffer so data can be
// 				written at the begining of the file. Also sets the write position
// 				to be at the begining of the created space.
//
// Abstract: 
//
// Returns: stiRESULT_SUCCESS
// 			stiRESULT_ERROR
//
stiHResult CVMCircularBuffer::CreatePrefixBuffer(
	uint32_t un32EndingOffset,
	uint32_t un32PrefixSize)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32BytesFreed = 0;
	SCircularBufferNode *pstPrefixNode = nullptr;

	CVM_LOCK ("CreatePrefixBuffer");

	// Find the node to split.
	// Need to locate the requested offset within the linked list.
	SCircularBufferNode* pstCurrent = nullptr;

	for (pstCurrent = m_stCircularBufferLinkedList.pNext; 
		 pstCurrent != &m_stCircularBufferLinkedList;
		 pstCurrent = pstCurrent->pNext)
	{
		if (un32EndingOffset == pstCurrent->un32Offset)
		{
			CreateSkipBackBufferNode(&pstCurrent, un32PrefixSize, &pstPrefixNode, &un32BytesFreed);
			m_un32CurrentFileOffset = un32EndingOffset - un32PrefixSize;
			pstPrefixNode->un32Offset = pstCurrent->un32Offset - pstPrefixNode->un32Bytes;

			m_pCurrentWritePosition = pstPrefixNode->pBuffer;
			m_un32EndOfWindowOffset = m_un32CurrentFileOffset;
			m_un32BeginningOfWindowOffset = m_un32CurrentFileOffset;
			break;
		}
	}

	CVM_UNLOCK ("CreatePrefixBuffer");

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::CreateSkipBackBufferNode
//
// Description: This function will take a staring node (the last node we want to
// 				keep) and walk the list backwards releasing space untill we have
// 				created a node large enough to hold nunberOfBytes.
//
// Abstract: 
//
// Returns: stiRESULT_SUCCESS
// 			stiRESULT_ERROR
//
stiHResult CVMCircularBuffer::CreateSkipBackBufferNode(
	SCircularBufferNode **pstStartNode,
	uint32_t un32NumberOfBytes,
	SCircularBufferNode **ppstSkipBackNode,
	uint32_t *pun32NumberOfBytesReleased)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SCircularBufferNode *pstMergeNode = nullptr;
	SCircularBufferNode *pstWorkNode = (*pstStartNode)->pPrev;
	uint32_t remainingByteCount = un32NumberOfBytes;

	while (remainingByteCount > 0 &&
		   !stiIS_ERROR(hResult))
	{
		pstMergeNode = nullptr;

		// Is this a piece of the buffer that is at the front.  If it is we
		// need to mark it invalid and move to the end of the buffer. (we don't
		// want to split this into 2 pieces.
		if (pstWorkNode == &m_stCircularBufferLinkedList)
		{
			// Restart the freeing process.
			remainingByteCount = un32NumberOfBytes;
		}

		if (pstWorkNode->un32Bytes > remainingByteCount)
		{
			hResult = CreateNewNode(pstWorkNode,
									pstWorkNode->un32Bytes - remainingByteCount, 
									remainingByteCount,
									eCB_INVALID);

			if (pstWorkNode->eState == eCB_VALID_NOT_VIEWED)
			{
				*pun32NumberOfBytesReleased += remainingByteCount;
			}
			pstMergeNode = pstWorkNode->pNext;
		}
		else if (pstWorkNode->un32Bytes > 0 &&
				 pstWorkNode->un32Bytes <= remainingByteCount)
		{
			if (pstWorkNode->eState == eCB_VALID_NOT_VIEWED)
			{
				*pun32NumberOfBytesReleased += pstWorkNode->un32Bytes;
			}
			pstWorkNode->un32Offset = 0;
			pstWorkNode->eState = eCB_INVALID;
			pstMergeNode = pstWorkNode;
		}

		if (pstMergeNode)
		{
			// Try to merge the node with the previous one.
			if (MergeCapable(pstMergeNode, eNext, eCB_INVALID) &&
				pstMergeNode->pNext != &m_stCircularBufferLinkedList)
			{
				SCircularBufferNode *pstNextNode = pstMergeNode->pNext;

				// Update the current node and remove the next node.
				pstMergeNode->un32Bytes += pstNextNode->un32Bytes;
				RemoveNodeFromCircularBufferLinkedList (pstNextNode);
	
				if (pstNextNode == *pstStartNode)
				{
					*pstStartNode = nullptr;
				}

				// Since we have merged the node to the right into this node, we can free 
				// the node to the right.
				stiHEAP_FREE (pstNextNode);
			}
			remainingByteCount = un32NumberOfBytes - pstMergeNode->un32Bytes;
		}
		pstWorkNode = pstWorkNode->pPrev;
	}

	if (!stiIS_ERROR(hResult) &&
		pstMergeNode)
	{
		// If the prevous pointer's buffer pointer is null then we are pointing
		// at the root node and the buffer is the beginning of the ring buffer.
		if (pstMergeNode->pPrev->pBuffer == nullptr)
		{
			// Set the merge node's buffer pointer.
			pstMergeNode->pBuffer = m_pCircularBuffer;
		}
		else
		{
			// Set the merge node's buffer pointer.
			pstMergeNode->pBuffer = pstMergeNode->pPrev->pBuffer + pstMergeNode->pPrev->un32Bytes;
		}

		// If the current write postion is pointed into our skipback buffer then move it back
		// to the from of the buffer.
		if (pstMergeNode->pBuffer < m_pCurrentWritePosition &&
			pstMergeNode->pBuffer + pstMergeNode->un32Bytes > m_pCurrentWritePosition)
		{
			m_pCurrentWritePosition = pstMergeNode->pBuffer;
		}
		*ppstSkipBackNode = pstMergeNode;

		// Reset the end of the offset window.
		m_un32CurrentFileOffset -= *pun32NumberOfBytesReleased;
		m_un32EndOfWindowOffset = m_un32CurrentFileOffset;
	}

	return hResult;
}

EstiBool CVMCircularBuffer::IsDataInBuffer (
	uint32_t un32Offset, 
	uint32_t un32NumberOfBytes)
{
	stiLOG_ENTRY_NAME (CVMCircularBuffer::IsDataInBuffer);
	EstiBool bFound = estiFALSE;

	stiDEBUG_TOOL (g_stiMP4CircBufDebug,
		stiTrace("<CVMCircularBuffer::IsDataInBuffer>\n");
		stiTrace("\tm_un32BeginningOfWindowOffset = %d m_un32EndOfWindowOffset = %d\n", m_un32BeginningOfWindowOffset, m_un32EndOfWindowOffset);
		stiTrace("\tun32Offset = %d un32NumberOfBytes = %d> \n", un32Offset, un32NumberOfBytes);
	);

	CVM_LOCK ("IsDataInBuffer");

	// Is the buffer requested within the window that we have in memory?
	if (un32Offset >= m_un32BeginningOfWindowOffset && un32Offset + un32NumberOfBytes <= m_un32EndOfWindowOffset)
	{
		bFound = estiTRUE;
	}

	CVM_UNLOCK ("IsDataInBuffer");

	return bFound;
}

////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::GetBuffer
//
// Description: 
//
// Abstract:
//
// Returns: EstiBufferResult
//	estiBUFFER_OK
//	estiBUFFER_ERROR - possibly due to requesting a buffer larger than un32MAX_BUFFER_SIZE if the data is wrapped 
//					   from the end of the circular buffer back to the beginning.
//	estiBUFFER_OFFSET_OVERWRITTEN
//	estiBUFFER_OFFSET_NOT_YET_AVAILABLE
//	estiBUFFER_OFFSET_PAST_EOF
//
//
CVMCircularBuffer::EstiBufferResult CVMCircularBuffer::GetBuffer (
	uint32_t un32Offset, 
	uint32_t un32NumberOfBytes,
	uint8_t** ppBuffer) 
{
	stiLOG_ENTRY_NAME (CVMCircularBuffer::GetBuffer);
	EstiBufferResult eResult = estiBUFFER_OK;

	stiDEBUG_TOOL (g_stiMP4CircBufDebug,
		stiTrace("<CVMCircularBuffer::GetBuffer> un32Offset = %d un32NumberOfBytes = %d> \n", un32Offset, un32NumberOfBytes);
	);

	CVM_LOCK ("GetBuffer");
	// Is the buffer requested within the window that we have in memory?
	if (un32Offset >= m_un32BeginningOfWindowOffset && 
		un32Offset + un32NumberOfBytes <= m_un32EndOfWindowOffset)
	{
		// Need to locate the requested offset within the linked list.
		SCircularBufferNode* pstCurrent = nullptr;

		for (pstCurrent = m_stCircularBufferLinkedList.pNext; 
			 pstCurrent != &m_stCircularBufferLinkedList;
			 pstCurrent = pstCurrent->pNext)
		{
			// Is the requested offset within this node?
			// Is the data in this node valid AND is the requested offset within this node?
			if (eCB_INVALID != pstCurrent->eState && 
				un32Offset >= pstCurrent->un32Offset && 
				un32Offset < pstCurrent->un32Offset + pstCurrent->un32Bytes)
			{
				// At least the beginning of the requested data is within this node.  We don't care if it is
				// wholly contained within this node ... we know that it is within the window in memory.
				*ppBuffer = pstCurrent->pBuffer + (un32Offset - pstCurrent->un32Offset);

				// Does the requested data wrap around the end of the buffer?
				// Not only do we need to compare to the address of the Reserved Space but we also need
				// to avoid the data move if it has previously been done.  We will know this if the
				// current node already contains the data.
				uint8_t* pEndOfBuffer = *ppBuffer + un32NumberOfBytes;
				if (pEndOfBuffer > m_pReservedSpace &&
					pEndOfBuffer > pstCurrent->pBuffer + pstCurrent->un32Bytes)
				{
					// We need to move the wrapped data into the reserved space.
					// How much data wraps?
					uint32_t un32NbrWrappedBytes = pEndOfBuffer - (pstCurrent->pBuffer + pstCurrent->un32Bytes);
#if 0
	stiDEBUG_TOOL (g_stiMP4CircBufDebug,
		SCircularBufferNode* pstCurrent;
		stiTrace ("<CVMCircularBuffer::GetBuffer> Entering.  Linked list is as follows:\n");
		for (pstCurrent = m_stCircularBufferLinkedList.pNext;
		pstCurrent != &m_stCircularBufferLinkedList;
		pstCurrent = pstCurrent->pNext)
		{
			stiTrace ("\t<Node %p> offset = %d, size = %d, state = %s\n", 
				pstCurrent, pstCurrent->un32Offset, pstCurrent->un32Bytes, StateGet (pstCurrent));
		} // end for
	);
#endif
					// Will it fit into the reserved space?
					if (un32MAX_BUFFER_SIZE >= un32NbrWrappedBytes)
					{
						// It will fit into the reserved space.

						// Save the offset where the wrap begins
						uint32_t un32WrapOffset = un32Offset + un32NumberOfBytes - un32NbrWrappedBytes;

						stiDEBUG_TOOL (g_stiMP4CircBufDebug,
							stiTrace("<CVMCircularBuffer::GetBuffer> Copying wrapped data into reserved area.\n");
						);

						// Copy to the reserved space and update the current node to reflect the copied data. 
						memcpy (pstCurrent->pBuffer + pstCurrent->un32Bytes, 
								m_pCircularBuffer, 
								un32NbrWrappedBytes);
						pstCurrent->un32Bytes += un32NbrWrappedBytes;

						// Since we moved these we can mark the copied area at the beginning of the circular
						// buffer as INVALID.  
						SCircularBufferNode* pstNode = nullptr;

						for (pstNode = pstCurrent->pNext; 
							 pstNode != pstCurrent;
							 pstNode = pstNode->pNext)
						{
							// Is the wrapped offset found in this node?
							if (un32WrapOffset >= pstNode->un32Offset &&
								un32WrapOffset < pstNode->un32Offset + pstNode->un32Bytes)
							{
								// This data should all reside within this node.
								// Does it consume the whole node?
								if (pstNode->un32Bytes == un32NbrWrappedBytes)
								{
									// The wrapped data consumes the whole node.
									// Update the node to reflect the copy.
									pstNode->eState = eCB_INVALID;
									pstNode->un32Offset = 0;
									stiDEBUG_TOOL (g_stiMP4CircBufDebug,
										stiTrace("<CVMCircularBuffer::GetBuffer> Wrapped data cleared at "
										" beginning of buffer.  Updated the node to represent it.\n");
									);
								} // end if

								// Is it found to the left, in the center or the right side of the node?
								else if (un32WrapOffset == pstNode->un32Offset)
								{
									// It is to the left.  Create a new node to represent the wrapped data.
									auto  pstNew = 
										(SCircularBufferNode*)stiHEAP_ALLOC (sizeof (SCircularBufferNode));
									if (nullptr == pstNew)
									{
										// Log an error
									} // end if
									else
									{
										stiDEBUG_TOOL (g_stiMP4CircBufDebug,
											stiTrace("<CVMCircularBuffer::GetBuffer> Wrapped data cleared at "
											"beginning of buffer.  A new node to the left for "
											"the cleared data.\n");
										);
										// Insert the new node in the linked list
										InsertNodeIntoCircularBufferLinkedList (pstNew, pstNode);
										
										// Update the new node
										pstNew->pBuffer = pstNode->pBuffer;
										pstNew->un32Bytes = un32NbrWrappedBytes;
										pstNew->un32Offset = 0;
										pstNew->eState = eCB_INVALID;

										// Update the current node
										pstNode->pBuffer += un32NbrWrappedBytes;
										pstNode->un32Offset += un32NbrWrappedBytes;
										pstNode->un32Bytes -= un32NbrWrappedBytes;
									} // end else
								} // end else if
#if 0 
/*
This section of code should never happen.  A stream that wraps around to the beginning
of the buffer better not have data anywhere except at the beginning of the next node!
*/
								else if (un32WrapOffset + un32NbrWrappedBytes < 
										 pstNode->un32Offset + pstNode->un32Bytes)									
								{
									// It is in the middle.  We need two new nodes; the first will represent
									// the wrapped area and the second will represent the data that comes
									// thereafter.
									SCircularBufferNode* pstNew =
										(SCircularBufferNode*)stiHEAP_ALLOC (sizeof (SCircularBufferNode));
									if (NULL == pstNew)
									{
										// Log an error
									} // end if
									else
									{
										SCircularBufferNode* pstNew2 =  
											(SCircularBufferNode*)stiHEAP_ALLOC (sizeof (SCircularBufferNode));
										if (NULL == pstNew2)
										{
											// Log an error
										} // end if
										else
										{
											stiDEBUG_TOOL (g_stiMP4CircBufDebug,
												stiTrace("<CVMCircularBuffer::GetBuffer> Wrapped data cleared at "
												"beginning of buffer.  Added two nodes, \nfirst for "
												"cleared data and second for the data that followed.\n");
											);
											// Insert the nodes into the linked list
											InsertNodeIntoCircularBufferLinkedList (pstNew, pstNode->pNext);
											InsertNodeIntoCircularBufferLinkedList (pstNew2, pstNode->pNext);

											// Update the first node to represent the wrapped area.
											pstNew->pBuffer = pstNode->pBuffer + un32WrapOffset - pstNode->un32Offset;
											pstNew->un32Bytes = un32NbrWrappedBytes;
											pstNew->un32Offset = 0;
											pstNew->eState = eCB_INVALID;

											// Update the second node to represent the area after the 
											// wrapped data.
											pstNew2->pBuffer = pstNew->pBuffer + pstNew->un32Bytes;
											pstNew2->un32Bytes = pstNode->pBuffer + pstNode->un32Bytes - 
												pstNew2->pBuffer;
											pstNew2->un32Offset = pstNew->un32Offset + pstNew->un32Bytes;
											pstNew2->eState = pstNode->eState;

											// Update the current node
											pstNode->un32Bytes = pstNew->pBuffer - pstNode->pBuffer;
										} // end else
									} // end else 
								} // end else if
								else
								{
									// It is at the right end.  See if it will merge with the node to the 
									// right.
									if (MergeCapable (pstNode, eNext, eCB_INVALID))
									{
										stiDEBUG_TOOL (g_stiMP4CircBufDebug,
											stiTrace("<CVMCircularBuffer::GetBuffer> Wrapped data cleared at beginning "
											"of buffer.  Merged with node to the right.\n");
										);
										// It can merge with the node to the right
										pstNode->pNext->pBuffer -= un32NbrWrappedBytes;
										pstNode->pNext->un32Bytes += un32NbrWrappedBytes;
										pstNode->pNext->un32Offset -= un32NbrWrappedBytes;

										// Update the current node to reflect the change
										pstNode->un32Bytes -= un32NbrWrappedBytes;
									} // end if
									else
									{
										// It can't merge with the node to the right.  We'll have to create
										// a new node for it.
										SCircularBufferNode* pstNew = 
											(SCircularBufferNode*)stiHEAP_ALLOC (sizeof (SCircularBufferNode));
										if (NULL == pstNew)
										{
											// Log an error
										} // end if
										else
										{
											stiDEBUG_TOOL (g_stiMP4CircBufDebug,
												stiTrace("<CVMCircularBuffer::GetBuffer> Wrapped data cleared at beginning "
												"of buffer.  Node added to the right to describe it.\n");
											);
											// Insert the new node into the linked list
											InsertNodeIntoCircularBufferLinkedList (pstNew, pstNode->pNext);
											
											// Update the new node to reflect the wrapped area
											pstNew->pBuffer = pstNode->pBuffer + un32WrapOffset - 
												pstNode->un32Offset;
											pstNew->un32Bytes = un32NbrWrappedBytes;
											pstNew->un32Offset = 0;
											pstNew->eState = eCB_INVALID;

											// Update the current node
											pstNode->un32Bytes -= un32NbrWrappedBytes;
										} // end else
									} // end else
								} // end else
#else
								else
								{
									printf ("<MP4File::GetBuffer> Error!  Wrapped data didn't continue at the beginning of the next node!\n");
								} // end else
#endif
								//
								// We found the node we were looking for so
								// break out of the loop.
								//
								break;
								
							} // end if
						} // end for
					} // end if
					else
					{
						stiDEBUG_TOOL (g_stiMP4CircBufDebug,
							stiTrace("<CVMCircularBuffer::GetBuffer> Requesting a wrapped buffer that is too large for reserved area.\n");
						);
						eResult = estiBUFFER_ERROR;
						*ppBuffer = nullptr;
					} // end else
				} // end if
				
				//
				// We found the node we were looking for so 
				// break out of the loop
				//
				break;
				
			} // end if
		} // end for
		
		//
		// We didn't find the node that contained the file offset
		// so report it as an issue.
		//
		if (pstCurrent == &m_stCircularBufferLinkedList)
		{
			eResult = estiBUFFER_ERROR;
		}
	} // end if
	else
	{
		// The requested offset isn't within the current window
		// Did we already pass it or is it yet to come?
		if (un32Offset < m_un32BeginningOfWindowOffset)
		{
			eResult = estiBUFFER_OFFSET_OVERWRITTEN;
			stiDEBUG_TOOL (g_stiMP4CircBufDebug,
				stiTrace("<CVMCircularBuffer::GetBuffer> Requesting a buffer that has been overwritten.\n");
			);
		} // end if
		else if (un32Offset + un32NumberOfBytes > m_un64MaxFileOffset)
		{
			eResult = estiBUFFER_OFFSET_PAST_EOF;
			stiDEBUG_TOOL (g_stiMP4CircBufDebug,
				stiTrace("<CVMCircularBuffer::GetBuffer> Requesting a buffer that is beyond the EOF.\n");
			);
		}
		else
		{
			eResult = estiBUFFER_OFFSET_NOT_YET_AVAILABLE;
			stiDEBUG_TOOL (g_stiMP4CircBufDebug,
				stiTrace("<CVMCircularBuffer::GetBuffer> Requesting a buffer that hasn't yet arrived.\n");
			);
		} // end else
	} // end else

	CVM_UNLOCK ("GetBuffer");

	return (eResult);
} // end CVMCircularBuffer::GetBuffer

////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::GetCurrentWritePosition
//
// Description: Returns the next place in the circular buffer to write to.
//
// Abstract:
//
// Returns: 
//	The size of the buffer pointed to by ppWriteBuffer or 0 if a buffer couldn't be found.
//
//
uint32_t CVMCircularBuffer::GetCurrentWritePosition (
	uint8_t** ppWriteBuffer) 
{
	stiLOG_ENTRY_NAME (CVMCircularBuffer::GetCurrentWritePosition);
	uint32_t un32RetVal = 0;

	//SCircularBufferNode* pstNode = NULL;
	*ppWriteBuffer = nullptr;

	// Traverse the linked list and update the CircularBuffer linked list at the 
	// location of the current write position.
	SCircularBufferNode* pstCurrent = nullptr;
	EstiBool bFound = estiFALSE;
	stiHResult hResult = stiRESULT_SUCCESS;

	CVM_LOCK ("GetCurrentWritePosition");

	for (pstCurrent = m_stCircularBufferLinkedList.pNext; 
		 pstCurrent != &m_stCircularBufferLinkedList && !bFound; 
		 pstCurrent = pstCurrent->pNext) 
	{
		// Does the current write pointer exist within this node?
		if (m_pCurrentWritePosition >= pstCurrent->pBuffer && 
			m_pCurrentWritePosition < pstCurrent->pBuffer + pstCurrent->un32Bytes)
		{
			// It is within this node.  Is this node ready for more data?
			switch (pstCurrent->eState)
			{
				case eCB_INVALID:
				
					// This node is ready for data.  Set the write pointer to be returned.
					*ppWriteBuffer = m_pCurrentWritePosition;
					
					// Determine the length of the buffer.  If m_pCurrentWritePosition isn't
					// at the beginning of this node, we need to calculate the buffer size.
					if (m_pCurrentWritePosition == pstCurrent->pBuffer)
					{
						un32RetVal = pstCurrent->un32Bytes;
					} // end if
					else
					{
						// Calculate the buffer size by subtracting the current write position from
						// the address of the end of the buffer described by this node.
						un32RetVal = pstCurrent->un32Bytes + pstCurrent->pBuffer - m_pCurrentWritePosition;
					} // end else
					
					break;

				case eCB_VALID_NOT_VIEWED:

					// This node hasn't yet been viewed.  We've apparently need to read and free some 
					// of the data before we can store any more.  Simply break out and allow the 
					// initialized values to be returned.
					stiDEBUG_TOOL (g_stiMP4CircBufDebug,
						stiTrace("<CVMCircularBuffer::GetCurrentWritePosition> No available space to receive into.\n");
					);
					break;

				case eCB_VALID_VIEWED:

					// We need to use buffer space that has been previously used.  First mark a portion 
					// as invalid and then return it.
					stiDEBUG_TOOL (g_stiMP4CircBufDebug,
						stiTrace("<CVMCircularBuffer::GetCurrentWritePosition> Reusing previous space.\n");
					);
					
					// Is the current write position found at the beginning of this node?
					if (m_pCurrentWritePosition != pstCurrent->pBuffer)
					{
						// The current write position is in the middle of this node.
						// Create a new node that refers to the beginning of this node.
						auto  pstNew = 
							(SCircularBufferNode*)stiHEAP_ALLOC (sizeof (SCircularBufferNode));
						stiTESTCOND(nullptr != pstNew, stiRESULT_ERROR);

						// Insert the new node into the circular buffer linked list
						InsertNodeIntoCircularBufferLinkedList (pstNew, pstCurrent);

						// Update the new node
						pstNew->pBuffer = pstCurrent->pBuffer;
						pstNew->un32Bytes = m_pCurrentWritePosition - pstCurrent->pBuffer;
						pstNew->un32Offset = pstCurrent->un32Offset;
						pstNew->eState = eCB_VALID_VIEWED;
						
						// Update the current node
						pstCurrent->pBuffer = m_pCurrentWritePosition;
						pstCurrent->un32Bytes -= pstNew->un32Bytes;
						pstCurrent->un32Offset = pstNew->un32Offset + pstNew->un32Bytes;
					} // end if

					// Now, we should have the current pointer (pstCurrent->pBuffer) pointing 
					// to the current write pointer (m_pCurrentWritePosition).
						
					// Does the first un32REUSE_BUFFER_SIZE bytes of this node contain area within 
					// the reserved space?  If so, we  need to reduce the node to contain only that 
					// which is prior to the reserved space area.
					if (m_pReservedSpace >= pstCurrent->pBuffer &&
						m_pReservedSpace < pstCurrent->pBuffer + pstCurrent->un32Bytes &&
						pstCurrent->un32Bytes < un32REUSE_BUFFER_SIZE)
					{
						pstCurrent->un32Bytes = m_pReservedSpace - pstCurrent->pBuffer;
						stiDEBUG_TOOL (g_stiMP4CircBufDebug,
							stiTrace("<CVMCircularBuffer::GetCurrentWritePosition> Reserved space being returned.\n");
						);
					} // end if

					
					// If the node is greater than un32REUSE_BUFFER_SIZE bytes, break off a chunk that is 
					// un32REUSE_BUFFER_SIZE bytes.
					if (pstCurrent->un32Bytes > un32REUSE_BUFFER_SIZE)
					{
						// Add a new node that describes everything to the right of what we will return.
						auto  pstNew = 
							(SCircularBufferNode*)stiHEAP_ALLOC (sizeof (SCircularBufferNode));
						stiTESTCOND(nullptr != pstNew, stiRESULT_ERROR);

						// Insert the new node into the circular buffer linked list
						InsertNodeIntoCircularBufferLinkedList (pstNew, pstCurrent->pNext);

						// Update the new node
						pstNew->pBuffer = pstCurrent->pBuffer + un32REUSE_BUFFER_SIZE;
						pstNew->un32Bytes = pstCurrent->un32Bytes - un32REUSE_BUFFER_SIZE;
						pstNew->un32Offset = pstCurrent->un32Offset + un32REUSE_BUFFER_SIZE;
						pstNew->eState = pstCurrent->eState;

						// Update the current node
						pstCurrent->un32Bytes = un32REUSE_BUFFER_SIZE;
						pstCurrent->eState = eCB_INVALID;
					} // end if
					else
					{
						// The buffer isn't larger than un32REUSE_BUFFER_SIZE, simply change the state to invalid.
						pstCurrent->eState = eCB_INVALID;
					} // end else

					if (!stiIS_ERROR(hResult))
					{
						// This node is now ready for data.  Set the write pointer to be returned.
						*ppWriteBuffer = m_pCurrentWritePosition;
						un32RetVal = pstCurrent->un32Bytes;

						// Update the Beginning Of Window offset
						m_un32BeginningOfWindowOffset = pstCurrent->pNext->un32Offset;
					} // end if
					
					break;

				default:
					break;
			} // end switch

			// Since we have found the node, we can bail out of the loop.
			bFound = estiTRUE;
		} // end if
	} // end for

STI_BAIL:	

	CVM_UNLOCK ("GetCurrentWritePosition");

	stiDEBUG_TOOL (g_stiMP4CircBufDebug,
		stiTrace ("<CVMCircularBuffer::GetCurrentWritePosition> Returning %d bytes.\n", un32RetVal);
	);
	return (un32RetVal);
} // end CVMCircularBuffer::GetCurrentWritePosition

////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::GetDataSizeInBuffer
//
// Description: This function loops through the nodes and creates 2 totals.
// 				One is the amount of data found in any node that's state is 
// 				set to VALID_NOT_VIEWED (this would be the pTotalDataSize).
// 				The second is the amount of contiguous data from the read ponter
// 				to either the write pointer or the end of the buffer.
//
// Abstract: 
//
// Returns: Total amount of data in the buffer that has not been viewed.
//
uint32_t CVMCircularBuffer::GetDataSizeInBuffer(
	uint32_t  un32Offset,
	uint32_t *pTotalDataSize) const
{
	uint32_t un32ContiguousDataSize = 0;
	uint32_t un32TotalDataSize = 0;
	SCircularBufferNode* pstCurrent = nullptr;
	EstiBool bFoundOffset = estiFALSE;

	CVM_LOCK ("GetDataSizeInBuffer");

	// Loop through the buffer nodes.
	for (pstCurrent = m_stCircularBufferLinkedList.pNext; 
		 pstCurrent != &m_stCircularBufferLinkedList; 
		 pstCurrent = pstCurrent->pNext) 
	{
		// Did we find the read node?
		if (!bFoundOffset &&
			un32Offset >= pstCurrent->un32Offset && 
			un32Offset < pstCurrent->un32Offset + pstCurrent->un32Bytes)
		{
			bFoundOffset = estiTRUE;
		}

		if (pstCurrent->eState == eCB_VALID_NOT_VIEWED)
		{
			un32TotalDataSize += pstCurrent->un32Bytes;
	
			if (bFoundOffset)
			{
				un32ContiguousDataSize += pstCurrent->un32Bytes;
			}
		}
	}

	CVM_UNLOCK ("GetDataSizeInBuffer");
	if (pTotalDataSize)
	{
		*pTotalDataSize = un32TotalDataSize;
	}

	return un32ContiguousDataSize;
}

////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::GetSkipBackWritePosition
//
// Description: Returns the buffer of the node created for skipback data.
//
// Abstract:
//
// Returns: The number of bytes in the buffer.
//
//
uint32_t CVMCircularBuffer::GetSkipBackWritePosition(
	uint32_t un32Offset,
	uint32_t un32NumberOfBytes,
	uint8_t **ppWriteBuffer)
{
	uint32_t un32RetVal = 0;

	//SCircularBufferNode* pstNode = NULL;
	*ppWriteBuffer = nullptr;

	// Traverse the linked list and update the CircularBuffer linked list at the 
	// location of the current write position.
	SCircularBufferNode* pstCurrent = nullptr;
	EstiBool bFound = estiFALSE;

	CVM_LOCK ("GetSkipBackWritePosition");

	for (pstCurrent = m_stCircularBufferLinkedList.pNext; 
		 pstCurrent != &m_stCircularBufferLinkedList && !bFound; 
		 pstCurrent = pstCurrent->pNext) 
	{
		// Is the current node the one we are looking for?
		if (pstCurrent->eState == eCB_INVALID &&
			pstCurrent->un32Offset == un32Offset && 
			pstCurrent->un32Bytes == un32NumberOfBytes)
		{
			// Set the write pointer to be returned.
			*ppWriteBuffer = pstCurrent->pBuffer;
			
			// Set the number of bytes returned.
			un32RetVal = pstCurrent->un32Bytes;
			
			// Set the beginnning of the offset window to the begining of this node if the window
			// is located to the right of this point (i.e. later in the file).
			if (m_un32BeginningOfWindowOffset > pstCurrent->un32Offset)
			{
				m_un32BeginningOfWindowOffset = pstCurrent->un32Offset;
			}

			bFound = estiTRUE;			
		}
	}

	CVM_UNLOCK ("GetSkipBackWritePosition");

	return un32RetVal;
}

////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::IsBufferEmpty
//
// Description: Returns back whether buffer is empty or has data. 
//
// Abstract:
//
// Returns: EstiBool
//	estiTRUE - The Buffer is Empty
//	estiFALSE - The Buffer has data.
//
//
EstiBool CVMCircularBuffer::IsBufferEmpty () const
{
	EstiBool bEmpty = estiTRUE;

	CVM_LOCK ("IsBufferEmpty");
	for (SCircularBufferNode *pstCurrent = m_stCircularBufferLinkedList.pNext; 
		 pstCurrent != &m_stCircularBufferLinkedList; 
		 pstCurrent = pstCurrent->pNext) 
	{
		if (pstCurrent->eState == eCB_VALID_NOT_VIEWED)
		{
			bEmpty = estiFALSE;
			break;
		}
	}

	CVM_UNLOCK ("IsBufferEmpty");

	return bEmpty;
}

////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::IsBufferSpaceAvailable
//
// Description: Returns back whether specified space is available in the buffer. 
//				If pun32BytesFree is passed in, it will return the number of 
// 				bytes available only if the free space is less than the needed 
// 				space.
// 
// Abstract:
//
// Returns: EstiBool
//	estiTRUE - The nodes can be merged
//	estiFALSE - The nodes can NOT be merged
//
//
EstiBool CVMCircularBuffer::IsBufferSpaceAvailable(
		uint32_t un32BytesNeeded,
		uint32_t *pun32BytesFree)
{
	uint32_t un32BytesAvailable = 0;
	SCircularBufferNode* pstCurrent = nullptr;
	EstiBool bFoundEnd = estiFALSE;

	CVM_LOCK ("IsBufferSpaceAvailable");

	for (pstCurrent = m_stCircularBufferLinkedList.pNext; 
		 pstCurrent != &m_stCircularBufferLinkedList && un32BytesAvailable < un32BytesNeeded && !bFoundEnd; 
		 pstCurrent = pstCurrent->pNext) 
	{
		// Does the current write pointer exist within this node?
		if (m_pCurrentWritePosition >= pstCurrent->pBuffer && 
			m_pCurrentWritePosition < pstCurrent->pBuffer + pstCurrent->un32Bytes)
		{

			// We found the write cursor's node now calculate the number of bytes we
			// have that are free.

			SCircularBufferNode* pstWritePosNode = pstCurrent;
			while (pstCurrent->pNext != pstWritePosNode && 
				   un32BytesAvailable < un32BytesNeeded && 
				   !bFoundEnd) 
			{
				// It is within this node.  Is this node ready for more data?
				switch (pstCurrent->eState)
				{
					case eCB_INVALID:
					case eCB_VALID_VIEWED:
					{
						// Determine the length of the buffer.  If m_pCurrentWritePosition isn't
						// at the beginning of this node, we need to calculate the buffer size.
						if (m_pCurrentWritePosition > pstCurrent->pBuffer && 
							m_pCurrentWritePosition < pstCurrent->pBuffer + pstCurrent->un32Bytes)
						{
							// Calculate the buffer size by subtracting the current write position from
							// the address of the end of the buffer described by this node.
							un32BytesAvailable += pstCurrent->un32Bytes + pstCurrent->pBuffer - m_pCurrentWritePosition;
						}
						else
						{
							un32BytesAvailable += pstCurrent->un32Bytes;
						} 
					}
						break;
					case eCB_VALID_NOT_VIEWED:
					{
						// The write cursor is up against the read cursor so there is no
						// space.  Break out.
						bFoundEnd = estiTRUE;
					}
						break;
					default:
						break;
				}
				pstCurrent = pstCurrent->pNext; 
			}
			bFoundEnd = estiTRUE;
		}
	}
	CVM_UNLOCK ("IsBufferSpaceAvailable");


	if (pun32BytesFree != nullptr)
	{
		*pun32BytesFree = un32BytesAvailable;
	}

	return un32BytesAvailable >= un32BytesNeeded ? estiTRUE : estiFALSE;
}

////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::MergeCapable
//
// Description: Determine if two nodes can be merged. 
//
// Abstract:
//	To be able to merge, the neighbor node must:
//		- have the state of that passed in
//		- the offsets between the two nodes to be merged must be contiguous
//
// Returns: EstiBool
//	estiTRUE - The nodes can be merged
//	estiFALSE - The nodes can NOT be merged
//
//
EstiBool CVMCircularBuffer::MergeCapable (
	SCircularBufferNode* pstNode,
	ENodeDirection eNode,
	ECircularBufferNodeState eState)
{
	stiLOG_ENTRY_NAME (CVMCircularBuffer::MergeCapable);
	EstiBool bResult = estiFALSE;
	if (nullptr != pstNode)
	{
		switch (eNode)
		{
			case ePrev:
				stiDEBUG_TOOL (g_stiMP4CircBufDebug,
					stiTrace("<CVMCircularBuffer::MergeCapable> Comparing node %p with previous node %p.\n", pstNode, pstNode->pPrev);
				);

				if (eState == pstNode->pPrev->eState)
				{
					// The state is good, now ensure contiguousness (unless it is in the eCB_INVALID state).
					if (eState == eCB_INVALID || 
						pstNode->pPrev->un32Offset + pstNode->pPrev->un32Bytes == pstNode->un32Offset)
					{
						// They can merge.
						bResult = estiTRUE;
					} // end if
				} // end if
				break;
				
			case eNext:
				stiDEBUG_TOOL (g_stiMP4CircBufDebug,
					stiTrace("<CVMCircularBuffer::MergeCapable> Comparing node %p with next node %p.\n", pstNode, pstNode->pNext);
				);

				if (eState == pstNode->pNext->eState)
				{
					// The state is good, now ensure contiguousness (unless it is in the eCB_INVALID state).
					if (eState == eCB_INVALID || 
						pstNode->un32Offset + pstNode->un32Bytes == pstNode->pNext->un32Offset)
					{
						// They can merge.
						bResult = estiTRUE;
					} // end if
				} // end if
				break;
				
			default:
				break;
		} // end switch
	} // end if	

	stiDEBUG_TOOL (g_stiMP4CircBufDebug,
		stiTrace("<CVMCircularBuffer::MergeCapable> Returning %s\n", bResult ? "True" : "False");
	);
	return (bResult);
} // end CVMCircularBuffer::MergeCapable

////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::ResetBufferToValidNotViewed
//
// Description: 
//
// Abstract:  This method verifies that the offset passed in is within the
//	current window.  If so, it resets everything between that position and
//	the next node that is ValidNotViewed OR to the node preceeding the current
//	write pointer back to ValidNotViewed.   Where possible, nodes are also 
//	merged again.
//
//	*** NOTE *** This method assumes that the circular buffer is not being modified
//	by any other process while this reset is occurring.  If it becomes necessary to
//	be able to modify the buffer simultaneously, a mutex will need to be added to
//	protect the data.
//
//	Another assumption being made is that the area to be reset will not wrap around
//	to the beginning of the buffer.  In other words, no data in the buffer prior to 
//	the offset passed in, will be reset.
//
// Returns: stiHResult
//	stiRESULT_SUCCESS - Success
//	stiRESULT_ERROR - Not able to reset the requested area of the circular buffer OR it isn't
//				within the window that we have in memory.
//
//
stiHResult CVMCircularBuffer::ResetBufferToValidNotViewed (
	uint32_t un32Offset)
{
	stiLOG_ENTRY_NAME (CVMCircularBuffer::ResetBufferToValidNotViewed);
	stiHResult hResult = stiRESULT_SUCCESS;
	stiDEBUG_TOOL (g_stiMP4CircBufDebug,
		stiTrace("<CVMCircularBuffer::ResetBufferToValidNotViewed> Entry, offset = %d.\n", un32Offset);
	);

	CVM_LOCK ("ResetBufferToValidNotViewed");

	// Is the offset within the window that we have in memory?
	if (un32Offset >= m_un32BeginningOfWindowOffset && un32Offset <= m_un32EndOfWindowOffset)
	{
		// Need to locate the requested offset within the linked list.
		SCircularBufferNode* pstCurrent = nullptr;
		SCircularBufferNode* pstOffsetNode = nullptr;
		EstiBool bFoundRequestedOffset = estiFALSE;
		for (pstCurrent = m_stCircularBufferLinkedList.pNext; 
			 pstCurrent != &m_stCircularBufferLinkedList && !bFoundRequestedOffset; 
			 pstCurrent = pstCurrent->pNext)
		{
			// Is the data in this node valid AND is the requested offset within this node?
			if (eCB_INVALID != pstCurrent->eState && 
				un32Offset >= pstCurrent->un32Offset && 
				un32Offset < pstCurrent->un32Offset + pstCurrent->un32Bytes)
			{
				// Found the node that contains the requested offset.
				bFoundRequestedOffset = estiTRUE;
				pstOffsetNode = pstCurrent;

				// Does the requested offset begin at the beginning of this node AND
				// is the node NOT in the ValidNotViewed state?
				if (un32Offset != pstCurrent->un32Offset && 
					eCB_VALID_VIEWED == pstCurrent->eState)	
				{
					// Need to create a new node and describe the data prior to 
					// this offset in the new node.
					auto  pstNew = 
						(SCircularBufferNode*)stiHEAP_ALLOC (sizeof (SCircularBufferNode));
					if (nullptr == pstNew)
					{
						// Log an error
					} // end if
					else
					{
						stiDEBUG_TOOL (g_stiMP4CircBufDebug,
							stiTrace("<CVMCircularBuffer::ResetBufferToValidNotViewed> A new node to the left for "
							"the data prior to the passed in offset.\n");
						);
						// Insert the new node in the linked list
						InsertNodeIntoCircularBufferLinkedList (pstNew, pstCurrent);

						// Calculate the difference between the passed in offset and the current offset
						uint32_t un32OffsetDelta = un32Offset - pstCurrent->un32Offset;

						// Update the new node
						pstNew->pBuffer = pstCurrent->pBuffer;
						pstNew->un32Offset = pstCurrent->un32Offset;
						pstNew->un32Bytes = un32OffsetDelta;
						pstNew->eState = pstCurrent->eState;

						// Update the current node
						pstCurrent->pBuffer += un32OffsetDelta;
						pstCurrent->un32Bytes -= un32OffsetDelta;
						pstCurrent->un32Offset = un32Offset;
					} // end else
				} // end if
			} // end if
		} // end for

		// Was the requested offset found?
		if (bFoundRequestedOffset)
		{
			// Is the current node already VALID_NOT_VIEWED?
			if (eCB_VALID_NOT_VIEWED != pstOffsetNode->eState)
			{
				// Is this node the last node prior to the tail node?
				if (pstOffsetNode->pNext == &m_stCircularBufferLinkedList)
				{
					// Since it is the only node, all we need to do is change the state back to
					// ValidNotViewed
					pstOffsetNode->eState = eCB_VALID_NOT_VIEWED;
				} // end if
				else
				{
					// There are more than one node that will possibly need to be reset.

					// Look for the next node that is already marked as VALID_NOT_VIEWED or 
					// find the node that represents the last piece of data before wrapping.
					SCircularBufferNode* pstMergeNode = nullptr;
					SCircularBufferNode* pstTempNode = nullptr;
					SCircularBufferNode* pstPreWrapNode = nullptr;
					EstiBool bFoundMergeNode = estiFALSE;
					EstiBool bFoundInvalidNode = estiFALSE;

					for (pstTempNode = pstOffsetNode->pNext; 
						 pstTempNode != &m_stCircularBufferLinkedList && 
						 !bFoundMergeNode && !bFoundInvalidNode;
						 pstTempNode = pstTempNode->pNext)
					{
						// Is this node marked as VALID_NOT_VIEWED?
						if (eCB_VALID_NOT_VIEWED == pstTempNode->eState)
						{
							// We've found the node that we want to merge with.  All nodes between pstCurrent and
							// pstMergeNode need to be freed and then pstMergeNode needs to be updated to include
							// all that area.
							bFoundMergeNode = estiTRUE;
							pstMergeNode = pstTempNode;
						} // end if

						else if (eCB_VALID_VIEWED == pstTempNode->eState)
						{
							// This could be the node just prior to wrapping.  Store it.
							pstPreWrapNode = pstTempNode;
						} // end if

						// If we find an INVALID node, we need to bail out.
						else if (eCB_INVALID == pstTempNode->eState)
						{
							bFoundInvalidNode = estiTRUE;
						} // end else if
					} // end for

					// If we didn't find an invalid node, and we didn't find the merge node, set the merge node
					// to the pre-wrap node.
					if (!bFoundInvalidNode && !bFoundMergeNode)
					{
						// Did we ever set a PreWrap node?
						if (nullptr != pstPreWrapNode)
						{
							pstMergeNode = pstPreWrapNode;
							bFoundMergeNode = estiTRUE;
						} // end if
					} // end if

					// Did we find the node that marks where we want to merge to?
					if (bFoundMergeNode)
					{
						// We need to merge all nodes between (and including) pstCurrent and pstMergeNode
						// and change the state to ValidNotViewed
						SCircularBufferNode *pNext = nullptr;
						SCircularBufferNode *pstMergeNextNode = pstMergeNode->pNext;
						do
						{
							pNext = pstOffsetNode->pNext;
							pstOffsetNode->un32Bytes += pNext->un32Bytes;
							RemoveNodeFromCircularBufferLinkedList (pNext);
							stiHEAP_FREE (pNext);
						} while (pstOffsetNode->pNext != pstMergeNextNode);
					} // end if

						// Change the state of the Current node and set the result to success.
						pstOffsetNode->eState = eCB_VALID_NOT_VIEWED;
				} // end else
			} // end else
		} // end if
	} // end if

	CVM_UNLOCK ("ResetBufferToValidNotViewed");

	stiDEBUG_TOOL (g_stiMP4CircBufDebug,
		stiTrace("<CVMCircularBuffer::ResetBufferToValidNotViewed> Exit, hResult = 0x%0x.\n", hResult);
	);

	return (hResult);
} // end CVMCircularBuffer::ResetBufferToValidNotViewed

////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::PrintNodeList
//
// Description: 
//
// Abstract:
//
// Returns: None
//
//
void CVMCircularBuffer::PrintNodeList(FILE *pFile)
{
	SCircularBufferNode* pstCurrent = nullptr;

	CVM_LOCK ("PrintNodeList");

	int nNodeCount = 0;
	for (pstCurrent = m_stCircularBufferLinkedList.pNext; 
		 pstCurrent != &m_stCircularBufferLinkedList; 
		 pstCurrent = pstCurrent->pNext) 
	{
		if (pFile)
		{
			fprintf(pFile, "node = %p, buffer = %p, offset = %d, bufferSize = %d state = %s\n", 
					pstCurrent, pstCurrent->pBuffer, pstCurrent->un32Offset, pstCurrent->un32Bytes, 
					pstCurrent->eState == eCB_INVALID ? "eCB_INVALID" : pstCurrent->eState == eCB_VALID_VIEWED ? "eCB_VALID_VIEWED" : "eCB_VALID_NOT_VIEWED");
		}
		else
		{
			stiTrace("node = %p, buffer = %p, offset = %d, bufferSize = %d state = %s\n", 
					 pstCurrent, pstCurrent->pBuffer, pstCurrent->un32Offset, pstCurrent->un32Bytes, 
					 pstCurrent->eState == eCB_INVALID ? "eCB_INVALID" : pstCurrent->eState == eCB_VALID_VIEWED ? "eCB_VALID_VIEWED" : "eCB_VALID_NOT_VIEWED");
		}
		nNodeCount++;
	}

	if (pFile)
	{
		fprintf(pFile, "\n");
	}

	CVM_UNLOCK ("PrintNodeList");
}

////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::ReleaseBuffer
//
// Description: 
//
// Abstract:
//
// Returns: EstiResult
//	estiOK - Success
//	estiERROR - Failure
//
//
stiHResult CVMCircularBuffer::ReleaseBuffer (
	uint32_t un32Offset, 
	uint32_t un32NumberOfBytes)
{
	stiLOG_ENTRY_NAME (CVMCircularBuffer::ReleaseBuffer);
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiMP4CircBufDebug,
		stiTrace("<CVMCircularBuffer::ReleaseBuffer> Entry, offset = %d, bytes = %d.\n", un32Offset, un32NumberOfBytes);
	);

	CVM_LOCK ("ReleaseBuffer");

	// If the whole buffer being released isn't within the current window, update 
	// the passed in parameters to that portion that is within the current window.

	// Deal with that that exceeds the current window first.  Does the requested offset
	// begin before the end of the current window and extend beyond the window?
	if (un32Offset < m_un32EndOfWindowOffset &&
		un32Offset + un32NumberOfBytes > m_un32EndOfWindowOffset)
	{
		// Reduce the requested number of bytes by the amount that goes past the end of
		// the current window.
		stiDEBUG_TOOL (g_stiMP4CircBufDebug,
			stiTrace("<CVMCircularBuffer::ReleaseBuffer> Requested buffer adjusted from 0x%08x-0x%08x to 0x%08x-0x%08x.\n",
			un32Offset, un32Offset + un32NumberOfBytes, 
			un32Offset, m_un32EndOfWindowOffset);
		);
		un32NumberOfBytes -= (un32Offset + un32NumberOfBytes - m_un32EndOfWindowOffset);
	} // end if

	// Now check to see if the requested offset is prior to the beginning of the buffer.
	// Does the requested offset begin before the beginning of the current window and end
	// within the window?
	if (un32Offset < m_un32BeginningOfWindowOffset &&
		un32Offset + un32NumberOfBytes > m_un32BeginningOfWindowOffset)
	{
		// It is, we need to not only move the offset to the beginning of the window
		// but reduce the requested bytes by the amount shifted.
		stiDEBUG_TOOL (g_stiMP4CircBufDebug,
			stiTrace("<CVMCircularBuffer::ReleaseBuffer> Requested buffer adjusted from 0x%08x-0x%08x to 0x%08x-0x%08x.\n",
			un32Offset, un32Offset + un32NumberOfBytes, 
			m_un32BeginningOfWindowOffset, m_un32BeginningOfWindowOffset + un32NumberOfBytes - 
			(m_un32BeginningOfWindowOffset - un32Offset));
		);
		un32NumberOfBytes -= (m_un32BeginningOfWindowOffset - un32Offset);
		un32Offset = m_un32BeginningOfWindowOffset;
	} // end if

	// Is the buffer being released within the window that we have in memory?
	if (un32Offset >= m_un32BeginningOfWindowOffset && un32Offset + un32NumberOfBytes <= m_un32EndOfWindowOffset)
	{

		// Find the node(s) that contain(s) this buffer
		SCircularBufferNode* pstCurrent = nullptr;
		EstiBool bFound = estiFALSE;

		for (pstCurrent = m_stCircularBufferLinkedList.pNext; 
			 pstCurrent != &m_stCircularBufferLinkedList && !bFound;
			 pstCurrent = pstCurrent->pNext)
		{
			// Does the offset begin within this node?
			if (eCB_INVALID != pstCurrent->eState && 
				un32Offset >= pstCurrent->un32Offset && 
				un32Offset < pstCurrent->un32Offset + pstCurrent->un32Bytes)
			{
				// We've found an area to be released.
				// Is the whole thing within this node? (it could also extend past this node)
				if (un32Offset + un32NumberOfBytes <= pstCurrent->un32Offset + pstCurrent->un32Bytes)
				{
					// It is totally within this node.  Keep track that we will complete with this iteration.
					bFound = estiTRUE;
				} // end if

				// Does the area that we are freeing consume this whole node?
				if (un32Offset == pstCurrent->un32Offset && 
					un32Offset + un32NumberOfBytes >= pstCurrent->un32Offset + pstCurrent->un32Bytes)
				{
					// The whole thing is to be freed.
					stiDEBUG_TOOL (g_stiMP4CircBufDebug,
						stiTrace ("<CVMCircularBuffer::ReleaseBuffer> Area to be freed consumes all of node %p\n", pstCurrent);
					);
					
					// Get a copy of the left and right nodes.
					SCircularBufferNode* pLeft = pstCurrent->pPrev;
					SCircularBufferNode* pRight = pstCurrent->pNext;
						
					// Update the passed in offset and number of bytes to reflect that portion that
					// will be dealt with here.
					un32Offset += pstCurrent->un32Bytes;
					un32NumberOfBytes -= pstCurrent->un32Bytes;

					// If this node has already been marked as viewed, we can just continue on.
					if (eCB_VALID_VIEWED != pstCurrent->eState)
					{
						// Can it merge with the node to the left?
						if (MergeCapable (pstCurrent, ePrev, eCB_VALID_VIEWED))
						{
							stiDEBUG_TOOL (g_stiMP4CircBufDebug,
								stiTrace ("<CVMCircularBuffer::ReleaseBuffer> Freeing node %p and merging with left node %p.\n",
								pstCurrent, pLeft);
							);
						
							// Update the Previous and Next nodes so that we can remove the current node.
							pLeft->un32Bytes += pstCurrent->un32Bytes;
							RemoveNodeFromCircularBufferLinkedList (pstCurrent);

							// Since we have merged with the node to the left, we can free this node.
							stiHEAP_FREE (pstCurrent);

							// Since we merged with the node to the left and have deleted the current, 
							// modify the current pointer to be the previous pointer so that the "for" 
							// loop can successfully continue.
							pstCurrent = pLeft;
						} // end if

						// Can it merge with the node to the right? (This applies whether the merge to the left
						// occurred or not so it shouldn't be treated as an "else if").
						if (MergeCapable (pstCurrent, eNext, eCB_VALID_VIEWED))
						{
							stiDEBUG_TOOL (g_stiMP4CircBufDebug,
								stiTrace ("<CVMCircularBuffer::ReleaseBuffer> Freeing node %p and merging with right node %p.\n",
								pstCurrent, pLeft);
							);

							// Update the current node and remove the next node.
							pstCurrent->un32Bytes += pRight->un32Bytes;
							RemoveNodeFromCircularBufferLinkedList (pRight);

							// Since we have merged the node to the right into this node, we can free 
							// the node to the right.
							stiHEAP_FREE (pRight);
						} // end else if
						else
						{
							// Since it can't be merged, update it to reflect that it has been viewed.
							stiDEBUG_TOOL (g_stiMP4CircBufDebug,
								stiTrace ("<CVMCircularBuffer::ReleaseBuffer> Node %p cannot merge ... changing state to "
								"'VALID_VIEWED'.\n", pstCurrent);
							);
						
							pstCurrent->eState = eCB_VALID_VIEWED;
						} // end else
					} // end if
					else
					{
						stiDEBUG_TOOL (g_stiMP4CircBufDebug,
							stiTrace ("<CVMCircularBuffer::ReleaseBuffer> Node %p, that contains the area being released, "
							"is already marked as 'VALID_VIEWED'\n", pstCurrent);
						);
						
					} // end else

					// Check to see if we are freeing a frame that is half at the end of the
					// buffer and half at the beginning.

					if (!bFound &&
						un32Offset != 0 &&
						un32NumberOfBytes != 0 &&
						pstCurrent->pNext->un32Offset != un32Offset)
					{
						pstCurrent = &m_stCircularBufferLinkedList;
					}
				} // end if

				// Determine if the area being freed is in the left, in the middle or in the right
				// of this node.
				else if (un32Offset == pstCurrent->un32Offset)
				{
					stiDEBUG_TOOL (g_stiMP4CircBufDebug,
						stiTrace ("<CVMCircularBuffer::ReleaseBuffer> The area to be freed is in the left of node %p\n", pstCurrent);
					);
					
					// If this node has already been marked as viewed, we can just continue on.
					if (eCB_VALID_VIEWED != pstCurrent->eState)
					{
						// The area is in the left of this node.
						// Can it be joined with the node to the left?
						if (MergeCapable (pstCurrent, ePrev, eCB_VALID_VIEWED))
						{
							stiDEBUG_TOOL (g_stiMP4CircBufDebug,
								stiTrace ("<CVMCircularBuffer::ReleaseBuffer> Joining with node %p to the left\n", pstCurrent->pPrev);
							);
							
							// Update the Previous to contain this area.
							pstCurrent->pPrev->un32Bytes += un32NumberOfBytes;

							// Modify the current buffer to point to the area after the freed data.
							pstCurrent->pBuffer += un32NumberOfBytes;
							pstCurrent->un32Bytes -= un32NumberOfBytes;
							pstCurrent->un32Offset += un32NumberOfBytes;
						} // end if
						else
						{
							// It cannot be joined.  This will require the creation of a new node to
							// represent the area in this node beyond what is being released.
							auto  pstNew = 
								(SCircularBufferNode*)stiHEAP_ALLOC (sizeof (SCircularBufferNode));
							if (nullptr == pstNew)
							{
								// Log an error
								hResult = stiRESULT_MEMORY_ALLOC_ERROR;
							} // end if
							else
							{
								stiDEBUG_TOOL (g_stiMP4CircBufDebug,
									stiTrace ("<CVMCircularBuffer::ReleaseBuffer> Cannot be joined with node to the left.\n");
									stiTrace ("\tCreated node %p to describe the unfreed area of this node (the right portion).\n", pstNew);
								);

								// Insert the new node into the circular buffer linked list
								InsertNodeIntoCircularBufferLinkedList (pstNew, pstCurrent->pNext);

								// Update the new node
								pstNew->pBuffer = pstCurrent->pBuffer + un32NumberOfBytes;
								pstNew->un32Bytes = pstCurrent->un32Bytes - un32NumberOfBytes;
								pstNew->un32Offset = pstCurrent->un32Offset + un32NumberOfBytes;
								pstNew->eState = pstCurrent->eState;

								// Now update the current node
								pstCurrent->un32Bytes = un32NumberOfBytes;
								pstCurrent->eState = eCB_VALID_VIEWED;
							} // end else
						} // end else
					} // end if
					else
					{
						stiDEBUG_TOOL (g_stiMP4CircBufDebug,
							stiTrace ("<CVMCircularBuffer::ReleaseBuffer> Node %p, that contains the area being released, "
							"is already marked as 'VALID_VIEWED'\n", pstCurrent);
						);
					} // end else
				} // end else if

				// Is it in the middle?
				else if (un32Offset + un32NumberOfBytes < pstCurrent->un32Offset + pstCurrent->un32Bytes)
				{
					stiDEBUG_TOOL (g_stiMP4CircBufDebug,
						stiTrace ("<CVMCircularBuffer::ReleaseBuffer> The area to be freed is in the middle of node %p\n", pstCurrent);
					);

					// If this node has already been marked as viewed, we can just continue on.
					if (eCB_VALID_VIEWED != pstCurrent->eState)
					{
						// It is in the middle.  We will need two new nodes; the first will represent the area
						// being released and the second will represent the area within the current node to the 
						// right of that being released.
						auto  pstMiddle = 
							(SCircularBufferNode*)stiHEAP_ALLOC (sizeof (SCircularBufferNode));
						if (nullptr == pstMiddle)
						{
							// Log an error
							hResult = stiRESULT_MEMORY_ALLOC_ERROR;
						} // end if
						else
						{
							auto  pstRight =
								(SCircularBufferNode*)stiHEAP_ALLOC (sizeof (SCircularBufferNode));
							if (nullptr == pstRight)
							{
								// Cleanup memory allocated for pstMiddle.
								stiHEAP_FREE(pstMiddle);

								// Log an error
								hResult = stiRESULT_MEMORY_ALLOC_ERROR;
							} // end if
							else
							{
								// Insert the new nodes into the circular buffer linked list
								InsertNodeIntoCircularBufferLinkedList (pstMiddle, pstCurrent->pNext);
								InsertNodeIntoCircularBufferLinkedList (pstRight, pstMiddle->pNext);

								// Update the middle node with the area to be released
								pstMiddle->pBuffer = pstCurrent->pBuffer + (un32Offset - pstCurrent->un32Offset);
								pstMiddle->un32Bytes = un32NumberOfBytes;
								pstMiddle->un32Offset = un32Offset;
								pstMiddle->eState = eCB_VALID_VIEWED;

								// Update the right node
								pstRight->pBuffer = pstMiddle->pBuffer + un32NumberOfBytes;
								pstRight->un32Bytes = pstCurrent->pBuffer + pstCurrent->un32Bytes - pstRight->pBuffer;
								pstRight->un32Offset = pstMiddle->un32Offset + un32NumberOfBytes;
								pstRight->eState = pstCurrent->eState;

								// Update the current node
								pstCurrent->un32Bytes = pstMiddle->pBuffer - pstCurrent->pBuffer;
	
								stiDEBUG_TOOL (g_stiMP4CircBufDebug,
									stiTrace ("<CVMCircularBuffer::ReleaseBuffer> The area to be freed is in a newly created "
									"node %p\n", pstMiddle);
									stiTrace ("\tThe area to the right of the freed data will "
									"be in another new node %p\n", pstRight);
									stiTrace ("\tStates:  Current [%s], Middle [%s], Right [%s]\n",
									StateGet (pstCurrent), StateGet (pstMiddle), StateGet (pstRight));
								);
							} // end else
						} // end else
					} // end if
					else
					{
						stiDEBUG_TOOL (g_stiMP4CircBufDebug,
							stiTrace ("<CVMCircularBuffer::ReleaseBuffer> Node %p, that contains the area being released, "
							"is already marked as 'VALID_VIEWED'\n", pstCurrent);
						);
					} // end else
				} // end else if

				// It must be to the right end of this node.
				else
				{
					stiDEBUG_TOOL (g_stiMP4CircBufDebug,
						stiTrace ("<CVMCircularBuffer::ReleaseBuffer> The area to be freed is in the right of node %p\n", pstCurrent);
					);

					// If this node has already been marked as viewed, we can just continue on.
					if (eCB_VALID_VIEWED != pstCurrent->eState)
					{
						// Can it be joined with the node to the right?
						if (MergeCapable (pstCurrent, eNext, eCB_VALID_VIEWED))
						{ 
							stiDEBUG_TOOL (g_stiMP4CircBufDebug,
								stiTrace ("<CVMCircularBuffer::ReleaseBuffer> Joining with node %p to the right\n", pstCurrent->pNext);
							);
							// Update the Next to contain this area.
							pstCurrent->pNext->un32Bytes += un32NumberOfBytes;
							pstCurrent->pNext->pBuffer -= un32NumberOfBytes;

							// Remove the current node from the linked list
							RemoveNodeFromCircularBufferLinkedList (pstCurrent);

							// Since we have merged with the node to the right, we can free this node.
							stiHEAP_FREE (pstCurrent);
							pstCurrent = nullptr;

							// Break out of the loop.
							break;
						} // end if
	
						// It cannot be joined.  This will require the creation of a new node to
						// represent the area in this node being released.
						auto  pstNew = 
							(SCircularBufferNode*)stiHEAP_ALLOC (sizeof (SCircularBufferNode));
						if (nullptr == pstNew)
						{
							// Log an error
							hResult = stiRESULT_MEMORY_ALLOC_ERROR;
						} // end if
						else
						{
							stiDEBUG_TOOL (g_stiMP4CircBufDebug,
								stiTrace ("<CVMCircularBuffer::ReleaseBuffer> Cannot be joined with node to the right.\n");
								stiTrace ("\tCreated node %p to describe the freed area.\n", pstNew);
							);

							// Insert the new node into the circular buffer linked list
							InsertNodeIntoCircularBufferLinkedList (pstNew, pstCurrent->pNext);

							// Update the new node
							pstNew->pBuffer = pstCurrent->pBuffer + pstCurrent->un32Bytes - un32NumberOfBytes;
							pstNew->un32Bytes = un32NumberOfBytes;
							pstNew->un32Offset = un32Offset;
							pstNew->eState = eCB_VALID_VIEWED;

							// Now update the current node
							pstCurrent->un32Bytes -= un32NumberOfBytes;
						} // end else
					} // end if
					else
					{
						stiDEBUG_TOOL (g_stiMP4CircBufDebug,
							stiTrace ("<CVMCircularBuffer::ReleaseBuffer> Node %p, that contains the area being released, "
							"is already marked as 'VALID_VIEWED'\n", pstCurrent);
						);
					} // end else
				} // end else
			} // end if
		} // end for
	} //end if

	CVM_UNLOCK ("ReleaseBuffer");

	return (hResult);
} // end CVMCircularBuffer::ReleaseBuffer


////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::ReleaseSkipBackBuffer
//
// Description: Release buffer space needed to skip back.
//
// Abstract:
//
// return: stiHResult
//
stiHResult CVMCircularBuffer::ReleaseSkipBackBuffer (
	uint32_t un32Offset,
	uint32_t un32NumberOfBytes,
	uint32_t *pun32NumberOfBytesReleased)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	*pun32NumberOfBytesReleased = 0;

	// Find the offset in the node list.
	SCircularBufferNode *pstCurrent = nullptr;
	EstiBool bFound = estiFALSE;
	SCircularBufferNode *pstSkipBackNode = nullptr;

	CVM_LOCK ("ReleaseSkipBackBuffer");

	for (pstCurrent = m_stCircularBufferLinkedList.pNext; 
		 pstCurrent != &m_stCircularBufferLinkedList && !bFound && !stiIS_ERROR(hResult);
		 pstCurrent = pstCurrent->pNext)
	{
		// Check to see if the offset is in the middle of the node.
		if (pstCurrent->un32Offset < un32Offset && 
			un32Offset < pstCurrent->un32Offset + pstCurrent->un32Bytes)
		{
			// Release the Front part of the node (it is part of what we need).
			hResult = CreateNewNode(pstCurrent,
									0, un32Offset - pstCurrent->un32Offset,
									eCB_INVALID);

			// We don't need to do anything with the node we just created because
			// CreateSkipBackBufferNode() will find it as the fist node used to 
			// create the needed node.
			if (pstCurrent->eState == eCB_VALID_NOT_VIEWED)
			{
				// Update the number of bytes that we released.
				*pun32NumberOfBytesReleased += pstCurrent->pPrev->un32Bytes;
			}
		}

		// If the node's buffer is the same as the offset we have found the end of
		// the buffer.
		if (pstCurrent->un32Offset == un32Offset)
		{
			hResult = CreateSkipBackBufferNode(&pstCurrent, 
											   un32NumberOfBytes,
											   &pstSkipBackNode, 
											   pun32NumberOfBytesReleased);
			bFound = estiTRUE;

			if (pstCurrent == nullptr)
			{
				break;
			}
		}
	}

	if (!stiIS_ERROR(hResult) &&
		pstSkipBackNode)
	{
		// Set the new node's offset to be the offset of the desired data.
		pstSkipBackNode->un32Offset = un32Offset - un32NumberOfBytes;
		memset (pstSkipBackNode->pBuffer, 0, un32NumberOfBytes);
	}

	CVM_UNLOCK ("ReleaseSkipBackBuffer");

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::InsertNodeIntoCircularBufferLinkedList
//
// Description: Insert a node into the linked list.
//
// Abstract:
//
// return: none
//
void CVMCircularBuffer::InsertNodeIntoCircularBufferLinkedList (
	IN SCircularBufferNode* pstNew, 	// The node to be inserted
	IN SCircularBufferNode* pstCurrent)	// The node to be inserted before
{
	pstNew->pPrev = pstCurrent->pPrev;
	pstNew->pNext = pstCurrent;
	pstCurrent->pPrev->pNext = pstNew;
	pstCurrent->pPrev = pstNew;

	stiDEBUG_TOOL (g_stiMP4CircBufDebug,
		stiTrace ("<CVMCircularBuffer::InsertNodeIntoCircularBuffer> Inserted node %p\n", pstNew);
	);
} // end CVMCircularBuffer::InsertNodeIntoCircularBufferLinkedList


////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::RemoveNodeFromCircularBufferLinkedList
//
// Description: Remove a node from the linked list.
//
// Abstract:
//
// return: none
//
void CVMCircularBuffer::RemoveNodeFromCircularBufferLinkedList (
	IN SCircularBufferNode* pstCurrent) 	// The node to be removed
{
	pstCurrent->pPrev->pNext = pstCurrent->pNext;
	pstCurrent->pNext->pPrev = pstCurrent->pPrev;

	stiDEBUG_TOOL (g_stiMP4CircBufDebug,
		stiTrace ("<CVMCircularBuffer::RemoveNodeFromCircularBuffer> Removed node %p\n", pstCurrent);
	);
} // end CVMCircularBuffer::RemoveNodeFromCircularBufferLinkedList


////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::InsertIntoCircularBufferLinkedList
//
// Description: Insert newly received data into the linked list.
//
// Abstract:
//	The CircularBuffer linked list was designed as follows:
// 		Any time a buffer is used in the EventFileGet function, that buffer
//		will be described in whole by a single node in the linked list.  As 
//		such, when this function is called, pNewBuffer will be the same address 
//		as pBuffer in one of the nodes.  It may or may not consume the whole 
//		node.  The exception being if ReleaseBuffer is called on a node just prior
//		to this node in the linked list could cause a merge of the two nodes and
//		therefore, pNewBuffer would fall into the middle of the joined nodes.
//
// Returns: stiHResult
//
//
stiHResult CVMCircularBuffer::InsertIntoCircularBufferLinkedList (
	uint8_t* pNewBuffer, 
	uint32_t un32Bytes,
	EstiBool bSkipBackData)
{
	stiLOG_ENTRY_NAME (CVMCircularBuffer::InsertIntoCircularBufferLinkedList);
	
	stiDEBUG_TOOL (g_stiMP4CircBufDebug,
	   stiTrace("<CVMCircularBuffer::InsertIntoCircularBufferLinkedList> with %d bytes\n", un32Bytes);
	);

	stiHResult hResult = stiRESULT_SUCCESS;
	static EstiBool bPrintList = estiFALSE;

	SCircularBufferNode* pCurrent = nullptr;
	EstiBool bFound = estiFALSE;

	CVM_LOCK ("InsertIntoCircularBufferLinkedList");

	for (pCurrent = m_stCircularBufferLinkedList.pNext; 
		 pCurrent != &m_stCircularBufferLinkedList && !bFound; 
		 pCurrent = pCurrent->pNext)
	{
		// Is this buffer described in the current node?
		if (pNewBuffer >= pCurrent->pBuffer && pNewBuffer < pCurrent->pBuffer + pCurrent->un32Bytes)
		{
			bFound = estiTRUE;

			// It is.  Does it match the size of the new buffer?
			if (pCurrent->un32Bytes != un32Bytes)
			{
				// Since it doesn't consume the whole node, we need to create a new node and update the current one
				auto  pstNew = (SCircularBufferNode*) stiHEAP_ALLOC (sizeof (SCircularBufferNode));

				if (nullptr != pstNew)
				{
					// Is the new data at the beginning, the middle or the end of the current node?
					if (pNewBuffer == pCurrent->pBuffer)
					{
						// Temporarily store the current nodes offset to be able to restore it (if needed).
						// This is done so that when the MergeCapable function is called, the offset reflects
						// the data being stored (not that that formerly was in the node).
						uint32_t un32TempOffset = pCurrent->un32Offset;
						if (!bSkipBackData)
						{
							pCurrent->un32Offset = m_un32CurrentFileOffset;
						}

						// If the previous node is in the eCB_VALID_NOT_VIEWED state, we can simply 
						// merge the new data into that node and update the current node.
						if (!bSkipBackData &&
							MergeCapable (pCurrent, ePrev, eCB_VALID_NOT_VIEWED))
						{
							// We won't need the newly created node.  Return it.
							stiHEAP_FREE (pstNew);
							pstNew = nullptr;

							// Modify the previous node to include the new data area.
							pCurrent->pPrev->un32Bytes += un32Bytes;

							// Modify the current node to NOT include the new data area.
							pCurrent->un32Bytes -= un32Bytes;
							pCurrent->pBuffer += un32Bytes;
							
							// Restore the temporary offset
							pCurrent->un32Offset = un32TempOffset;
						} // end if
						else
						{
							// It is at the beginning.  The current node will be updated to reflect the 
							// incoming data.
							// Insert the new node into the circular buffer linked list
							InsertNodeIntoCircularBufferLinkedList (pstNew, pCurrent->pNext);

							pstNew->pBuffer = pCurrent->pBuffer + un32Bytes;
							pstNew->un32Bytes = pCurrent->un32Bytes - un32Bytes;
							pstNew->un32Offset = bSkipBackData ? pCurrent->un32Offset + un32Bytes : 0;
							pstNew->eState = eCB_INVALID;
					
							// Update the current node
							pCurrent->un32Bytes = un32Bytes;
							pCurrent->eState = eCB_VALID_NOT_VIEWED;
						} // end else
					} // end if
					else if (pNewBuffer + un32Bytes < pCurrent->pBuffer + pCurrent->un32Bytes)
					{
						// It is in the middle.  We need an additional new node.  The first new node
						// will reflect the incoming data and the second will reflect the area in the 
						// current node AFTER the buffer described by the first new node.
						auto  pst2ndNew = 
							(SCircularBufferNode*) stiHEAP_ALLOC (sizeof (SCircularBufferNode));
						if (nullptr != pst2ndNew)
						{
							// How many bytes are to be in each node?
							uint32_t un32LeftNodeBytes = pNewBuffer - pCurrent->pBuffer;
							uint32_t un32RightNodeBytes = pCurrent->un32Bytes - un32LeftNodeBytes - un32Bytes;

							// Insert the new node into the circular buffer linked list
							InsertNodeIntoCircularBufferLinkedList (pstNew, pCurrent->pNext);
							InsertNodeIntoCircularBufferLinkedList (pst2ndNew, pstNew->pNext);

							// Update the first new node
							pstNew->pBuffer = pNewBuffer;
							pstNew->un32Bytes = un32Bytes;
							pstNew->un32Offset = m_un32CurrentFileOffset;
							pstNew->eState = eCB_VALID_NOT_VIEWED;
					
							// Update the second new node
							pst2ndNew->pBuffer = pstNew->pBuffer + pstNew->un32Bytes;
							pst2ndNew->un32Bytes = un32RightNodeBytes;
							pst2ndNew->un32Offset = 0;
							pst2ndNew->eState = eCB_INVALID;
							
							// Update the current node
							pCurrent->un32Bytes = un32LeftNodeBytes;

						} // end if
						else
						{
							// We had an error so free up the new node that was allocated.
							stiHEAP_FREE(pstNew);
							pstNew = nullptr;
										 
							// ACTION SCI ERC - Log an error???
							stiTHROW(stiRESULT_ERROR);
						} // end else
					} // end else if
					else
					{
						// It is at the end.  The new node will reflect the incoming data.
						// Update the new node
						// Insert into the circular buffer linked list
						InsertNodeIntoCircularBufferLinkedList (pstNew, pCurrent->pNext);
						pstNew->pBuffer = pNewBuffer;
						pstNew->un32Bytes = un32Bytes;
						pstNew->un32Offset = m_un32CurrentFileOffset;
						pstNew->eState = eCB_VALID_NOT_VIEWED;
						
						// Update the current node
						pCurrent->un32Bytes -= un32Bytes;
					} // end else
				} // end if
				else
				{
					// ACTION SCI ERC - Log an error???
					stiTHROW(stiRESULT_ERROR);
				} // end else
			} // end if
			else
			{
				// It consumes the full current node.  

				// Temporarily store the current nodes offset to be able to restore it (if needed).
				// This is done so that when the MergeCapable function is called, the offset reflects
				// the data being stored (not that that formerly was in the node).
#if 0 // Unused variable?
				uint32_t un32TempOffset = pCurrent->un32Offset;
#endif
				if (!bSkipBackData)
				{
					pCurrent->un32Offset = m_un32CurrentFileOffset;
				}

				// Can it merge with the node to the left?
				if (!bSkipBackData &&
					MergeCapable (pCurrent, ePrev, eCB_VALID_NOT_VIEWED))
				{
					// Merging with node to the left
					pCurrent->pPrev->un32Bytes += un32Bytes;

					// Remove the current node
					SCircularBufferNode* pNodeToDelete = pCurrent;
					RemoveNodeFromCircularBufferLinkedList (pNodeToDelete);
					pCurrent = pCurrent->pPrev;
					stiHEAP_FREE (pNodeToDelete);
					pNodeToDelete = nullptr;
				} // end if
				
				// Since it can't merge left, update the current node's state and offset
				else
				{
					pCurrent->eState = eCB_VALID_NOT_VIEWED;
				} // end else
			} // end else

			// Update the current file offset for the next data to be read
			if (!bSkipBackData)
			{
				m_un32CurrentFileOffset += un32Bytes;
			}
			
			// If we were successful at updating the linked list, update the 
			// EndOfWindowOffset
			if (!bSkipBackData &&
				!stiIS_ERROR(hResult))
			{
				m_un32EndOfWindowOffset = m_un32CurrentFileOffset;

				// Increment the current write position pointer.  Don't pass the Reserved Space pointer.
				if (pNewBuffer + un32Bytes < m_pReservedSpace)
				{
					m_pCurrentWritePosition = pNewBuffer + un32Bytes; 
				} // end if
				else
				{
					// We have filled the buffer and need to start from the beginning.
					m_pCurrentWritePosition = m_pCircularBuffer;
					bPrintList = estiTRUE;
					stiDEBUG_TOOL (g_stiMP4CircBufDebug,
						stiTrace("<CVMCircularBuffer::InsertIntoCircularBufferLinkedList> Resetting "
						"CurrentWritePosition to beginning of buffer.\n");
					);
				} // end else
			} // end if
		} // end if
	} // end for


#if 1
	stiDEBUG_TOOL (g_stiMP4CircBufDebug,
		if (bPrintList)
		{
			static int nNbr = 0;
			SCircularBufferNode* pstCurrent;
			stiTrace ("<CVMCircularBuffer::InsertIntoCircularBufferLinkedList> Exiting.  Linked list is as follows:\n");
			for (pstCurrent = m_stCircularBufferLinkedList.pNext;
				 pstCurrent != &m_stCircularBufferLinkedList;
				 pstCurrent = pstCurrent->pNext)
			{
				stiTrace ("\t<Node %p> offset = %d, size = %d, state = %s\n", 
					pstCurrent, pstCurrent->un32Offset, pstCurrent->un32Bytes, StateGet (pstCurrent));
			} // end for
			if (++nNbr >= 5)
			{
				nNbr = 0;
				bPrintList = estiFALSE;
			} // end if
		} // end if
	);
#endif

STI_BAIL:	

	CVM_UNLOCK ("InsertIntoCircularBufferLinkedList");

	return (hResult);
} // end CVMCircularBuffer::InsertIntoCircularBufferLinkedList


////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::StateGet
//
// Description: 
//
// Abstract:
//
// Returns: A string representing the current state of this node.
//
//
const char* CVMCircularBuffer::StateGet (
	SCircularBufferNode* pstNode)
{
	const char* pszState = "Unknown";
	if (nullptr != pstNode)
	{
		switch (pstNode->eState)
		{
			case eCB_INVALID:
				pszState = "Invalid";
				break;
				
			case eCB_VALID_NOT_VIEWED:
				pszState = "Valid Not Viewed";
				break;
				
			case eCB_VALID_VIEWED:
				pszState = "Valid Viewed";
				break;

			default:
				break;
		} // end switch
	} // end if

	return (pszState);
} // end StateGet

////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::Lock
//
// Description: Locks the class protection mutex
//
// Abstract:
//
// Returns:
//	None
//
void CVMCircularBuffer::Lock() const
{
	stiOSMutexLock (m_muxBufferAccess);
}

////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::Unlock
//
// Description: Locks the class protection mutex
//
// Abstract:
//
// Returns:
//	None
//
void CVMCircularBuffer::Unlock() const
{
	stiOSMutexUnlock (m_muxBufferAccess);
}

////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::MaxBufferingSizeGet
//
// Description: Get maximum size for buffering.
//
// Abstract:
//
// Returns: 
//	uint32_t - maximum size for buffering
//
//
uint32_t CVMCircularBuffer::MaxBufferingSizeGet ()
{
	return (un32MAX_CIRCULAR_BUFFER_SIZE - un32MAX_BUFFER_SIZE);
}


////////////////////////////////////////////////////////////////////////////////
//; CVMCircularBuffer::MaxFileOffsetSet
//
// Description: Sets the maximum offset to be requested from the circular buffer.
//
// Abstract: This set is relative to the actual file size downloaded (or the sizeof
// reported to be downloaded.
//
// Returns: 
//	uint32_t - maximum offset to be requested.
//
//
void CVMCircularBuffer::MaxFileOffsetSet (
	uint64_t un64Offset)
{
	CVM_LOCK ("MaxFileOffsetSet");

	m_un64MaxFileOffset = un64Offset;

	CVM_UNLOCK ("MaxFileOffsetSet");
}

