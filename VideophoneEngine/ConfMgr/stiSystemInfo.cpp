////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved
//
//  Module Name:	stiSystemInfo
//
//  File Name:	stiSystemInfo.cpp
//
//  Owner:		Eugene Christensen
//
//	Abstract:
//		This module contains methods that are provided for serializing/
//		deserializing system information into/from a string for
//		transmission.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
#include "stiSystemInfo.h"
#include <cstring>
#include <cstdio>
#include "stiMem.h"
#include "stiTrace.h"
#include "stiTools.h"
#include "CstiCall.h"
#include "CstiCallInfo.h"
#include "CstiProtocolCall.h"
#include "CstiSipCall.h"
#include "CstiSipCallLeg.h"
#include <sstream>
#include <iomanip>
#include <climits>

//
// Constants
//
const uint8_t stiTAG_LEN = 4;
const uint8_t nstiUINT8_LEN = 5;				// The # of chars to write it
const uint8_t nstiUINT16_LEN = 5;				// The # of chars to write it
const uint8_t nstiUINT32_LEN = 8;				// The # of chars to write it
const uint8_t nstiBOOL_LEN = 1;				// The # of chars to write it

static const std::string INFO_HEADER = stiINFO_HEADER_CSTR;

//#define SERILIZE_DESERIALIZE_TEST


//
// Typedefs
//

enum EstiDataTypeTag
{
	// WARNING! DO NOT DELETE any of the items in this enumeration.  If an
	// item is no longer used, change the name of the ID to include _UNUSED.
	// All entries must have an assigned entry.  Do not let the compiler assign values.
	// NOTE:  The user specific data should use data tags that are assigned
	// 	values between 0 - 254.  The system specific data will use data tags
	//	between 255 - 511.  Other derivatives could use values 512 and above.

	// User specific data
//	estiTAG_USER_ID = 0,		// A textual user id--no longer used
//	estiTAG_DIRECTORY_SERVER = 1,
//	estiTAG_FIRST_NAME = 2,
//	estiTAG_LAST_NAME = 3,
//	estiTAG_NAME = 4,			// This tag has been deprecated.
//	estiTAG_EMAIL_ADDRESS = 5,
//	estiTAG_COMPANY_NAME = 6,
	estiTAG_PHONE_NUMBER = 7,
	estiTAG_AUTO_DETECTED_PUBLIC_IP = 8,
//	estiTAG_CITY_STATE = 9,
//	estiTAG_COUNTRY = 10,
//	estiTAG_COMMENTS = 11,
	estiTAG_DIAL_STRING = 12,
//	estiTAG_CATEGORY = 13,
//	estiTAG_ATTRIBUTES = 14,
//	estiTAG_DIAL_TYPE_UNUSED = 15,		// Not used by VP-200
//	estiTAG_CD_KEY = 16,		// Sorenson Vision Style CD-KEY in format SEN-XXX-XXXX-XXXX
//	estiTAG_SERIAL_NUMBER = 17,	// Sorenson Media Style Serial Number in format XXXXXX-XXXXXX-XXXXXX-XXXXXX-XXXXXX
	estiTAG_MAC_ADDRESS = 18,
	estiTAG_VCO_CALLBACK = 19,
	estiTAG_VRS_INTERFACE_MODE = 20,
//	estiTAG_DIRECTORY_RESOLVED = 21,
//	estiTAG_LIGHT_RING_PATTERN = 22,
	estiTAG_TOLL_FREE_NUMBER = 23,
	estiTAG_SORENSON_NUMBER = 24,
	estiTAG_USERID = 25,		// The integer id of the user, used on the backend
//	estiTAG_LATER = 26,		// Indicates more SInfo is coming in a custom message later.
	estiTAG_LOCAL_NUMBER = 27,
	estiTAG_HEARING_NUMBER = 28,
	estiTAG_RETURN_CALL_DIAL_METHOD = 29, // The dial method that should be used by the Called Party to return the call
	estiTAG_REGISTERED = 30, // Flag indicating the user has registered
	estiTAG_ALTERNATE_NAME = 31, // Contains the alternate name (such as interpreter id)
	estiTAG_VRS_LANGUAGE = 32, // Language to use for VRS Interpreter (as an integer id)
	estiTAG_VERIFY_ADDRESS = 33, // Informs the interpreter that the address needs to be verified with the customer.
	estiTAG_VRS_LANGUAGE_STRING = 34, // Language to use for VRS Interpreter (as a char string)
	estiTAG_VCO_TYPE_PREF = 35, // Type of VCO preferred by the endpoint (0 = none, 1 = 1-line, 2 = 2-line)
	estiTAG_VCO_ACTIVE = 36, // Indicates whether this is a VCO call or not
	estiTAG_RING_GROUP_LOCAL_NUMBER = 37, // The Ring Group local number
	estiTAG_RING_GROUP_TOLL_FREE_NUMBER = 38, // The Ring Group toll free number
	estiTAG_GROUP_USER_ID = 39, // The Ring Group user id
	estiTAG_AUTO_SPEED_SETTING = 40,
	estiTAG_ADD_MISSED_CALL = 41, // callee must leave missed call (because caller did not do directory resolve)
	estiTAG_VRS_CALL_ID = 42, // Contains the VRS Call ID (needed to log the terp Id)
	estiTAG_VRS_FOCUSED_ROUTING = 43, // Contains a value that relay can use to route calls to interpreters with a matching value.
	
	// System specific data
//	estiTAG_AUDIO_REDUNDANCY_TYPE = 255,
//	estiTAG_HARDWARE_VERSION_ID = 256,
//	estiTAG_CONNECTION_TYPE = 257,
//	estiTAG_PRODUCT_ID = 258,			// When not an end product, this is the
//	estiTAG_PRODUCT_VERSION_ID = 259,	// info about the deliverable to the
										// customer.
//	estiTAG_HARDWARE_ID = 260,
	estiTAG_PRODUCT_NAME = 261,
	estiTAG_PRODUCT_VERSION = 262,
	estiTAG_SORENSON_SIP_VERSION = 263

};

//
// Forward Declarations
//
#ifdef SERILIZE_DESERIALIZE_TEST
stiHResult SerializeDeserializeTest ();
#endif

//
// Globals
//

//
// Locals
//


////////////////////////////////////////////////////////////////////////////////
//; PhoneNumberStrip
//
// Description: Strips formatting characters from a phonenumber.
//
// Abstract:
//	This function removes any -, (, ), + or space from a phonenumber.
//
// Returns:
//	None
//
static size_t PhoneNumberStrip(
	char *pszDestination,
	const char *pszSource,
	int nMaxLength)
{
	size_t nLength = strlen (pszSource);
	int j = 0;

	for (size_t i = 0; i < nLength && j < nMaxLength; i++)
	{
		if ('-' != pszSource[i] && '(' != pszSource[i] &&
			')' != pszSource[i] && '+' != pszSource[i] &&
			' ' != pszSource[i])
		{
			pszDestination[j++] = pszSource[i];
		} // end if
	} // end for

	if (j < nMaxLength)
	{
		// Add a NULL terminator now
		pszDestination[j] = '\0';
	}
#if __APPLE__
	return std::min(strlen(pszDestination), (size_t)nMaxLength);
#else
	return strnlen(pszDestination, nMaxLength);
#endif
}


