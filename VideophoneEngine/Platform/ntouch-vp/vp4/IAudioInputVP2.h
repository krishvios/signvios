/*!
 * \file IAudioInputVP2.h
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2018 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "IstiAudioInput.h"


class IAudioInputVP2 : public IstiAudioInput
{
public:

	/*!
	 * \brief Set the input layer to unconditionally use the onboard headset microphone
	 *
	 * \param set - true to set the value, false to let the underlying system determine
	 *
	 * \return stiRESULT_SUCCESS or an error code.
	 */
	virtual stiHResult headsetMicrophoneForce(bool set) = 0;

};

