///
/// \file CstiAudibleRinger.h
/// \brief Declaration of the Audible Ringer class
///
/// Sorenson Communications Inc. Confidential. --  Do not distribute
/// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
///


#ifndef CSTIAUDIBLERINGER_H
#define CSTIAUDIBLERINGER_H

#include "IstiAudibleRinger.h"
#include "CstiAudioHandler.h"
//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//

//
// Class Declaration
//

class CstiAudibleRinger : public IstiAudibleRinger
{
public:

	CstiAudibleRinger () = default;
	
	CstiAudibleRinger (const CstiAudibleRinger &other) = delete;
	CstiAudibleRinger (CstiAudibleRinger &&other) = delete;
	CstiAudibleRinger &operator= (const CstiAudibleRinger &other) = delete;
	CstiAudibleRinger &operator= (CstiAudibleRinger &&other) = delete;

	virtual ~CstiAudibleRinger () = default;
	
	stiHResult Initialize (
		CstiAudioHandler *pAudioHandler);
	
	stiHResult Start () override;
	stiHResult Stop () override;

	stiHResult TonesSet (
		EstiTone eTone,
		const SstiTone *pstiTone,
		unsigned int unCount,
		unsigned int unRepeatCount
		) override;

	stiHResult VolumeSet (
		int volume) override;
	
	int VolumeGet ();
	
	stiHResult PitchSet (
		int pitch) override;
	
	EstiTone PitchGet ();

protected:

private:

	CstiAudioHandler *m_pAudioHandler{nullptr};
};

#endif // CSTIAUDIBLERINGER_H