////////////////////////////////////////////////////////////////////////////////
//; ElementSet
//
// Description: Set the given element into the call object
//
// Abstract:
//	This method takes the incoming string, replaces all occurrences of "||"
//	with a single '|'.  Once this replacement has been handled, it stores the
//	data in the appropriate member variable identified by n32Tag.
//
// Returns:
//	estiTRUE if the element is found and set, else estiFALSE
//
EstiBool ElementSet (
    CstiCallSP call,
    CstiSipCallLegSP callLeg,
	int32_t n32Tag,			// Tag representing which member it is
	const std::string &element)	// Data to be set into the member variable
{
	stiLOG_ENTRY_NAME (ElementSet);

	EstiBool bResult = estiFALSE;
	
	// Does the string contain anything and did we successfully allocate the
	// new string memory?
	if (!element.empty () && call)
	{
			// Yes!
		    CstiProtocolCallSP protocolCall = call->ProtocolCallGet ();
			auto poRemoteCallInfo = (CstiCallInfo *)call->RemoteCallInfoGet ();

			switch ((EstiDataTypeTag) n32Tag)
			{
				case estiTAG_ALTERNATE_NAME:
				{
					protocolCall->RemoteAlternateNameSet (element.c_str ());

					if (call->RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_VIDEOPHONE))
					{
						// Remove "Interpreter " from the start of the interpreter ID
						std::string terpId = element;
						const std::string prefix = "Interpreter ";
						auto i = terpId.find (prefix);
						if (i != std::string::npos)
						{
							terpId.erase (i, prefix.length ());
						}
						call->vrsAgentIdSet (terpId);
					}
					bResult = estiTRUE;
					break;
				}

				case estiTAG_VRS_CALL_ID:
				{
					call->vrsCallIdSet(element);
					bResult = estiTRUE;
					break;
				}

				case estiTAG_AUTO_DETECTED_PUBLIC_IP:
				    call->RemoteIpAddressSet (element);
					bResult = estiTRUE;
					break;

				case estiTAG_DIAL_STRING:
					// This tag is no longer used.  From now on this information will come from the SIP address.
					bResult = estiTRUE;
					break;

				case estiTAG_RETURN_CALL_DIAL_METHOD:
				{
					auto  eMethod = (EstiDialMethod)atoi (element.c_str ());

					switch (eMethod)
					{
						case estiDM_BY_DIAL_STRING:
						case estiDM_BY_DS_PHONE_NUMBER:
						case estiDM_BY_VRS_PHONE_NUMBER:
						case estiDM_BY_VRS_WITH_VCO:
						case estiDM_UNKNOWN:
						case estiDM_BY_OTHER_VRS_PROVIDER:
						    call->RemoteDialMethodSet (eMethod);
							bResult = estiTRUE;
							break;

						default:
							// Don't set it since we don't know what it is.
							break;
					}
					break;
				}

				case estiTAG_MAC_ADDRESS:
				    protocolCall->RemoteMacAddressSet (element.c_str ());
					bResult = estiTRUE;
					break;

				case estiTAG_PHONE_NUMBER:
				{
					SstiUserPhoneNumbers stPhoneNumbers = *poRemoteCallInfo->UserPhoneNumbersGet ();
					const int nFieldLen = sizeof (stPhoneNumbers.szPreferredPhoneNumber);
					PhoneNumberStrip(stPhoneNumbers.szPreferredPhoneNumber, element.c_str (), nFieldLen - 1);
					stPhoneNumbers.szPreferredPhoneNumber[nFieldLen - 1] = '\0';
					poRemoteCallInfo->UserPhoneNumbersSet (&stPhoneNumbers);
					bResult = estiTRUE;
					break;
				}

				case estiTAG_HEARING_NUMBER:
				{
					SstiUserPhoneNumbers stPhoneNumbers = *poRemoteCallInfo->UserPhoneNumbersGet ();
					const int nFieldLen = sizeof (stPhoneNumbers.szHearingPhoneNumber);
					PhoneNumberStrip(stPhoneNumbers.szHearingPhoneNumber, element.c_str (), nFieldLen - 1);
					stPhoneNumbers.szHearingPhoneNumber[nFieldLen - 1] = '\0';
					poRemoteCallInfo->UserPhoneNumbersSet (&stPhoneNumbers);
					bResult = estiTRUE;
					break;
				}

				case estiTAG_VCO_CALLBACK:
				    protocolCall->RemoteVcoCallbackSet (element.c_str ());
					bResult = estiTRUE;
					break;

				case estiTAG_VRS_INTERFACE_MODE:
				    protocolCall->RemoteInterfaceModeSet (
						(EstiInterfaceMode)atoi (element.c_str ()));
					bResult = estiTRUE;
					break;

				case estiTAG_TOLL_FREE_NUMBER:
				{
					SstiUserPhoneNumbers stPhoneNumbers = *poRemoteCallInfo->UserPhoneNumbersGet ();
					const int nFieldLen = sizeof (stPhoneNumbers.szTollFreePhoneNumber);
					strncpy (stPhoneNumbers.szTollFreePhoneNumber, element.c_str (), nFieldLen - 1);
					stPhoneNumbers.szTollFreePhoneNumber[nFieldLen - 1] = '\0';
					poRemoteCallInfo->UserPhoneNumbersSet (&stPhoneNumbers);
					bResult = estiTRUE;
					break;
				}

				case estiTAG_SORENSON_NUMBER:
				{
					SstiUserPhoneNumbers stPhoneNumbers = *poRemoteCallInfo->UserPhoneNumbersGet ();
					const int nFieldLen = sizeof (stPhoneNumbers.szSorensonPhoneNumber);
					strncpy (stPhoneNumbers.szSorensonPhoneNumber, element.c_str (), nFieldLen - 1);
					stPhoneNumbers.szSorensonPhoneNumber[nFieldLen - 1] = '\0';
					poRemoteCallInfo->UserPhoneNumbersSet (&stPhoneNumbers);
					bResult = estiTRUE;
					break;
				}

				case estiTAG_LOCAL_NUMBER:
				{
					SstiUserPhoneNumbers stPhoneNumbers = *poRemoteCallInfo->UserPhoneNumbersGet ();
					const int nFieldLen = sizeof (stPhoneNumbers.szLocalPhoneNumber);
					strncpy (stPhoneNumbers.szLocalPhoneNumber, element.c_str (), nFieldLen - 1);
					stPhoneNumbers.szLocalPhoneNumber[nFieldLen - 1] = '\0';
					poRemoteCallInfo->UserPhoneNumbersSet (&stPhoneNumbers);
					bResult = estiTRUE;
					break;
				}

				case estiTAG_RING_GROUP_LOCAL_NUMBER:
				{
					SstiUserPhoneNumbers stPhoneNumbers = *poRemoteCallInfo->UserPhoneNumbersGet ();
					const int nFieldLen = sizeof (stPhoneNumbers.szRingGroupLocalPhoneNumber);
					strncpy (stPhoneNumbers.szRingGroupLocalPhoneNumber, element.c_str (), nFieldLen - 1);
					stPhoneNumbers.szRingGroupLocalPhoneNumber[nFieldLen - 1] = '\0';
					poRemoteCallInfo->UserPhoneNumbersSet (&stPhoneNumbers);
					bResult = estiTRUE;
					break;
				}
				
				case estiTAG_RING_GROUP_TOLL_FREE_NUMBER:
				{
					SstiUserPhoneNumbers stPhoneNumbers = *poRemoteCallInfo->UserPhoneNumbersGet ();
					const int nFieldLen = sizeof (stPhoneNumbers.szRingGroupTollFreePhoneNumber);
					strncpy (stPhoneNumbers.szRingGroupTollFreePhoneNumber, element.c_str (), nFieldLen - 1);
					stPhoneNumbers.szRingGroupTollFreePhoneNumber[nFieldLen - 1] = '\0';
					poRemoteCallInfo->UserPhoneNumbersSet (&stPhoneNumbers);
					bResult = estiTRUE;
					break;
				}
				
				case estiTAG_USERID:
					poRemoteCallInfo->UserIDSet (element);
					bResult = estiTRUE;
					break;

				case estiTAG_GROUP_USER_ID:
					poRemoteCallInfo->GroupUserIDSet (element);
					bResult = estiTRUE;
					break;

				case estiTAG_VCO_TYPE_PREF:
				    call->RemoteVcoTypeSet ((EstiVcoType)atoi (element.c_str ()));
					break;

				case estiTAG_VCO_ACTIVE:
				    call->RemoteIsVcoActiveSet ((EstiBool)atoi (element.c_str ()));
					break;

				case estiTAG_REGISTERED:
				    call->RemoteRegisteredSet ((ETriState)atoi (element.c_str ()));
					bResult = estiTRUE;
					break;

				case estiTAG_VRS_LANGUAGE:
				    call->RemotePreferredLanguageSet (atoi (element.c_str ()));
					bResult = estiTRUE;
					break;

				case estiTAG_VRS_LANGUAGE_STRING:
				    call->RemotePreferredLanguageSet (element);
					bResult = estiTRUE;
					break;

				case estiTAG_VERIFY_ADDRESS:
				    call->VerifyAddressSet (atoi (element.c_str ()) ? estiTRUE : estiFALSE);
					bResult = estiTRUE;
					break;

				case estiTAG_PRODUCT_NAME:

				    if (callLeg)
					{
						callLeg->RemoteProductNameSet (element.c_str ());
					}
					else
					{
						stiASSERT (estiFALSE);
					}

					break;

				case estiTAG_PRODUCT_VERSION:

				    if (callLeg)
					{
						callLeg->RemoteProductVersionSet (element.c_str ());
					}
					else
					{
						stiASSERT (estiFALSE);
					}

					break;

				case estiTAG_SORENSON_SIP_VERSION:

				    if (callLeg)
					{
						callLeg->RemoteSIPVersionSet (atoi(element.c_str ()));
					}
					else
					{
						stiASSERT (estiFALSE);
					}

					break;
					
				case estiTAG_AUTO_SPEED_SETTING:
					
				    if (callLeg)
					{
						callLeg->RemoteAutoSpeedSettingSet((EstiAutoSpeedMode)atoi(element.c_str ()));
					}
					else
					{
						stiASSERT (estiFALSE);
					}
					
					break;
					
				case estiTAG_ADD_MISSED_CALL:
				{
					call->AddMissedCallSet(atoi (element.c_str ()) != 0);

					break;
				}
					
				case estiTAG_VRS_FOCUSED_ROUTING:
				{
					callLeg->vrsFocusedRoutingReceivedSet (element);
					break;
				}
			} // end switch
	}
	
	return (bResult);
} // end ElementSet


