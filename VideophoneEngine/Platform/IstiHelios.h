/*!
 * \file IstiHelios.h 
 * \brief Declaration of the Helios interface 
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2017 Sorenson Communications, Inc. -- All rights reserved
 * 
 */
#ifndef ISTIHELIOS_H
#define ISTIHELIOS_H

//
// Includes
//

#include "stiDefs.h"
#include <string>
#include <vector>

//
// Forward Declarations
//

//
// Globals
//

/*!
 * \ingroup PlatformLayer
 * \brief This class represents the Helios manager
 *  
 */
class IstiHelios
{
public:

	IstiHelios (const IstiHelios &other) = delete;
	IstiHelios (IstiHelios &&other) = delete;
	IstiHelios &operator= (const IstiHelios &other) = delete;
	IstiHelios &operator= (IstiHelios &&other) = delete;

	/*!
	 * \brief Get the instance of the Helios Ringer object.
	 * 
	 * \return IstiHelios*
	 */
	static IstiHelios *InstanceGet ();

	/*!
	 * \brief Starts Helios Ringer with custom pattern
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult CustomPatternStart (
		const std::vector<std::string> &frames) = 0;

	/*!
	 * \brief Start Helios Ringer with standard pattern
	 *
	 * arg1 - default pattern number
	 * arg2 - red
	 * arg3 - green
	 * arg4 - blue
	 * arg5 - brighness
	 *
	 * \return stiHResult
	 */

	virtual stiHResult PresetPatternStart (
		int pattern,
		unsigned char r,
		unsigned char g,
		unsigned char b,
		unsigned char brightness) = 0;

	/*!
	 * \brief Stops Helios Ringer.
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult Stop () = 0;

	/*!
	 * \brief sets the SignMail light on or off
	 *
	 * \return stiHResult
	 */
	virtual stiHResult SignMailSet (bool set) = 0;

	/*!
	 * \brief sets the  MissedCall light on or off
	 *
	 * \return stiHResult
	 */
	virtual stiHResult MissedCallSet (bool set) = 0;

protected:

	IstiHelios () = default;
	virtual ~IstiHelios () = default;

};

#endif // ISTIHELIOS_H
