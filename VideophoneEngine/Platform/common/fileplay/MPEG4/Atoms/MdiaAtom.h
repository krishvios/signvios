// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef MDIAATOM_H
#define MDIAATOM_H

#include "Atom.h"
#include "CDataHandler.h"


class MdhdAtom_c;
class MinfAtom_c;


class MdiaAtom_c : public Atom_c
{
public:

	MdiaAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	CDataHandler *m_pDataHandler;

	SMResult_t SetDataHandler(
		CDataHandler* pDataHandler);

	void CloseDataHandler() override;

	unsigned int m_unSampleDescrIndex;

	MdhdAtom_c *m_pMdhd{nullptr};

	unsigned int m_unSampleIndex;
	uint32_t m_unSampleSize;
	unsigned int m_unLastSampleInChunk;
	unsigned int m_unChunkIndex;
	unsigned int m_unLastChunk;
	unsigned int m_unSampleToChunkIndex;
	uint64_t m_un64SampleOffset;
	
private:

	MinfAtom_c *m_pMinf{nullptr};
};


#endif // MDIAATOM_H

