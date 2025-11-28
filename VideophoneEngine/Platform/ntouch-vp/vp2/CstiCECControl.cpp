// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2017 Sorenson Communications, Inc. -- All rights reserved

#include "CstiCECControl.h"

stiHResult CstiCECControl::Initialize(CstiMonitorTask *monitorTask, const std::string &device, const std::string &name)
{
	m_callbacks.Clear();
	m_callbacks.CBCecLogMessage = CstiCECControl::cecLogMessage;
	// m_callbacks.CBCecKeyPress = CstiCECControl::cecKeyPress;
	m_callbacks.CBCecCommand = CstiCECControl::cecCommand;
	m_callbacks.CBCecConfigurationChanged = CstiCECControl::cecConfigurationChanged;
	m_callbacks.CBCecAlert = CstiCECControl::cecAlert;
	m_callbacks.CBCecMenuStateChanged = CstiCECControl::cecMenuStateChanged;
	m_callbacks.CBCecSourceActivated = CstiCECControl::cecSourceActivated;

	return CstiCECControlBase::Initialize (monitorTask, device, name);
}

