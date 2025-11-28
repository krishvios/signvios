///
/// \file CstiLightRing.h
/// \brief Declaration of the LightRing class
///
///
/// Copyright (C) 2010 Sorenson Communications, Inc. All rights reserved. **
///


#ifndef CSTILIGHTRING_H
#define CSTILIGHTRING_H

#include "IstiLightRing.h"

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

class CstiLightRing : public IstiLightRing
{
public:

	CstiLightRing ();
	
	~CstiLightRing ();
	
	virtual void PatternSet (
		Pattern patternId,
		EstiLedColor color,
		int brightness);
	
	virtual  void Start(
		int nSeconds);
		
	virtual  void Stop();

protected:

private:

};

#endif // CSTILIGHTRING_H
