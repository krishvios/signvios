// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef VMHDATOM_H
#define VMHDATOM_H

#include "FullAtom.h"


class VmhdAtom_c : public FullAtom_c
{
public:
	
	VmhdAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	uint32_t GetSize() override;

	SMResult_t Write(
		CDataHandler* pDataHandler) override;

private:

	uint64_t m_un64Reserved1;
};

#endif // VMHDATOM_H

