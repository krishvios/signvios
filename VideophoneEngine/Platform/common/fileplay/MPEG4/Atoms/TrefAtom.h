// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef TREFATOM_H
#define TREFATOM_H

#include "Atom.h"
#include "TrakAtom.h"


typedef struct Ref_t
{
	uint32_t unType;
	uint32_t *pTrakRef;
	uint32_t unCount;
	uint32_t unAllocated;
} Ref_t;


class TrefAtom_c : public Atom_c
{
public:
	
	TrefAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	uint32_t GetSize() override;

	void FreeAtom() override;

	SMResult_t AddReference(
		TrakAtom_c *pTrackAtom,
		uint32_t unRefType,
		uint32_t *punRefIndex);

	SMResult_t GetReference(
		uint32_t unRefType,
		uint32_t unRefIndex,
		uint32_t *punTrackID);

	SMResult_t Write(
		CDataHandler* pDataHandler) override;

private:

	Ref_t MpodRef{};
	Ref_t DpndRef{};
	Ref_t SyncRef{};
	Ref_t HintRef{};
	Ref_t IpirRef{};
};


#endif // TREFATOM_H

