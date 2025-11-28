// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef STBLATOM_H
#define STBLATOM_H

#include "Atom.h"
#include "SorensonMP4.h"


class StscAtom_c;
class StcoAtom_c;
class StszAtom_c;
class SttsAtom_c;


class StblAtom_c : public Atom_c
{
public:

	StblAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

private:

	StscAtom_c *m_pStsc{nullptr};
	StcoAtom_c *m_pStco{nullptr};
	StszAtom_c *m_pStsz{nullptr};
	SttsAtom_c *m_pStts{nullptr};
};


#endif // STBLATOM_H

