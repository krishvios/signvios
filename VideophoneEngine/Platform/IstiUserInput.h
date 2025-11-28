// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "stiSVX.h"
#include "ISignal.h"

/*! 
 * \ingroup PlatformLayer
 * \brief This class represents the user input device
 * 
 */
class IstiUserInput
{
public:

	/*! 
	* \brief IR codes/signals
	*/ 
	enum RemoteButtons
	{
		eSMRK_POWER_TOGGLE = 1,
		eSMRK_SHOW_CONTACTS,
		eSMRK_SHOW_MISSED,
		eSMRK_SHOW_MAIL,
		eSMRK_FAR_FLASH,
		eSMRK_MENU,
		eSMRK_HOME,         // ntouch uses "Home" instead of "Menu"
		eSMRK_BACK_CANCEL,
		eSMRK_UP,
		eSMRK_LEFT,
		eSMRK_ENTER,
		eSMRK_RIGHT,
		eSMRK_DOWN,
		eSMRK_DISPLAY,
		eSMRK_STATUS,       // ntouch uses "Status" instead of "Display"
		eSMRK_KEYBOARD,
		eSMRK_ZERO,
		eSMRK_ONE,
		eSMRK_TWO,
		eSMRK_THREE,
		eSMRK_FOUR,
		eSMRK_FIVE,
		eSMRK_SIX,
		eSMRK_SEVEN,
		eSMRK_EIGHT,
		eSMRK_NINE,
		eSMRK_STAR,
		eSMRK_POUND,
		eSMRK_DOT,
		eSMRK_TILT_UP,
		eSMRK_PAN_LEFT,
		eSMRK_PAN_RIGHT,
		eSMRK_TILT_DOWN,
		eSMRK_NEAR,
		eSMRK_FAR,
		eSMRK_SPARE_A,
		eSMRK_SPARE_B,
		eSMRK_SPARE_C,
		eSMRK_SPARE_D,
		eSMRK_ZOOM_OUT,
		eSMRK_ZOOM_IN,
		eSMRK_SPEAKER,
		eSMRK_CLEAR,        // ntouch uses "Clear" instead of "Speaker"
		eSMRK_VIEW_SELF,
		eSMRK_VIEW_MODE,
		eSMRK_VIEW_POS,
		eSMRK_AUDIO_PRIV,
		eSMRK_DND,
		eSMRK_HELP,         // ntouch uses "Help" instead of "No Calls"
		eSMRK_VIDEO_PRIV,
		eSMRK_SVRS,
		eSMRK_EXIT_UI,
		eSMRK_FOCUS,
		eSMRK_CALL,
		eSMRK_HANGUP,
		eSMRK_BSP_ENTER,	// want to be able to distinguish between the bsp and remote button keys
		eSMRK_BSP_RIGHT,
		eSMRK_BSP_LEFT,
		eSMRK_BSP_UP,
		eSMRK_BSP_DOWN,
		eSMRK_VIDEO_CENTER,
		eSMRK_SATURATION,
		eSMRK_BRIGHTNESS,
		eSMRK_SETTINGS,
		eSMRK_CALL_HISTORY,
		eSMRK_FAVORITES,
		eSMRK_CANCEL,
		eSMRK_CUSTOMER_SERVICE_INFO,
		eSMRK_CALL_CUSTOMER_SERVICE,
		eSMRK_MISSED_CALLS,
		eSMRK_SEARCH,
		eSMRK_CAMERA_CONTROLS,
		eSMRK_BSP_POWER		// on lumina2, the power key is the svrs button
	};

	/*!
	 * \brief Get instance
	 * 
	 * \return IstiUserInput* 
	 */
	static IstiUserInput *InstanceGet ();

	virtual ISignal<int> &inputSignalGet () = 0;
	virtual ISignal<int> &remoteInputSignalGet () = 0;
	virtual ISignal<int,bool> &buttonStateSignalGet () = 0;

protected:

private:

};
