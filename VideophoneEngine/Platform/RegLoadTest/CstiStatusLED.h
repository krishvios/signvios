///
/// \file CstiStatusLED.h
/// \brief Declaration of the Status LED class
///
///
/// Sorenson Communications Inc. Confidential. --  Do not distribute
/// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
///


#ifndef CSTISTATUSLED_H
#define CSTISTATUSLED_H

#include "IstiStatusLED.h"

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

class CstiStatusLED : public IstiStatusLED
{
public:

	CstiStatusLED ();
	
	virtual ~CstiStatusLED ();
	
	static IstiStatusLED *InstanceGet ();

	virtual stiHResult Start(
		EstiLed eLED,
		unsigned unBlinkRate);
	
	virtual stiHResult Stop(
		EstiLed eLED);

	virtual stiHResult Enable (
		EstiLed eLED,
		EstiBool bEnable);

	virtual stiHResult PulseNotificationsEnable (
		bool enable
		);
	
protected:

private:

};

#endif // CSTISTATUSLED_H
