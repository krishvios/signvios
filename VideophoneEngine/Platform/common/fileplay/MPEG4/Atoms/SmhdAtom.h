// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef SMHDATOM_H
#define SMHDATOM_H

#include "FullAtom.h"

class SmhdAtom_c : public FullAtom_c
{
public:
	
	SmhdAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	uint32_t GetSize() override;

	SMResult_t Write(
		CDataHandler* pDataHandler) override;

private:

	uint32_t m_unReserved1;
};

#endif // SMHDATOM_H

