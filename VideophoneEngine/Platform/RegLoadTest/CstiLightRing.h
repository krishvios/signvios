///
/// \file CstiLightRing.h
/// \brief Declaration of the LightRing class
///
///
/// Sorenson Communications Inc. Confidential. --  Do not distribute
/// Copyright 2022 Sorenson Communications, Inc. -- All rights reserved
///


#ifndef CSTILIGHTRING_H
#define CSTILIGHTRING_H

#include "IstiLightRing.h"
#include "CstiSignal.h"

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
	
	virtual ~CstiLightRing ();
	
	virtual void PatternSet (
		Pattern patternId,
		EstiLedColor color,
		int brightness,
		int flasherBrightness
		);
		
	virtual void notificationPatternSet (
		NotificationPattern patter) {};
	  
	virtual  void Start(
		int nSeconds);
		
	virtual  void Stop();
	
	virtual void notificationsStart (
		int nDuration, 
		int nBrightness) {};
		
	virtual void notificationsStop () {};
	
	virtual ISignal<>& lightRingStoppedSignalGet ()
	{
		return lightRingStoppedSignal;
	}

	virtual ISignal<>& notificationStoppedSignalGet ()
	{
		return notificationStoppedSignal;
	}
	

protected:

private:
	CstiSignal <> lightRingStoppedSignal;
	CstiSignal <> notificationStoppedSignal;

};

#endif // CSTILIGHTRING_H
