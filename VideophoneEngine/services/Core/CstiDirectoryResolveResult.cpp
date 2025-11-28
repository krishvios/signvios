////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiDirectoryResolveResult
//
//	File Name:	CstiDirectoryResolveResult.cpp
//
//	Owner:		Scot L. Brooksby
//
//	Abstract:
//		Implement a directory resolution result object
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stiDefs.h"
#include "stiTools.h"
#include "stiMem.h"
#include "CstiDirectoryResolveResult.h"

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//

//
// Locals
//

//
// Function Declarations
//


////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::~CstiDirectoryResolveResult
//
// Description: Destructor
//
// Abstract:
//
// Returns:
//	None
//
CstiDirectoryResolveResult::~CstiDirectoryResolveResult ()
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::~CstiDirectoryResolveResult);

	// Free our internal strings.
	if (m_szName != nullptr)
	{
		stiHEAP_FREE (m_szName);
		m_szName = nullptr;
	} // end if

	if (m_szDialString != nullptr)
	{
		stiHEAP_FREE (m_szDialString);
		m_szDialString = nullptr;
	} // end if

#if 0
	if (m_szBlockedMsg != NULL)
	{
		stiHEAP_FREE (m_szBlockedMsg);
		m_szBlockedMsg = NULL;
	} // end if
#endif

	if (m_pszNewNumber != nullptr)
	{
		stiHEAP_FREE (m_pszNewNumber);
		m_pszNewNumber = nullptr;
	}

	if (m_pszFromName != nullptr)
	{
		stiHEAP_FREE (m_pszFromName);
		m_pszFromName = nullptr;
	}

#if 0
	if (m_pszRecordCodec != NULL)
	{
		stiHEAP_FREE (m_pszRecordCodec);
		m_pszRecordCodec = NULL;
	}
#endif

#if 0
	if (m_pszGreetingGUID != NULL)
	{
		stiHEAP_FREE (m_pszGreetingGUID);
		m_pszGreetingGUID = NULL;
	}
#endif

	if (m_pszGreetingURL != nullptr)
	{
		stiHEAP_FREE (m_pszGreetingURL);
		m_pszGreetingURL = nullptr;
	}
	
	if (m_pszGreetingURL2 != nullptr)
	{
		stiHEAP_FREE (m_pszGreetingURL2);
		m_pszGreetingURL2 = nullptr;
	}
	
	if (m_pszGreetingText != nullptr)
	{
		stiHEAP_FREE (m_pszGreetingText);
		m_pszGreetingText = nullptr;
	}

	if (m_pszSource != nullptr)
	{
		stiHEAP_FREE (m_pszSource);
		m_pszSource = nullptr;
	}
} // end CstiDirectoryResolveResult::~CstiDirectoryResolveResult


////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::DialStringSet
//
// Description: Sets the Dial String
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiDirectoryResolveResult::DialStringSet (
	const char * szDialString)
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::DialStringSet);

	EstiResult eResult = estiERROR;

	if (m_szDialString != nullptr)
	{
		stiHEAP_FREE (m_szDialString);
		m_szDialString = nullptr;
	} // end if

		if (nullptr != szDialString)
		{
				m_szDialString = (char *) stiHEAP_ALLOC (strlen (szDialString) + 1);
				if (m_szDialString != nullptr)
				{
						strcpy (m_szDialString, szDialString);

						eResult = estiOK;
				} // end if
				else
				{
					stiASSERT (estiFALSE);
				} // end if
		} // end if
		else
		{
			eResult = estiOK;
		} // end else

	return (eResult);
} // end CstiDirectoryResolveResult::DialStringSet


////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::NameSet
//
// Description: Sets the Display Name
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiDirectoryResolveResult::NameSet (
	const char * szName)
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::NameSet);

	EstiResult eResult = estiERROR;

		if (m_szName != nullptr)
		{
				stiHEAP_FREE (m_szName);
				m_szName = nullptr;
		} // end if

		if (nullptr != szName)
		{
				m_szName = (char *) stiHEAP_ALLOC (strlen (szName) + 1);
				if (m_szName != nullptr)
				{
						strcpy (m_szName, szName);

						eResult = estiOK;
				} // end if
				else
				{
					stiASSERT (estiFALSE);
				} // end if
		} // end if
		else
		{
			eResult = estiOK;
		} // end else

	return (eResult);
} // end CstiDirectoryResolveResult::NameSet


