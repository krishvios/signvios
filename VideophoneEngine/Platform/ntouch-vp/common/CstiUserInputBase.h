// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved

#pragma once

#include "IstiUserInput.h"
#include "CstiEventQueue.h"
#include "CstiEventTimer.h"
#include "CstiSignal.h"
#include <linux/input.h>

#include <map>


//
// Constants
//

#define stiUSER_INPUT_TASK_NAME             "stiUserInput"
#define stiMAX_FILENAME_LENGTH  64
#define stiMAX_EVENT_FILES      20

#define KEY_MASK			0x03f
#define IDENTIFIER_MASK		0x7c0
#define TOGGLE_MASK			0x800

//
// Typedefs
//
	
//
// Forward Declarations
//
class MonitorTaskBase;

//
// Globals
//

struct SstiKeyMap
{
	int nKey;
	bool bRepeatable;
};

//
// Class Declaration
//

class CstiUserInputBase : public CstiEventQueue, public IstiUserInput
{
public:

	CstiUserInputBase (
		const std::string &remoteDevInputName,
		const std::string &platformKeyDevInputName);

	~CstiUserInputBase () override;
	
	stiHResult Initialize ();
	
	CstiUserInputBase (const CstiUserInputBase &other) = delete;
	CstiUserInputBase (CstiUserInputBase &&other) = delete;
	CstiUserInputBase &operator= (const CstiUserInputBase &other) = delete;
	CstiUserInputBase &operator= (CstiUserInputBase &&other) = delete;

	stiHResult Startup ();

	ISignal<int> &inputSignalGet () override
	{
		return inputSignal;
	}

	ISignal<int> &remoteInputSignalGet () override
	{
		return remoteInputSignal;
	}

	ISignal<int,bool> &buttonStateSignalGet () override
	{
		return buttonStateSignal;
	}
	
protected:

	std::string m_remoteInputName;
	std::string m_platformKeyName;

	CstiSignalConnection::Collection m_signalConnections;

	CstiSignal<int> inputSignal;
	CstiSignal<int> remoteInputSignal;
	CstiSignal<int,bool> buttonStateSignal;

	int m_remoteFd {-1};

	int m_lastKeyPress {-1};
	int m_nKey {-1};

	vpe::EventTimer m_repeatButtonTimer;

	static const int stiREPEAT_BUTTON_DELAY_INITIAL {500};		// Initial repeat MPU button delay in milliseconds
	static const int stiREPEAT_BUTTON_DELAY_SUBSEQUENT {150};	// Subsequent repeat MPU button delay in milliseconds

	void remoteEventProcess (struct input_event, int);

	virtual MonitorTaskBase *monitorTaskBaseGet() = 0;

protected:
	std::map<int, SstiKeyMap> m_sMpuMap =
	{
		{KEY_ENTER, { IstiUserInput::eSMRK_BSP_ENTER, false }},
		{KEY_UP, { IstiUserInput::eSMRK_BSP_UP, true }},
		{KEY_DOWN, { IstiUserInput::eSMRK_BSP_DOWN, true }},
		{KEY_RIGHT, { IstiUserInput::eSMRK_BSP_RIGHT, true }},
		{KEY_LEFT, { IstiUserInput::eSMRK_BSP_LEFT, true }},
		{KEY_POWER, { IstiUserInput::eSMRK_BSP_POWER, false }}
	};

private:

	int m_mpuFd {-1};

	virtual void eventReadRemote () = 0;
	void eventReadMPU ();

	//
	// Event Handlers
	//
	void eventRepeatButtonTimer ();
	void eventKeyTimerHandle (int key);

	static const int REPEAT_KEY_DELAY_INITIAL {500000};			// Initial repeat key delay in microseconds
	static const int REPEAT_KEY_DELAY_SUBSEQUENT {150000};		// Subsequent repeat key delay in microseconds

	int m_delayTime {REPEAT_KEY_DELAY_INITIAL};
	int m_lastToggle {-1};
	struct timeval m_lastInputTime {};
	struct timeval m_lastEmitTime {};

	std::map<int, std::shared_ptr<vpe::EventTimer>> m_keyTimers;

private:

	virtual int shortKeyPressRemap (int key);
	virtual int longKeyPressRemap (int key);

};
