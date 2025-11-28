/*!
 * \file CstiCallStorage.cpp
 * \brief Storage anchoring for call objects to ensure that they exist as long
 *        as they are needed.  Normally shared pointers alone would give you
 *        this guarantee, but we pass raw call pointers up to the UI layer,
 *        because we are limited to a C interface.  To ensure those raw pointers
 *        remain valid, we store the shared_ptr call object here until the UI layer
 *        tells us it is done with the call pointer.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2000 – 2017 Sorenson Communications, Inc. -- All rights reserved
 */
#include "stiTrace.h"
#include "CstiCall.h"
#include "CstiCallStorage.h"
#include "algorithm"
#include "stiRemoteLogEvent.h"
#include "CstiSipCall.h"
#include "EndpointMonitor.h"


CstiCallStorage::CstiCallStorage ()
{
	m_signalConnections.push_back (vpe::EndpointMonitor::instanceGet()->queryCallStorageStateSignal.Connect (
		[this]
		{
			std::lock_guard<std::recursive_mutex> lk (m_mutex);

			vpe::EndpointMonitor::instanceGet()->callStorageCalls(m_callList);
		}));
}


/*!
 * \brief Locks the mutex for multi-threaded access
 */
void CstiCallStorage::lock() const
{
	m_mutex.lock ();
}


/*!
 * \brief Unlocks the mutex
 */
void CstiCallStorage::unlock() const
{
	m_mutex.unlock ();
}


/*!
 * \brief Gets a copy of all call objects
 */
std::list<CstiCallSP> CstiCallStorage::listGet () const
{
	std::lock_guard<std::recursive_mutex> lk (m_mutex);
	return m_callList;
}


/*!
 * \brief Removes a call object from the store, but only if the call has completed
 */
stiHResult CstiCallStorage::remove (
    CstiCall *call)
{
	stiLOG_ENTRY_NAME (CstiCallStorage::remove);
	auto hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lk (m_mutex);

	// Find the call in the list
	auto itr = std::find_if (m_callList.begin(), m_callList.end(),
	            [call](const CstiCallSP &c){ return call == c.get(); });
	if (itr != m_callList.end())
	{
		stiASSERTMSG (!call->StateValidate(esmiCS_CONNECTING | esmiCS_CONNECTED | esmiCS_HOLD_LCL | esmiCS_HOLD_RMT | esmiCS_HOLD_BOTH),
		              "Removed an active call object.  This indicates a programming error.");
		stiDEBUG_TOOL (g_stiCMCallObjectDebug,
		    stiTrace ("<CstiCallStorage::remove> Call object [%p] removed from store.\n", call);
		);

		vpe::EndpointMonitor::instanceGet ()->callRemoved (*itr);

		m_callList.erase (itr);
	}
	else
	{
		stiDEBUG_TOOL (g_stiCMCallObjectDebug,
		    stiTrace ("<CstiCallStorage::remove> Attempt to remove an invalid call object [%p]\n", call);
		);
	}

	stiDEBUG_TOOL (g_stiCMCallObjectDebug,
	    stiTrace ("<CstiCallStorage::remove> Number of calls in call storage: %d\n", (int)m_callList.size());
	);
	
	return hResult;
}


/*!
 * \brief Store a call object for later retrieval and to act as an anchor for
 *        the raw call pointers that get passed up to the UI layer.  Having a
 *        copy of the shared_ptr here ensures those raw pointers never dangle.
 */
void CstiCallStorage::store (
    const CstiCallSP &call)
{
	stiLOG_ENTRY_NAME (CstiCallStorage::store);

	std::lock_guard<std::recursive_mutex> lk (m_mutex);
	m_callList.push_back (call);

	vpe::EndpointMonitor::instanceGet ()->callAdded (call);

	stiDEBUG_TOOL (g_stiCMCallObjectDebug,
	    stiTrace ("<CstiCallStorage::store> New call object stored [%p]\n", call.get());
	    stiTrace ("<CstiCallStorage::store> Number of calls in call storage: %d\n", (int)m_callList.size());
	);
}


/*!
 * \brief Remove and stale objects in the disconnected state
 */
