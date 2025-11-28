/*!
 * \file ErrorCodeLookup.h
 * \brief Declaration of error code lookup.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef ERRORCODELOOKUP_H
#define ERRORCODELOOKUP_H

#include <time.h>
#include <string>

namespace Vp200Ui
{	
	class CErrorCodeLookup
	{
	public:
		//CErrorCodeLookup();
		
		static std::string GetCoreErrorMessage(unsigned int unErrorCode);
		static std::string GetMessageErrorMessage(unsigned int unErrorCode);
		static std::string GetStateNotifyErrorMessage(unsigned int unErrorCode);
	private:
		
	};
	
}
#endif // ERRORCODELOOKUP_H