////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::NewNumberSet
//
// Description: Sets the new number
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiDirectoryResolveResult::NewNumberSet(
	const char *pszNewNumber)
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::DialStringSet);

	EstiResult eResult = estiOK;

	//
	// Free the current number, if any.
	//
	if (m_pszNewNumber != nullptr)
	{
		stiHEAP_FREE (m_pszNewNumber);
		m_pszNewNumber = nullptr;
	}

	//
	// If a non-null pointer was passed in then allocate memory
	// and copy the string into the new buffer.
	//
	if (nullptr != pszNewNumber)
	{
		m_pszNewNumber = (char *) stiHEAP_ALLOC (strlen(pszNewNumber) + 1);

		if (m_pszNewNumber != nullptr)
		{
			strcpy (m_pszNewNumber, pszNewNumber);
		}
		else
		{
			eResult = estiERROR;
			stiASSERT (estiFALSE);
		}
	}

	return eResult;
} // end CstiDirectoryResolveResult::NewNumberSet


////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::FromNameSet
//
// Description: Sets the from name
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiDirectoryResolveResult::FromNameSet(
	const char *pszFromName)
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::FromNameSet);

	EstiResult eResult = estiOK;

	//
	// Free the current number, if any.
	//
	if (m_pszFromName != nullptr)
	{
		stiHEAP_FREE (m_pszFromName);
		m_pszFromName = nullptr;
	}

	//
	// If a non-null pointer was passed in then allocate memory
	// and copy the string into the new buffer.
	//
	if (nullptr != pszFromName)
	{
		m_pszFromName = (char *) stiHEAP_ALLOC (strlen(pszFromName) + 1);

		if (m_pszFromName != nullptr)
		{
			strcpy (m_pszFromName, pszFromName);
		}
		else
		{
			eResult = estiERROR;
			stiASSERT (estiFALSE);
		}
	}

	return eResult;
} // end CstiDirectoryResolveResult::FromNameSet


#if 0
////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::GreetingGUIDSet
//
// Description: Sets the Greeting GUID number
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiDirectoryResolveResult::GreetingGUIDSet(
	const char *pszGreetingGUID)
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::GreetingGUIDSet);

	EstiResult eResult = estiOK;

	//
	// Free the current number, if any.
	//
	if (m_pszGreetingGUID != NULL)
	{
		stiHEAP_FREE (m_pszGreetingGUID);
		m_pszGreetingGUID = NULL;
	}

	//
	// If a non-null pointer was passed in then allocate memory
	// and copy the string into the new buffer.
	//
	if (NULL != pszGreetingGUID)
	{
		m_pszGreetingGUID = (char *) stiHEAP_ALLOC (strlen(pszGreetingGUID) + 1);

		if (m_pszGreetingGUID != NULL)
		{
			strcpy (m_pszGreetingGUID, pszGreetingGUID);
		}
		else
		{
			eResult = estiERROR;
			stiASSERT (estiFALSE);
		}
	}

	return eResult;
} // end CstiDirectoryResolveResult::GreetingGUIDSet
#endif


////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::GreetingURLSet
//
// Description: Sets the Greeting URL
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiDirectoryResolveResult::GreetingURLSet (
	const char *pszGreetingURL)
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::GreetingURLSet);

	EstiResult eResult = estiOK;

	//
	// Free the current URL, if any.
	//
	if (m_pszGreetingURL != nullptr)
	{
		stiHEAP_FREE (m_pszGreetingURL);
		m_pszGreetingURL = nullptr;
	}

	//
	// If a non-null pointer was passed in then allocate memory
	// and copy the string into the new buffer.
	//
	if (nullptr != pszGreetingURL)
	{
		m_pszGreetingURL = (char *) stiHEAP_ALLOC (strlen(pszGreetingURL) + 1);

		if (m_pszGreetingURL != nullptr)
		{
			strcpy (m_pszGreetingURL, pszGreetingURL);
		}
		else
		{
			eResult = estiERROR;
			stiASSERT (estiFALSE);
		}
	}

	return eResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::GreetingURL2Set