/*!\brief Adds the 4 digit tag number to the transmit string
 *
 * \param pTransmitString The string that will be transmitted to the remote endpoint
 * \param n32Tag The tag number to add to the transmit string
 */
void ElementTagAdd (
	std::stringstream *pTransmitString,
	int32_t n32Tag)
{
	*pTransmitString << std::setfill('0') << std::setw(stiTAG_LEN) << n32Tag;
}


/*!\brief Adds the terminating '|' character to the transmit string
 *
 * \param pTransmitString The string that will be transmitted to the remote endpoint
 */
void ElementTerminatorAdd (
		std::stringstream *pTransmitString)
{
	*pTransmitString << "|";
}


////////////////////////////////////////////////////////////////////////////////
//; ElementSerialize
//
// Description: Build the transmit string for the given element
//
// Abstract:
//	Convert the input un8Element to a character string, then call the function
//	that will format the actual transmit string.
//
// Returns:
//	none
//
// ElementSerialize is overloaded for the common types:
//    EstiBool,int8_t,int16_t,int32_t,uint8_t,uint16_t,uint32_t,and char*
void ElementSerialize (
	std::stringstream *pTransmitString, // This will be the formatted transmit string
	int32_t n32Tag,				// This denotes the field where the data is
	const uint8_t un8Element)  // Value stored with the tag
{
	stiLOG_ENTRY_NAME (ElementSerialize);

	ElementTagAdd (pTransmitString, n32Tag);

	*pTransmitString << std::setfill('0') << std::setw(nstiUINT8_LEN) << (uint16_t)un8Element;

	ElementTerminatorAdd (pTransmitString);
} // end ElementSerialize


////////////////////////////////////////////////////////////////////////////////
//; ElementSerialize
//
// Description: Build the transmit string for the given element
//
// Abstract:
//	Convert the input un16Element to a character string, then call the function
//	that will format the actual transmit string.
//
// Returns:
//	none
//
// ElementSerialize is overloaded for the common types:
//    EstiBool,int8_t,int16_t,int32_t,uint8_t,uint16_t,uint32_t,and char*
void ElementSerialize (
	std::stringstream *pTransmitString, // This will be the formatted transmit string
	int32_t n32Tag,					// This denotes the field where the data is
	const uint16_t un16Element)		// Value stored with the tag
{
	stiLOG_ENTRY_NAME (ElementSerialize);

	ElementTagAdd (pTransmitString, n32Tag);

	*pTransmitString << std::setfill('0') << std::setw(nstiUINT16_LEN) << un16Element;

	ElementTerminatorAdd (pTransmitString);
} // end ElementSerialize