stiHResult CstiCallStorage::removeStaleObjects ()
{
	stiLOG_ENTRY_NAME (CstiCallStorage::removeStaleObjects);

	auto hResult = stiRESULT_SUCCESS;
	bool removedSome = false;

	std::lock_guard<std::recursive_mutex> lk (m_mutex);

	// A while is used because we do not want to advance on erasure
	auto it = m_callList.begin();

	// Check our store for stale call objects
	while (it != m_callList.end ())
	{
		auto call = *it;

		// Check for staleness
		if (call != nullptr &&
			call->StateValidate(esmiCS_DISCONNECTED) &&
		    estiSUBSTATE_NONE == call->SubstateGet() &&
		    call->secondsSinceCallEndGet() >= nSTALE_OBJECT_CHECK_DELAY_SEC)
		{
			stiDEBUG_TOOL (g_stiCMCallObjectDebug,
			    stiTrace ("<CallStorage::removeStaleObjects> purging stale call object [%p] from storage.  Reference count = %d\n",
			            call.get(), call.use_count());
			);

			stiDEBUG_TOOL (g_stiCMCallObjectDebug,
			    stiTrace ("<CstiCallStorage::removeStaleObjects> Call object  [%p] removed from store.\n", call.get());
			);

			// Remove this call reference so that it can be cleaned up
			it = m_callList.erase (it);

			CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());
			std::string callId;
			if (sipCall)
			{
				sipCall->CallIdGet (&callId);
			}

			// Log this event
			stiRemoteLogEventSend ("EventType=StaleCallObjRmvd CallDirection=%d CallResult=%d SipCallId=%s RefCount=%d",
			                        call->DirectionGet(),
			                        call->ResultGet(),
			                        callId.c_str(),
			                        call.use_count());
			removedSome = true;
		}
		else
		{
			// Advance to the next node in the list
			it++;
		}
	}

	stiDEBUG_TOOL (g_stiCMCallObjectDebug,
	    if (removedSome)
		{
	        stiTrace ("<CstiCallStorage::removeStaleObjects> Number of calls remaining in call storage: %d\n", (int)m_callList.size());
		}
	);

	return hResult;
}


/*!
 * \brief Get the first call object in the list, or nullptr if none
 */
CstiCallSP CstiCallStorage::headGet ()
{
	stiLOG_ENTRY_NAME (CstiCallStorage::headGet);
	std::lock_guard<std::recursive_mutex> lk (m_mutex);

	return m_callList.empty() ? nullptr : m_callList.front();
}


/*!
 * \brief Returns the number of call objects stored
 */
unsigned int CstiCallStorage::countGet ()
{
	stiLOG_ENTRY_NAME (CstiCallStorage::countGet);
	std::lock_guard<std::recursive_mutex> lk (m_mutex);

	return m_callList.size();
}


/*!
 * \brief Return the number of calls there are in the given state(s)
 */
unsigned int CstiCallStorage::countGet (
    uint32_t stateMask)
{
	stiLOG_ENTRY_NAME (CstiCallStorage::countGet);

	unsigned int count = 0;
	std::lock_guard<std::recursive_mutex> lk (m_mutex);
	
	for (const auto &call: m_callList)
	{
		// Is this call in one of the states passed in?
		if (call->StateValidate (stateMask))
		{
			count++;
		}
	}

	return count;
}


/*!
 * \brief Get a count of calls in an "active state"
 */
unsigned int CstiCallStorage::activeCallCountGet ()
{
	return countGet (esmiCS_CONNECTING | esmiCS_CONNECTED | esmiCS_HOLD_LCL | esmiCS_HOLD_RMT | esmiCS_HOLD_BOTH | esmiCS_TRANSFERRING | esmiCS_INIT_TRANSFER | esmiCS_IDLE);
}


/*!
 * \brief Returns true if any of the calls are in a leaving message state?
 */
bool CstiCallStorage::leavingMessageGet ()
{
	stiLOG_ENTRY_NAME (CstiCallStorage::leavingMessageGet);

	std::lock_guard<std::recursive_mutex> lk (m_mutex);

	for (const auto &call : m_callList)
	{
		// Is this call in one of the states passed in?
		if (call->StateValidate (esmiCS_DISCONNECTED) &&
		    (call->SubstateValidate (estiSUBSTATE_LEAVE_MESSAGE) ||
		     call->SubstateValidate (estiSUBSTATE_MESSAGE_COMPLETE)))
		{
			return true;
		}

	}

	return false;
}


/*!
 * \brief Get the first call object matching one of the given call states
*/
CstiCallSP CstiCallStorage::get (
	uint32_t stateMask)
{
	stiLOG_ENTRY_NAME (CstiCallStorage::get);
	std::lock_guard<std::recursive_mutex> lk(m_mutex);
	
	for (auto &call: m_callList)
	{
		// Is this call in one of the states passed in?
		if (call->StateValidate (stateMask))
		{
			return call;
		}
	}
	
	return nullptr;
}


/*!
 * \brief Finds and returns the shared_ptr that points to the same call
 *        object as the passed-in raw pointer.  This is used to map raw
 *        call pointers back to a shared_ptr, which happens when a raw
 *        pointer is passed down from the UI layer.
 */
CstiCallSP CstiCallStorage::lookup (
    IstiCall *call) const
{
	std::lock_guard<std::recursive_mutex> lk(m_mutex);

	auto it = std::find_if (m_callList.begin(), m_callList.end(),
	            [call](const CstiCallSP &c){ return call == c.get(); });

	if (it == m_callList.end())
	{
		stiASSERTMSG (false, "Call object does not exist - this is a bug\n");
		return nullptr;
	}

	return *it;
}


