// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved

#include "CstiLogging.h"

const int LOGGING_SERVICES_TIMER_DELAY_MS = 3 * 1000;


CstiLogging::CstiLogging ()
:
    CstiEventQueue("stiLOGGING"),
    m_loggingServicesStartupTimer(LOGGING_SERVICES_TIMER_DELAY_MS, this)
{

}

CstiLogging::~CstiLogging ()
{
    StopEventLoop ();
}

stiHResult CstiLogging::Initialize () 
{
    stiHResult hResult = stiRESULT_SUCCESS;

    m_signalConnections.push_back (m_loggingServicesStartupTimer.timeoutSignal.Connect (
        [this]
        {
            PostEvent (
                [this]
                {
                    eventBootUpVectorLog ();
                });
		}));

    StartEventLoop ();

    return hResult;
}

void CstiLogging::log (std::string message)
{
    stiRemoteLogEventSend(message.c_str());
}

void CstiLogging::onBootUpLog (std::string message)
{
    m_onBootUpVector.push_back(message);
}

void CstiLogging::currentTimerSet (bool set)
{
    m_currentTimeSet = set;
    m_loggingServicesStartupTimer.restart();
}

void CstiLogging::eventOnBootUpLog (std::string& message)
{
    if (m_currentTimeSet)
    {
        stiRemoteLogEventSend(message.c_str());
    }
}

void CstiLogging::eventBootUpVectorLog ()
{   
    if (!m_onBootUpVector.empty())
    {
        for (auto& item : m_onBootUpVector)
        {
            PostEvent (std::bind (&CstiLogging::eventOnBootUpLog, this, item));
        }
    }

    m_onBootUpVector.clear();
}