////////////////////////////////////////////////////////////////////////////////
//; ElementSerialize
//
// Description: Build the transmit string for the given element
//
// Abstract:
//	Convert the input un32Element to a character string, then call the function
//	that will format the actual transmit string.
//
// Returns:
//	none
//
// ElementSerialize is overloaded for the common types:
//    EstiBool,int8_t,int16_t,int32_t,uint8_t,uint16_t,uint32_t,and char*
void ElementSerialize (
	std::stringstream *pTransmitString, // This will be the formatted transmit string
	int32_t n32Tag,					// This denotes the field where the data is
	const uint32_t un32Element)		// Value stored with the tag
{
	stiLOG_ENTRY_NAME (ElementSerialize);
	
	ElementTagAdd (pTransmitString, n32Tag);

	*pTransmitString << std::setfill('0') << std::setw(nstiUINT32_LEN) << un32Element;

	ElementTerminatorAdd (pTransmitString);
} // end ElementSerialize


////////////////////////////////////////////////////////////////////////////////
//; ElementSerialize
//
// Description: Build the transmit string for the given element
//
// Abstract:
//	Convert the input n8Element to a character string, then call the function
//	that will format the actual transmit string.
//
// Returns:
//	none
//
// ElementSerialize is overloaded for the common types:
//    EstiBool,int8_t,int16_t,int32_t,uint8_t,uint16_t,uint32_t,and char*
void ElementSerialize (
	std::stringstream *pTransmitString, // This will be the formatted transmit string
	int32_t n32Tag,					// This denotes the field where the data is
	const int8_t n8Element)			// Value stored with the tag
{
	stiLOG_ENTRY_NAME (ElementSerialize);

	ElementSerialize (pTransmitString, n32Tag, (uint8_t)n8Element);
} // end ElementSerialize


////////////////////////////////////////////////////////////////////////////////
//; ElementSerialize
//
// Description: Build the transmit string for the given element
//
// Abstract:
//	Convert the input n16Element to a character string, then call the function
//	that will format the actual transmit string.
//
// Returns:
//	none
//
// ElementSerialize is overloaded for the common types:
//    EstiBool,int8_t,int16_t,int32_t,uint8_t,uint16_t,uint32_t,and char*
void ElementSerialize (
	std::stringstream *pTransmitString, // This will be the formatted transmit string
	int32_t n32Tag,					// This denotes the field where the data is
	const int16_t n16Element)			// Value stored with the tag
{
	stiLOG_ENTRY_NAME (ElementSerialize);

	ElementSerialize (pTransmitString, n32Tag, (uint16_t)n16Element);
} // end ElementSerialize


////////////////////////////////////////////////////////////////////////////////
//; ElementSerialize
//
// Description: Build the transmit string for the given element
//
// Abstract:
//	Convert the input n32Element to a character string, then call the function
//	that will format the actual transmit string.
//
// Returns:
//	none
//
// ElementSerialize is overloaded for the common types:
//    EstiBool,int8_t,int16_t,int32_t,uint8_t,uint16_t,uint32_t,and char*
void ElementSerialize (
	std::stringstream *pTransmitString, // This will be the formatted transmit string
	int32_t n32Tag,					// This denotes the field where the data is
	const int32_t n32Element) 			// Value stored with the tag
{
	stiLOG_ENTRY_NAME (ElementSerialize);
	
	ElementSerialize (pTransmitString, n32Tag, (uint32_t)n32Element);
} // end ElementSerialize


////////////////////////////////////////////////////////////////////////////////
//; ElementSerialize
//
// Description: Build the transmit string for the given element
//
// Abstract:
//	Convert the input bElement to a character string, then call the function
//	that will format the actual transmit string.
//
// Returns:
//	none
//
// ElementSerialize is overloaded for the common types:
//    EstiBool,int8_t,int16_t,int32_t,uint8_t,uint16_t,uint32_t,and char*
void ElementSerialize (
	std::stringstream *pTransmitString, // This will be the formatted transmit string
	int32_t n32Tag,					// This denotes the field where the data is
	const EstiBool bElement)			// Value stored with the tag
{
	stiLOG_ENTRY_NAME (ElementSerialize);

	ElementTagAdd (pTransmitString, n32Tag);

	*pTransmitString << std::setw(nstiBOOL_LEN) << (unsigned int)bElement;

	ElementTerminatorAdd (pTransmitString);
} // end ElementSerialize


////////////////////////////////////////////////////////////////////////////////
//; ElementSerialize
//
// Description: Build the transmit string for the given element
//
// Abstract:
//	Begin the resultant string with a formatted tag such as 000n where
//	000n represents the specific element being formatted.  Search through
//	the input string for all occurrences of '|'.  If found, replace with
//	two of the same (ie "||").
//
// Returns:
//	none
//
// ElementSerialize is overloaded for the common types:
//    EstiBool,int8_t,int16_t,int32_t,uint8_t,uint16_t,uint32_t,and char*
void ElementSerialize (
	std::stringstream *pTransmitString, // This will be the formatted transmit string
	int32_t n32Tag,					// This denotes the field where the data is
	std::string SourceString)		// This is the sub-string to add
{
	stiLOG_ENTRY_NAME (ElementSerialize);

	// Is there anything in the source string?
	if (!SourceString.empty ())
	{
		ElementTagAdd (pTransmitString, n32Tag);

		//
		// Find all instances of '|' and replace with '||'
		//
		size_t StartPos = 0;
		for (;;)
		{
			size_t PipePos = SourceString.find ('|', StartPos);

			if (PipePos == std::string::npos)
			{
				//
				// No more pipe characters were found
				//
				break;
			}

			//
			// Insert a single pipe character after the one found.
			//
			SourceString.insert (PipePos + 1, 1, '|');

			//
			// Use a value of 2 to skip past both the found pipe character and the inserted one.
			//
			StartPos = PipePos + 2;
		}

		//
		// Add the string and the pipe character at the end of the transmit string.
		//
		*pTransmitString << SourceString;

		ElementTerminatorAdd (pTransmitString);
	} // end if
} // end ElementSerialize


/*!\brief Locates the terminating pipe character for the element
 *
 * \param pszString Points to the first character in the element
 */
const char *TerminatorFind (
	const char *pszString)
{
	// Search for the end of the element (or end of the string)
	while (*pszString)
	{
		// If this is the end of the string, then quit.
		if (0 == *pszString)
		{
			break;
		} // end if

		// If there is a double terminating character, then skip both.
		if ('|' == *pszString &&
			'|' == *(pszString + 1))
		{
			pszString += 2;
			continue;
		} // end if

		// If this is the terminating character then quit.
		if ('|' == *pszString)
		{
			break;
		} // end if

		// Go on to the next character.
		pszString++;
	} // end while

	return pszString;
}


/*!\brief Parses an SInfo element into the tag number and element string
 *
 * \param pszElement A pointer to the first character of the element to parse
 * \param pn32Tag A pointer to a location to store the tag number
 * \param pValue A pointer to a location to store the element value
 * \param ppszEnd A pointer to the terminating pipe character for the element
 */
