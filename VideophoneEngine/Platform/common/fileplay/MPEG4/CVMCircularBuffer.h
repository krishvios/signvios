////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CVMCircularBuffer
//
//  File Name:	CVMCircularBuffer.h
//
//  Owner:
//
//	Abstract: This class contains the functions for video message circular buffer
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CVMCIRCULARBUFFER_H
#define CVMCIRCULARBUFFER_H

//
// Includes
//
#include "stiOS.h"
#include "stiDefs.h"
#include "stiMem.h"
#include "SorensonMP4.h"

//
// Constants
//
const uint32_t un32REUSE_BUFFER_SIZE = 2048;  // When reusing the buffer, how big of chunks do we wish to limit it to.

//
// Typedefs
//


//
// Forward Declarations
//

//
// Globals
//

class CVMCircularBuffer
{
public:
	
	enum EstiBufferResult
	{
		estiBUFFER_OK,
		estiBUFFER_ERROR,	// possibly due to requesting a buffer larger than nMAX_BUFFER_SIZE if the data is wrapped 
							// from the end of the circular buffer back to the beginning.
		estiBUFFER_OFFSET_OVERWRITTEN,
		estiBUFFER_OFFSET_NOT_YET_AVAILABLE,
		estiBUFFER_OFFSET_PAST_EOF,
	} ;

	CVMCircularBuffer ();
	
	CVMCircularBuffer (const CVMCircularBuffer &other) = delete;
	CVMCircularBuffer (CVMCircularBuffer &&other) = delete;
	CVMCircularBuffer &operator= (const CVMCircularBuffer &other) = delete;
	CVMCircularBuffer &operator= (CVMCircularBuffer &&other) = delete;

	virtual ~ CVMCircularBuffer ();

	EstiBool IsDataInBuffer (
		uint32_t un32Offset, 
		uint32_t un32NumberOfBytes);

	EstiBufferResult GetBuffer (
		uint32_t un32Offset, 
		uint32_t un32NumberOfBytes,
		uint8_t** ppBuffer); 
 
	EstiBool IsBufferEmpty () const;
	   
	EstiBool IsBufferSpaceAvailable (
		uint32_t un32BytesNeeded,
		uint32_t *pun32BytesFree = nullptr);

	stiHResult ReleaseBuffer (
		uint32_t un32Offset, 
		uint32_t un32NumberOfBytes);

	stiHResult ReleaseSkipBackBuffer (
		uint32_t un32Offset,
		uint32_t un32NumberOfBytes,
		uint32_t *pun32NumberOfBytesReleased);

	stiHResult InitCircularBuffer ();

	void CleanupCircularBuffer (
		EstiBool bFreeBuffer = estiFALSE,
		uint32_t un32Offset = 0);

	void SetNewOffset(uint32_t un32Offset)
	{
		m_un32BeginningOfWindowOffset = un32Offset;
		m_un32CurrentFileOffset = un32Offset;
	}
	uint32_t GetCurrentWriteOffest () const
	{
		return m_un32EndOfWindowOffset + 1;
	}
	uint32_t GetCurrentWritePosition (
		uint8_t** ppWriteBuffer);

	uint32_t GetDataSizeInBuffer(
		uint32_t un32Offset,
		uint32_t *pTotalDataSize = nullptr) const;

	uint32_t GetSkipBackWritePosition(
		uint32_t un32Offset,
		uint32_t un32NumberOfBytes,
		uint8_t **ppWriteBuffer);

	stiHResult InsertIntoCircularBufferLinkedList (
		uint8_t* pNewBuffer, 
		uint32_t nBytes,
		EstiBool bSkipBackData = estiFALSE);

	static uint32_t MaxBufferingSizeGet ();
	
	void PrintNodeList(
		FILE *pFile = nullptr);
	void MaxFileOffsetSet (
		uint64_t un64Offset);

	stiHResult ResetBufferToValidNotViewed (
		uint32_t un32Offset);

	stiHResult AdjustOffsets(
		uint32_t un32OffsetAdjustment);

	stiHResult ConfigureBufferForUpload();

	stiHResult CreatePrefixBuffer(
		uint32_t un32EndingOffset,
		uint32_t un32PrefixSize);

private:
	
	enum ECircularBufferNodeState
	{
		eCB_INVALID,
		eCB_VALID_NOT_VIEWED,
		eCB_VALID_VIEWED,
	};

	struct SCircularBufferNode
	{
		SCircularBufferNode* pPrev; // Previous node in the list
		SCircularBufferNode* pNext; // Next node in the list
		uint8_t* pBuffer;			// Pointer to the beginning address in the buffer that this node describes
		uint32_t un32Bytes;		// The size (in bytes) that this node describes
		uint32_t un32Offset;		// The offset relative to the beginning of the file
		ECircularBufferNodeState eState;	// eCB_INVALID, eCB_VALID_NOT_VIEWED, eCB_VALID_VIEWED
	};

	enum ENodeDirection
	{
		ePrev,
		eNext,
	};

	void Lock() const;
	void Unlock() const;

	stiHResult CreateNewNode(
		SCircularBufferNode *pstNodeToDivide,	  /// Node to be divided.
		uint32_t un32BeginOffset,		  /// Offset from the begining of the node.
		uint32_t un32NumberOfBytes,		   /// Number of bytes the new node should contain.
		ECircularBufferNodeState newNodeState);	  /// State of the new node.

	stiHResult CreateSkipBackBufferNode(
		SCircularBufferNode **pstStartNode,
		uint32_t un32NumberOfBytes,
		SCircularBufferNode **ppstSkipBackNode,
		uint32_t *pun32NumberOfBytesReleased);

	static const char* StateGet (
		SCircularBufferNode* pstNode);

	static void InsertNodeIntoCircularBufferLinkedList (
		SCircularBufferNode* pstNew, 
		SCircularBufferNode* pstCurrent);

	static void RemoveNodeFromCircularBufferLinkedList (
		SCircularBufferNode* pstCurrent);

	static EstiBool MergeCapable (
		SCircularBufferNode* pstNode,
		ENodeDirection eNode,
		ECircularBufferNodeState eState); 

	uint8_t* m_pCircularBuffer{nullptr};
	uint32_t m_un32BeginningOfWindowOffset{0};
	uint32_t m_un32EndOfWindowOffset{0};
	uint8_t* m_pCurrentWritePosition{nullptr};
	uint8_t* m_pReservedSpace{nullptr};
	SCircularBufferNode m_stCircularBufferLinkedList{};
	uint32_t m_un32CurrentFileOffset{0};
	uint64_t m_un64MaxFileOffset{0};
	mutable stiMUTEX_ID m_muxBufferAccess{nullptr};
};
#endif // CVMCIRCULARBUFFER_H
