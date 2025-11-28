// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef MDHDATOM_H
#define MDHDATOM_H

#include "FullAtom.h"
#include "SorensonMP4.h"


class MdhdAtom_c : public FullAtom_c
{
public:

	MdhdAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	uint32_t GetSize() override;

	uint64_t GetDuration() const
	{
		return m_un64Duration;
	}

	void SetDuration(
		uint64_t un64Duration)
	{
		m_un64Duration = un64Duration;
	}

	uint32_t GetTimeScale() const
	{
		return m_unTimeScale;
	}

	void SetTimeScale(
		uint32_t unTimeScale)
	{
		m_unTimeScale = unTimeScale;
	}

	SMResult_t Write(
		CDataHandler* pDataHandler) override;

private:

	uint64_t m_un64CreationTime{0};
	uint64_t m_un64ModificationTime;
	uint32_t m_unTimeScale;
	uint64_t m_un64Duration;

	uint16_t m_usLanguage;
	uint16_t m_usReserved1;
};


#endif // MDHDATOM_H

