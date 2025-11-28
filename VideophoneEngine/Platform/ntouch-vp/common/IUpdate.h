/*!
 *
 * \file IUpdate.h
 * \brief Declaration of the Update interface
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*/
//#pragma once

#ifndef IUPDATE_H
#define IUPDATE_H

//
// Includes
//
#include "stiError.h"
#include "stiSVX.h"
#include "CstiSignal.h"
#include <list>

//
// Forward Declarations
//

//
// External Declarations
//

//
// Globals
//


///
/// \brief Class declaration for IUpdate
///
/// Note that the functions are pure virutal and
/// each function placed will need to be instantiated
/// in CstiUpdate.cpp


class IUpdate
{
public:

	IUpdate (const IUpdate &other) = delete;
	IUpdate (IUpdate &&other) = delete;
	IUpdate &operator= (const IUpdate &other) = delete;
	IUpdate &operator= (IUpdate &&other) = delete;

	/*!
	 * \brief Get instance
	 * 
	 * \return IUpdate* 
	 */
	static IUpdate *InstanceGet ();

	//
	// Methods called by the Application
	//
	/*!
	 * \brief Start the update progress if one is available
	 *
	 * \return stiHResult
	 */
	virtual void updateDownload () = 0;
	virtual void updateInstall () = 0;
	virtual void update (const std::string &url) = 0;

	/*!
	 * \brief Pauses the download of a firmware update if one is in progress
	 *
	 * \return stiHResult
	 */
	virtual void pause () = 0;

	/*!
	 * \brief Resume the download of a firmware update if one is in progress
	 *
	 * \return stiHResult
	 */
	virtual void resume () = 0;
	
	//
	// Signals
	//
	CstiSignal<bool> updateNeededSignal;
	CstiSignal<int> downloadingSignal;
	CstiSignal<> downloadSuccessfulSignal;
	CstiSignal<int> programmingSignal;
	CstiSignal<> successfulSignal;
	CstiSignal<> errorSignal;
	
protected:

	IUpdate () = default;
	virtual ~IUpdate () = default;

};

#endif // IUPDATE_H
