/*!
* \file CDeviceController.h
* \brief This file defines a base class used to communicate with audio/video devices.
*
* \author Ting-Yu Yang
*
*  Copyright (C) 2003-2004 by Sorenson Media, Inc.  All Rights Reserved
*/

//
// Includes
//
#include "CDeviceController.h"
#include "stiTrace.h"

//
// Constants
//

//
// Typedefs
//

//
// Globals
//

//
// Locals
//

//
// Forward Declarations
//



// Constructor
CDeviceController::CDeviceController ()
	:
	m_nfd (0)
{
	m_LockMutex = stiOSNamedMutexCreate ("CDeviceController");
}

// Deconstructor
CDeviceController::~CDeviceController ()
{
	if(NULL != m_LockMutex)
	{
		stiOSMutexDestroy (m_LockMutex);
		m_LockMutex = NULL;
	}
}

HRESULT CDeviceController::DeviceOpen ()
{

	HRESULT errCode = NO_ERROR;
#ifdef stiDRIVER_READY   


	Lock ();
	
	
	Unlock ();
	
#endif

	return errCode;
}

HRESULT CDeviceController::DeviceClose ()
{

	HRESULT errCode = NO_ERROR;
#ifdef stiDRIVER_READY

	Lock ();
	
	Unlock ();
#endif
	return errCode;
}

int CDeviceController::GetDeviceFD ()
{
	return (m_nfd);
}


stiHResult CDeviceController::Lock ()
{
	return stiOSMutexLock (m_LockMutex);
}


void CDeviceController::Unlock ()
{
	stiOSMutexUnlock (m_LockMutex);
}



