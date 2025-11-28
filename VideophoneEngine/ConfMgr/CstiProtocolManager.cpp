/*!
 * \file CstiProtocolManager.cpp
 * \brief Base class for SipManager
 *
 *        TODO:  Provide description
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 – 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#include "CstiProtocolManager.h"
#include "stiTrace.h"
#include "IstiVideoInput.h"
#include "IstiAudioInput.h"
#include "IstiAudioOutput.h"
#include "IstiPlatform.h"
#include "CstiCallStorage.h"
#include "stiTools.h"
#include "IstiBlockListManager2.h"
#include "videoDefs.h"
#include "EndpointMonitor.h"
#ifdef __APPLE__
#include <sys/time.h>
#endif
#include <iomanip>

namespace
{

/*!
 * \brief append integer representation of undefined substate
 *
 * Only used in debugging to know when a substate is invalid and what
 * the substate is
 */
void undefinedSubstateAppend(std::string &stateBuffer, uint32_t substate)
{
	std::stringstream ss;

	// Set formatting
	ss << std::setfill('0') << std::setw(8) << std::hex << std::uppercase;

	ss << "Undefined (0x" << substate << ")";
	stateBuffer += ss.str();
}

PlatformCallStateChange convertCallStateChange(const SstiStateChange &stateChange)
{
	PlatformCallStateChange ret;

	ret.callIndex = stateChange.call->CallIndexGet ();

	ret.prevState = stateChange.ePrevState;
	ret.prevSubStates = stateChange.un32PrevSubStates;

	ret.newState = stateChange.eNewState;
	ret.newSubStates = stateChange.un32NewSubStates;

	return ret;
}

}

std::string StateStringBuild (
	EsmiCallState eState, // The state to be converted to text
	uint32_t un32SubState);   // The substate to be converted to text


////////////////////////////////////////////////////////////////////////////////
//; CstiProtocolManager::CstiProtocolManager
//
// Description: Constructor
//
// Abstract:
//  This is the default constructor
//
// Returns:
//  None
//
CstiProtocolManager::CstiProtocolManager (
	CstiCallStorage *pCallStorage,
	IstiBlockListManager2 *pBlockListManager,
	const std::string &taskName)
:
	CstiEventQueue (taskName),
	m_pBlockListManager (pBlockListManager),
#if defined(stiHOLDSERVER) || defined(stiMEDIASERVER)
	m_nChannelPortBase (49152),
	m_nChannelPortRange (65535 - m_nChannelPortBase + 1),
#endif
	m_pCallStorage (pCallStorage)
{
	stiLOG_ENTRY_NAME (CstiProtocolManager::CstiProtocolManager);

	m_signalConnections.push_back (callStateChangedSignal.Connect ([](const SstiStateChange &stateChange)
		{
			IstiPlatform::InstanceGet()->callStateChangedSignalGet ().Emit(
				// Convert one to the other
				convertCallStateChange(stateChange)
				);
		}));
}

////////////////////////////////////////////////////////////////////////////////
//; CstiProtocolManager::UserPhoneNumbersGet
//
// Description: Gets the current phone numbers in the configuration data object
//
// Abstract:
//
// Returns:
//	The phone numbers.
//
SstiUserPhoneNumbers *CstiProtocolManager::UserPhoneNumbersGet ()
{
	return &m_UserPhoneNumbers;
} // end CstiProtocolManager::UserPhoneNumbersGet


////////////////////////////////////////////////////////////////////////////////
//; CstiProtocolManager::UserPhoneNumbersSet
//
// Description: Sets the current phone numbers in the configuration data object
//
// Abstract:
//
// Returns:
//	estiOK (Success) or estiERROR (Failure)
//
stiHResult CstiProtocolManager::UserPhoneNumbersSet (
	const SstiUserPhoneNumbers *psUserPhoneNumbers)	// Current phone numbers
{
	stiLOG_ENTRY_NAME (CstiProtocolManager::UserPhoneNumbersSet);

	stiHResult hResult = stiRESULT_SUCCESS;
	
	Lock();
	m_UserPhoneNumbers = *psUserPhoneNumbers;
	Unlock();

	return (hResult);
} // end CstiProtocolManager::UserPhoneNumbersSet


////////////////////////////////////////////////////////////////////////////////
//; CstiProtocolManager::LocalDisplayNameSet
//
// Description: Change the Local Name that is transmitted to the remote endpoint
//
// Abstract:
//  This method causes the Local Name (the user's name) to be updated in the
//  Q.931 message that is transmitted to the remote endpoint during call setup (in
//  the DisplayName field).
//
// Returns:
//  estiOK (Success) estiERROR (Failure)estiRAS_NOT_REGISTERED
//
stiHResult CstiProtocolManager::LocalDisplayNameSet (
	const std::string &displayName)
{
	stiLOG_ENTRY_NAME (CstiProtocolManager::LocalDisplayNameSet);

	Lock();
	m_displayName = displayName;
	Unlock();

	return stiRESULT_SUCCESS;
} // end CstiProtocolManager::LocalDisplayNameSet


