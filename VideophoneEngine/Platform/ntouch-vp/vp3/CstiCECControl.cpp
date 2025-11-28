// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved

#include "CstiCECControl.h"

stiHResult CstiCECControl::Initialize(CstiMonitorTask *monitorTask, const std::string &device, const std::string &name)
{
	m_callbacks.Clear();
	m_callbacks.logMessage = CstiCECControl::logMessage;
	// m_callbacks.keyPress = CstiCECControl::keyPress;
	m_callbacks.commandReceived = CstiCECControl::commandReceived;
	m_callbacks.configurationChanged = CstiCECControl::configurationChanged;
	m_callbacks.alert = CstiCECControl::alert;
	m_callbacks.menuStateChanged = CstiCECControl::cecMenuStateChanged;
	m_callbacks.sourceActivated = CstiCECControl::cecSourceActivated;

	return CstiCECControlBase::Initialize (monitorTask, device, name);
}

