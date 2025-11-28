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

	CstiStatusLED () = default;
	
	CstiStatusLED (const CstiStatusLED &other) = delete;
	CstiStatusLED (CstiStatusLED &&other) = delete;
	CstiStatusLED &operator= (const CstiStatusLED &other) = delete;
	CstiStatusLED &operator= (CstiStatusLED &&other) = delete;

	~CstiStatusLED () override = default;
	
	static IstiStatusLED *InstanceGet ();

	void Uninitialize ();

	stiHResult Start(
		EstiLed eLED,
		unsigned unBlinkRate) override;
	
	stiHResult Stop(
		EstiLed eLED) override;

	stiHResult Enable (
		EstiLed eLED,
		EstiBool bEnable) override;

	stiHResult PulseNotificationsEnable(
		bool enable) override;
	
protected:

private:

};

#endif // CSTISTATUSLED_H
