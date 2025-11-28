// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef MVHDATOM_H
#define MVHDATOM_H

#include "SorensonMP4.h"
#include "FullAtom.h"


class MvhdAtom_c : public FullAtom_c
{
public:

	MvhdAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	uint32_t GetSize() override;

	uint32_t GetNextTrackID();

	MP4TimeScale GetTimeScale() const
	{
		return m_unTimeScale;
	}

	void SetTimeScale(
		MP4TimeScale unTimeScale)
	{
		m_unTimeScale = unTimeScale;
	}

	void SetDuration(
		MP4TimeValue un64Duration)
	{
		m_un64Duration = un64Duration;
	}

	MP4TimeValue GetDuration() const
	{
		return m_un64Duration;
	}
		
	SMResult_t Write(
		CDataHandler* pDataHandler) override;

private:

	uint64_t m_un64CreationTime{0};
	uint64_t m_un64ModificationTime;
	MP4TimeScale m_unTimeScale;
	MP4TimeValue m_un64Duration;

	uint32_t m_unReserved1;
	uint16_t m_usReserved2;
	uint16_t m_usReserved3;
	uint32_t m_unReserved4[2]{};
	uint32_t m_unReserved5[9]{};
	uint32_t m_unReserved6[6]{};
	uint32_t m_unNextTrackID;

};


#endif // MVHDATOM_H

