///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	TkhdAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////

#include <cstring>

//
// Sorenson includes.
//
#include "SMCommon.h"

#include "TkhdAtom.h"
#include "MdiaAtom.h"
#include "HdlrAtom.h"
#include "MP4Common.h"
#include "MP4Parser.h"


///////////////////////////////////////////////////////////////////////////////
//; TkhdAtom_c::TkhdAtom_c
//
// Abstract:
//
// Returns:
//
TkhdAtom_c::TkhdAtom_c()
{
	m_unType = eAtom4CCtkhd;

	MP4GetCurrentTime(&m_un64CreationTime);
	
	m_un64ModificationTime = m_un64CreationTime;

	m_unTrackID = 0;

	m_un64Duration = 0;

	memset(m_unReserved1, 0, sizeof(m_unReserved1));

	m_sLayer = 0;;
	m_sAlternateGroup = 0;
	m_sVolume = 0;

	m_usReserved3 = 0;

	m_anMatrix[0] = 0x00010000;
	m_anMatrix[1] = 0;
	m_anMatrix[2] = 0;
	m_anMatrix[3] = 0;
	m_anMatrix[4] = 0x00010000;
	m_anMatrix[5] = 0;
	m_anMatrix[6] = 0;
	m_anMatrix[7] = 0;
	m_anMatrix[8] = 0x40000000;

	m_unWidth = 0;
	m_unHeight = 0;

	m_unFlags = 1; // BUGBUG - Should be set through enable track.
}


///////////////////////////////////////////////////////////////////////////////
//; TkhdAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t TkhdAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = ReadHeader(pDataHandler, &un64Offset, &un64Length);

	TESTRESULT();

	switch (m_unVersion)
	{
		case 0:
		
			ASSERTRT(5 * sizeof(uint32_t) <= un64Length, SMRES_INVALID_MOVIE);

			m_un64CreationTime = GetInt32(pDataHandler, un64Offset);
			m_un64ModificationTime = GetInt32(pDataHandler, un64Offset + sizeof(uint32_t));
			m_unTrackID = GetInt32(pDataHandler, un64Offset + 2 * sizeof(uint32_t));

			// 32-bit reserved

			m_un64Duration = GetInt32(pDataHandler, un64Offset + 4 * sizeof(uint32_t));

			un64Offset += 5 * sizeof(uint32_t);
			un64Length -= 5 * sizeof(uint32_t);

			break;
			
		case 1:
		
			ASSERTRT(2 * sizeof(uint32_t) + 3 * sizeof(uint64_t) <= un64Length, SMRES_INVALID_MOVIE);

			m_un64CreationTime = GetInt64(pDataHandler, un64Offset);
			m_un64ModificationTime = GetInt64(pDataHandler, un64Offset + sizeof(uint64_t));
			m_unTrackID = GetInt32(pDataHandler, un64Offset + 2 * sizeof(uint64_t));
			
			// 32-bit reserved

			m_un64Duration = GetInt64(pDataHandler, un64Offset + sizeof(uint32_t) + 2 * sizeof(uint64_t));

			un64Offset += 2 * sizeof(uint32_t) + 3 * sizeof(uint64_t);
			un64Length -= 2 * sizeof(uint32_t) + 3 * sizeof(uint64_t);

			break;
			
		default:
		
			ASSERTRT(eFalse, SMRES_INVALID_MOVIE);
	}	
	
	ASSERTRT(14 * sizeof(uint32_t) + 2 * sizeof(uint16_t) == un64Length, SMRES_INVALID_MOVIE);

	m_unReserved1[0] = GetInt32(pDataHandler, un64Offset);
	m_unReserved1[1] = GetInt32(pDataHandler, un64Offset + sizeof(uint32_t));

	m_sLayer = GetInt16(pDataHandler, un64Offset + 2 * sizeof(uint32_t));
	m_sAlternateGroup = GetInt16(pDataHandler, un64Offset + 2 * sizeof(uint32_t) + sizeof(int16_t));
	m_sVolume = GetInt16(pDataHandler, un64Offset + 2 * sizeof(uint32_t) + 2 * sizeof(int16_t));

	m_usReserved3 = GetInt16(pDataHandler, un64Offset + 2 * sizeof(uint32_t) + 3 * sizeof(uint16_t));

	m_anMatrix[0] = GetInt32(pDataHandler, un64Offset + 2 * sizeof(uint32_t) + 4 * sizeof(uint16_t));
	m_anMatrix[1] = GetInt32(pDataHandler, un64Offset + 3 * sizeof(uint32_t) + 4 * sizeof(uint16_t));
	m_anMatrix[2] = GetInt32(pDataHandler, un64Offset + 4 * sizeof(uint32_t) + 4 * sizeof(uint16_t));
	m_anMatrix[3] = GetInt32(pDataHandler, un64Offset + 5 * sizeof(uint32_t) + 4 * sizeof(uint16_t));
	m_anMatrix[4] = GetInt32(pDataHandler, un64Offset + 6 * sizeof(uint32_t) + 4 * sizeof(uint16_t));
	m_anMatrix[5] = GetInt32(pDataHandler, un64Offset + 7 * sizeof(uint32_t) + 4 * sizeof(uint16_t));
	m_anMatrix[6] = GetInt32(pDataHandler, un64Offset + 8 * sizeof(uint32_t) + 4 * sizeof(uint16_t));
	m_anMatrix[7] = GetInt32(pDataHandler, un64Offset + 9 * sizeof(uint32_t) + 4 * sizeof(uint16_t));
	m_anMatrix[8] = GetInt32(pDataHandler, un64Offset + 10 * sizeof(uint32_t) + 4 * sizeof(uint16_t));

	m_unWidth = GetInt32(pDataHandler, un64Offset + 11 * sizeof(uint32_t) + 4 * sizeof(uint16_t));
	m_unHeight = GetInt32(pDataHandler, un64Offset + 12 * sizeof(uint32_t) + 4 * sizeof(uint16_t));

	if ((m_unWidth & 0xFFFF) == 0)
	{
		m_unWidth >>= 16;
	}
	
	if ((m_unHeight & 0xFFFF) == 0)
	{
		m_unHeight >>= 16;
	}
	
smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; TkhdAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t TkhdAtom_c::GetSize()
{
	uint32_t unSize = 0;

	switch (m_unVersion)
	{
		case 0:

			unSize += 5 * sizeof(uint32_t);

			break;

		case 1:

			unSize += 2 * sizeof(uint32_t) + 3 * sizeof(uint64_t);

			break;
	}

	unSize += 14 * sizeof(uint32_t) + 2 * sizeof(uint16_t);

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; TkhdAtom_c::GetTrackID
//
// Abstract:
//
// Returns:
//
uint32_t TkhdAtom_c::GetTrackID() const
{
	return m_unTrackID;
}


///////////////////////////////////////////////////////////////////////////////
//; TkhdAtom_c::SetTrackID
//
// Abstract:
//
// Returns:
//
void TkhdAtom_c::SetTrackID(
	uint32_t unTrackID)
{
	m_unTrackID = unTrackID;
}


///////////////////////////////////////////////////////////////////////////////
//; TkhdAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t TkhdAtom_c::Write(
	CDataHandler* pDataHandler)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint32_t unSize;
	uint32_t unZero = 0;
	uint64_t un64StartPosition;
	uint64_t un64EndPosition;
	int i;
	TrakAtom_c *pTrakAtom;
	MdiaAtom_c *pMdiaAtom;
	HdlrAtom_c *pHdlrAtom;
	uint32_t unHandlerType;

	ASSERTRT(pDataHandler != nullptr, SMRES_INVALID_DATA_HANDLER);

	unSize = GetSize();

	//
	// Determine the track type.
	//
	pTrakAtom = (TrakAtom_c *)GetParent();

	ASSERTRT(pTrakAtom != nullptr, SMRES_INVALID_MOVIE);

	pMdiaAtom = (MdiaAtom_c *)pTrakAtom->GetChildAtom(eAtom4CCmdia);

	ASSERTRT(pMdiaAtom != nullptr, SMRES_INVALID_MOVIE);

	pHdlrAtom = (HdlrAtom_c *)pMdiaAtom->GetChildAtom(eAtom4CChdlr);

	ASSERTRT(pHdlrAtom != nullptr, SMRES_INVALID_MOVIE);

	SMResult = pHdlrAtom->GetHandlerType(&unHandlerType);

	TESTRESULT();

	//
	// Get the current position in the file.
	//
	SMResult = pDataHandler->DHGetPosition(&un64StartPosition);

	TESTRESULT();

	//
	// Write the atom header.
	//
	SMResult = WriteHeader(pDataHandler, unSize);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite4((uint32_t)m_un64CreationTime);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite4((uint32_t)m_un64ModificationTime);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite4(m_unTrackID);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite4(unZero);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite4((uint32_t)m_un64Duration);

	TESTRESULT();

	for (i = 0; i < 2; i++)
	{
		SMResult = pDataHandler->DHWrite4(m_unReserved1[i]);

		TESTRESULT();
	}

	SMResult = pDataHandler->DHWrite2(m_sLayer);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite2(m_sAlternateGroup);

	TESTRESULT();

	if (unHandlerType == eMediaTypeAudio)
	{
		SMResult = pDataHandler->DHWrite2(m_sVolume);

		TESTRESULT();
	}
	else
	{
		SMResult = pDataHandler->DHWrite2(0);

		TESTRESULT();
	}

	SMResult = pDataHandler->DHWrite2(m_usReserved3);

	TESTRESULT();

	for (i = 0; i < 9; i++)
	{
		SMResult = pDataHandler->DHWrite4(m_anMatrix[i]);

		TESTRESULT();
	}

	if (unHandlerType == eMediaTypeVisual)
	{
		SMResult = pDataHandler->DHWrite4(m_unWidth << 16);

		TESTRESULT();

		SMResult = pDataHandler->DHWrite4(m_unHeight << 16);

		TESTRESULT();
	}
	else
	{
		SMResult = pDataHandler->DHWrite4(0);

		TESTRESULT();

		SMResult = pDataHandler->DHWrite4(0);

		TESTRESULT();
	}

	//
	// Get end position.
	//
	SMResult = pDataHandler->DHGetPosition(&un64EndPosition);

	TESTRESULT();

	ASSERTRT(unSize == un64EndPosition - un64StartPosition, SMRES_WRITE_ERROR);

smbail:

	return SMResult;
}


