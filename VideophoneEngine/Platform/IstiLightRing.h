/*!
 *
 *\file IstiLightRing.h
 *\brief Declaration of the LightRing interface
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef ISTILIGHTRING_H
#define ISTILIGHTRING_H

//
// Includes
//
#include "stiSVX.h"
#include "ISignal.h"


//
// Forward Declarations
//

//
// Globals
//


/*!
 * \ingroup PlatformLayer
 * \brief The class represents the light ring
 *  
 */
class IstiLightRing
{
public:

	enum class Pattern
	{
		Off,
		Wipe,
		FilledWipe,
		Flash,
		SingleChaser,
		DoubleChaser,
		HalfChaser,
		DarkChaser,
		Pulse,
		AlternateFlash,
		FillAndUnfill,
		Emergency,
		Test
	};

	enum class NotificationPattern
	{
		None = 0,
		SignMail = 0x1,
		MissedCall = 0x10,
		Both = SignMail | MissedCall
	};

	enum EstiLedColor
	{
		estiLED_USE_PATTERN_COLORS = -2,
		estiLED_COLOR_OFF = -1,
		estiLED_COLOR_WHITE = 0,
		estiLED_COLOR_TEAL,
		estiLED_COLOR_LIGHT_BLUE,
		estiLED_COLOR_BLUE,
		estiLED_COLOR_VIOLET,
		estiLED_COLOR_MAGENTA,
		estiLED_COLOR_ORANGE,
		estiLED_COLOR_YELLOW,
		estiLED_COLOR_RED,
		estiLED_COLOR_PINK,
		estiLED_COLOR_GREEN,
		estiLED_COLOR_LIGHT_GREEN,
		estiLED_COLOR_CYAN
	};

	/*!
	 * \brief Retrieves object pointer for this interface.
	 * 
	 * 
	 * \return IstiLightRing* 
	 */
	static IstiLightRing * InstanceGet ();

	/*!
	 * \brief Sets the pattern identified by the pattern index.
	 * 
	 * \param poLightRingSample 
	 * \param unCount 
	 */
	virtual void PatternSet (
		Pattern patternId,
		EstiLedColor color,
		int brightness,
		int flasherBrightness) = 0;

	/*!
	 * \brief Sets the pattern identified by the pattern name.
	 * 
	 * \param vectorName
	 */
	virtual void notificationPatternSet (
		NotificationPattern pattern) = 0;
		
	/*!
	 * \brief Starts the LED light ring for nSeconds
	 * 
	 * \param nSeconds 
	 */
	virtual void Start (			/// Starts the light ring.
		int nSeconds) = 0;
		
	/*!
	 * \brief Stops the LED Light ring
	 */
	virtual void Stop () = 0;	/// Stops the light ring.

	/*!
	 * \brief Start light ring notifications
	 * 
	 * \param nDuration 
	 * \param nBrightness 
	 */
	virtual void notificationsStart (			/// Starts the light ring.
		int nDuration,
		int nBrightness) = 0;
		
	/*!
	 * \brief Stop light ring notifications
	 */
	virtual void notificationsStop () = 0;	/// Stops the light ring.

	virtual ISignal<>& lightRingStoppedSignalGet () = 0;
	virtual ISignal<>& notificationStoppedSignalGet () = 0;

protected:
};

#endif // ISTILIGHTRING_H
