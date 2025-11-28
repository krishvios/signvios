// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved

#include "CstiUserInputBase.h"
#include "stiTrace.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "stiTools.h"
#include "CstiExtendedEvent.h"
#include <sstream>

#define REMOTE_IDENTIFIER	0x2c0			// 0x2c0 >> 6 = 0xb
#define LONG_PRESS_MS 2000
#define REMOTE_TIMEOUT_US 250000

std::map<int, SstiKeyMap> sRemoteMap =
{
	{0x01, { IstiUserInput::eSMRK_CANCEL, false }},
	{0x04, { IstiUserInput::eSMRK_AUDIO_PRIV, false }},
	{0x05, { IstiUserInput::eSMRK_STATUS, false }},
	{0x06, { IstiUserInput::eSMRK_FAVORITES, false }},
	{0x07, { IstiUserInput::eSMRK_SHOW_CONTACTS, false }},
	{0x08, { IstiUserInput::eSMRK_KEYBOARD, false }},
	{0x09, { IstiUserInput::eSMRK_SEARCH, false}},
	{0x0d, { IstiUserInput::eSMRK_ENTER, false }},
	{0x0e, { IstiUserInput::eSMRK_CUSTOMER_SERVICE_INFO, false }},
	{0x0f, { IstiUserInput::eSMRK_VIDEO_CENTER, false }},
	{0x10, { IstiUserInput::eSMRK_SATURATION, false }},
	{0x11, { IstiUserInput::eSMRK_SHOW_MAIL, false }},
	{0x12, { IstiUserInput::eSMRK_DND, false }},
	{0x13, { IstiUserInput::eSMRK_SETTINGS, false }},
	{0x14, { IstiUserInput::eSMRK_CALL_HISTORY, false }},
	{0x15, { IstiUserInput::eSMRK_MISSED_CALLS, false }},
	{0x17, { IstiUserInput::eSMRK_BRIGHTNESS, false }},
	{0x19, { IstiUserInput::eSMRK_ZOOM_OUT, true }},
	{0x1a, { IstiUserInput::eSMRK_ZOOM_IN, true }},
	{0x1b, { IstiUserInput::eSMRK_PAN_LEFT, true }},
	{0x1c, { IstiUserInput::eSMRK_PAN_RIGHT, true }},
	{0x20, { IstiUserInput::eSMRK_CALL, false }},
	{0x21, { IstiUserInput::eSMRK_HANGUP, false }},
	{0x22, { IstiUserInput::eSMRK_TILT_UP, true }},
	{0x23, { IstiUserInput::eSMRK_TILT_DOWN, true }},
	{0x25, { IstiUserInput::eSMRK_VIEW_MODE, false }},
	{0x26, { IstiUserInput::eSMRK_BACK_CANCEL, true }},
	{0x27, { IstiUserInput::eSMRK_FAR_FLASH, false }},
	{0x28, { IstiUserInput::eSMRK_HOME, false }},
	{0x29, { IstiUserInput::eSMRK_VIDEO_PRIV, false }},
	{0x2a, { IstiUserInput::eSMRK_UP, true }},
	{0x2b, { IstiUserInput::eSMRK_LEFT, true }},
	{0x2c, { IstiUserInput::eSMRK_RIGHT, true }},
	{0x2d, { IstiUserInput::eSMRK_DOWN, true }},
	{0x2e, { IstiUserInput::eSMRK_FOCUS, false }},
	{0x30, { IstiUserInput::eSMRK_ZERO, true }},
	{0x31, { IstiUserInput::eSMRK_ONE, true }},
	{0x32, { IstiUserInput::eSMRK_TWO, true }},
	{0x33, { IstiUserInput::eSMRK_THREE, true }},
	{0x34, { IstiUserInput::eSMRK_FOUR, true }},
	{0x35, { IstiUserInput::eSMRK_FIVE, true }},
	{0x36, { IstiUserInput::eSMRK_SIX, true }},
	{0x37, { IstiUserInput::eSMRK_SEVEN, true }},
	{0x38, { IstiUserInput::eSMRK_EIGHT, true }},
	{0x39, { IstiUserInput::eSMRK_NINE, true }},
	{0x3a, { IstiUserInput::eSMRK_STAR, false }},
	{0x3b, { IstiUserInput::eSMRK_POUND, false }},
	{0x0c, { IstiUserInput::eSMRK_VIEW_POS, false }},
	{0x3d, { IstiUserInput::eSMRK_CAMERA_CONTROLS, false }},
	{0x3e, { IstiUserInput::eSMRK_EXIT_UI, false }},
};


