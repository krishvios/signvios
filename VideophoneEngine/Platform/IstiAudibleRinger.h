/*!
 * \file IstiAudibleRinger.h 
 * \brief Declaration of the Audible Ringer interface 
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 * 
 */
#ifndef ISTIAUDIBLERINGER_H
#define ISTIAUDIBLERINGER_H

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
 * \brief This structure describes a single note for the audible ringer.
 */
struct SstiTone
{
	unsigned long un32Frequency; //! Frequency
	unsigned char un8Volume;	 //! Remove this? complicates things on VIS
	unsigned char un8Duration;   //! Duration
};


/*!
 * \ingroup PlatformLayer
 * \brief This class represents the audible ringer
 *  
 */
class IstiAudibleRinger
{
public:
	/*!
	 * \brief Get the instance of the Audible Ringer object.
	 * 
	 * \return IstiAudibleRinger* 
	 */
	static IstiAudibleRinger *InstanceGet ();

	/*!
	 * \brief Starts Audible Ringer.
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult Start() = 0;	 

	/*!
	 * \brief Stops Audible Ringer.
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult Stop() = 0;	 

	/*! 
	* \enum EstiTone
	* 
	* \brief Set tone type
	* 
	*/ 
	enum EstiTone
	{
		estiTONE_RING = 0,   //! Type is a ring tone (03/20/14 JMG: Keeping this here just in case anybody still uses this.)

		estiTONE_LOW = 0,    //! Low Pitch
		estiTONE_MEDIUM,     //! Medium Pitch
		estiTONE_HIGH,       //! High Pitch

		estiTONE_MAX
	};
	
	/*!
	 * \brief Sets 
	 * 
	 * \param eTone Ring or custom tone
	 * \param pstiTone Pointer to the array of tones structures
	 * \param unCount Number of entries in the array of tones
	 * \param unRepeatCount Number of times the array of tones should be played before stopping
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult TonesSet (
		EstiTone eTone,
		const SstiTone *pstiTone,
		unsigned int unCount,
		unsigned int unRepeatCount
		) = 0;

#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
	virtual stiHResult PitchSet (
		int nPitch)
	{
		stiUNUSED_ARG (nPitch);

		return stiRESULT_SUCCESS;
	}


	virtual stiHResult VolumeSet (
		int nVolume)
	{
		stiUNUSED_ARG (nVolume);

		return stiRESULT_SUCCESS;
	} 
#endif // APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4

protected:

};

#endif // ISTIAUDIBLERINGER_H
