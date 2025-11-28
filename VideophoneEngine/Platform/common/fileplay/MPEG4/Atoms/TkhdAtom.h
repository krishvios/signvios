// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef TKHDATOM_H
#define TKHDATOM_H

#include "FullAtom.h"
#include "SorensonMP4.h"


class TkhdAtom_c : public FullAtom_c
{
public:

	TkhdAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	uint32_t GetSize() override;

	uint32_t GetTrackID() const;

	void SetTrackID(
		uint32_t unTrackID);

	void SetDuration(
		uint64_t un64Duration)
	{
		m_un64Duration = un64Duration;
	}

	uint64_t GetDuration() const
	{
		return m_un64Duration;
	}

	void SetDimensions(
		uint32_t unWidth,		// Fixed 16.16
		uint32_t unHeight)		// Fixed 16.16
	{
		m_unWidth = unWidth;
		m_unHeight = unHeight;
	}

	void GetDimensions(
		uint32_t *punWidth,
		uint32_t *punHeight) const
	{
		*punWidth = m_unWidth;
		*punHeight = m_unHeight;
	}

	void SetVolume(
		int16_t sVolume)		// Fixed 8.8
	{
		m_sVolume = sVolume;
	}

	SMResult_t Write(
		CDataHandler* pDataHandler) override;

private:

	uint64_t m_un64CreationTime{0};
	uint64_t m_un64ModificationTime;
	uint32_t m_unTrackID;
	uint64_t m_un64Duration;

	uint32_t m_unReserved1[2]{};
	int16_t m_sLayer;
	int16_t m_sAlternateGroup;
	int16_t m_sVolume;			// Fixed 8.8
	uint16_t m_usReserved3;
	int32_t m_anMatrix[9]{};

	uint32_t m_unWidth;		// Fixed 16.16
	uint32_t m_unHeight;		// Fixed 16.16
};


#endif // TKHDATOM_H