CstiUserInputBase::CstiUserInputBase (
	const std::string &remoteDevInputName,
	const std::string &platformKeyDevInputName)
:
	CstiEventQueue (stiUSER_INPUT_TASK_NAME),
	m_remoteInputName (remoteDevInputName),
	m_platformKeyName (platformKeyDevInputName),
	m_repeatButtonTimer (0, this)
{
}


stiHResult CstiUserInputBase::Initialize()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	int i;
	int tmpFd;
	char returnedName[stiMAX_FILENAME_LENGTH + 1];

	for (i = 0; i < stiMAX_EVENT_FILES; i++)
	{
		std::stringstream fileName;

		fileName << "/dev/input/event" << i;

		tmpFd = open (fileName.str ().c_str (), O_RDONLY);

		if (tmpFd < 0)
		{
			break;
		}

		if (ioctl(tmpFd, EVIOCGNAME(sizeof(returnedName)), returnedName) < 0)
		{
			close (tmpFd);
			tmpFd = -1;
		}
		else if (returnedName == m_remoteInputName)
		{
			stiDEBUG_TOOL(g_stiUserInputDebug,
				vpe::trace("CstiUserInputBase: remote event file = ", fileName.str (), "\n");
			);

			m_remoteFd = tmpFd;
		}
		else if (returnedName == m_platformKeyName)
		{
			stiDEBUG_TOOL(g_stiUserInputDebug,
				vpe::trace("CstiUserInputBase: platform key event file = ", fileName.str (), "\n");
			);

			m_mpuFd = tmpFd;
		}
		else
		{
			close (tmpFd);
			tmpFd = -1;
		}
	}

	stiASSERT (m_remoteInputName.empty () || m_remoteFd != -1);
	stiASSERT (m_platformKeyName.empty () || m_mpuFd != -1);
	stiASSERT (i < stiMAX_EVENT_FILES);

	if (m_remoteFd != -1)
	{
		if (!FileDescriptorAttach (m_remoteFd, std::bind(&CstiUserInputBase::eventReadRemote, this)))
		{
			stiASSERTMSG (estiFALSE, "%s: Can't attach remoteFd %d\n", __func__, m_remoteFd);
		}
	}

	if (m_mpuFd != -1)
	{
		if (!FileDescriptorAttach (m_mpuFd, std::bind(&CstiUserInputBase::eventReadMPU, this)))
		{
			stiASSERTMSG (estiFALSE, "%s: Can't attach mpuFd %d\n", __func__, m_mpuFd);
		}
	}

	m_signalConnections.push_back (m_repeatButtonTimer.timeoutSignal.Connect (
			std::bind(&CstiUserInputBase::eventRepeatButtonTimer, this)));

	return hResult;
}


CstiUserInputBase::~CstiUserInputBase ()
{
	StopEventLoop ();

	if (m_remoteFd > -1)
	{
		FileDescriptorDetach (m_remoteFd);

		close (m_remoteFd);
		m_remoteFd = -1;
	}

	if (m_mpuFd > -1)
	{
		FileDescriptorDetach (m_mpuFd);

		close (m_mpuFd);
		m_mpuFd = -1;
	}
}


stiHResult CstiUserInputBase::Startup ()
{
	StartEventLoop ();

	return stiRESULT_SUCCESS;
}


// Base class implementation simply returns key code as received
int CstiUserInputBase::shortKeyPressRemap (int key)
{
	return key;
}


// Base class implementation simply returns key code as received
int CstiUserInputBase::longKeyPressRemap (int key)
{
	return key;
}


