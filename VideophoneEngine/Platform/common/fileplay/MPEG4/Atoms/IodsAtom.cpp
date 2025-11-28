///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	IodsAtom.cpp
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#include <cstring>

//
// Sorenson includes
//
#include "SorensonErrors.h"
#include "SMCommon.h"
#include "SMMemory.h"
//#include "Bitstream.h"

#include "IodsAtom.h"
#include "TkhdAtom.h"
#include "MP4Parser.h"
#include "MP4Common.h"

//#include "Objects.h"


///////////////////////////////////////////////////////////////////////////////
//; IodsAtom_c::IodsAtom_c
//
// Abstract:
//
// Returns:
//
IodsAtom_c::IodsAtom_c()
{
	m_unType = 'iods';

	m_pIOD = NULL;

} // IodsAtom_c::IodsAtom_c


///////////////////////////////////////////////////////////////////////////////
//; IodsAtom_c::SetIOD
//
// Abstract:
//
// Returns:
//
SMResult_t IodsAtom_c::SetIOD(
	InitialObjectDescr_i *pIOD)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	ObjectEncoder_i *pObjectEncoder = NULL;

	pObjectEncoder = CreateObjectEncoder();

	m_pIOD = (uint8_st *)SMRealloc(m_pIOD, 1024); // BUGBUG - How do we know how big to make the buffer?

	ASSERTRT(m_pIOD != NULL, SMRES_ALLOC_FAILED);

	m_unLength = 1024;

	SMResult = pObjectEncoder->EncodeDescriptor(pIOD, m_pIOD, &m_unLength);

	TESTRESULT();

smbail:

	if (pObjectEncoder)
	{
		DestroyObjectEncoder(pObjectEncoder);

		pObjectEncoder = NULL;
	}

	return SMResult;
} // IodsAtom_c::SetIOD


///////////////////////////////////////////////////////////////////////////////
//; IodsAtom_c::GetIOD
//
// Abstract:
//
// Returns:
//
InitialObjectDescr_i *IodsAtom_c::GetIOD()
{
	SMResult_t SMResult = SMRES_SUCCESS;
	InitialObjectDescr_i *pIOD = NULL;
	ObjectDecoder_i *pObjectDecoder = NULL;

	pObjectDecoder = CreateObjectDecoder();

	ASSERTRT(pObjectDecoder != NULL, SMRES_ALLOC_FAILED);

	pIOD = (InitialObjectDescr_i *)pObjectDecoder->DecodeDescriptor((char *)m_pIOD, m_unLength, NULL, NULL);

smbail:

	if (pObjectDecoder)
	{
		DestroyObjectDecoder(pObjectDecoder);
	}

	if (SMResult != SMRES_SUCCESS)
	{
		if (pIOD != NULL)
		{
			pIOD->Release();

			pIOD = NULL;
		}
	}

	return pIOD;
}


///////////////////////////////////////////////////////////////////////////////
//; IodsAtom_c::Parse
//
// Abstract:
//
// Returns:
//
SMResult_t IodsAtom_c::Parse(
	CDataHandler* pDataHandler,
	uint64_st un64Offset,
	uint64_st un64Length)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	SMResult = (pDataHandler, &un64Offset, &un64Length);

	TESTRESULT();

	if (m_pIOD)
	{
		SMFreePtr(m_pIOD);

		m_pIOD = NULL;
	}

	m_unLength = (uint32_st)un64Length;

	ASSERTRT(m_unLength > 0, SMRES_INVALID_MOVIE);

	m_pIOD = (uint8_st *)GetDataPtr(pDataHandler, un64Offset, m_unLength);

	ASSERTRT(m_pIOD != NULL, SMRES_ALLOC_FAILED);

smbail:

	return SMResult;
} // IodsAtom_c::Parse


///////////////////////////////////////////////////////////////////////////////
//; IodsAtom_c::GetSize
//
// Abstract:
//
// Returns:
//
uint32_st IodsAtom_c::GetSize()
{
	uint32_st unSize = 0;

	unSize += m_unLength;

	unSize += GetHeaderSize(unSize);

	return unSize;
}


///////////////////////////////////////////////////////////////////////////////
//; IodsAtom_c::FreeAtom
//
// Abstract:
//
// Returns:
//
void IodsAtom_c::FreeAtom()
{
	if (m_pIOD)
	{
		SMFreePtr(m_pIOD);

		m_pIOD = NULL;
	}

	Atom_c::FreeAtom();
} // IodsAtom_c::FreeAtom


///////////////////////////////////////////////////////////////////////////////
//; IodsAtom_c::Write
//
// Abstract:
//
// Returns:
//
SMResult_t IodsAtom_c::Write(
	CDataHandler* pDataHandler)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint32_st unSize;
	uint64_st un64StartPosition;
	uint64_st un64EndPosition;

	ASSERTRT(m_pIOD != NULL, SMRES_INVALID_MOVIE);

	ASSERTRT(pDataHandler != NULL, SMRES_INVALID_DATA_HANDLER);

	unSize = GetSize();

	SMResult = pDataHandler->DHGetPosition(&un64StartPosition);

	TESTRESULT();

	//
	// Write the atom header.
	//
	SMResult = WriteHeader(pDataHandler, unSize);

	TESTRESULT();

	SMResult = pDataHandler->DHWrite(m_pIOD, m_unLength);

	TESTRESULT();

	//
	// Get end position.
	//
	SMResult = pDataHandler->DHGetPosition(&un64EndPosition);

	TESTRESULT();

	ASSERTRT(unSize == un64EndPosition - un64StartPosition, SMRES_WRITE_ERROR);

smbail:

	return SMResult;
} // IodsAtom_c::Write


