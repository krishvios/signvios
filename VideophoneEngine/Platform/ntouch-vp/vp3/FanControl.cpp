// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2020-2021 Sorenson Communications, Inc. -- All rights reserved

#include "FanControl.h"
#include "stiTools.h"
#include <fstream>

#define DUTY_CYCLE_PATH "/sys/class/pwm/pwmchip0/pwm3/duty_cycle"

#define PWM_PERIOD 40000
#define DUTY_CYCLE_MULTIPLIER (PWM_PERIOD/100)


int FanControl::dutyCycleGet ()
{
	auto dutyCycle {0};
	std::ifstream ifs {DUTY_CYCLE_PATH, std::ifstream::in};

	if (ifs.is_open ())
	{
		std::string line;
		std::getline (ifs, line);

		try
		{
			dutyCycle = std::stoi (line);
		}
		catch (...)
		{
			vpe::trace ("error reading fan duty cycle.\n");
		}
    }

	return dutyCycle / DUTY_CYCLE_MULTIPLIER;
}

stiHResult FanControl::dutyCycleSet (int dutyCycle)
{
	auto hResult {stiRESULT_SUCCESS};
	std::string command {};

	stiTESTCOND_NOLOG (m_testMode, stiRESULT_ERROR);
	stiTESTCOND_NOLOG (dutyCycle >= 0 && dutyCycle <= 100, stiRESULT_INVALID_PARAMETER);

	command = "echo " + std::to_string (dutyCycle * DUTY_CYCLE_MULTIPLIER) + " > " + DUTY_CYCLE_PATH;
	
	if (system (command.c_str ()))
	{
		stiASSERTMSG (false, "error: %s\n", command.c_str ());
	}

STI_BAIL:

	return hResult;
}

void FanControl::testModeSet (bool testMode)
{
	if (testMode != m_testMode)
	{
		m_testMode = testMode;

		if (m_testMode)
		{
			thermaldStop ();
			dutyCycleSet (50);
		}
		else
		{
			thermaldStart ();
		}
		
	}
}

void FanControl::thermaldStart ()
{
	std::string command = "systemctl start thermald &";
	
	if (system (command.c_str ()))
	{
		stiASSERTMSG (false, "error: %s\n", command.c_str ());
	}
}

void FanControl::thermaldStop ()
{
	std::string command = "systemctl stop thermald &";
	
	if (system (command.c_str ()))
	{
		stiASSERTMSG (false, "error: %s\n", command.c_str ());
	}
}
