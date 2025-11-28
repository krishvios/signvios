// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved

#pragma once

#include "CstiUserInputBase.h"
#include "libcec/cectypes.h"

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
class CstiCECControl;

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
		CstiMonitorTask *pMonitorTask,
		std::shared_ptr<CstiCECControl> cecControl);
	
	CstiUserInput (const CstiUserInput &other) = delete;
	CstiUserInput (CstiUserInput &&other) = delete;
	CstiUserInput &operator= (const CstiUserInput &other) = delete;
	CstiUserInput &operator= (CstiUserInput &&other) = delete;

protected:

	MonitorTaskBase *monitorTaskBaseGet () override;

private:
	// Event handlers
	void eventReadCEC (const CEC::cec_keypress &keyPress);
	void eventReadRemote () override;

	int shortKeyPressRemap (int key) override;
	int longKeyPressRemap (int key) override;

private:

	CstiSignalConnection::Collection m_signalConnections;

	CstiMonitorTask *m_monitorTask {nullptr};
};