/*!\brief Sets the return info for callback purposes
*
*  This function is used to set the information needed by the remote system
*  to call back to the desired location (not always the originating system).
*  This data will be passed in SInfo to the remote system.
*
*/
stiHResult CstiProtocolManager::LocalReturnCallInfoSet (
	EstiDialMethod eMethod, // The dial method to use for the return call
    std::string dialString)	// The dial string to dial on the return call
{
	stiLOG_ENTRY_NAME (CstiProtocolManager::LocalReturnCallInfoSet);

	Lock();

	m_eLocalReturnCallDialMethod =  eMethod;
	m_localReturnCallDialString = dialString;

	Unlock();

	return stiRESULT_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiProtocolManager::CallAccept
//
// Description: This function is used to accept an incoming call.
//
// Abstract:
//  This function is used to validate the CstiProtocolCall object, and accept an
//  incoming call.  This is done by setting the valid capability set,
//  depending on the action taken.
//
// Returns:
//  estiOK (success) or estiERROR (failure)
//
stiHResult CstiProtocolManager::CallAccept (
    CstiCallSP call)
{
	stiLOG_ENTRY_NAME (CstiProtocolManager::CallAccept);
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (call, stiRESULT_ERROR);

	// Do we have a valid pointer to a call object?
	hResult = call->Accept ();

	if (stiIS_ERROR (hResult))
	{
		call->HangUp ();
	}

STI_BAIL:
	return hResult;

}


//:-----------------------------------------------------------------------------
//:
//:          State Handling Methods
//:
//:-----------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//; CstiProtocolManager::NextStateSet
//
// Description: Set the next state
//
// Abstract:
//  This method handles the progression from one state to the next, calling
//  additional functions to handle specific state changes.
//
// Returns:
//  estiOK (Success), estiERROR (Failure)
//
stiHResult CstiProtocolManager::NextStateSet (
    CstiCallSP call,
	const EsmiCallState eNewState,     // The new state that is desired
	const uint32_t un32NewSubState)   // The new substate, Default=NONE
{
	stiLOG_ENTRY_NAME (CstiProtocolManager::NextStateSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (call != nullptr, stiRESULT_ERROR);

	stiDEBUG_TOOL (g_stiCMStateTracking,
	    StateChangeTrack (call, eNewState, un32NewSubState);
	); // end stiDEBUG_TOOL

	if (call->StateGet() != eNewState || call->SubstateGet() != un32NewSubState)
	{
		switch (eNewState)
		{
			case esmiCS_CONNECTING:
			    hResult = StateConnectingProcess (call, eNewState, un32NewSubState);
				break;
				
			case esmiCS_CONNECTED:
			    hResult = StateConnectedProcess (call, eNewState, un32NewSubState);
				break;
				
			case esmiCS_DISCONNECTING:
			    hResult = StateDisconnectingProcess (call, eNewState, un32NewSubState);
				break;
				
			case esmiCS_DISCONNECTED:
			    hResult = StateDisconnectedProcess (call, eNewState, un32NewSubState);
				break;
				
			case esmiCS_HOLD_LCL:
			    hResult = StateHoldLclProcess (call, eNewState, un32NewSubState);
				break;
				
			case esmiCS_HOLD_RMT:
			    hResult = StateHoldRmtProcess (call, eNewState, un32NewSubState);
				break;
				
			case esmiCS_HOLD_BOTH:
			    hResult = StateHoldBothProcess (call, eNewState, un32NewSubState);
				break;
				
			case esmiCS_CRITICAL_ERROR:
			    hResult = StateCriticalErrorProcess (call, eNewState, un32NewSubState);
				break;
				
			case esmiCS_UNKNOWN:
			case esmiCS_IDLE:
			case esmiCS_INIT_TRANSFER:
			case esmiCS_TRANSFERRING:
				// Log an error
				stiASSERT (estiFALSE);
				break;
		} // end switch
		
		// Did we change states successfully?
		if (!stiIS_ERROR (hResult))
		{
			// Yes! Get a copy of the old state to pass into the app notification
			// method
			EsmiCallState ePrevState = esmiCS_UNKNOWN;
			uint32_t un32PrevSubState = estiSUBSTATE_NONE;
			if (call)
			{
				ePrevState = call->StateGet ();
				un32PrevSubState = call->SubstateGet ();
				
				// Remember our new state.
				call->StateSet (eNewState, un32NewSubState);
				
				SstiStateChange StateChange;
				
				StateChange.call = call;
				StateChange.ePrevState = ePrevState;
				StateChange.un32PrevSubStates = un32PrevSubState;
				StateChange.eNewState = eNewState;
				StateChange.un32NewSubStates = un32NewSubState;
				
				// Notify the CM of the state change.
				callStateChangedSignal.Emit (StateChange);

				vpe::EndpointMonitor::instanceGet ()->callStateChanged (StateChange);
			} // end if
		} // end if
		else
		{
			// No! We need to fall into the error state and let the application
			// know.  The handler functions should not return errors under
			// expected error conditions.
			stiDEBUG_TOOL (g_stiCMStateTracking,
					stiTrace ("ERROR: Call state transition failed (%s)\n", StateStringBuild (eNewState, un32NewSubState).c_str());
								); // end stiDEBUG_TOOL
			NextStateSet (call, esmiCS_CRITICAL_ERROR);
			stiASSERT (estiFALSE);
		} // end else
	}

STI_BAIL:

	return (hResult);
} // end CstiProtocolManager::NextStateSet


//:-----------------------------------------------------------------------------
//:
//:          Private Methods
//:
//:-----------------------------------------------------------------------------


//:-----------------------------------------------------------------------------
//:
//:                   DEBUG FUNCTIONS
//:
//:-----------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////////
//; CstiProtocolManager::StateChangeTrack
//
// Description: Track state changes
//
// Abstract:
//  This method is responsible for calling a function that will display what
//  state changes are occurring.
//
// Returns:
//
void CstiProtocolManager::StateChangeTrack (
    CstiCallSP call,
	EsmiCallState eNewState,
	int32_t n32NewSubState)
{
	std::string currentStateName;
	std::string newStateName;
	EsmiCallState eCallState = esmiCS_UNKNOWN;
	uint32_t un32Substate = estiSUBSTATE_NONE;

	if (call)
	{
		eCallState = call->StateGet ();
		un32Substate = call->SubstateGet ();
	} // end if

	currentStateName = StateStringBuild (eCallState, un32Substate);
	newStateName = StateStringBuild (eNewState, n32NewSubState);

	stiTrace ("<%s> Call %p - NewState = %s, OldState = %s\n",
			m_taskName.c_str(), call.get(), newStateName.c_str(), currentStateName.c_str());
}


////////////////////////////////////////////////////////////////////////////////
//; CstiProtocolManager::StateStringBuild
//
// Description: Build a text string representative of the current state
//
// Abstract:
//  This method looks at the current state of the system and creates a text
//  string that represents such state.
//
// Returns:
//  A pointer to the text string
//

std::string StateStringBuild (
	EsmiCallState eState, // The state to be converted to text
	uint32_t un32SubState)   // The substate to be converted to text
{
	stiLOG_ENTRY_NAME (CstiProtocolManager::StateStringBuild);

	std::string stateName;

	switch (eState)
	{
		case esmiCS_UNKNOWN:
			stateName = "UNKNOWN:";
			break;

		case esmiCS_IDLE:
			stateName = "IDLE:";
			break;
		case esmiCS_HOLD_LCL:
			stateName = "HOLD_LCL:";

			// Are there only valid bits set (valid substates)?
			if (0 == (un32SubState & ~(estiSUBSTATE_NEGOTIATING_RMT_HOLD |
									estiSUBSTATE_NEGOTIATING_LCL_RESUME |
//									estiSUBSTATE_LCL_AUDIO |
//									estiSUBSTATE_RMT_AUDIO |
//									estiSUBSTATE_LCL_TEXT |
//									estiSUBSTATE_RMT_TEXT |
//									estiSUBSTATE_LCL_VIDEO |
//									estiSUBSTATE_RMT_VIDEO |
									estiSUBSTATE_HELD)))
			{
				// Yes.
				// Determine the hold/resume, remote/local event
				bool multistate = false;
				if (0 < (estiSUBSTATE_NEGOTIATING_RMT_HOLD & un32SubState))
				{
					stateName += "Hold_Rmt";
					multistate = true;
				} // end if
				
				if (0 < (estiSUBSTATE_NEGOTIATING_LCL_RESUME & un32SubState))
				{
					if (multistate)
					{
						stateName += "|" ;
					}
					stateName += "Resume_Lcl";
					multistate = true;
				} // end else if
				
				if (estiSUBSTATE_HELD == un32SubState)
				{
					if (multistate)
					{
						stateName += "|" ;
					}
					stateName += "Held";
					multistate = true;
				} // end else if
				
				if (!multistate) // didn't find at least one
				{
					stateName += "Unknown";
				} // end else

			}  // end if
			else
			{
				// No.  There is an invalid substate set.  Set it to "Undefined"
				undefinedSubstateAppend (stateName, un32SubState);
			}  // end else
			break;

		case esmiCS_HOLD_RMT:
			stateName = "HOLD_RMT:";

			// Are there only valid bits set (valid substates)?
			if (0 == (un32SubState & ~(estiSUBSTATE_NEGOTIATING_LCL_HOLD |
									estiSUBSTATE_NEGOTIATING_RMT_RESUME |
//									estiSUBSTATE_LCL_AUDIO |
//									estiSUBSTATE_RMT_AUDIO |
//									estiSUBSTATE_LCL_TEXT |
//									estiSUBSTATE_RMT_TEXT |
//									estiSUBSTATE_LCL_VIDEO |
//									estiSUBSTATE_RMT_VIDEO |
									estiSUBSTATE_HELD)))
			{
				// Yes.
				// Determine the hold/resume, remote/local event
				bool multistate = false;
				if (0 < (estiSUBSTATE_NEGOTIATING_LCL_HOLD & un32SubState))
				{					
					stateName += "Hold_Lcl";
					multistate = true;
				} // end if
				
				if (0 < (estiSUBSTATE_NEGOTIATING_RMT_RESUME & un32SubState))
				{
					if (multistate)
					{
						stateName += "|" ;
					}
					stateName += "Resume_Rmt";
					multistate = true;
				} // end else if
				
				if (estiSUBSTATE_HELD == un32SubState)
				{
					if (multistate)
					{
						stateName += "|" ;
					}
					stateName += "Held";
					multistate = true;
				} // end else if
				
				if (!multistate)
				{
					stateName += "Unknown";
				} // end else


			}  // end if
			else
			{
				// No.  There is an invalid substate set.  Set it to "Undefined"
				undefinedSubstateAppend (stateName, un32SubState);
			}  // end else
			break;


		case esmiCS_HOLD_BOTH:
			stateName = "HOLD_BOTH:";

			// Are there only valid bits set (valid substates)?
			if (0 == (un32SubState & ~(estiSUBSTATE_NEGOTIATING_LCL_RESUME |
									estiSUBSTATE_NEGOTIATING_RMT_RESUME |
//									estiSUBSTATE_LCL_AUDIO |
//									estiSUBSTATE_RMT_AUDIO |
//									estiSUBSTATE_LCL_TEXT |
//									estiSUBSTATE_RMT_TEXT |
//									estiSUBSTATE_LCL_VIDEO |
//									estiSUBSTATE_RMT_VIDEO |
									estiSUBSTATE_HELD)))
			{
				// Yes.
				// Determine the hold/resume, remote/local event
				bool multistate = false;
				if (0 < (estiSUBSTATE_NEGOTIATING_LCL_RESUME & un32SubState))
				{
					stateName += "Resume_Lcl";
					multistate = true;
				}
				
				if (0 < (estiSUBSTATE_NEGOTIATING_RMT_RESUME & un32SubState))
				{
					if (multistate)
					{
						stateName += "|" ;
					}

					stateName += "Resume_Rmt";
					multistate = true;
				} // end else if
				
				if (estiSUBSTATE_HELD == un32SubState)
				{
					if (multistate)
					{
						stateName += "|" ;
					}

					stateName += "Held";
					multistate = true;
				} // end else if
				
				if (!multistate)
				{
					stateName += "Unknown";
				} // end else

			}  // end if
			else
			{
				// No.  There is an invalid substate set.  Set it to "Undefined"
				undefinedSubstateAppend (stateName, un32SubState);
			}  // end else
			break;

		case esmiCS_CONNECTING:
			stateName = "CONNECTING:";
			switch (un32SubState)
			{
				case estiSUBSTATE_NONE:
					stateName += "NONE";
					break;

				case estiSUBSTATE_CALLING:
					stateName += "CALLING";
					break;

				case estiSUBSTATE_ANSWERING:
					stateName += "ANSWERING";
					break;

				case estiSUBSTATE_RESOLVE_NAME:
					stateName += "RESOLVE_NAME";
					break;

				case estiSUBSTATE_WAITING_FOR_REMOTE_RESP:
					stateName += "WAITING_FOR_REMOTE_RESP";
					break;

				case estiSUBSTATE_WAITING_FOR_USER_RESP:
					stateName += "WAITING_FOR_USER_RESP";
					break;

				default:
					undefinedSubstateAppend (stateName, un32SubState);
					break;
			}  // end switch
			break;

		case esmiCS_CONNECTED:
			stateName = "CONNECTED:";
			switch (un32SubState)
			{
				case estiSUBSTATE_NONE:
					stateName += "NONE";
					break;

				case estiSUBSTATE_ESTABLISHING:
					stateName += "ESTABLISHING";
					break;

				case estiSUBSTATE_CONFERENCING:
					stateName += "CONFERENCING";
					break;

				default:
					// Handle transitions to HOLD states too.
					// Are there only valid bits set (valid substates)?
					if (0 == (un32SubState & ~(estiSUBSTATE_NEGOTIATING_LCL_HOLD |
											estiSUBSTATE_NEGOTIATING_RMT_HOLD //|
//											estiSUBSTATE_LCL_AUDIO |
//											estiSUBSTATE_RMT_AUDIO |
//											estiSUBSTATE_LCL_TEXT |
//											estiSUBSTATE_RMT_TEXT |
//											estiSUBSTATE_LCL_VIDEO |
//											estiSUBSTATE_RMT_VIDEO
						)))
					{
						// Yes.
						// Determine the hold/resume, remote/local event
						bool multistate = false;
						if (0 < (estiSUBSTATE_NEGOTIATING_LCL_HOLD & un32SubState))
						{
							stateName += "Hold_Lcl";
							multistate = true;
						} // end if
						
						if (0 < (estiSUBSTATE_NEGOTIATING_RMT_HOLD & un32SubState))
						{
							if (multistate)
							{
								stateName += "|" ;
							}
							stateName += "Hold_Rmt";
							multistate = true;
						} // end else if
						
						if(!multistate)
						{
							stateName += "Unknown";
						} // end else

					}  // end if
					else
					{
						// No.  There is an invalid substate set.  Set it to "Undefined"
						undefinedSubstateAppend (stateName, un32SubState);
					}  // end else
					break;
				} // end switch (un32SubState)
			break;

		case esmiCS_DISCONNECTING:
			stateName = "DISCONNECTING:";
			switch (un32SubState)
			{
				case estiSUBSTATE_NONE:
					stateName += "NONE";
					break;

				case estiSUBSTATE_UNKNOWN:
					stateName += "UNKNOWN";
					break;

				case estiSUBSTATE_BUSY:
					stateName += "BUSY";
					break;

				case estiSUBSTATE_REJECT:
					stateName += "REJECT";
					break;

				case estiSUBSTATE_UNREACHABLE:
					stateName += "UNREACHABLE";
					break;

				case estiSUBSTATE_LOCAL_HANGUP:
					stateName += "LOCAL_HANGUP";
					break;

				case estiSUBSTATE_REMOTE_HANGUP:
					stateName += "REMOTE_HANGUP";
					break;

				case estiSUBSTATE_SHUTTING_DOWN:
					stateName += "SHUTTING_DOWN";
					break;

				case estiSUBSTATE_CREATE_VRS_CALL:
					stateName += "CREATE_VRS_CALL";
					break;

				case estiSUBSTATE_ERROR_OCCURRED:
					stateName += "ERROR_OCCURRED";
					break;

				default:
					undefinedSubstateAppend (stateName, un32SubState);
					break;
			}  // end switch
			break;

		case esmiCS_DISCONNECTED:
			stateName = "DISCONNECTED:";
			switch (un32SubState)
			{
				case estiSUBSTATE_NONE:
					stateName += "NONE";
					break;

				case estiSUBSTATE_UNKNOWN:
					stateName += "UNKNOWN";
					break;

				case estiSUBSTATE_BUSY:
					stateName += "BUSY";
					break;

				case estiSUBSTATE_REJECT:
					stateName += "REJECT";
					break;

				case estiSUBSTATE_UNREACHABLE:
					stateName += "UNREACHABLE";
					break;

				case estiSUBSTATE_LOCAL_HANGUP:
					stateName += "LOCAL_HANGUP";
					break;

				case estiSUBSTATE_REMOTE_HANGUP:
					stateName += "REMOTE_HANGUP";
					break;

				case estiSUBSTATE_SHUTTING_DOWN:
					stateName += "SHUTTING_DOWN";
					break;

				case estiSUBSTATE_ERROR_OCCURRED:
					stateName += "ERROR_OCCURRED";
					break;

				case estiSUBSTATE_LEAVE_MESSAGE:
					stateName += "LEAVE_MESSAGE";
					break;

				case estiSUBSTATE_MESSAGE_COMPLETE:
					stateName += "MESSAGE_COMPLETE";
					break;

				default:
					undefinedSubstateAppend (stateName, un32SubState);
					break;
			}  // end switch
			break;

		case esmiCS_CRITICAL_ERROR:
			stateName = "CRITICAL_ERROR:";
			break;

		default:
			undefinedSubstateAppend (stateName, un32SubState);
			break;
	}  // end switch

	return (stateName);
} // end CstiProtocolManager::StateStringBuild



/*!\brief Sets the default provider agreement status
 *
 *  \return nothing
 */
void CstiProtocolManager::DefaultProviderAgreementSet (bool bAgreed)
{
	m_bDefaultProviderAgreementSigned = bAgreed;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiProtocolManager::GetChannelPortBase
//
// Description: Gets the current Channel port base
//
// Returns:
//	the base
//
uint16_t CstiProtocolManager::GetChannelPortBase () {
	return m_nChannelPortBase;
} // end CstiProtocolManager::GetChannelPortBase


////////////////////////////////////////////////////////////////////////////////
//; CstiProtocolManager::GetChannelPortRange
//
// Description: Gets the current Channel port range
//
// Returns:
//	the range
//
uint16_t CstiProtocolManager::GetChannelPortRange ()
{
	return m_nChannelPortRange;
} // end CstiProtocolManager::GetChannelPortRange


////////////////////////////////////////////////////////////////////////////////
//; CstiProtocolManager::ConfigurationSet
//
// Description: Sets the current configuration
//
// Returns:
//	the range
//
void CstiProtocolManager::ConfigurationSet (
	EstiInterfaceMode eLocalInterfaceMode,
	bool bEnforceAuthorizedPhoneNumber)
{
	Lock ();

	m_eLocalInterfaceMode = eLocalInterfaceMode;
	m_bEnforceAuthorizedPhoneNumber = bEnforceAuthorizedPhoneNumber;

	Unlock ();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiProtocolManager::ConferenceParamsSet
//
// Description:
//
// Abstract:
//
// Returns:
//	estiOK (Success) or estiERROR (Failure)
//
stiHResult CstiProtocolManager::ConferenceParamsSet (
	const SstiConferenceParams *pstConferenceParams)
{
	stiLOG_ENTRY_NAME (CstiProtocolManager::ConferenceParamsSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	m_stConferenceParams = *pstConferenceParams;

	Unlock ();

	return (hResult);
} // end CstiProtocolManager::ConferenceParamsSet


void CstiProtocolManager::AutoRejectSet (
	bool bAutoReject)
{
	Lock ();

	m_bAutoReject = bAutoReject;

	Unlock ();
}


void CstiProtocolManager::MaxCallsSet (
	unsigned int maxCalls)
{
	Lock ();

	m_maxCalls = maxCalls;

	Unlock ();
}


///\brief Sets the product name
///
void CstiProtocolManager::productSet (
	ProductType productType, ///< The name of the product
	const std::string &version) /// < The version of the product
{
	Lock ();
	m_productType = productType;
	m_version = version;
	Unlock ();
}


///\brief Determine the number of calls there are in the given state(s).
///
unsigned int CstiProtocolManager::CallObjectsCountGet (
	uint32_t un32StateMask)
{
	stiLOG_ENTRY_NAME (CstiProtocolManager::CallObjectsCountGet);

	return m_pCallStorage->countGet (un32StateMask);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiProtocolManager::UserIdsSet
//
// Description: Sets the currently authenticated user id
//
// Abstract:
//
// Returns:
//	estiOK when successful, otherwise estiERROR
//
stiHResult CstiProtocolManager::UserIdsSet (
	const std::string &userId,
	const std::string &groupUserId)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	m_userId = userId;
	m_groupUserId = groupUserId;

	return hResult;
} // end CstiProtocolManager::UserIdsSet

/////////////////////////////////////////////////////////////////////////////////
//; CstiProtocolManager::OutgoingToneSet
//
// Description: Set the tone played for a failed outgoing call.
//
// Abstract:
//
// Returns:
//  none
//
stiHResult CstiProtocolManager::OutgoingToneSet (
	EstiToneMode eTone)
{
	return stiRESULT_SUCCESS;
}


int CstiProtocolManager::G711AudioRecordMaxSampleRateGet () const
{
	return (IstiAudioOutput::InstanceGet ()->MaxPacketDurationGet ());
}


int CstiProtocolManager::G711AudioPlaybackMaxSampleRateGet () const
{
	return (IstiAudioInput::InstanceGet ()->MaxPacketDurationGet ());
}


stiHResult CstiProtocolManager::VideoPlaybackCodecsGet (
	std::list<EstiVideoCodec> *pCodecs)
{
	stiHResult hResult = IstiVideoOutput::InstanceGet ()->VideoCodecsGet (pCodecs);

	LimitVideoCodecsPerPropertyTable (pCodecs);

	return hResult;
}


stiHResult CstiProtocolManager::VideoRecordCodecsGet (
	std::list<EstiVideoCodec> *pCodecs)
{
	stiHResult hResult = IstiVideoInput::InstanceGet ()->VideoCodecsGet (pCodecs);

	LimitVideoCodecsPerPropertyTable (pCodecs);

	return hResult;
}

void CstiProtocolManager::LimitVideoCodecsPerPropertyTable (
		std::list<EstiVideoCodec> *pCodecs)
{
	// If the property table has "AllowedVideoEncoders", remove those that are not enabled
	if (estiVIDEOCODEC_ALL != m_stConferenceParams.eAllowedVideoEncoders)
	{
		for (auto it = pCodecs->begin (); it != pCodecs->end ();)
		{
			EstiVideoCodec2 eCodec = estiVIDEOCODEC_NONE;

			// Convert from the EstiVideoCodec to EstiVideoCodec so that we are testing the appropriate thing.
			switch (*it)
			{
				case estiVIDEO_NONE:
					eCodec = estiVIDEOCODEC_NONE;
					break;

				case estiVIDEO_H263:
					eCodec = estiVIDEOCODEC_H263;
					break;

				case estiVIDEO_H264:
					eCodec = estiVIDEOCODEC_H264;
					break;

				case estiVIDEO_H265:
					eCodec = estiVIDEOCODEC_H265;
					break;

				case estiVIDEO_RTX:
					break;
			}

			if (!(eCodec & m_stConferenceParams.eAllowedVideoEncoders))
			{
				it = pCodecs->erase (it);
			}
			else
			{
				it++;
			}
		}
	}
}

stiHResult CstiProtocolManager::AudioCodecsGet (
	std::list<EstiAudioCodec> *pCodecs)
{
  return IstiAudioOutput::InstanceGet ()->CodecsGet (pCodecs);
}


///\brief Reports to the calling object which packetization schemes this device is capable of.
///
///\brief This method is reporting the schemes we are capable of using for Video Playback.
stiHResult CstiProtocolManager::VideoPlaybackPacketizationSchemesGet (
	std::list<EstiPacketizationScheme> *pPacketizationSchemes)
{
	// Add schemes in the order that you would like preference to be given during channel negotiations.	
	pPacketizationSchemes->push_back (estiH265_NON_INTERLEAVED);
	pPacketizationSchemes->push_back (estiH264_NON_INTERLEAVED);
	pPacketizationSchemes->push_back (estiH264_SINGLE_NAL);

	return stiRESULT_SUCCESS;
}


//\brief Reports to the calling object which packetization schemes this device is capable of.
///\brief Add schemes in the order that you would like preference to be given during channel
///\brief negotiations.
stiHResult CstiProtocolManager::VideoRecordPacketizationSchemesGet (
	std::list<EstiPacketizationScheme> *pPacketizationSchemes)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Add the supported packetizations to the list.  We do the packetization in CstiVideoRecord, therefore we do not
	// need to obtain this list form the platform layer.
	pPacketizationSchemes->push_back (estiH265_NON_INTERLEAVED);
	pPacketizationSchemes->push_back (estiH264_NON_INTERLEAVED);
//	pPacketizationSchemes->push_back (estiH264_INTERLEAVED);
#if APPLICATION != APP_NTOUCH_IOS && APPLICATION != APP_NTOUCH_MAC && APPLICATION != APP_DHV_IOS && APPLICATION != APP_NTOUCH_VP3 && APPLICATION != APP_NTOUCH_VP4
	pPacketizationSchemes->push_back (estiH264_SINGLE_NAL);
#endif

	return hResult;
}

stiHResult CstiProtocolManager::PlaybackCodecCapabilitiesGet (EstiVideoCodec eCodec, SstiVideoCapabilities *pstCaps)
{
	stiHResult hResult = IstiVideoOutput::InstanceGet ()->CodecCapabilitiesGet (eCodec, pstCaps);

	stiTESTRESULT ();

	if (estiVIDEO_H264 == eCodec)
	{
		auto pstH264Caps = (SstiH264Capabilities*)pstCaps;

		// Verify that any upgrades are truly upgrades for the selected profile.  If they are found to be equal to
		// or less than the expected amount for the given level, then change it back to 0 so it won't be signaled
		// as an upgrade.
		switch (pstH264Caps->eLevel)
		{
			case estiH264_LEVEL_1:
				if (pstH264Caps->unCustomMaxMBPS && (int)pstH264Caps->unCustomMaxMBPS <= nMAX_MBPS_1)
				{
					pstH264Caps->unCustomMaxMBPS = 0;
					stiASSERT (estiFALSE);
				}
				if (pstH264Caps->unCustomMaxFS && (int)pstH264Caps->unCustomMaxFS <= nMAX_FS_1)
				{
					pstH264Caps->unCustomMaxFS = 0;
					stiASSERT (estiFALSE);
				}
				break;

// 			  case estiH264_LEVEL_1_B:
//				  break;

			case estiH264_LEVEL_1_1:
				if (pstH264Caps->unCustomMaxMBPS && (int)pstH264Caps->unCustomMaxMBPS <= nMAX_MBPS_1_1)
				{
					pstH264Caps->unCustomMaxMBPS = 0;
					stiASSERT (estiFALSE);
				}
				if (pstH264Caps->unCustomMaxFS && (int)pstH264Caps->unCustomMaxFS <= nMAX_FS_1_1)
				{
					pstH264Caps->unCustomMaxFS = 0;
					stiASSERT (estiFALSE);
				}
				break;

			case estiH264_LEVEL_1_2:
				if (pstH264Caps->unCustomMaxMBPS && (int)pstH264Caps->unCustomMaxMBPS <= nMAX_MBPS_1_2)
				{
					pstH264Caps->unCustomMaxMBPS = 0;
					stiASSERT (estiFALSE);
				}
				if (pstH264Caps->unCustomMaxFS && (int)pstH264Caps->unCustomMaxFS <= nMAX_FS_1_2)
				{
					pstH264Caps->unCustomMaxFS = 0;
					stiASSERT (estiFALSE);
				}
				break;

			case estiH264_LEVEL_1_3:
				if (pstH264Caps->unCustomMaxMBPS && (int)pstH264Caps->unCustomMaxMBPS <= nMAX_MBPS_1_3)
				{
					pstH264Caps->unCustomMaxMBPS = 0;
					stiASSERT (estiFALSE);
				}
				if (pstH264Caps->unCustomMaxFS && (int)pstH264Caps->unCustomMaxFS <= nMAX_FS_1_3)
				{
					pstH264Caps->unCustomMaxFS = 0;
					stiASSERT (estiFALSE);
				}
				break;

			case estiH264_LEVEL_2:
				if (pstH264Caps->unCustomMaxMBPS && (int)pstH264Caps->unCustomMaxMBPS <= nMAX_MBPS_2)
				{
					pstH264Caps->unCustomMaxMBPS = 0;
					stiASSERT (estiFALSE);
				}
				if (pstH264Caps->unCustomMaxFS && (int)pstH264Caps->unCustomMaxFS <= nMAX_FS_2)
				{
					pstH264Caps->unCustomMaxFS = 0;
					stiASSERT (estiFALSE);
				}
				break;

			case estiH264_LEVEL_2_1:
				if (pstH264Caps->unCustomMaxMBPS && (int)pstH264Caps->unCustomMaxMBPS <= nMAX_MBPS_2_1)
				{
					pstH264Caps->unCustomMaxMBPS = 0;
					stiASSERT (estiFALSE);
				}
				if (pstH264Caps->unCustomMaxFS && (int)pstH264Caps->unCustomMaxFS <= nMAX_FS_2_1)
				{
					pstH264Caps->unCustomMaxFS = 0;
					stiASSERT (estiFALSE);
				}
				break;

			case estiH264_LEVEL_2_2:
				if (pstH264Caps->unCustomMaxMBPS && (int)pstH264Caps->unCustomMaxMBPS <= nMAX_MBPS_2_2)
				{
					pstH264Caps->unCustomMaxMBPS = 0;
					stiASSERT (estiFALSE);
				}
				if (pstH264Caps->unCustomMaxFS && (int)pstH264Caps->unCustomMaxFS <= nMAX_FS_2_2)
				{
					pstH264Caps->unCustomMaxFS = 0;
					stiASSERT (estiFALSE);
				}
				break;

			case estiH264_LEVEL_3:
				if (pstH264Caps->unCustomMaxMBPS && (int)pstH264Caps->unCustomMaxMBPS <= nMAX_MBPS_3)
				{
					pstH264Caps->unCustomMaxMBPS = 0;
					stiASSERT (estiFALSE);
				}
				if (pstH264Caps->unCustomMaxFS && (int)pstH264Caps->unCustomMaxFS <= nMAX_FS_3)
				{
					pstH264Caps->unCustomMaxFS = 0;
					stiASSERT (estiFALSE);
				}
				break;

			case estiH264_LEVEL_3_1:
				if (pstH264Caps->unCustomMaxMBPS && (int)pstH264Caps->unCustomMaxMBPS <= nMAX_MBPS_3_1)
				{
					pstH264Caps->unCustomMaxMBPS = 0;
					stiASSERT (estiFALSE);
				}
				if (pstH264Caps->unCustomMaxFS && (int)pstH264Caps->unCustomMaxFS <= nMAX_FS_3_1)
				{
					pstH264Caps->unCustomMaxFS = 0;
					stiASSERT (estiFALSE);
				}
				break;

			case estiH264_LEVEL_3_2:
				if (pstH264Caps->unCustomMaxMBPS && (int)pstH264Caps->unCustomMaxMBPS <= nMAX_MBPS_3_2)
				{
					pstH264Caps->unCustomMaxMBPS = 0;
					stiASSERT (estiFALSE);
				}
				if (pstH264Caps->unCustomMaxFS && (int)pstH264Caps->unCustomMaxFS <= nMAX_FS_3_2)
				{
					pstH264Caps->unCustomMaxFS = 0;
					stiASSERT (estiFALSE);
				}
				break;

			case estiH264_LEVEL_4 :
				if (pstH264Caps->unCustomMaxMBPS && (int)pstH264Caps->unCustomMaxMBPS <= nMAX_MBPS_4)
				{
					pstH264Caps->unCustomMaxMBPS = 0;
					stiASSERT (estiFALSE);
				}
				if (pstH264Caps->unCustomMaxFS && (int)pstH264Caps->unCustomMaxFS <= nMAX_FS_4)
				{
					pstH264Caps->unCustomMaxFS = 0;
					stiASSERT (estiFALSE);
				}
				break;

			case estiH264_LEVEL_4_1:
				if (pstH264Caps->unCustomMaxMBPS && (int)pstH264Caps->unCustomMaxMBPS <= nMAX_MBPS_4_1)
				{
					pstH264Caps->unCustomMaxMBPS = 0;
					stiASSERT (estiFALSE);
				}
				if (pstH264Caps->unCustomMaxFS && (int)pstH264Caps->unCustomMaxFS <= nMAX_FS_4_1)
				{
					pstH264Caps->unCustomMaxFS = 0;
					stiASSERT (estiFALSE);
				}
				break;

			case estiH264_LEVEL_4_2:
				if (pstH264Caps->unCustomMaxMBPS && (int)pstH264Caps->unCustomMaxMBPS <= nMAX_MBPS_4_2)
				{
					pstH264Caps->unCustomMaxMBPS = 0;
					stiASSERT (estiFALSE);
				}
				if (pstH264Caps->unCustomMaxFS && (int)pstH264Caps->unCustomMaxFS <= nMAX_FS_4_2)
				{
					pstH264Caps->unCustomMaxFS = 0;
					stiASSERT (estiFALSE);
				}
				break;

			case estiH264_LEVEL_5:
				if (pstH264Caps->unCustomMaxMBPS && (int)pstH264Caps->unCustomMaxMBPS <= nMAX_MBPS_5)
				{
					pstH264Caps->unCustomMaxMBPS = 0;
					stiASSERT (estiFALSE);
				}
				if (pstH264Caps->unCustomMaxFS && (int)pstH264Caps->unCustomMaxFS <= nMAX_FS_5)
				{
					pstH264Caps->unCustomMaxFS = 0;
					stiASSERT (estiFALSE);
				}
				break;

			case estiH264_LEVEL_5_1:
				if (pstH264Caps->unCustomMaxMBPS && (int)pstH264Caps->unCustomMaxMBPS <= nMAX_MBPS_5_1)
				{
					pstH264Caps->unCustomMaxMBPS = 0;
					stiASSERT (estiFALSE);
				}
				if (pstH264Caps->unCustomMaxFS && (int)pstH264Caps->unCustomMaxFS <= nMAX_FS_5_1)
				{
					pstH264Caps->unCustomMaxFS = 0;
					stiASSERT (estiFALSE);
				}
				break;

//			default:	// Comment so that if other levels are introduced, we get a compile error.
		}
	}

STI_BAIL:

	return (hResult);
}


stiHResult CstiProtocolManager::RecordCodecCapabilitiesGet (EstiVideoCodec eCodec, SstiVideoCapabilities *pstCaps)
{
	return IstiVideoInput::InstanceGet ()->CodecCapabilitiesGet (eCodec, pstCaps);
}

std::string CstiProtocolManager::productNameGet ()
{
	return productNameConvert (m_productType);
}

ProductType CstiProtocolManager::productGet ()
{
	return m_productType;
}


// end file CstiProtocolManager.cpp
