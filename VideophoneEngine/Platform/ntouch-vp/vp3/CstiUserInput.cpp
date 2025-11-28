// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved

#include "CstiUserInput.h"
#include "CstiMonitorTask.h"
#include "stiTrace.h"
#include "CstiCECControl.h"

#include <libcec/cectypes.h>
#include <linux/input.h>
#include <errno.h>
#include <string.h>

CstiUserInput::CstiUserInput()
:
	CstiUserInputBase ("gpio_ir_recv", "gpio-keys")
{
}

stiHResult CstiUserInput::Initialize(
	CstiMonitorTask *monitorTask,
	std::shared_ptr<CstiCECControl> cecControl)
{
	m_monitorTask = monitorTask;

	m_signalConnections.push_back(cecControl->cecKeyPressSignal.Connect(
		[this](const CEC::cec_keypress &key) {
			eventReadCEC(key);
		})
	);

	SstiKeyMap newEntry { IstiUserInput::eSMRK_BSP_ENTER, false };

	m_sMpuMap.emplace(KEY_LEFTSHIFT, newEntry);

	return CstiUserInputBase::Initialize();
}

MonitorTaskBase *CstiUserInput::monitorTaskBaseGet ()
{
	return m_monitorTask;
}

std::map<CEC::cec_user_control_code, IstiUserInput::RemoteButtons> cecKeyMap
{
	{ CEC::CEC_USER_CONTROL_CODE_SELECT, IstiUserInput::eSMRK_ENTER },
	{ CEC::CEC_USER_CONTROL_CODE_ENTER, IstiUserInput::eSMRK_ENTER },
	{ CEC::CEC_USER_CONTROL_CODE_UP, IstiUserInput::eSMRK_UP },
	{ CEC::CEC_USER_CONTROL_CODE_DOWN, IstiUserInput::eSMRK_DOWN },
	{ CEC::CEC_USER_CONTROL_CODE_LEFT, IstiUserInput::eSMRK_LEFT },
	{ CEC::CEC_USER_CONTROL_CODE_RIGHT, IstiUserInput::eSMRK_RIGHT },
	{ CEC::CEC_USER_CONTROL_CODE_NUMBER0, IstiUserInput::eSMRK_ZERO },
	{ CEC::CEC_USER_CONTROL_CODE_NUMBER1, IstiUserInput::eSMRK_ONE },
	{ CEC::CEC_USER_CONTROL_CODE_NUMBER2, IstiUserInput::eSMRK_TWO },
	{ CEC::CEC_USER_CONTROL_CODE_NUMBER3, IstiUserInput::eSMRK_THREE },
	{ CEC::CEC_USER_CONTROL_CODE_NUMBER4, IstiUserInput::eSMRK_FOUR },
	{ CEC::CEC_USER_CONTROL_CODE_NUMBER5, IstiUserInput::eSMRK_FIVE },
	{ CEC::CEC_USER_CONTROL_CODE_NUMBER6, IstiUserInput::eSMRK_SIX },
	{ CEC::CEC_USER_CONTROL_CODE_NUMBER7, IstiUserInput::eSMRK_SEVEN },
	{ CEC::CEC_USER_CONTROL_CODE_NUMBER8, IstiUserInput::eSMRK_EIGHT },
	{ CEC::CEC_USER_CONTROL_CODE_NUMBER9, IstiUserInput::eSMRK_NINE },
	{ CEC::CEC_USER_CONTROL_CODE_EXIT, IstiUserInput::eSMRK_BACK_CANCEL },
};

void CstiUserInput::eventReadCEC (const CEC::cec_keypress &keyPress)
{
	auto iter = cecKeyMap.find(keyPress.keycode);

	stiDEBUG_TOOL(g_stiUserInputDebug,
		stiTrace("%s: got keypress %d\n", __func__, keyPress.keycode);
	);

	if (iter != cecKeyMap.end())
	{
		m_nKey = iter->second;
		if (keyPress.duration)			// key release event
		{
			m_repeatButtonTimer.stop();
			m_lastKeyPress = -1;
		}
		else							// key press (or hold) event
		{
			if (m_lastKeyPress == -1)
			{
				inputSignal.Emit (m_nKey);
				m_repeatButtonTimer.timeoutSet (stiREPEAT_BUTTON_DELAY_INITIAL);
				m_repeatButtonTimer.restart();
				m_lastKeyPress = static_cast<int>(keyPress.keycode);
			}
		}
	}
	else
	{
		stiDEBUG_TOOL(g_stiUserInputDebug,
			stiTrace("%s: got a key we're not interested in %d\n", __func__, m_lastKeyPress);
		);
		m_lastKeyPress = -1;
	}
}

void CstiUserInput::eventReadRemote ()
{
	struct input_event event;

	auto numberBytesRead = stiOSRead (m_remoteFd, (void *)&event, sizeof (struct input_event));

	if (numberBytesRead < 0)
	{
		stiDEBUG_TOOL(g_stiUserInputDebug,
			stiTrace("CstiUserInput:eventReadRemote: Read error: %s\n", strerror(errno));
		);
		close(m_remoteFd);
		m_remoteFd = -1;
	}
	else if (numberBytesRead < (signed int)sizeof(struct input_event))
	{
		stiASSERTMSG (false, "Number of bytes read: %d\n", numberBytesRead);
	}
	else
	{
		remoteEventProcess (event, EV_MSC);
	}
}


int CstiUserInput::shortKeyPressRemap (int key)
{
	if (key == IstiUserInput::eSMRK_BSP_ENTER)
	{
		return IstiUserInput::eSMRK_SVRS;
	}

	return key;
}

int CstiUserInput::longKeyPressRemap (int key)
{
	if (key == IstiUserInput::eSMRK_BSP_ENTER)
	{
		return IstiUserInput::eSMRK_CALL_CUSTOMER_SERVICE;
	}

	return key;
}
