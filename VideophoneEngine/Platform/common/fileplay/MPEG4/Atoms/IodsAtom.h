///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	IodsAtom.h
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#ifndef IODSATOM_H
#define IODSATOM_H

//
// Sorenson includes
//
//#include "Bitstream.h"

#include "FullAtom.h"
#include "TrakAtom.h"


class IodsAtom_c : public FullAtom_c
{
public:

	IodsAtom_c();
	
	~IodsAtom_c () override;

	IodsAtom_c (const IodsAtom_c &other) = delete;
	IodsAtom_c (IodsAtom_c &&other) = delete;
	IodsAtom_c &operator = (const IodsAtom_c &other) = delete;
	IodsAtom_c &operator = (IodsAtom_c &&other) = delete;

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	uint32_t GetSize() override;

	void FreeAtom() override;

	SMResult_t SetIOD(
		InitialObjectDescr_i *pIOD);

	InitialObjectDescr_i *GetIOD();

	SMResult_t Write(
		CDataHandler* pDataHandler) override;

	SMResult_t AddTrack(
		TrakAtom_c *pTrakAtom);

	uint32_t GetTrackCount();

	uint32_t GetTrackID(
		uint32_t unIndex);

private:

	uint8_t *m_pIOD;		// Encoded IOD.
	uint32_t m_unLength;	// Length of encoded IOD.
};


#endif // IODSATOM_H