//
// Description: Sets the second Greeting URL
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiDirectoryResolveResult::GreetingURL2Set (
	const char *pszGreetingURL2)
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::GreetingURL2Set);
	
	EstiResult eResult = estiOK;
	
	//
	// Free the current URL, if any.
	//
	if (m_pszGreetingURL2 != nullptr)
	{
		stiHEAP_FREE (m_pszGreetingURL2);
		m_pszGreetingURL2 = nullptr;
	}
	
	//
	// If a non-null pointer was passed in then allocate memory
	// and copy the string into the new buffer.
	//
	if (nullptr != pszGreetingURL2)
	{
		m_pszGreetingURL2 = (char *) stiHEAP_ALLOC (strlen(pszGreetingURL2) + 1);
		
		if (m_pszGreetingURL2 != nullptr)
		{
			strcpy (m_pszGreetingURL2, pszGreetingURL2);
		}
		else
		{
			eResult = estiERROR;
			stiASSERT (estiFALSE);
		}
	}
	
	return eResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::GreetingTextSet
//
// Description: Sets the Greeting Text
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiDirectoryResolveResult::GreetingTextSet (
	const char *pszGreetingText)
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::GreetingTextSet);
	
	EstiResult eResult = estiOK;
	
	//
	// Free the current Text, if any.
	//
	if (m_pszGreetingText != nullptr)
	{
		stiHEAP_FREE (m_pszGreetingText);
		m_pszGreetingText = nullptr;
	}
	
	//
	// If a non-null pointer was passed in then allocate memory
	// and copy the string into the new buffer.
	//
	if (nullptr != pszGreetingText)
	{
		m_pszGreetingText = (char *) stiHEAP_ALLOC (strlen(pszGreetingText) + 1);
		
		if (m_pszGreetingText != nullptr)
		{
			strcpy (m_pszGreetingText, pszGreetingText);
		}
		else
		{
			eResult = estiERROR;
			stiASSERT (estiFALSE);
		}
	}
	
	return eResult;
}


#if 0
////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::RecordCodecSet
//
// Description: Sets the Greeting GUID number
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiDirectoryResolveResult::RecordCodecSet(
	const char *pszRecordCodec)
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::RecordCodecSet);

	EstiResult eResult = estiOK;

	//
	// Free the current number, if any.
	//
	if (m_pszRecordCodec != NULL)
	{
		stiHEAP_FREE (m_pszRecordCodec);
		m_pszRecordCodec = NULL;
	}

	//
	// If a non-null pointer was passed in then allocate memory
	// and copy the string into the new buffer.
	//
	if (NULL != pszRecordCodec)
	{
		m_pszRecordCodec = (char *) stiHEAP_ALLOC (strlen(pszRecordCodec) + 1);

		if (m_pszRecordCodec != NULL)
		{
			strcpy (m_pszRecordCodec, pszRecordCodec);
		}
		else
		{
			eResult = estiERROR;
			stiASSERT (estiFALSE);
		}
	}

	return eResult;
} // end CstiDirectoryResolveResult::RecordCodecSet
#endif


////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::UriListSet
//
// Description: Sets the URI list
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiDirectoryResolveResult::UriListSet (
	const std::list<SstiURIInfo> &List)
{
	m_UriList = List;

	return estiOK;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::UriListGet
//
// Description: Gets the URI list.
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiDirectoryResolveResult::UriListGet (
	std::list<SstiURIInfo> *pList) const
{
	*pList = m_UriList;

	return estiOK;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::SourceSet
//
// Description: Sets the Source
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiDirectoryResolveResult::SourceSet (
	const char *pszSource)
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::SourceSet);

	EstiResult eResult = estiOK;

	//
	// Free the current URL, if any.
	//
	if (m_pszSource != nullptr)
	{
		stiHEAP_FREE (m_pszSource);
		m_pszSource = nullptr;
	}

	//
	// If a non-null pointer was passed in then allocate memory
	// and copy the string into the new buffer.
	//
	if (nullptr != pszSource)
	{
		m_pszSource = (char *) stiHEAP_ALLOC (strlen(pszSource) + 1);

		if (m_pszSource != nullptr)
		{
			strcpy (m_pszSource, pszSource);
		}
		else
		{
			eResult = estiERROR;
			stiASSERT (estiFALSE);
		}
	}

	return eResult;
}

// end file CstiDirectoryResolveResult.cpp
