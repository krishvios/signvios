//////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	MP4Parser.cpp
//
//  Abstract:	
//
//////////////////////////////////////////////////////////////////////////////

#include "SMMemory.h"
#include "SMCommon.h"

#include "MP4Parser.h"
#include "MP4Common.h"

#include "SorensonMP4.h"

#include "CttsAtom.h"
#include "DinfAtom.h"
#include "DrefAtom.h"
#include "ElstAtom.h"
#include "HdlrAtom.h"
#include "HmhdAtom.h"
#include "MdhdAtom.h"
#include "MdiaAtom.h"
#include "MinfAtom.h"
#include "NmhdAtom.h"
#include "SmhdAtom.h"
#include "StblAtom.h"
#include "StcoAtom.h"
#include "StsdAtom.h"
#include "StscAtom.h"
#include "StssAtom.h"
#include "StszAtom.h"
#include "SttsAtom.h"
#include "TkhdAtom.h"
#include "TrefAtom.h"
#include "UdtaAtom.h"
#include "UnknownAtom.h"
#include "UrlAtom.h"
#include "UrnAtom.h"
#include "VmhdAtom.h"

#include "CServerDataHandler.h"

#ifdef WIN32
// Windows SDK defines AddAtom as AddAtomA
#ifdef AddAtom
#undef AddAtom
#endif
#endif

#if 0 // for debugging purposes
///////////////////////////////////////////////////////////////////////////////
//; FourCCToText
//
// Abstract: Converts a four character code to a text string for debugging.
//
// Returns: None.
//
static void FourCCToText(
	uint32_t unType,
	char *pszBuffer)
{
	memcpy(pszBuffer, &unType, sizeof(uint32_t));

	pszBuffer[sizeof(uint32_t)] = '\0';
}
#endif


///////////////////////////////////////////////////////////////////////////////
//; AllocateAtom
//
// Abstract:
//
// Returns:
//
static Atom_c *AllocateAtom(
	uint32_t unType)
{
	Atom_c *pAtom;

	switch (unType)
	{
		case eAtom4CCftyp:

			pAtom = new FtypAtom_c;

			break;

		case eAtom4CCmoov:

			pAtom = new MoovAtom_c;

			break;

		case eAtom4CCtrak:

			pAtom = new TrakAtom_c;

			break;

		case eAtom4CCmdia:

			pAtom = new MdiaAtom_c;

			break;

		case eAtom4CCminf:

			pAtom = new MinfAtom_c;

			break;

		case eAtom4CCstbl:

			pAtom = new StblAtom_c;

			break;

		case eAtom4CCmvhd:

			pAtom = new MvhdAtom_c;

			break;
#if !defined(stiLINUX) && !defined(WIN32)
		case eAtom4CCiods:

			pAtom = new IodsAtom_c;

			break;
#endif
		case eAtom4CCtkhd:

			pAtom = new TkhdAtom_c;

			break;

		case eAtom4CCmdhd:

			pAtom = new MdhdAtom_c;

			break;

		case eAtom4CChdlr:

			pAtom = new HdlrAtom_c;

			break;

		case eAtom4CCvmhd:

			pAtom = new VmhdAtom_c;

			break;

		case eAtom4CCsmhd:

			pAtom = new SmhdAtom_c;

			break;

		case eAtom4CChmhd:

			pAtom = new HmhdAtom_c;

			break;

		case eAtom4CCnmhd:

			pAtom = new NmhdAtom_c;

			break;

		case eAtom4CCdref:

			pAtom = new DrefAtom_c;

			break;

		case eAtom4CCstts:

			pAtom = new SttsAtom_c;

			break;

		case eAtom4CCctts:

			pAtom = new CttsAtom_c;

			break;

		case eAtom4CCstsd:

			pAtom = new StsdAtom_c;

			break;

		case eAtom4CCstsz:

			pAtom = new StszAtom_c;

			break;

		case eAtom4CCstsc:

			pAtom = new StscAtom_c;

			break;

		case eAtom4CCstco:

			pAtom = new StcoAtom_c;

			break;

		case eAtom4CCco64:

			pAtom = new StcoAtom_c;

			pAtom->m_unType = unType;

			break;

		case eAtom4CCstss:

			pAtom = new StssAtom_c;

			break;

/*
		case eAtom4CCstsh:

			pAtom = new StshAtom_c;

			break;
*/


		case eAtom4CCelst:

			pAtom = new ElstAtom_c;

			break;

		case eAtom4CCtref:

			pAtom = new TrefAtom_c;

			break;

		case eAtom4CCurl:

			pAtom = new UrlAtom_c;

			break;

		case eAtom4CCurn:

			pAtom = new UrnAtom_c;

			break;

		case eAtom4CCudta:

			pAtom = new UdtaAtom_c;

			break;

		case eAtom4CCdinf:

			pAtom = new DinfAtom_c;

			break;

		case eAtom4CCedts:
		case eAtom4CCstdp:
		case eAtom4CCmdat:
		case eAtom4CCfree:
		case eAtom4CCskip:

			pAtom = new Atom_c;

			pAtom->m_unType = unType;

			break;

		default:

			pAtom = new UnknownAtom_c;

			pAtom->m_unType = unType;

	}

	return pAtom;
}

