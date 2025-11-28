// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef FTYPATOM_H
#define FTYPATOM_H

#include "Atom.h"


class FtypAtom_c : public Atom_c
{
public:

	FtypAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	void FreeAtom() override;

	uint32_t GetSize() override;

	SMResult_t Write(
		CDataHandler* pDataHandler) override;

	void GetBrands(
		uint32_t **ppBrands,
		unsigned int *punBrandLength) const;

	SMResult_t SetBrands(
		const uint32_t *pBrands,
		unsigned int unBrandLength);

private:

	uint32_t *m_punBrands;
	unsigned int m_unBrandLength;
};

#endif // FTYPATOM_H

