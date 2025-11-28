///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	UdtaAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////

#include "SMCommon.h"
#include "SorensonErrors.h"

#include "MP4Common.h"
#include "UdtaAtom.h"


///////////////////////////////////////////////////////////////////////////////
//; UdtaAtom_c::UdtaAtom_c
//
// Abstract:
//
// Returns:
//
UdtaAtom_c::UdtaAtom_c()
{
	m_unType = eAtom4CCudta;

	m_UserDataList.pNext = &m_UserDataList;
	m_UserDataList.pPrev = &m_UserDataList;
} // UdtaAtom_c::UdtaAtom_c


///////////////////////////////////////////////////////////////////////////////
//; UdtaAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t UdtaAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	UserData_t *pUserData = nullptr;
	uint32_t unSize;

	for (;;)
	{
		if (un64Length == 0)	
		{
			break;	
		}

		unSize = GetInt32(pDataHandler, un64Offset);

		ASSERTRT(unSize <= un64Length, SMRES_INVALID_MOVIE);

		if (unSize == 0)
		{
			break;
		}

		pUserData = (UserData_t *)SMAllocPtr(sizeof(UserData_t));
		
		ASSERTRT(pUserData != nullptr, SMRES_ALLOC_FAILED);
		
		pUserData->unType = GetInt32(pDataHandler, un64Offset + sizeof(uint32_t));
		
		pUserData->unLength = unSize - 2 * sizeof(uint32_t);
		
		pUserData->pucData = (uint8_t *)GetDataPtr(pDataHandler, un64Offset + 2 * sizeof(uint32_t), pUserData->unLength);
		
		ASSERTRT(pUserData->pucData != nullptr, SMRES_ALLOC_FAILED);
		
		pUserData->pNext = &m_UserDataList;
		pUserData->pPrev = m_UserDataList.pPrev;
		
		m_UserDataList.pPrev->pNext = pUserData;
		m_UserDataList.pPrev = pUserData;

		pUserData = nullptr;
		
		un64Offset += unSize;
		un64Length -= unSize;

		if (un64Length == 0)
		{
			break;
		}
	}

smbail:

	if (pUserData)
	{
		if (pUserData->pucData)
		{
			SMFreePtr(pUserData->pucData);

			pUserData->pucData = nullptr;
		}

		SMFreePtr(pUserData);

		pUserData = nullptr;
	}
	
	return SMResult;
} // UdtaAtom_c::Parse


///////////////////////////////////////////////////////////////////////////////
//; UdtaAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_t UdtaAtom_c::GetSize()
{
	uint32_t unSize = 0;
	UserData_t *pUserData;

	for (pUserData = m_UserDataList.pNext; pUserData != &m_UserDataList; pUserData = pUserData->pNext)
	{
		unSize += 2 * sizeof(uint32_t);

		unSize += pUserData->unLength;
	}

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; UdtaAtom_c::FreeAtom
//
// Abstract:
//
// Returns:
//
void UdtaAtom_c::FreeAtom()
{
	UserData_t *pUserData;
	UserData_t *pNextUserData;

	for (pUserData = m_UserDataList.pNext; pUserData != &m_UserDataList; pUserData = pNextUserData)
	{
		pNextUserData = pUserData->pNext;

		pUserData->pNext->pPrev = pUserData->pPrev;
		pUserData->pPrev->pNext = pUserData->pNext;

		SMFreePtr(pUserData->pucData);

		SMFreePtr(pUserData);
	}

	Atom_c::FreeAtom();
} // UdtaAtom_c::FreeAtom


///////////////////////////////////////////////////////////////////////////////
//; UdtaAtom_c::AddUserData
//
// Abstract:
//
// Returns:
//
SMResult_t UdtaAtom_c::AddUserData(
	uint32_t unType,
	const uint8_t *pucData,
	uint32_t unLength)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	UserData_t *pUserData;

	pUserData = (UserData_t *)SMAllocPtr(sizeof(UserData_t));

	ASSERTRT(pUserData != nullptr, SMRES_ALLOC_FAILED);

	pUserData->unType = unType;

	pUserData->pucData = (uint8_t *)SMAllocPtr(unLength);

	ASSERTRT(pUserData->pucData != nullptr, SMRES_ALLOC_FAILED);

	memcpy(pUserData->pucData, pucData, unLength);

	pUserData->unLength = unLength;

	pUserData->pNext = &m_UserDataList;
	pUserData->pPrev = m_UserDataList.pPrev;

	m_UserDataList.pPrev->pNext = pUserData;
	m_UserDataList.pPrev = pUserData;

smbail:
	if (SMResult != SMRES_SUCCESS &&
		pUserData)
	{
		SMFreePtr(pUserData);
		pUserData = nullptr;
	}

	return SMResult;
} // UdtaAtom_c::AddUserData


///////////////////////////////////////////////////////////////////////////////
//; UdtaAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t UdtaAtom_c::Write(
	CDataHandler* pDataHandler)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint32_t unSize;
	uint64_t un64StartPosition;
	uint64_t un64EndPosition;
	UserData_t *pUserData;

	ASSERTRT(pDataHandler != nullptr, SMRES_INVALID_DATA_HANDLER);

	unSize = GetSize();

	SMResult = pDataHandler->DHGetPosition(&un64StartPosition);

	TESTRESULT();

	//
	// Write the atom header.
	//
	SMResult = WriteHeader(pDataHandler, unSize);

	TESTRESULT();

	for (pUserData = m_UserDataList.pNext; pUserData != &m_UserDataList; pUserData = pUserData->pNext)
	{
		SMResult = pDataHandler->DHWrite4(pUserData->unLength + 2 * sizeof(uint32_t));

		SMResult = pDataHandler->DHWrite4(pUserData->unType);

		TESTRESULT();

		SMResult = pDataHandler->DHWrite(pUserData->pucData, pUserData->unLength);
	}

	//
	// Get end position.
	//
	SMResult = pDataHandler->DHGetPosition(&un64EndPosition);

	TESTRESULT();

	ASSERTRT(unSize == un64EndPosition - un64StartPosition, SMRES_WRITE_ERROR);

smbail:

	return SMResult;
} // UdtaAtom_c::Write


