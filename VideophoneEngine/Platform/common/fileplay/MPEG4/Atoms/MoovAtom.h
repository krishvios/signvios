// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef MOOVATOM_H
#define MOOVATOM_H

#include "MP4Track.h"
#include "Atom.h"
#include "MvhdAtom.h"
#include "IodsAtom.h"
#include "FtypAtom.h"
#include "TrakAtom.h"
#include "CDataHandler.h"


class MoovAtom_c : public Atom_c
{
public:
	
	MoovAtom_c();

	SMResult_t Parse(
		CDataHandler* pDataHandler,
		uint64_t un64Offset,
		uint64_t un64Length) override;

	SMResult_t UpdateDuration();

	SMResult_t SetupTracks();

	SMResult_t GetTrackCount(
		int *pnTrackCount) const;

	SMResult_t GetIndTrack(uint32_t unTrackNumber,
						   TrakAtom_c *pTrack) const;

	SMResult_t GetNextTrackID(
		uint32_t *punNextID) const;

	SMResult_t AddTrack(
		TrakAtom_c *pTrack);

	SMResult_t NewTrack(
		uint32_t newTrackFlags,
		TrakAtom_c *pTrack);

	SMResult_t NewTrackWithID(
		uint32_t unNewTrackFlags,
		uint32_t unNewTrackID,
		TrakAtom_c *pTrack);

	SMResult_t MdatMoved(
		int32_t Offset);

	void SetMovie(
		Movie_t *pMovie)
	{
		m_pMovie = pMovie;
	}

	Movie_t *GetMovie() const
	{
		return m_pMovie;
	}

	MvhdAtom_c *m_pMvhd{nullptr};              /* the movie header */

private:

	Movie_t *m_pMovie{nullptr};

	IodsAtom_c *m_pIods{nullptr};              /* the initial object descriptor */
};


#endif // MOOVATOM_H