/*!
 * \brief Get the call object identified by the passed in appData
 */
CstiCallSP CstiCallStorage::getByAppData (
    size_t appData)
{
	stiLOG_ENTRY_NAME (CstiCallStorage::getByAppData);
	std::lock_guard<std::recursive_mutex> lk(m_mutex);

	for (auto call : m_callList)
	{
		// Is this the call with the AppData passed in?
		if (appData == call->AppDataGet())
		{
			return call;
		}
	}

	return nullptr;
}


/*!
 * \brief Get the call object that is "offering" from the list
 */
CstiCallSP CstiCallStorage::incomingGet ()
{
	stiLOG_ENTRY_NAME (CstiCallStorage::incomingGet);
	std::lock_guard<std::recursive_mutex> lk(m_mutex);

	for (auto call : m_callList)
	{
		// Is this call in one of the states passed in?
		if (call->StateGet() == esmiCS_CONNECTING && call->DirectionGet() == estiINCOMING)
		{
			return call;
		}
	}
	
	return nullptr;
}


/*!
 * \brief Get the call object that is in "In Progress" from the call list
 */
CstiCallSP CstiCallStorage::outgoingGet ()
{
	stiLOG_ENTRY_NAME (CstiCallStorage::outgoingGet);
	std::lock_guard<std::recursive_mutex> lk(m_mutex);

	for (auto call : m_callList)
	{
		// Is this call in one of the states passed in?
		if (call->StateGet() == esmiCS_CONNECTING && call->DirectionGet() == estiOUTGOING)
		{
			return call;
		}
	}
	
	return nullptr;
}

 CstiCallSP CstiCallStorage::getByCallIndex (int callIndex)
 {
	 std::lock_guard<std::recursive_mutex> lk (m_mutex);

	 for (auto call : m_callList)
	 {
		 if (call->CallIndexGet () == callIndex)
		 {
			 return call;
		 }
	 }

	 return nullptr;
 }


/*!
 * \brief Call StatsCollect on each call object
 */
stiHResult CstiCallStorage::collectStats ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::lock_guard<std::recursive_mutex> lk(m_mutex);

	for (const auto &call : m_callList)
	{
		call->StatsCollect ();
	}

	return hResult;
}


void CstiCallStorage::removeAll ()
{
	std::lock_guard<std::recursive_mutex> lk(m_mutex);

	m_callList.clear();
}

#ifdef stiHOLDSERVER
/*!
 * \brief Get the nth call object from the call list
 */
CstiCallSP CstiCallStorage::nthCallGet (
    uint32_t nthCallNumber)
{
	stiLOG_ENTRY_NAME (CstiCallStorage::nthCallGet);
	std::lock_guard<std::recursive_mutex> lk(m_mutex);

	for (auto call : m_callList)
	{
		// Is this call the Nth call?
		if (nthCallNumber == call->NthCallNumberGet())
		{
			return call;
		}
	}

	return nullptr;
}


/*!
 * \brief Hangup one call in the list
 *        TODO:  The method is misnamed... it only hangs up one call
 */
void CstiCallStorage::hangupAllCalls ()
{
	std::lock_guard<std::recursive_mutex> lk(m_mutex);

	for (auto call : m_callList)
	{
		if (!call->StateValidate (esmiCS_DISCONNECTING | esmiCS_DISCONNECTED))
		{
			// Found the call we are looking for.  Hangup and break out of the loop.
			call->HangUp ();
			break;
		}
	}
}


/*!
 * \brief CstiCallStorage::CallObjectGetByLocalPhoneNbr
 */
CstiCallSP CstiCallStorage::CallObjectGetByLocalPhoneNbr (
    const char *phoneNumber)
{
	std::lock_guard<std::recursive_mutex> lk(m_mutex);

	for (auto call : m_callList)
	{
		ICallInfo *rmtInfo = call->RemoteCallInfoGet ();
		SstiUserPhoneNumbers phoneNumbers = *rmtInfo->UserPhoneNumbersGet ();
		if (0 == strcmp (phoneNumbers.szLocalPhoneNumber, phoneNumber))
		{
			return call;
		}
	}

	return nullptr;
}


/*!
 * \brief CstiCallStorage::CallObjectGetByHSCallId
 */
CstiCallSP CstiCallStorage::CallObjectGetByHSCallId (
    __int64 n64CallID)
{
	std::lock_guard<std::recursive_mutex> lk(m_mutex);

	for (auto call : m_callList)
	{
		if (call->HSCallIdGet() == n64CallID)
		{
			return call;
		}
	}

	return nullptr;
}
#endif // stiHOLDSERVER