stiHResult ElementParse (
	const char *pszElement,
	int32_t *pn32Tag,
	std::string *pValue,
	const char **ppszEnd)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	const char *pszStart = pszElement;
	const char *pszEnd = pszElement;
	int32_t n32Tag = 0;

	pszEnd = TerminatorFind (pszStart);

	//
	// The element must be terminated with a pipe character.
	// If it is not then this is a malformed element.
	//
	stiTESTCOND (*pszEnd == '|', stiRESULT_ERROR);

	// Now pszStart points to the start of the string, and
	// pszEnd will point at the terminator.

	// Are there at least 4 characters in the resulting string?
	if ((pszEnd - pszStart) >= 4)
	{
		// Yes! Make a number out of the first four elements.
		char szNum[5];

		szNum[0] = pszStart[0];
		szNum[1] = pszStart[1];
		szNum[2] = pszStart[2];
		szNum[3] = pszStart[3];
		szNum[4] = 0;

		n32Tag = strtol (szNum, (char**)nullptr, 10);

		// Now reset the start pointer.
		pszStart += 4;
	} // end if
	else
	{
		// No!
		stiTHROW (stiRESULT_ERROR);
	} // end else

	//
	// Copy the string value into the outgoing string parameter.
	// The pointer arithmetic will capture the string value but
	// will not copy the terminating pipe element.
	// If the string value is an empty string (pszEnd == pszStart) then just
	// clear the outgoing string parameter.
	//
	if (pszEnd > pszStart)
	{
		pValue->assign (pszStart, pszEnd - pszStart);

		//
		// Replace any occurrence of a double pipe with a single pipe character.
		//
		size_t nPos = 0;

		for (;;)
		{
			nPos = pValue->find ("||", nPos);

			if (nPos == std::string::npos)
			{
				break;
			}

			pValue->replace(nPos, 2, "|");

			nPos++;
		}
	}
	else
	{
		pValue->clear ();
	}

	*pn32Tag = n32Tag;
	*ppszEnd = pszEnd;

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; SystemInfoDeserialize
//
// Description: Populates SystemData object with data from a serialized stream
//
// Abstract:
//	This method takes the serialized data passed in, extracts the individual
//	components from it then puts them in the member variables.
//
// Returns:
//	none
//
stiHResult SystemInfoDeserialize (
    CstiCallSP call,
    CstiSipCallLegSP callLeg,
	const char *pszString)	// This is the serialized data that needs to be
								// pulled apart and placed in the individual
								// member variables.
{
	stiLOG_ENTRY_NAME (SystemInfoDeserialize);

	stiHResult hResult = stiRESULT_SUCCESS;
	const char *pszCurrent = nullptr;
	char *pszStringDup = nullptr;

	//
	// Make sure we a valid pointer.
	//
	stiTESTCOND (pszString != nullptr, stiRESULT_ERROR);

	// The transmit string is formatted as follows:
	// SInfo:XXXXaaaaaaaaaaaaa|XXXXaaaaaaaaaaaaa|XXXXaaaaaaaaaaaaa|
	// Where:
	//  XXXX is a number between 0000 and 9999 that identifies the type
	//	of information that follows.
	//  aaaaaaaaaaaaa is the string of characters (NOT NULL TERMINATED!).
	//	Any embedded "|" characters in the string will be replaced with
	//	"||".  | is the "|" character and is used to signal the end of this
	//	element.  The total string ends with a single null terminator.
	stiDEBUG_TOOL (g_stiSInfoDebug,
		stiTrace ("<SystemInfoDeserialize> string = [%s]\n", pszString);
	);

	pszStringDup = new char[strlen (pszString) + 1];
	stiTESTCOND (pszStringDup != nullptr, stiRESULT_ERROR);
	
	strcpy (pszStringDup, pszString);
	
	// First, Check to make sure that the string starts with the SINFO header
	if (0 == strncmp (pszStringDup, INFO_HEADER.c_str (), INFO_HEADER.length ()))
	{
		std::string Value;
		int32_t n32Tag;

		// Skip past the tag
		pszCurrent = pszStringDup + INFO_HEADER.length ();

		while (*pszCurrent)
		{
			hResult = ElementParse (pszCurrent, &n32Tag, &Value, &pszCurrent);
			stiTESTRESULT ();

			// Set this element.
			ElementSet (call, callLeg, n32Tag, Value);

			pszCurrent++;
		} // end while
	} // end if
	else if (callLeg->RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_DEVICE))
	{
		// We only want to throw an error and log the information if we know
		// that the remote endpoint is a Sorenson device.
		std::string RemoteProductName;
		callLeg->RemoteProductNameGet(&RemoteProductName);

		std::string callID;
		call->CallIdGet(&callID);

		stiTHROWMSG (stiRESULT_ERROR, "SInfo=\"%.*s\" callID=%s RemoteProductName=\"%s\"",
				INFO_HEADER.length (), pszStringDup, callID.c_str (), RemoteProductName.c_str ());
	}
	else
	{
		// We don't know if the remote endpoint is third party or not. Return an error but don't log anything.
		stiTESTCOND_NOLOG (false, stiRESULT_ERROR);
	}

STI_BAIL:

	if (pszStringDup)
	{
		delete [] pszStringDup;
		pszStringDup = nullptr;
	}

	return hResult;
	
} // end SystemInfoDeserialize


stiHResult SystemInfoDeserialize (
    CstiCallSP call,
	const char *pszString)	// This is the serialized data that needs to be
							// pulled apart and placed in the individual
							// member variables.
{
	return SystemInfoDeserialize (call, nullptr, pszString);
}


stiHResult SystemInfoDeserialize (
    CstiSipCallLegSP callLeg,
	const char *pszString)
{
	return SystemInfoDeserialize (callLeg->m_sipCallWeak.lock()->CstiCallGet(), callLeg, pszString);
}


