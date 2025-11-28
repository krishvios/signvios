// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

/*!
 * \file MonitorTaskBase2.h
 * \brief See MonitorTaskBase2.cpp
 */

#ifndef CMONITOR_TASK_BASE2_H
#define CMONITOR_TASK_BASE2_H

//
// Includes
//
#include "MonitorTaskBase.h"
#include "IstiVideoOutput.h"
#include "IstiPlatform.h"
#include "stiTools.h"
#include "CstiTimer.h"
#include <cstdint>
#include <memory>


//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//
struct udev;
struct udev_monitor;

//
// Globals
//
class MonitorTaskBase2 : public MonitorTaskBase
{
public:

	MonitorTaskBase2 () = default;

	~MonitorTaskBase2 () override;

	virtual stiHResult Startup ();

	virtual stiHResult Shutdown ();

	stiHResult Initialize ();

	virtual stiHResult HdmiInStatusGet (
		int *pnHdmiInStatus);

	stiHResult RcuConnectedStatusGet (
		bool *pbRcuConnected) override;

	bool HdmiHotPlugGet ();

	void headsetInitialStateGet();
	bool microphoneConnectedGet ();
	bool headphoneConnectedGet ();

public:
	//
	// Event Signals
	//
	CstiSignal<> mfddrvdStatusChangedSignal;
	CstiSignal<int> hdmiInStatusChangedSignal;
	CstiSignal<> hdmiHotPlugStateChangedSignal;
	CstiSignal<bool> microphoneConnectedSignal;
	CstiSignal<bool> headphoneConnectedSignal;

protected: 

	virtual bool isCameraAvailable () = 0;

	std::string systemExecute (const std::string &cmd);

private:
	
	void HdmiOutHotPlugCheck ();

	void EventUdevReadHandler ();
		
	void micEventReadHandler ();
	void headphoneEventReadHandler ();
	int readHeadsetState (
		int fd);
	int getHeadsetFd (
		int type);

	void headsetCurrentStateGet(int type, int tmpFd);

	bool m_bInitialized = false;
	
	bool m_bRcuConnected = false;
	
	int m_udevFd = -1;
	struct udev *m_pUdev = nullptr;
	struct udev_monitor *m_pUdevMonitor = nullptr;

	bool m_bHotPlugDetected = false;

	int m_microphone = -1;
	int m_headphone = -1;

	int m_microphoneFd = -1;
	int m_headphoneFd = -1;

};


#endif //#ifndef CMONITOR_TASK_BASE2_H
