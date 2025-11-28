// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved

/*!
 * \file CstiCECControl.h
 * \brief See CstiCECControl.cpp
 */

#pragma once

#include "CstiCECControlBase.h"

class CstiCECControl : public CstiCECControlBase
{
public:
	CstiCECControl() = default;

	~CstiCECControl() override = default;

	stiHResult Initialize(CstiMonitorTask *monitorTask, const std::string &device, const std::string &name);

	/*
	 * pass through functions for libcec
	 */

	static inline void logMessage (void *ptr, const CEC::cec_log_message *message)
	{
		auto self = static_cast<CstiCECControl *>(ptr);
		self->cecLogMessage(ptr, *message);
	}

	static inline void commandReceived (void *ptr, const CEC::cec_command *command)
	{
		auto self = static_cast<CstiCECControl *>(ptr);
		self->cecCommand(ptr, *command);
	}

	static inline void configurationChanged (void *ptr, const CEC::libcec_configuration *config)
	{
		auto self = static_cast<CstiCECControl *>(ptr);
		self->cecConfigurationChanged(ptr, *config);
	}

	static inline void alert (void *ptr, const CEC::libcec_alert alert, const CEC::libcec_parameter p)
	{
		auto self = static_cast<CstiCECControl *>(ptr);
		self->cecAlert(ptr, alert, p);
	}

	static void keyPress (void *ptr, const CEC::cec_keypress *key)
	{
		auto self = static_cast<CstiCECControl *>(ptr);
		self->cecKeyPress(ptr, *key);
	}
};
