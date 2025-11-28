///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	HdlrAtom.h
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#ifndef HDLRATOM_H
#define HDLRATOM_H

//
// Sorenson includes.
//
#include "SMTypes.h"

#include "FullAtom.h"


class HdlrAtom_c : public FullAtom_c
{
public:

	HdlrAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	uint32_t GetSize() override;

	void FreeAtom() override;

	SMResult_t SetHandlerType(
		uint32_t unHandlerType,
		const char *pszName);

	SMResult_t GetHandlerType(
		uint32_t *punHandlerType);

	SMResult_t Write(
		CDataHandler* pDataHandler) override;

private:

	uint32_t m_unReserved1;
	uint32_t m_unHandlerType;
	uint8_t m_unReserved2[12]{};
	char *m_pszName;
};


#endif // HDLRATOM_H

