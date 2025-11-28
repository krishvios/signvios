/*!
 * \file CstiPropertySender.cpp
 * \brief Module for sending property values to the core server.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#include "CstiPropertySender.h"
#include "stiSVX.h"
#include "stiTrace.h"
#include "PropertyManager.h"
#include "CstiCoreResponse.h"
#include "CstiCoreRequest.h"
#include "CstiCoreService.h"
#include "stiTaskInfo.h"
#include "CstiExtendedEvent.h"

using namespace WillowPM;


//
// Constructor for the property sender.
//
CstiPropertySender::CstiPropertySender (
	int maxPropertiesToSend, // Maximum number of properties to send in a batch
	int delayTime,			// Delay time in milliseconds
	int errorDelayTime)		// For an error, delay time in milliseconds
:
	m_bUserAuthenticated (false),
	m_maxPropertiesToSend (maxPropertiesToSend),
	m_delayTime (delayTime),
	m_errorDelayTime (errorDelayTime),
	m_bRequestPending (false),
	m_unUserRequestId (0),
	m_unPhoneRequestId (0),
	m_nUserCount (0),
	m_nPhoneCount (0),
	m_eventQueue(CstiEventQueue::sharedEventQueueGet ()),
	m_sendTimer (delayTime, &CstiEventQueue::sharedEventQueueGet ()),
	m_pCoreService (nullptr)
{
	m_signalConnections.push_back (m_sendTimer.timeoutSignal.Connect ([this]() {
		SendProperties ();
	}));
}


//
// Destructor for the property sender
//
CstiPropertySender::~CstiPropertySender ()
{
	std::list<SstiProperty *>::iterator i;

	for (i = m_PropertyList.begin(); i != m_PropertyList.end(); )
	{
		delete *i;

		i = m_PropertyList.erase(i);
	}
}


//
// Sets the core service object.
//
void CstiPropertySender::CoreServiceSet (CstiCoreService *pCoreService)
{
	m_eventQueue.PostEvent ([this, pCoreService]() {
		m_pCoreService = pCoreService;

		if (m_pCoreService)
		{
			m_clientAuthSignalConnection = m_pCoreService->clientAuthenticateSignal.Connect ([this](const ServiceResponseSP<ClientAuthenticateResult>& result) {
				m_eventQueue.PostEvent (std::bind (&CstiPropertySender::clientAuthenticateHandle, this, result));
				});
		}
		else
		{
			m_clientAuthSignalConnection.Disconnect ();
		}
	});
}


//
// Sets the timer to the appropriate value
// for sending the next batch of property values.
//
void CstiPropertySender::TimerSet (
	int delayTime)			// in milliseconds
{
	m_sendTimer.timeoutSet (delayTime);
	m_sendTimer.start ();
}


//
// Cancels sending properties
//
void CstiPropertySender::PropertySendCancel ()
{
	m_eventQueue.PostEvent ([this]() {
		stiDEBUG_TOOL (g_stiPSDebug,
			stiTrace ("Cancelling the use of PropertySender...\n");
		);

		// If we have a timer, cancel it in case it is currently running.
		m_sendTimer.stop ();

		// Reset the flag denoting we have a request in the system.
		PropertyRequestEnd ();

		// Remove any pending properties waiting to be sent to the core service.
		std::list<SstiProperty*>::iterator i;

		for (i = m_PropertyList.begin (); i != m_PropertyList.end (); )
		{
			delete* i;

			i = m_PropertyList.erase (i);
		}
	});
}

void CstiPropertySender::PropertySend (const std::string& name, int nType)
{
	m_eventQueue.PostEvent (std::bind(&CstiPropertySender::propertySendHandle, this, name, nType));
}


//
// Adds a property to the list of properties to send.
// If the property already exists then the entry in the list
// is marked to be resent if a request is already in progress.
//
void CstiPropertySender::propertySendHandle (
	const std::string& name,
	int nType)
{
	stiDEBUG_TOOL (g_stiPSDebug,
		stiTrace("Sending property: %s\n", name.c_str());
	);

	//
	// Search the current list to see if the
	// property is already in the list and has not
	// been sent yet.
	//
	EstiBool bFound = estiFALSE;

	stiDEBUG_TOOL (g_stiPSDebug,
		stiTrace("Searching for property %s in list\n", name.c_str());
	);

	for (auto property : m_PropertyList)
	{
		if (property->Name == name && property->nType == nType && property->state == estiPSSPending)
		{
			stiDEBUG_TOOL (g_stiPSDebug,
				stiTrace("Found property %s in list\n", name.c_str());
			);

			if ((nType == estiPTypePhone && m_unPhoneRequestId != 0)
			 || (nType == estiPTypeUser && m_unUserRequestId != 0))
			{
				property->resend = true;
			}

			bFound = estiTRUE;

			break;
		} // end if
	}

	if (!bFound)
	{
		stiDEBUG_TOOL (g_stiPSDebug,
			stiTrace("Did not find property %s in list\n", name.c_str());
		);

		auto pProperty = new SstiProperty;
		pProperty->Name = name;
		pProperty->nType = nType;
		pProperty->state = estiPSSPending;
		pProperty->resend = false;

		m_PropertyList.push_back(pProperty);
	}

	//
	// If there is no response pending
	// and the list is not empty
	// then either send the properties
	// or set a timer to send them later.
	//
	if (!m_bRequestPending
	 && !m_PropertyList.empty ())
	{
		if (m_PropertyList.size () >= static_cast<size_t>(m_maxPropertiesToSend))
		{
			stiDEBUG_TOOL (g_stiPSDebug,
				stiTrace("Calling SendProperties directly\n");
			);

			m_sendTimer.stop ();

			SendProperties ();
		}
		else
		{
			stiDEBUG_TOOL (g_stiPSDebug,
				stiTrace("Setting timer to call SendProperties\n");
			);

			TimerSet(m_delayTime);
		}
	}
	else
	{
		stiDEBUG_TOOL (g_stiPSDebug,
			stiTrace("Pending Request: %d (%d, %d)\n", m_bRequestPending, m_unUserRequestId, m_unPhoneRequestId);
			stiTrace("List %s empty\n", m_PropertyList.empty () ? "is" : "is not");
		);
	}
}


//
// Callback function for core to notify the property sender
// of the request id for the last request sent.
//
stiHResult CstiPropertySender::RequestSendCallback (
	int32_t n32Message,
	size_t MessageParam,
	size_t CallbackParam,
	size_t CallbackFromId)
{
	auto pThis = (CstiPropertySender *)CallbackParam;

	pThis->m_eventQueue.PostEvent ([pThis, n32Message, MessageParam]() {
		pThis->requestSendHandle (static_cast<EstiCmMessage>(n32Message), static_cast<unsigned int>(MessageParam));
	});
	
	return stiRESULT_SUCCESS;
}

void CstiPropertySender::requestSendHandle (EstiCmMessage message, unsigned int requestId)
{
	if (message == estiMSG_CORE_ASYNC_REQUEST_ID)
	{
		if (m_nUserCount > 0)
		{
			m_unUserRequestId = requestId;
		}

		if (m_nPhoneCount > 0)
		{
			m_unPhoneRequestId = requestId;
		}
	}
	else if (message == estiMSG_CORE_ASYNC_REQUEST_FAILED)
	{
		PropertyRequestEnd ();

		m_unUserRequestId = 0;
		m_unPhoneRequestId = 0;

		//
		// An error occured so reset the timer
		// so we can try again later.
		//
		TimerSet (m_errorDelayTime);
	}
}


//
// Sends the properties that are in the list
// If a property is in the list but is no
// longer in the property table then the entry is
// removed.
//
void CstiPropertySender::SendProperties()
{
	CstiCoreRequest *poRequest = nullptr;
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_pCoreService && !m_bRequestPending && m_bUserAuthenticated)
	{
		std::list<SstiProperty *>::iterator i;
		int numberSending = 0;
		std::string DataType;
		std::string Value;
		CstiCoreRequest::ESettingType eSettingType = CstiCoreRequest::eSETTING_TYPE_STRING;

		m_nUserCount = 0;
		m_nPhoneCount = 0;

		poRequest = new CstiCoreRequest();

		for (i = m_PropertyList.begin (); i != m_PropertyList.end ();)
		{
			int nResult = PropertyManager::getInstance ()->getPropertyValue ((*i)->Name, &DataType,
																&Value, PropertyManager::Temporary);

			if (nResult != PM_RESULT_SUCCESS)
			{
				stiDEBUG_TOOL (g_stiPSDebug,
					stiTrace("############### Property not found: %s #############\n", (*i)->Name.c_str());
				);

				delete *i;

				i = m_PropertyList.erase(i);
			}
			else
			{
				// NOTE: even though we don't support enum types any
				// longer, provide this for backwards compatibilty
				if (strcmp(DataType.c_str (), XT_INT) == 0
				 || strcmp(DataType.c_str (), XT_ENUM) == 0)
				{
					eSettingType = CstiCoreRequest::eSETTING_TYPE_INT;
				}
				else if (strcmp(DataType.c_str (), XT_STRING) == 0)
				{
					eSettingType = CstiCoreRequest::eSETTING_TYPE_STRING;
				}

				switch ((*i)->nType)
				{
					case estiPTypePhone:

						poRequest->phoneSettingsSet ((*i)->Name.c_str(), Value.c_str (), eSettingType);

						++m_nPhoneCount;

						break;

					case estiPTypeUser:

						poRequest->userSettingsSet ((*i)->Name.c_str (), Value.c_str (), eSettingType);

						++m_nUserCount;

						break;
				}

				(*i)->state = estiPSSSending;
				++numberSending;

				if (numberSending >= m_maxPropertiesToSend)
				{
					break;
				}

				i++;
			}
		}

		if (numberSending > 0)
		{
			stiDEBUG_TOOL (g_stiPSDebug,
				stiTrace("Sending properties\n");
			);

			hResult = m_pCoreService->RequestSendAsync (poRequest, RequestSendCallback, (size_t)this);
			stiTESTRESULT ();

			poRequest = nullptr;

			{
				std::lock_guard<std::mutex> lock (m_sendingMutex);
				m_bRequestPending = true;
			}
		}
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		//
		// An error occured so reset the timer
		// so we can try again later.
		//
		TimerSet (m_delayTime);
	}

	if (poRequest)
	{
		delete poRequest;
		poRequest = nullptr;
	}
}


/// Block until any pending property request has finished or has been canceled.
void CstiPropertySender::PropertySendWait (int timeoutMilliseconds)
{
	std::unique_lock<std::mutex> lock (m_sendingMutex);
	if (m_bRequestPending)
	{
		m_sendingCondition.wait_for (
			lock,
			std::chrono::milliseconds (timeoutMilliseconds),
			[this] { return !m_bRequestPending; });
	}
}


void CstiPropertySender::PropertyRequestEnd ()
{
	{
		std::lock_guard<std::mutex> lock (m_sendingMutex);
		m_bRequestPending = false;
	}
	
	m_sendingCondition.notify_all ();
}


//
// Updates the sending state for each property.  Properties that
// are in a sending state are removed unless they have been marked
// to be resent.
// If the previous request failed then the properties marked sending
// are marked as pending.
//
void CstiPropertySender::UpdateSendingState(
	EstiResult eResult,
	int nType)
{
	if (eResult == estiOK)
	{
		//
		// Remove the properties that were sent from the list.
		//
		std::list<SstiProperty *>::iterator i;

		for (i = m_PropertyList.begin(); i != m_PropertyList.end(); )
		{
			if ((*i)->nType == nType
			 && (*i)->state == estiPSSSending)
			{
				if ((*i)->resend)
				{
					stiDEBUG_TOOL (g_stiPSDebug,
						stiTrace("Resending %s\n", (*i)->Name.c_str());
					);

					(*i)->resend = false;

					i++;
				}
				else
				{
					stiDEBUG_TOOL (g_stiPSDebug,
						stiTrace("Removing %s\n", (*i)->Name.c_str());
					);

					delete *i;

					i = m_PropertyList.erase(i);
				}
			} // end if
			else
			{
				i++;
			}
		}

	}
	else
	{
		//
		// Mark all "sending" properties as "pending" properties.
		//
		for (auto property : m_PropertyList)
		{
			if (property->nType == nType && property->state == estiPSSSending)
			{
				property->state = estiPSSPending;
				property->resend = false;
			} // end if
		}
	}
}

/*!\brief Signal callback that is raised when a ClientAuthenticateResult core response is received.
 *
 */