///////////////////////////////////////////////////////////////////////////////
//; ParseAtomHeader
//
// Abstract:
//
// Returns:
//
static SMResult_t ParseAtomHeader(
	CDataHandler *pDataHandler,
	uint64_t un64Offset,
	uint64_t *pun64AtomSize,
	uint32_t *pun32AtomType,
	uint32_t *pun32HeaderSize)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint32_t unSize;
	uint32_t unType;
	uint64_t un64Size;

	SMResult = pDataHandler->DHRead(un64Offset, sizeof(uint32_t), &unSize);

	unSize = BigEndian4(unSize);

	SMResult = pDataHandler->DHRead(un64Offset + sizeof(uint32_t), sizeof(uint32_t), &unType);
	TESTRESULT();

	*pun32AtomType = BigEndian4(unType);

	ASSERTRT(unSize != 0 || unType != 0, SMRES_INVALID_MOVIE);

	if (unSize == 1)
	{
		SMResult = pDataHandler->DHRead(un64Offset + 2 * sizeof(uint32_t), sizeof(uint64_t), &un64Size);
		TESTRESULT();

		*pun64AtomSize = BigEndian8(un64Size);

		*pun32HeaderSize = 2 * sizeof(uint32_t) + sizeof(uint64_t);
	}
	else
	{
		*pun64AtomSize = unSize;

		*pun32HeaderSize = 2 * sizeof(uint32_t);
	}

smbail:

	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; CreateAtom
//
// Abstract:
//
// Returns:
//
static SMResult_t CreateAtom(
	CDataHandler *pDataHandler,
	uint64_t un64Offset,
	uint64_t un64Size,
	uint32_t un32Type,
	uint32_t un32HeaderSize,
	Atom_c *pParent,
	Atom_c **ppAtom)
{
	SMResult_t SMResult = SMRES_SUCCESS;

	Atom_c *pAtom = nullptr;

	pAtom = AllocateAtom(un32Type);

	if (pParent)
	{
		pParent->AddAtom(pAtom);
	}

	SMResult = pAtom->Parse(pDataHandler, un64Offset + un32HeaderSize, un64Size - un32HeaderSize);

	TESTRESULT();

smbail:                                                        

	*ppAtom = pAtom;

	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; ParseMovie_r
//
// Abstract:
//
// Returns:
//
SMResult_t ParseMovie_r(
	CDataHandler *pDataHandler,
	uint64_t un64Offset,
	uint64_t un64TotalSize,
	Atom_c *pParent)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint64_t un64Size = 0;
	uint32_t unType = 0;
	uint32_t unHeaderSize = 0;
	Atom_c *pAtom = nullptr;

	// If we don't have a parent pointer then don't parse the data, because
	// we will just loose the memory allocated in CreateAtom().
	ASSERTRT(pParent != nullptr, SMRES_INVALID_PARAMETER);

	for (;;)
	{

		// Parse the atom header
		SMResult = ParseAtomHeader (pDataHandler, un64Offset, &un64Size, &unType, &unHeaderSize);
		
		ASSERTRT(un64Size <= un64TotalSize, SMRES_INVALID_MOVIE);
		
		// Create Atom
		SMResult = CreateAtom (pDataHandler, un64Offset, un64Size, unType, unHeaderSize, pParent, &pAtom);
		
		TESTRESULT();

		un64Offset += un64Size;
		un64TotalSize -= un64Size;

		if (un64TotalSize == 0)
		{
			break;
		}
	}

smbail:

	return SMResult;
}


