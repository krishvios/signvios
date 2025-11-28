// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved

#pragma once

#include <string>
#include <vector>
#include <iostream>

#include "CstiEventQueue.h"
#include "CstiSignal.h"
#include "CstiTimer.h"
#include "IstiLogging.h"

#include "stiDefs.h"
#include "stiRemoteLogEvent.h"


/*! \class CstiLogging 
 *  
 * \brief Manages Splunk logging during and after boot-up.
 * 
 * \note To use this class include the header. Then use the following to 
 * obtain the logging object instance and use the logging methods.
 * 
 * Example code:
 * -------------
 *  IstiLogging *pLogging = IstiLogging::InstanceGet();
 *  pLogging->onBootUpLog("EventType=HDMIDisplayMode DisplayMode=1920x1080\n");
 *                              
 */

class CstiLogging : public IstiLogging, public CstiEventQueue
{
public:
    CstiLogging ();
    CstiLogging (const CstiLogging &other) = delete;
    CstiLogging (CstiLogging &&other) = delete;
    CstiLogging &operator= (const CstiLogging &other) = delete;
    CstiLogging &operator= (CstiLogging &&other) = delete;

    ~CstiLogging ();

    stiHResult Initialize ();
    void log (std::string message) override;
    void onBootUpLog (std::string message) override;
    void currentTimerSet (bool set);

private:
    bool m_currentTimeSet {false};
    
    std::vector<std::string> m_onBootUpVector;

    vpe::EventTimer m_loggingServicesStartupTimer;

    void eventOnBootUpLog (std::string& message);
    void eventBootUpVectorLog ();

protected:
    CstiSignalConnection::Collection m_signalConnections;
};
