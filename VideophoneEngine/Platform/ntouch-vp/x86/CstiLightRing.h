///
/// \file CstiLightRing.h
/// \brief Declaration of the LightRing class
///
///
/// Sorenson Communications Inc. Confidential. --  Do not distribute
/// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
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
class CstiMonitorTask;
class CstiHelios;

//
// Globals
//

//
// Class Declaration
//

class CstiLightRing : public IstiLightRing
{
public:

	CstiLightRing () = default;;
	
	CstiLightRing (const CstiLightRing &other) = delete;
	CstiLightRing (CstiLightRing &&other) = delete;
	CstiLightRing &operator= (const CstiLightRing &other) = delete;
	CstiLightRing &operator= (CstiLightRing &&other) = delete;

	virtual ~CstiLightRing () = default;
	
	stiHResult Initialize (CstiMonitorTask *monitorTask, std::shared_ptr<CstiHelios> &helios);

	stiHResult Startup ();

	void PatternSet (
		Pattern patternId,
		EstiLedColor color,
		int brightness,
		int flasherBrightness) override;
		
	 void notificationPatternSet (NotificationPattern pattern) override; 

	 void Start(
		int nSeconds) override;
		
	 void Stop() override;

	 void notificationsStart (
		int nSeconds,
		int nBrightness) override;
		
	 void notificationsStop () override;

	 ISignal<>& lightRingStoppedSignalGet () override;
	 ISignal<>& notificationStoppedSignalGet () override;

protected:

private:
	 CstiSignal<> lightRingStoppedSignal;
	 CstiSignal<> notificationStoppedSignal;

};

#endif // CSTILIGHTRING_H