///////////////////////////////////////////////////////////////////////////////
//; ParseMovie
//
// Abstract:
//
// Returns:
//
SMResult_t ParseMovie(
	Movie_t *pMovie)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint64_t un64TotalSize;
	uint64_t un64Offset;
	uint64_t un64Size = 0;
	uint32_t unType = 0;
	uint32_t unHeaderSize = 0;
	Atom_c *pAtom = nullptr;
	FileSpace_t *pFileSpace;

	(pMovie->pDataHandler)->DHGetFileSize(&un64TotalSize);

	//
	// This loop parses the atoms at the file level.  CreateAtom will parse the atom and in turn call
	// ParseMovie_r for the child atoms.  The file level atoms are parsed here so that their offsets
	// and lengths can be stored for later use when saving the file.
	//
	un64Offset = 0;

	//
	// Free the moov atom if this movie object already has one.
	//
	if (pMovie->pMoovAtom)
	{

		pMovie->pMoovAtom->FreeAtom();

		pMovie->pMoovAtom = nullptr;
	}

	//
	// Free the ftyp atom if this movie object already has one.
	//
	if (pMovie->pFtypAtom)
	{

		pMovie->pFtypAtom->FreeAtom();

		pMovie->pFtypAtom = nullptr;
	}

	for (;;)
	{
		// Parse the atom header
		SMResult = ParseAtomHeader (pMovie->pDataHandler, un64Offset, &un64Size, &unType, &unHeaderSize);
		
		ASSERTRT(un64Size <= un64TotalSize, SMRES_INVALID_MOVIE);
		
		// Create Atom
		SMResult = CreateAtom (pMovie->pDataHandler, un64Offset, un64Size, unType, unHeaderSize, nullptr, &pAtom);

		TESTRESULT();

		pFileSpace = (FileSpace_t *)SMAllocPtr(sizeof(FileSpace_t));

		ASSERTRT(pFileSpace != nullptr, SMRES_ALLOC_FAILED);

		pFileSpace->unType = pAtom->m_unType;
		pFileSpace->un64Offset = un64Offset;
		pFileSpace->un64Length = un64Size;

		pFileSpace->pNext = &pMovie->FileSpaceList;
		pFileSpace->pPrev = pMovie->FileSpaceList.pPrev;

		pMovie->FileSpaceList.pPrev->pNext = pFileSpace;
		pMovie->FileSpaceList.pPrev = pFileSpace;

		//
		// At the top level we only want to keep the moov atom and the ftyp atom.  All other atoms may be
		// freed.
		//
		switch (pAtom->m_unType)
		{
			case eAtom4CCmoov:
                                                        
				pMovie->pMoovAtom = (MoovAtom_c *)pAtom;

				break;

			case eAtom4CCftyp:

				pMovie->pFtypAtom = (FtypAtom_c *)pAtom;

				break;

			default:

				pAtom->FreeAtom();

				break;
		}

		// We have assigned the atom to someone else or free'd it so set it
		// to NULL so we know if we need to delete it or not.
		pAtom = nullptr;

		un64Offset += un64Size;
		un64TotalSize -= un64Size;

		if (un64TotalSize == 0)
		{
			break;
		}
	}

	ASSERTRT(pMovie->pMoovAtom != nullptr, SMRES_INVALID_MOVIE);

smbail:
	if (pAtom)
	{
		pAtom->FreeAtom();
	}

	return SMResult;
}