////////////////////////////////////////////////////////////////////////////////
//; SystemInfoSerialize
//
// Description: Format local system information into a serialized string.
//
// Abstract:
//	This function places the initial identifier on the string then repeatedly
//	calls the methods to add all the individual pieces of information to the
//	string to be sent.  Each call passes in the string as it is to that point
//	and it is added to.
//
// The transmit string is formatted as follows:
// SInfo:XXXXaaaaaaaaaaaaa|XXXXaaaaaaaaaaaaa|XXXXaaaaaaaaaaaaa|
// Where:
//      XXXX is a number between 0000 and 9999 that identifies the type
//      of information that follows.
//      aaaaaaaaaaaaa is the string of characters (NOT NULL TERMINATED!).
//      Any embedded "|" characters in the string will be replaced with
//      "||".
//      | is the"|" character and is used to signal the end of this element.
//      The total string ends with a single null terminator.
//
// Returns:
//	estiOK when successful, estiERROR otherwise
//
stiHResult SystemInfoSerialize (
	std::string *transmitString,				///< This new string will contain the serialized data
    CstiSipCallSP sipCall,						///< The sip call containing directory-resolved information
	bool bDefaultProviderAgreementSigned,	///< Has this agreement been signed?
	EstiInterfaceMode eLocalInterfaceMode,	///< The local interface mode.
	const char *pszAutoDetectedPublicIp,	///< The auto detected public IP address
	EstiVcoType ePreferredVCOType,			///< The type of VCO to use: 0 = none, 1 = 1-line, 2 = 2-line
	const char *pszVCOCallbackNumber,		///< The VCO callback number
	bool bVerifyAddress,					///< Does the address need to be verified (e.g. for 911 calls)?
	const char *pszProductName,
	const char *pszProductVersion,
	int nSorensonSIPVersion,
	bool bBlockCallerID,
	EstiAutoSpeedMode eAutoSpeedSetting)
{
	stiLOG_ENTRY_NAME (SystemInfoSerialize);

	stiHResult hResult = stiRESULT_SUCCESS;
	std::stringstream TransmitString;
	
	auto call = sipCall->CstiCallGet ();

#ifdef SERILIZE_DESERIALIZE_TEST
	SerializeDeserializeTest ();
#endif

	// First put the tag on the beginning of the string
	TransmitString << INFO_HEADER;

	// Ported mode differs from the rest of the interface modes.
	// Is it Ported Mode?
	if (estiPORTED_MODE == eLocalInterfaceMode)
	{
		ElementSerialize (&TransmitString, estiTAG_VRS_INTERFACE_MODE, (int32_t)eLocalInterfaceMode);
		ElementSerialize (&TransmitString, estiTAG_AUTO_DETECTED_PUBLIC_IP, pszAutoDetectedPublicIp);

		// Always signal unregistered for ported users  (fix bug 21340)
		ElementSerialize (&TransmitString, estiTAG_REGISTERED, estiFALSE);

		// Obtain the MAC address formatted like "08:00:2B:00:00:01"
		std::string uniqueID;
		if (estiOK == stiOSGetUniqueID (&uniqueID))
		{
			ElementSerialize (&TransmitString, estiTAG_MAC_ADDRESS, uniqueID);
		} // end if

		//
		// The following are new properties that are to only be sent in the proprietary
		// (non-restricted) SInfo.
		//
		EstiDialMethod eRtnMethod = estiDM_BY_DS_PHONE_NUMBER;
		auto poLocalCallInfo = (CstiCallInfo *)call->LocalCallInfoGet ();
		const SstiUserPhoneNumbers *psLocalPhoneNumbers = poLocalCallInfo->UserPhoneNumbersGet ();

		std::string returnDialString = psLocalPhoneNumbers->szPreferredPhoneNumber;

		if (!returnDialString.empty ())
		{
			ElementSerialize (&TransmitString, estiTAG_RETURN_CALL_DIAL_METHOD, (int32_t)eRtnMethod);
			ElementSerialize (&TransmitString, estiTAG_DIAL_STRING, returnDialString);
		}
	} // end else if

	// All other interface modes
	else
	{
		//
		// If there is an alternate name then send it.
		//
		auto alternateName = call->localAlternateNameGet ();

		if (alternateName.length() > 0)
		{
			ElementSerialize (&TransmitString, estiTAG_ALTERNATE_NAME, alternateName);
		}

		//
		// If there is VRS Call ID send it.
		//
		auto vrsCallId = call->vrsCallIdGet ();

		if (vrsCallId.length() > 0)
		{
			ElementSerialize (&TransmitString, estiTAG_VRS_CALL_ID, vrsCallId);
		}

		auto poLocalCallInfo = (CstiCallInfo *)call->LocalCallInfoGet ();
		const SstiUserPhoneNumbers *psLocalPhoneNumbers = poLocalCallInfo->UserPhoneNumbersGet ();

		EstiDialMethod eRtnMethod = estiDM_UNKNOWN;
		std::string returnDialString;
		EstiBool bSendPhoneNumbers = estiFALSE;

		//
		// If we are not dialing by VRS then consider using the return dial type and dial string.
		// This has the effect that if we are dialing via VRS then we will appear as ourselves
		// instead of the group we are in.
		//
		EstiDialMethod eDialMethod = call->DialMethodGet ();
		if (eDialMethod != estiDM_BY_VRS_PHONE_NUMBER
		 && eDialMethod != estiDM_BY_VRS_WITH_VCO)
		{
			if (call->DialedOwnRingGroupGet())
			{
				returnDialString = psLocalPhoneNumbers->szLocalPhoneNumber;
			}
			else
			{
				eRtnMethod = call->LocalReturnCallDialMethodGet ();
				call->LocalReturnCallDialStringGet (&returnDialString);
			}

			if (returnDialString.empty ())
			{
				eRtnMethod = estiDM_UNKNOWN;
			}

			//
			// Only when we are in interpreter mode and
			// there is no overriding call back number do
			// we consider using the hearing number as the
			// callback number.
			//
			if (eRtnMethod == estiDM_UNKNOWN
			 && estiINTERPRETER_MODE == eLocalInterfaceMode)
			{
				eRtnMethod = estiDM_BY_VRS_PHONE_NUMBER;
				returnDialString = psLocalPhoneNumbers->szHearingPhoneNumber;

				ElementSerialize (&TransmitString, estiTAG_HEARING_NUMBER,
								psLocalPhoneNumbers->szHearingPhoneNumber);
				ElementSerialize (&TransmitString, estiTAG_PHONE_NUMBER,
								psLocalPhoneNumbers->szHearingPhoneNumber);
			}
		}
		
		// VcoActive checks the dial method to see if we are using vco
		bool bVcoActive = eDialMethod == estiDM_BY_VRS_WITH_VCO;
		
		// LocalVcoActive is for hearing to deaf calls; the deaf phone checks its contact to
		//  see if the hearing phone is listed as VCO, and if so, the LocalVcoActive flag is set
		auto bLocalVcoActive = call->LocalIsVcoActiveGet();
		const char *pszVcoActive = (bVcoActive || bLocalVcoActive) ? "1" : "0";
		
		//estiTAG_VCO_TYPE_PREF = 35, // Type of VCO preferred by the endpoint (0 = none, 1 = 1-line, 2 = 2-line)
		switch (ePreferredVCOType)
		{
			case estiVCO_NONE:
				ElementSerialize (&TransmitString, estiTAG_VCO_TYPE_PREF, "0");
				ElementSerialize (&TransmitString, estiTAG_VCO_ACTIVE, "0");
				break;
			case estiVCO_ONE_LINE:
				ElementSerialize (&TransmitString, estiTAG_VCO_TYPE_PREF, "1");
				ElementSerialize (&TransmitString, estiTAG_VCO_ACTIVE, pszVcoActive);
				break;
			case estiVCO_TWO_LINE:
				ElementSerialize (&TransmitString, estiTAG_VCO_TYPE_PREF, "2");
				ElementSerialize (&TransmitString, estiTAG_VCO_ACTIVE, pszVcoActive);
				ElementSerialize (&TransmitString, estiTAG_VCO_CALLBACK, pszVCOCallbackNumber);
				break;
		}

		if (eRtnMethod == estiDM_UNKNOWN)
		{
			//
			// If we have reached this point we are either dialing by VRS
			// or we have no group callback number so we should appear as
			// ourselves.
			//
			eRtnMethod = estiDM_BY_DS_PHONE_NUMBER;
			if (call->DialedOwnRingGroupGet())
			{
				returnDialString = psLocalPhoneNumbers->szLocalPhoneNumber;
			}
			else
			{
				returnDialString = psLocalPhoneNumbers->szPreferredPhoneNumber;
			}

			ElementSerialize (&TransmitString, estiTAG_PHONE_NUMBER,
							  psLocalPhoneNumbers->szPreferredPhoneNumber);

			bSendPhoneNumbers = estiTRUE;
		}
		else
		{
			if (estiDM_BY_DS_PHONE_NUMBER == eRtnMethod)
			{
				ElementSerialize (&TransmitString, estiTAG_PHONE_NUMBER, returnDialString);
			}
		}

		ElementSerialize (&TransmitString, estiTAG_VRS_INTERFACE_MODE, (int32_t)eLocalInterfaceMode);

		ElementSerialize (&TransmitString, estiTAG_AUTO_DETECTED_PUBLIC_IP, pszAutoDetectedPublicIp);

		// Obtain the MAC address formatted like "08:00:2B:00:00:01"
		std::string uniqueID;
		if (estiOK == stiOSGetUniqueID (&uniqueID))
		{
			ElementSerialize (&TransmitString, estiTAG_MAC_ADDRESS, uniqueID);
		} // end if

		//
		// If we determined previously that we should send the list of phone numbers
		// then send them
		//
		if (bSendPhoneNumbers)
		{
			// toll-free number
			ElementSerialize (&TransmitString, estiTAG_TOLL_FREE_NUMBER, psLocalPhoneNumbers->szTollFreePhoneNumber);

			// sorenson number
			ElementSerialize (&TransmitString, estiTAG_SORENSON_NUMBER, psLocalPhoneNumbers->szSorensonPhoneNumber);

			// local number
			ElementSerialize (&TransmitString, estiTAG_LOCAL_NUMBER, psLocalPhoneNumbers->szLocalPhoneNumber);

			//
			// Ring group local number.
			//
			ElementSerialize (&TransmitString, estiTAG_RING_GROUP_LOCAL_NUMBER, psLocalPhoneNumbers->szRingGroupLocalPhoneNumber);
			
			//
			// Ring group toll-free number.
			//
			ElementSerialize (&TransmitString, estiTAG_RING_GROUP_TOLL_FREE_NUMBER, psLocalPhoneNumbers->szRingGroupTollFreePhoneNumber);
		}

		// the backend id of the current user
		std::string userID;

		poLocalCallInfo->UserIDGet(&userID);

		ElementSerialize (&TransmitString, estiTAG_USERID, userID);

		// the backend group id of the current user
		std::string groupUserID;

		poLocalCallInfo->GroupUserIDGet(&groupUserID);

		ElementSerialize (&TransmitString, estiTAG_GROUP_USER_ID, groupUserID);

		if (estiDM_UNKNOWN != eRtnMethod &&
			!returnDialString.empty ())
		{
			ElementSerialize (&TransmitString, estiTAG_RETURN_CALL_DIAL_METHOD, (int32_t)eRtnMethod);

			if (bBlockCallerID)
			{
				ElementSerialize (&TransmitString, estiTAG_DIAL_STRING, ""); // No Caller ID
			}
			else
			{
				ElementSerialize (&TransmitString, estiTAG_DIAL_STRING, returnDialString);
			}
		}

		if (eLocalInterfaceMode == estiHEARING_MODE)
		{
			// Always signal unregistered for hearing users to prevent them from getting to an interpreter (fix bug 21399)
			ElementSerialize (&TransmitString, estiTAG_REGISTERED, estiFALSE);
		}
		else
		{
			ElementSerialize (&TransmitString, estiTAG_REGISTERED, bDefaultProviderAgreementSigned ? estiTRUE : estiFALSE);
		}

		if (bVerifyAddress)
		{
			ElementSerialize (&TransmitString, estiTAG_VERIFY_ADDRESS, (uint8_t)1);
		}

		OptString preferredLanguage;
		call->LocalPreferredLanguageGet (&preferredLanguage);

		// Add the language of choice to be used in a VRS call.
		if (preferredLanguage && CstiCall::g_szDEFAULT_RELAY_LANGUAGE != preferredLanguage)
		{
			ElementSerialize (&TransmitString, estiTAG_VRS_LANGUAGE_STRING, *preferredLanguage);
			ElementSerialize (&TransmitString, estiTAG_VRS_LANGUAGE, (int32_t)call->LocalPreferredLanguageGet ());
		}

		ElementSerialize (&TransmitString, estiTAG_PRODUCT_NAME, pszProductName);

		ElementSerialize (&TransmitString, estiTAG_PRODUCT_VERSION, pszProductVersion);

		ElementSerialize (&TransmitString, estiTAG_SORENSON_SIP_VERSION, nSorensonSIPVersion);

		ElementSerialize (&TransmitString, estiTAG_AUTO_SPEED_SETTING, (int32_t)eAutoSpeedSetting);

		// Tell the remote endpoint if they are responsible for logging their own missed call.
		if (call->RemoteAddMissedCallGet ())
		{
			ElementSerialize(&TransmitString, estiTAG_ADD_MISSED_CALL, estiTRUE);
		}
		else
		{
			ElementSerialize(&TransmitString, estiTAG_ADD_MISSED_CALL, estiFALSE);
		}
		
		if (!sipCall->vrsFocusedRoutingSentGet().empty())
		{
			ElementSerialize(&TransmitString, estiTAG_VRS_FOCUSED_ROUTING, sipCall->vrsFocusedRoutingSentGet());
		}
		
	} // end else

	stiDEBUG_TOOL (g_stiSInfoDebug,
		stiTrace ("<SystemInfoSerialize> string = [%s]\n", TransmitString.str ().c_str ());
	);

	*transmitString = TransmitString.str ();

	return hResult;
} // end SystemInfoSerialize


