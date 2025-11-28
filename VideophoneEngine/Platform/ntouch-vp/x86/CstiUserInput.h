// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#pragma once

#include "CstiUserInputBase.h"

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

//
// Globals
//

//
// Class Declaration
//

class CstiUserInput : public CstiUserInputBase
{
public:

	CstiUserInput ()
	:
		CstiUserInputBase ({}, {})
	{
	}
	
	CstiUserInput (const CstiUserInput &other) = delete;
	CstiUserInput (CstiUserInput &&other) = delete;
	CstiUserInput &operator= (const CstiUserInput &other) = delete;
	CstiUserInput &operator= (CstiUserInput &&other) = delete;

	~CstiUserInput () override = default;

	stiHResult Initialize(CstiMonitorTask *monitorTask)
	{
		m_monitorTask = monitorTask;

		return stiRESULT_SUCCESS;
	}

protected:

	MonitorTaskBase *monitorTaskBaseGet () override
	{
		return m_monitorTask;
	}

private:

	//
	// Event Handlers
	//
	void eventReadRemote () override {};
//	void eventMfddrvdStatusChanged ();

//	bool m_mfddrvdRunning = false;

	CstiMonitorTask *m_monitorTask {nullptr};

};
