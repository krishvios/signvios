// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef NMHDATOM_H
#define NMHDATOM_H

#include "FullAtom.h"


class NmhdAtom_c : public FullAtom_c
{
public:
	
	NmhdAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	uint32_t GetSize() override;

	SMResult_t Write(
		CDataHandler* pDataHandler) override;

private:

};

#endif // NMHDATOM_H