void CstiPropertySender::clientAuthenticateHandle (const ServiceResponseSP<ClientAuthenticateResult>& clientAuthenticateResult)
{
	if (clientAuthenticateResult->responseSuccessful)
	{
		m_bUserAuthenticated = true;

		//
		// Start the timer to send the properties again
		// if we are not still waiting on a response
		// and there are properties to be sent.
		//
		if (!m_bRequestPending
			&& !m_PropertyList.empty ())
		{
			TimerSet (m_delayTime);
		}
	}
}


void CstiPropertySender::ResponseCheck (const CstiCoreResponse* poResponse)
{
	m_eventQueue.PostEvent (std::bind (&CstiPropertySender::responseCheckHandle, this, poResponse->RequestIDGet(), poResponse->ResponseGet(), poResponse->ResponseResultGet()));
}


//
// Checks to see if the response is the one the property sender
// is waiting for.  Even though phone settings and user settings
// are sent in one request the responses come back separately thus
// we need to wait for them separately.
//
void CstiPropertySender::responseCheckHandle(unsigned int requestId, CstiCoreResponse::EResponse response, EstiResult responseResult)
{
	if (m_bRequestPending
	 && (requestId == m_unPhoneRequestId
	 || requestId == m_unUserRequestId))
	{
		switch(response)
		{
			case CstiCoreResponse::ePHONE_SETTINGS_SET_RESULT:

				UpdateSendingState(responseResult, estiPTypePhone);

				m_unPhoneRequestId = 0;

				break;

			case CstiCoreResponse::eUSER_SETTINGS_SET_RESULT:

				UpdateSendingState(responseResult, estiPTypeUser);

				m_unUserRequestId = 0;

				break;

			default:

				//
				// Do nothing
				//
				break;
		}

		//
		// Start the timer to send the properties again
		// if we are not still waiting on a response
		// and there are properties to be sent.
		//
		if (m_unUserRequestId == 0
		 && m_unPhoneRequestId == 0)
		{
			PropertyRequestEnd ();

			if (!m_PropertyList.empty ())
			{
				TimerSet (m_delayTime);
			}
		}
	}
}