void CstiUserInputBase::remoteEventProcess (struct input_event Event, int eventType)
{
	stiDEBUG_TOOL(g_stiUserInputDebug,
		stiTrace("CstiUserInputBase::Task: input is from remote\n");
	);

	SstiKeyMap stKeyValue {};

	if (Event.code == eventType && (Event.value & IDENTIFIER_MASK) == REMOTE_IDENTIFIER)
	{
		// If input hasn't been received in more than
		// REMOTE_TIMEOUT_US, treat as a new key press
		timeval Diff {};
		timevalSubtract (&Diff, &Event.time, &m_lastInputTime);
		
		auto elapsedMicroseconds = Diff.tv_sec * 1000000 + Diff.tv_usec;
		if (elapsedMicroseconds > REMOTE_TIMEOUT_US)
		{
			m_lastToggle = -1;
			m_lastKeyPress = -1;
		}
		m_lastInputTime = Event.time;
		
		// Get key and toggle for this input
		int maskedValue = Event.value & KEY_MASK;
		int thisToggle = Event.value & TOGGLE_MASK;

		stiDEBUG_TOOL(g_stiUserInputDebug,
			stiTrace("CstiUserInputBase-> remote masked value = 0x%x, toggle = %d\n", maskedValue, thisToggle);
		);

		if (sRemoteMap.count(maskedValue) == 0)
		{
			m_lastToggle = -1;
			m_lastKeyPress = -1;
			maskedValue = -1;
		}
		else
		{
			stKeyValue = sRemoteMap[maskedValue];
			m_nKey = stKeyValue.nKey;
			stiDEBUG_TOOL(g_stiUserInputDebug,
				stiTrace("CstiUserInputBase::Task: ThisToggle = %d, LastToggle = %d\n", thisToggle, m_lastToggle);
			);

			if (maskedValue != m_lastKeyPress || thisToggle != m_lastToggle)	// it's a separate remote button press
			{
				m_lastToggle = thisToggle;
				m_lastKeyPress = maskedValue;
				m_delayTime = REPEAT_KEY_DELAY_INITIAL;

				stiDEBUG_TOOL(g_stiUserInputDebug,
					stiTrace("CstiUserInputBase::Task: sending key %d\n", m_nKey);
				);

				inputSignal.Emit (m_nKey);
				m_lastEmitTime = Event.time;

				remoteInputSignal.Emit (m_nKey);
			}
			else							// held the button down
			{
				//
				// If this is a repeatable key then check to see if it has been
				// long enough since that last time we generated a key press
				//
				stiDEBUG_TOOL(g_stiUserInputDebug,
					stiTrace("CstiUserInputBase::Task: repeatable = %s\n", stKeyValue.bRepeatable ? "true" : "false");
				);

				if (stKeyValue.bRepeatable)
				{
					timevalSubtract (&Diff, &Event.time, &m_lastEmitTime);

					elapsedMicroseconds = Diff.tv_sec * 1000000 + Diff.tv_usec;

					if (elapsedMicroseconds >= m_delayTime)
					{
						m_delayTime = REPEAT_KEY_DELAY_SUBSEQUENT;
						stiDEBUG_TOOL(g_stiUserInputDebug,
							stiTrace("CstiUserInputBase::Task: sending repeated key %d\n", m_nKey);
						);
						inputSignal.Emit (m_nKey);
						m_lastEmitTime = Event.time;
					}
					else
					{
						stiDEBUG_TOOL(g_stiUserInputDebug,
							stiTrace("CstiUserInputBase::Task: time between repeats is not long enough, Elapsed: %d, Delay: %d\n", elapsedMicroseconds, m_delayTime);
						);
					}
				}
			}
		}
	}
	else
	{
		if (Event.type != EV_SYN)
		{
			stiDEBUG_TOOL(g_stiUserInputDebug,
				stiTrace("CstiUserInputBase::Task: Invalid remote data: Event.code = %d, Event.value = 0x%02x\n", Event.code, Event.value);
			);
		}
		else
		{
			stiDEBUG_TOOL(g_stiUserInputDebug,
				stiTrace("CstiUserInputBase::Task: type was EV_SYN\n");
			);
		}
	}
}


