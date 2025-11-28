///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	UdtaAtom.h
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#ifndef UDTAATOM_H
#define UDTAATOM_H


#include "Atom.h"

typedef struct UserData_t
{
	uint32_t unType;
	uint8_t *pucData;
	uint32_t unLength;

	UserData_t *pNext;
	UserData_t *pPrev;

} UserData_t;


class UdtaAtom_c : public Atom_c
{
public:
	
	UdtaAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	uint32_t GetSize() override;

	void FreeAtom() override;

	SMResult_t AddUserData(
		uint32_t unType,
		const uint8_t *pucData,
		uint32_t unLength);

	SMResult_t Write(
		CDataHandler* pDataHandler) override;

private:

	UserData_t m_UserDataList{};
};



#endif // UDTAATOM_H