///////////////////////////////////////////////////////////////////////////////
//; ParseAtom
//
// Abstract:
//
// Returns:
//
SMResult_t ParseAtom(
	Movie_t *pMovie,
	uint64_t un64FileOffset,
	uint64_t *pun64DataParsed,
	uint32_t *pun32AtomType,
	uint32_t *pun32HeaderSize)
{
	SMResult_t SMResult = SMRES_SUCCESS;
	uint64_t un64TotalSize = 0;
	uint32_t unType = 0;
	uint64_t un64Size = 0;
	uint32_t unHeaderSize = 0;
	Atom_c *pAtom = nullptr;
#if 0
	FileSpace_t *pFileSpace;
#endif
	auto pDataHandler = (CServerDataHandler *) pMovie->pDataHandler;

	// Set to 0 to indicate no atom has been parsed yet
	*pun64DataParsed = 0;
	*pun32HeaderSize = 0;
	*pun32AtomType = 0;

	pDataHandler->DHGetFileSize(&un64TotalSize);
	
	// Parse the atom header
	SMResult = ParseAtomHeader (pDataHandler, un64FileOffset, &un64Size, &unType, &unHeaderSize);
	
	ASSERTRT(un64Size <= un64TotalSize, SMRES_INVALID_MOVIE);

#if 0 //Maybe for video message record
	pFileSpace = (FileSpace_t *)SMAllocPtr(sizeof(FileSpace_t));
	
	ASSERTRT(pFileSpace != NULL, SMRES_ALLOC_FAILED);
	
	pFileSpace->unType = unType;
	pFileSpace->un64Offset = un64FileOffset;
	pFileSpace->un64Length = un64Size;
	
	pFileSpace->pNext = &pMovie->FileSpaceList;
	pFileSpace->pPrev = pMovie->FileSpaceList.pPrev;
	
	pMovie->FileSpaceList.pPrev->pNext = pFileSpace;
	pMovie->FileSpaceList.pPrev = pFileSpace;
#endif

	//
	// At the top level we only want to keep the moov atom and the ftyp atom.  All other atoms may be
	// freed.
	//
	switch (unType)
	{
	case eAtom4CCmoov:
		
		// Make sure the whole atom is in the buffer before creating atom
		ASSERTRT(estiTRUE == pDataHandler->IsDataInBuffer (static_cast<uint32_t>(un64FileOffset), static_cast<uint32_t>(un64Size)), SMRES_READ_ERROR);
		
		SMResult = CreateAtom (pDataHandler, un64FileOffset, un64Size, unType, unHeaderSize, nullptr, &pAtom);
		
		TESTRESULT();

		//
		// Free the moov atom if this movie object already has one.
		//
		if (pMovie->pMoovAtom)
		{

			pMovie->pMoovAtom->FreeAtom();
			
			pMovie->pMoovAtom = nullptr;
		}

		pMovie->pMoovAtom = (MoovAtom_c *)pAtom;

		pAtom = nullptr;
		
		break;
		
	case eAtom4CCftyp:
		
		// Make sure the whole atom is in the buffer before creating atom
		ASSERTRT(estiTRUE == pDataHandler->IsDataInBuffer (static_cast<uint32_t>(un64FileOffset), static_cast<uint32_t>(un64Size)), SMRES_READ_ERROR);
		
		SMResult = CreateAtom (pDataHandler, un64FileOffset, un64Size, unType, unHeaderSize, nullptr, &pAtom);
		
		TESTRESULT();
		
		//
		// Free the ftyp atom if this movie object already has one.
		//
		if (pMovie->pFtypAtom)
		{
			pMovie->pFtypAtom->FreeAtom();
			
			pMovie->pFtypAtom = nullptr;
		}

		pMovie->pFtypAtom = (FtypAtom_c *)pAtom;
		
		pAtom = nullptr;

		break;
	}

	// Successful parse an atom, update the info
	*pun64DataParsed = un64Size;
	*pun32AtomType = unType;
	*pun32HeaderSize = unHeaderSize;

smbail:

	if (pAtom)
	{
		pAtom->FreeAtom();
	}

	return SMResult;
}


