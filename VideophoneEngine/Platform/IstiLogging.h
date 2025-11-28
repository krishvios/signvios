// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019-2020 Sorenson Communications, Inc. -- All rights reserved

#ifndef ISTILOGGING_H
#define ISTILOGGING_H

#include "ISignal.h"

class IstiLogging
{
public:
	/*!
	 * \brief Retrieves object pointer for this interface.
	 * 
	 * 
	 * \return IstiLogging* 
	 */
	static IstiLogging * InstanceGet ();
	
	virtual void log (std::string message) = 0;
	virtual void onBootUpLog (std::string message)  = 0;
};

#endif // ISTILOGGING_H
