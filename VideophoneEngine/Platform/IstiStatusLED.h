/*!
 * \file IstiStatusLED.h                         
 * \brief Declaration of the Status LED interface
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef ISTISTATUSLED_H
#define ISTISTATUSLED_H

//
// Includes
//
#include "stiError.h"

//
// Forward Declarations
//

//
// Globals
//


/*! 
 * \ingroup PlatformLayer
 * \brief This class represents the status LED devices
 *  
 */
class IstiStatusLED
{
public:

	IstiStatusLED (const IstiStatusLED &other) = delete;
	IstiStatusLED (IstiStatusLED &&other) = delete;
	IstiStatusLED &operator= (const IstiStatusLED &other) = delete;
	IstiStatusLED &operator= (IstiStatusLED &&other) = delete;

	/*! 
	*  \brief This enumeration defines the LEDs
	* 
	*  \enum EstiLed
	*/ 
	enum EstiLed
	{
		estiLED_CAMERA_ACTIVE = 0, 	//!< Act on the "Active" LED.
		estiLED_POWER,  			//!< Act on the "Power" LED.
		estiLED_STATUS, 			//!< Act on the "Status" LED.
		estiLED_SIGNMAIL,
		estiLED_MISSED_CALL,
		estiLED_LAST,				//!< Invalid LED type, only use to know how many LEDs we support
	};
	
	/*!
	 * \brief 
	 * 
	 * \return IstiStatusLED* 
	 */
	static IstiStatusLED *InstanceGet ();

	/*!
	 * \brief Start the LED Blinking
	 * 
	 * \param eLED 
	 * \param unBlinkRate 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult Start(
		EstiLed eLED,
		unsigned unBlinkRate) = 0;
		
	/*!
	 * \brief Stop the LED blinking
	 * 
	 * \param eLED 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult Stop(
		EstiLed eLED) = 0;

	/*!
	 * \brief Enables or disables the specified LED.
	 * When the LED is re-enabled the last specified blink rate will continue.
	 * 
	 * \param eLED The LED to enable or disable.
	 * \param bEnable
	 * 
	 * \return Success or failure
	 */
	virtual stiHResult Enable (
		EstiLed eLED,
		EstiBool bEnable) = 0;

	/*!
	 * \brief Enables/disables the SignMail/Missed Call notification LED on the Pulse device
	 *
	 * \param enable Enables/disables Pulse LEDs
	 *
	 * \return stiHResult
	 */
	virtual stiHResult PulseNotificationsEnable(
		bool enable) = 0;

	virtual void ColorSet(EstiLed eLED, uint8_t red, uint8_t green, uint8_t blue)
	{
		stiUNUSED_ARG (eLED);
		stiUNUSED_ARG (red);
		stiUNUSED_ARG (green);
		stiUNUSED_ARG (blue);
	}
	
protected:

	IstiStatusLED () = default;
	virtual ~IstiStatusLED () = default;
};

#endif // ISTISTATUSLED_H