#ifdef SERILIZE_DESERIALIZE_TEST
stiHResult SerializeDeserializeTest ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	const char *pszTestString = "00004294967295|00012147483647|000265535|000332767|000400255|000500127|000600000000|00074294967295|000800000|000965535|001000000|001100255|00121|00130|0015A|0016|||0017|||||0018Single pipe test || with text|0019Double pipe test |||| with text|";

	enum EDataType
	{
		eINT32,
		eINT16,
		eINT8,
		eUINT32,
		eUINT16,
		eUINT8,
		eBOOL,
		eSTRING
	};

	struct TestEntry
	{
		EDataType eDataType;

		uint32_t un32Value;
		int32_t n32Value;
		uint16_t un16Value;
		int16_t n16Value;
		uint8_t un8Value;
		int8_t n8Value;
		EstiBool bValue;
		const char *pszValue;
	};

	TestEntry Tests[] =
	{
		{eINT32, 0, (int32_t)-1},
		{eINT32, 0, (int32_t)0x7FFFFFFF},
		{eINT16, 0, 0, 0, (int16_t)-1},
		{eINT16, 0, 0, 0, (int16_t)0x7FFF},
		{eINT8,  0, 0, 0, 0, 0, (int8_t)-1},
		{eINT8,  0, 0, 0, 0, 0, (int8_t)0x7F},

		{eUINT32, (uint32_t)0},
		{eUINT32, (uint32_t)0xFFFFFFFF},
		{eUINT16, 0, 0, (uint16_t)0},
		{eUINT16, 0, 0, (uint16_t)0xFFFF},
		{eUINT8,  0, 0, 0, 0, (uint8_t)0},
		{eUINT8,  0, 0, 0, 0, (uint8_t)0xFF},

		{eBOOL,   0, 0, 0, 0, 0, 0, estiTRUE},
		{eBOOL,   0, 0, 0, 0, 0, 0, estiFALSE},

		{eSTRING, 0, 0, 0, 0, 0, 0, estiTRUE, ""}, // Empty string test: should not produce an element in the transmit string
		{eSTRING, 0, 0, 0, 0, 0, 0, estiTRUE, "A"}, // Single character test.
		{eSTRING, 0, 0, 0, 0, 0, 0, estiTRUE, "|"}, // Single pipe test: pipe should be escaped.
		{eSTRING, 0, 0, 0, 0, 0, 0, estiTRUE, "||"}, // Double pipe test: both pipes should be escaped.
		{eSTRING, 0, 0, 0, 0, 0, 0, estiTRUE, "Single pipe test | with text"},
		{eSTRING, 0, 0, 0, 0, 0, 0, estiTRUE, "Double pipe test || with text"}
	};
	unsigned int unNumTests = sizeof (Tests) / sizeof (Tests[0]);

	std::stringstream TransmitString;

	for (unsigned int i = 0; i < unNumTests; i++)
	{
		switch (Tests[i].eDataType)
		{
			case eINT32:

				ElementSerialize (&TransmitString, i, Tests[i].n32Value);

				break;

			case eINT16:

				ElementSerialize (&TransmitString, i, Tests[i].n16Value);

				break;

			case eINT8:

				ElementSerialize (&TransmitString, i, Tests[i].n8Value);

				break;

			case eUINT32:

				ElementSerialize (&TransmitString, i, Tests[i].un32Value);

				break;

			case eUINT16:

				ElementSerialize (&TransmitString, i, Tests[i].un16Value);

				break;

			case eUINT8:

				ElementSerialize (&TransmitString, i, Tests[i].un8Value);

				break;

			case eBOOL:

				ElementSerialize (&TransmitString, i, Tests[i].bValue);

				break;

			case eSTRING:

				ElementSerialize (&TransmitString, i, Tests[i].pszValue);

				break;
		}
	}

	stiTrace ("Transmit String: %s\n", TransmitString.str ().c_str ());

	if (strcmp (pszTestString, TransmitString.str ().c_str ()) == 0)
	{
		//
		// Now parse the transmit string back into the original values and make sure
		// they match.
		//
		const char *pszCurrent = TransmitString.str ().c_str ();
		std::string Value;
		int32_t n32Tag;

		while (*pszCurrent)
		{
			hResult = ElementParse (pszCurrent, &n32Tag, &Value, &pszCurrent);
			stiTESTRESULT ();

			stiTESTCOND (n32Tag >= 0 && n32Tag < (signed)unNumTests, stiRESULT_ERROR);

			switch (Tests[n32Tag].eDataType)
			{
				case eINT32:
				{
					long long int llValue = strtoll (Value.c_str (), NULL, 10);
					stiTESTCOND (llValue != LLONG_MIN && llValue != LLONG_MAX, stiRESULT_ERROR);
					stiTESTCOND (llValue >= 0 && llValue <= 0xFFFFFFFF, stiRESULT_ERROR);

					int32_t n32Value = (int32_t)llValue;
					stiTESTCOND (n32Value == Tests[n32Tag].n32Value, stiRESULT_ERROR);

					break;
				}

				case eINT16:
				{
					long long int llValue = strtoll (Value.c_str (), NULL, 10);
					stiTESTCOND (llValue != LLONG_MIN && llValue != LLONG_MAX, stiRESULT_ERROR);
					stiTESTCOND (llValue >= 0 && llValue <= 0xFFFF, stiRESULT_ERROR);

					int16_t n16Value = (int16_t)llValue;
					stiTESTCOND (n16Value == Tests[n32Tag].n16Value, stiRESULT_ERROR);

					break;
				}

				case eINT8:
				{
					long long int llValue = strtoll (Value.c_str (), NULL, 10);
					stiTESTCOND (llValue != LLONG_MIN && llValue != LLONG_MAX, stiRESULT_ERROR);
					stiTESTCOND (llValue >= 0 && llValue <= 0xFF, stiRESULT_ERROR);

					int8_t n8Value = (int8_t)llValue;
					stiTESTCOND (n8Value == Tests[n32Tag].n8Value, stiRESULT_ERROR);

					break;
				}

				case eUINT32:
				{
					long long int llValue = strtoll (Value.c_str (), NULL, 10);
					stiTESTCOND (llValue != LLONG_MIN && llValue != LLONG_MAX, stiRESULT_ERROR);
					stiTESTCOND (llValue >= 0 && llValue <= 0xFFFFFFFF, stiRESULT_ERROR);

					uint32_t un32Value = (uint32_t)llValue;
					stiTESTCOND (un32Value == Tests[n32Tag].un32Value, stiRESULT_ERROR);

					break;
				}

				case eUINT16:
				{
					long long int llValue = strtoll (Value.c_str (), NULL, 10);
					stiTESTCOND (llValue != LLONG_MIN && llValue != LLONG_MAX, stiRESULT_ERROR);
					stiTESTCOND (llValue >= 0 && llValue <= 0xFFFF, stiRESULT_ERROR);

					uint16_t un16Value = (uint16_t)llValue;
					stiTESTCOND (un16Value == Tests[n32Tag].un16Value, stiRESULT_ERROR);

					break;
				}

				case eUINT8:
				{
					long long int llValue = strtoll (Value.c_str (), NULL, 10);
					stiTESTCOND (llValue != LLONG_MIN && llValue != LLONG_MAX, stiRESULT_ERROR);
					stiTESTCOND (llValue >= 0 && llValue <= 0xFF, stiRESULT_ERROR);

					uint8_t un8Value = (uint8_t)llValue;
					stiTESTCOND (un8Value == Tests[n32Tag].un8Value, stiRESULT_ERROR);

					break;
				}

				case eBOOL:
				{
					long long int llValue = strtoll (Value.c_str (), NULL, 10);
					stiTESTCOND (llValue != LLONG_MIN && llValue != LLONG_MAX, stiRESULT_ERROR);
					stiTESTCOND (llValue >= 0 && llValue <= 0xFF, stiRESULT_ERROR);

					EstiBool bValue = (EstiBool)llValue;
					stiTESTCOND (bValue == Tests[n32Tag].bValue, stiRESULT_ERROR);

					break;
				}

				case eSTRING:

					if (strcmp (Value.c_str (), Tests[n32Tag].pszValue) != 0)
					{
						stiTHROW (stiRESULT_ERROR);
					}

					break;
			}

			pszCurrent++;
		}
	}
	else
	{
		stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
}
#endif

// end file stiSystemInfo.cpp
