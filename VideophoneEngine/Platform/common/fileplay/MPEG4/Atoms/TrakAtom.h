// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef TRAKATOM_H
#define TRAKATOM_H

#include "Atom.h"
#include "SorensonMP4.h"

class MoovAtom_c;
class ElstAtom_c;


class TrakAtom_c : public Atom_c
{
public:

	TrakAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	Movie_t *GetMovie() const;

private:

	MoovAtom_c *m_pMoov{nullptr};
	ElstAtom_c *m_pElst{nullptr};
};


#endif // TRAKATOM_H