void CstiUserInputBase::eventReadMPU ()
{
	struct input_event Event {};
	SstiKeyMap stKeyValue {};

	//stiTrace("CstiUserInput-> input was from keypad\n");
	auto numberBytesRead = stiOSRead(m_mpuFd, (void *)&Event, sizeof(struct input_event));

	if (numberBytesRead < 0)
	{
		stiASSERTMSG (false, "Errno = %d\n", errno);
	}
	else if (numberBytesRead < (signed int)sizeof(struct input_event))
	{
		stiASSERTMSG (false, "Number of bytes read: %d\n", numberBytesRead);
	}
	else if (Event.type == EV_KEY)
	{
		m_lastKeyPress = Event.code;
		if (m_sMpuMap.count(m_lastKeyPress) == 0)
		{
			m_lastKeyPress = -1;
			stiTrace("Received unknown -MPU- keypress %d (0x%2.2x", Event.code, Event.code);
			if (isprint(Event.code))
			{
				stiTrace(", '%c'", Event.code);
			}
			stiTrace(")\n");
		}
		else
		{
			stKeyValue = m_sMpuMap[m_lastKeyPress];

			if (stKeyValue.bRepeatable)
			{
				m_nKey = stKeyValue.nKey;
				if (Event.value == 0)			// key release event
				{
					m_repeatButtonTimer.stop();
					m_lastKeyPress = -1;
				}
				else if (Event.value == 1)		// key press event
				{
					inputSignal.Emit (m_nKey);

					m_repeatButtonTimer.timeoutSet (stiREPEAT_BUTTON_DELAY_INITIAL);
					m_repeatButtonTimer.restart();
				}
			}
			else // Not repeatable...short press or long press
			{
				auto key = stKeyValue.nKey;
				
				if (Event.value == 1)
				{
					auto itr = m_keyTimers.find (key);

					if (itr != m_keyTimers.end ())
					{
						// This key has a timer. Restart it.
						itr->second->restart ();
					}
					else
					{
						// Key timer is not in map. Emplace one.
						auto timer = std::make_shared<vpe::EventTimer>(LONG_PRESS_MS, this);

						m_keyTimers.emplace (key, timer);

						m_signalConnections.push_back (timer->timeoutSignal.Connect (
						[this, key]{eventKeyTimerHandle (key);}
						));

						timer->restart ();
					}
				}
				else if (Event.value == 0)
				{
					auto itr = m_keyTimers.find (key);
					
					if (itr != m_keyTimers.end () && itr->second->isActive ())
					{
						// A running key timer was found for this key (key was released before timer expired)
						// This is a short press. Stop the timer and emit.
						itr->second->stop ();
						inputSignal.Emit (shortKeyPressRemap (key));
					}
				}
			}

			// Emit button pressed/released for objects that need this information
			buttonStateSignal.Emit (stKeyValue.nKey, Event.value == 1);			
		}
	}
	else
	{
		stiDEBUG_TOOL(g_stiUserInputDebug,
			stiTrace("CstiUserInputBase::Task: Invalid mpu key data: Event.type = %d\n", Event.type);
		);
	}
}


/*!\brief Timer event handler
 *
 */
void CstiUserInputBase::eventRepeatButtonTimer ()
{
//	auto  nKeyPress = (int)((CstiExtendedEvent *)poEvent)->ParamGet (1);

	//
	// If this timer is a repeat key press event for the current key then
	// go ahead and do the callback and reset the timer. Otherwise, do nothing.
	//
//	if (nKeyPress == m_lastKeyPress)
	{
		inputSignal.Emit (m_nKey);
		m_repeatButtonTimer.timeoutSet (stiREPEAT_BUTTON_DELAY_SUBSEQUENT);
		m_repeatButtonTimer.restart();
	}
}


void CstiUserInputBase::eventKeyTimerHandle (int key)
{
	auto itr = m_keyTimers.find (key);

	if (itr != m_keyTimers.end ())
	{
		// This key timer expired before the button was released
		// This is a long press.
		inputSignal.Emit (longKeyPressRemap (key));
	}	
}
