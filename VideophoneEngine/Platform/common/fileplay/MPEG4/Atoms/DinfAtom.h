// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef DINFATOM_H
#define DINFATOM_H

#include "Atom.h"


class DinfAtom_c : public Atom_c
{
public:

	DinfAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

private:
};

#endif // DINFATOM_H

