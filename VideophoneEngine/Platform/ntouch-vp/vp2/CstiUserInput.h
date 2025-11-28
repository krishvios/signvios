// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved

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

	CstiUserInput ();
	~CstiUserInput () override = default;
	
	stiHResult Initialize (
		CstiMonitorTask *pMonitorTask);
	
	CstiUserInput (const CstiUserInput &other) = delete;
	CstiUserInput (CstiUserInput &&other) = delete;
	CstiUserInput &operator= (const CstiUserInput &other) = delete;
	CstiUserInput &operator= (CstiUserInput &&other) = delete;

protected:

	MonitorTaskBase *monitorTaskBaseGet () override;

private:

	//
	// Event Handlers
	//
	void eventMfddrvdStatusChanged ();
	void eventReadRemote () override;

private:

	bool m_mfddrvdRunning = false;
	CstiMonitorTask *m_monitorTask;
};
