// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef MINFATOM_H
#define MINFATOM_H

#include "Atom.h"


class StblAtom_c;


class MinfAtom_c : public Atom_c
{
public:

	MinfAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;
			
private:

	StblAtom_c *m_pStbl{nullptr};
};


#endif // MINFATOM_H

