// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

/*!
 * \file CstiMonitorTask.h
 * \brief See CstiMonitorTask.cpp
 */

#ifndef CMONITOR_TASK_H
#define CMONITOR_TASK_H

//
// Includes
//
#include "MonitorTaskBase2.h"


class CstiMonitorTask : public MonitorTaskBase2
{
public:

	CstiMonitorTask () = default;
	~CstiMonitorTask () override = default;

	CstiMonitorTask (const CstiMonitorTask &other) = delete;
	CstiMonitorTask (CstiMonitorTask &&other) = delete;
	CstiMonitorTask &operator= (const CstiMonitorTask &other) = delete;
	CstiMonitorTask &operator= (CstiMonitorTask &&other) = delete;

protected:
	bool isCameraAvailable () override;

};


#endif //#ifndef CMONITORTASK_H
