/*!
 * \file stiRemoteLogEvent.h
 * \brief See stiRemoteLogEvent.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef STIREMOTELOGEVENT_H
#define STIREMOTELOGEVENT_H

#include <cstdlib>
#include <string>
#include "stiDefs.h"


typedef stiHResult (*pRemoteLogEventSend)(size_t *, const char *);


void stiRemoteLogEventSend (
	const char *pFormat,	//!< Format specifier (like printf).
	...);					//!< Additional parameters (see ANSI function printf for more details).
	
void stiRemoteLogEventSend(
	std::string *pMessage);

void stiRemoteLogEventRemoteLoggingSet(
	size_t *poRemoteLogger,
	pRemoteLogEventSend pmRemoteLogEventSend);

	
#endif // STIREMOTELOGEVENT_H

