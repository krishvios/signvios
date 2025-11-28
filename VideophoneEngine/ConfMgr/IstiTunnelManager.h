/*!
 *
 *\file IstiRingGroupManager.h
 *\brief Declaration of the TunnelManager interface
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef ISTITUNNELMANAGER_H
#define ISTITUNNELMANAGER_H


#include "stiSVX.h"

/*!
 * \ingroup VideophoneEngineLayer 
 * \brief Manages the tunnel.
 *  
 */
class IstiTunnelManager
{
public:

	enum EApplicationState
	{
		eApplicationIOSDidEnterBackground = 0,
		eApplicationIOSWillEnterForeground = 1,
		eApplicationIOSWillTerminate = 2,
		eApplicationIOSHasNetwork = 3,
		eApplicationIOSHasNoNetwork = 4,
		eApplicationAndroidOnCreate = 5,
		eApplicationAndroidOnRestart = 6,
		eApplicationAndroidOnStart = 7,
		eApplicationAndroidOnResume = 8,
		eApplicationAndroidOnPause = 9,
		eApplicationAndroidOnStop = 10,
		eApplicationAndroidOnDestroy = 11,
		eApplicationAndroidHasNetwork = 12,
		eApplicationAndroidHasNoNetwork = 13,
		eApplicationWindowsHasNetwork = 14,
		eApplicationWindowsHasNoNetwork = 15,
		eApplicationSorensonHasNetwork = 16,
		eApplicationSorensonHasNoNetwork = 17,
	};

	virtual void ApplicationEventSet(IstiTunnelManager::EApplicationState eApplicationState) = 0;
	virtual void NetworkChangeNotify() = 0;

};

#endif // ISTITUNNELMANAGER_H
