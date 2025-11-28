///
/// \file CstiAudibleRinger.h
/// \brief Declaration of the Audible Ringer class
///
///
/// Sorenson Communications Inc. Confidential. --  Do not distribute
/// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
///


#ifndef CSTIAUDIBLERINGER_H
#define CSTIAUDIBLERINGER_H

#include "IstiAudibleRinger.h"

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

	CstiAudibleRinger ();
	
	virtual ~CstiAudibleRinger ();
	
	virtual stiHResult Start();
	virtual stiHResult Stop();

	virtual stiHResult TonesSet (
		EstiTone eTone,
		const SstiTone *pstiTone,
		unsigned int unCount,
		unsigned int unRepeatCount
		);

protected:

private:
	SstiTone *m_pstiTone;
	

};

#endif // CSTIAUDIBLERINGER_H
