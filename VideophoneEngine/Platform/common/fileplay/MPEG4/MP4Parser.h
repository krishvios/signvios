//////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	MP4Parser.h
//
//  Abstract:	
//
//////////////////////////////////////////////////////////////////////////////
#ifndef MP4PARSER_H
#define MP4PARSER_H

#include "SMTypes.h"

#include "MP4Common.h"

SMResult_t ParseMovie(
	Movie_t *pMovie);

SMResult_t ParseAtom(
	Movie_t *pMovie,
	uint64_t un64FileOffset,
	uint64_t *pun64DataParsed,
	uint32_t *pun32AtomType,
	uint32_t *pun32HeaderSize);

SMResult_t ParseMovie_r(
	CDataHandler *pDataHandler,
	uint64_t un64Offset,
	uint64_t un64TotalSize,
	Atom_c *pParent);

#endif // MP4PARSER_H

