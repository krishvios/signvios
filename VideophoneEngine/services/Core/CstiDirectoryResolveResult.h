////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiDirectoryResolveResult
//
//	File Name:	CstiDirectoryResolveResult.h
//
//	Owner:		Scot L. Brooksby
//
//	Abstract:
//		See CstiDirectoryResolveResult.cpp
//
////////////////////////////////////////////////////////////////////////////////
#pragma once

//
// Includes
//
#include "stiDefs.h"
#include "stiSVX.h"
#include "stiTrace.h"
#include <string>
#include <list>

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
// Class Declaration
//

class CstiDirectoryResolveResult
{
public:
	CstiDirectoryResolveResult () = default;
	~CstiDirectoryResolveResult ();

	inline const char * NameGet () const;
	EstiResult NameSet (const char * szName);

	inline const char * DialStringGet () const;
	EstiResult DialStringSet (const char * szDialString);

	inline EstiBool BlockedGet () const;
	inline EstiResult BlockedSet (EstiBool bBlocked);

	inline EstiBool AnonymousBlockedGet () const;
	inline EstiResult AnonymousBlockedSet (EstiBool bBlocked);

	inline const char * NewNumberGet () const;
	EstiResult NewNumberSet (const char * szDialString);

	inline const char * FromNameGet () const;
	EstiResult FromNameSet (const char * pszFromName);

	inline EstiBool VideoMessageBoxFullGet () const;
	inline EstiResult VideoMessageBoxFullSet (EstiBool bMailBoxFull);

	EstiResult UriListSet (
		const std::list <SstiURIInfo> &List);
	EstiResult UriListGet (
		std::list <SstiURIInfo> *pList) const;
	
	inline const char *SourceGet () const;
	EstiResult SourceSet (const char *pszSource);

	inline EstiBool CountdownEmbeddedGet () const;
	inline EstiResult CountdownEmbeddedSet (EstiBool bCountdownIncluded);

	inline const char *GreetingURLGet () const;
	EstiResult GreetingURLSet (const char *pszGreetingURL);
	
	inline const char *GreetingURL2Get () const;
	EstiResult GreetingURL2Set (const char *pszGreetingURL2);
	
	inline const char *GreetingTextGet () const;
	EstiResult GreetingTextSet (const char *pszGreetingText);

	inline eSignMailGreetingType GreetingPreferenceGet () const;
	inline EstiResult GreetingPreferenceSet (eSignMailGreetingType eGreetingPreference);

	inline int MaxNumberOfRingsGet () const;
	inline EstiResult MaxNumberOfRingsSet (int nMaxNumbOfRings);

	inline int MaxRecordSecondsGet () const;
	inline EstiResult MaxRecordSecondsSet (int nMaxRecordSeconds);

	inline EstiBool P2PMessageSupportedGet () const;
	inline EstiResult P2PMessageSupportedSet (EstiBool bP2PMessageSupported);

	inline int RemoteDownloadSpeedGet () const;
	inline EstiResult RemoteDownloadSpeedSet (int nRemoteDownloadSpeed);

private:
	char * m_szName{nullptr};
	char * m_szDialString{nullptr};
	EstiBool m_bBlocked{estiFALSE};
	EstiBool m_bAnonymousBlocked{estiFALSE};
	char * m_pszNewNumber{nullptr};
	char * m_pszFromName{nullptr};
	std::list <SstiURIInfo> m_UriList;
	char * m_pszSource{nullptr};

	EstiBool m_bMailBoxFull{estiFALSE};
	
	EstiBool m_bCountdownEmbedded{estiTRUE};
	eSignMailGreetingType m_eGreetingPreference{eGREETING_DEFAULT};
	char *m_pszGreetingURL{nullptr};
	char *m_pszGreetingURL2{nullptr};
	char *m_pszGreetingText{nullptr};
	EstiBool m_bP2PMessageSupported{estiFALSE};
	int m_nMaxNumbOfRings{10};
	int m_nMaxRecordSeconds{0};
	int m_nRemoteDownloadSpeed{512000};

}; // end class CstiDirectoryResolveResult

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::DialStringGet
//
// Description: Gets the dial string
//
// Abstract:
//
// Returns:
//	const char * - the dial string value, or NULL, if it does not exist.
//
const char * CstiDirectoryResolveResult::DialStringGet () const
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::DialStringGet);

	return (m_szDialString);
} // end CstiDirectoryResolveResult::DialStringGet


////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::NameGet
//
// Description: Gets the Display Name
//
// Abstract:
//
// Returns:
//	const char * - the name associated with this entry, or NULL
//
const char * CstiDirectoryResolveResult::NameGet () const
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::NameGet);

	return (m_szName);
} // end CstiDirectoryResolveResult::NameGet

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::BlockedGet
//
// Description: Gets the Blocked flag
//
// Abstract:
//
// Returns:
//	EstiBool
//
EstiBool CstiDirectoryResolveResult::BlockedGet () const
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::BlockedGet);

	return (m_bBlocked);
} // end CstiDirectoryResolveResult::BlockedGet


////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::BlockedSet
//
// Description: Sets the Blocked flag
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiDirectoryResolveResult::BlockedSet (
	EstiBool bBlocked)
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::BlockedSet);

	m_bBlocked = bBlocked;

	return (estiOK);
} // end CstiDirectoryResolveResult::BlockedSet

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::AnonymousBlockedGet
//
// Description: Gets the Anonymous Caller Blocked flag
//
// Abstract:
//
// Returns:
//	EstiBool
//
EstiBool CstiDirectoryResolveResult::AnonymousBlockedGet () const
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::AnonymousBlockedGet);
	
	return (m_bAnonymousBlocked);
} // end CstiDirectoryResolveResult::AnonymousBlockedGet


////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::AnonymousBlockedSet
//
// Description: Sets the Anonymous Caller Blocked flag
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiDirectoryResolveResult::AnonymousBlockedSet (
	EstiBool bBlocked)
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::AnonymousBlockedSet);
	
	m_bAnonymousBlocked = bBlocked;
	
	return (estiOK);
} // end CstiDirectoryResolveResult::AnonymousBlockedSet

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::NewNumberGet
//
// Description: Gets the new number
//
// Abstract:
//
// Returns:
//	const char * - the dial string value, or NULL, if it does not exist.
//
const char * CstiDirectoryResolveResult::NewNumberGet () const
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::NewNumberGet);

	return (m_pszNewNumber);
} // end CstiDirectoryResolveResult::NewNumberGet


////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::FromNameGet
//
// Description: Gets the "From Name"
//
// Abstract:
//
// Returns:
//	const char * - the from name, or NULL, if it does not exist.
//
const char * CstiDirectoryResolveResult::FromNameGet () const
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::FromNameGet);

	return (m_pszFromName);
} // end CstiDirectoryResolveResult::FromNameGet


////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::VideoMessageBoxFullGet
//
// Description: Gets the VideoMessageBoxFull flag
//
// Abstract:
//
// Returns:
//	EstiBool
//
EstiBool CstiDirectoryResolveResult::VideoMessageBoxFullGet () const
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::VideoMessageBoxFullGet);

	return (m_bMailBoxFull);
} // end CstiDirectoryResolveResult::VideoMessageBoxFullGet


////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::VideoMessageBoxFullSet
//
// Description: Sets the VideoMessageBoxFull flag
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiDirectoryResolveResult::VideoMessageBoxFullSet (
	EstiBool bMailBoxFull)
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::VideoMessageBoxFullSet);

	m_bMailBoxFull = bMailBoxFull;

	return (estiOK);
} // end CstiDirectoryResolveResult::VideoMessageBoxFullSet

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::CountdownEmbeddedGet
//
// Description: Gets the CountdownEmbedded flag
//
// Abstract:
//
// Returns:
//	EstiBool
//
EstiBool CstiDirectoryResolveResult::CountdownEmbeddedGet () const
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::CountdownEmbeddedGet);

	return (m_bCountdownEmbedded);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::CountdownEmbeddedSet
//
// Description: Sets the CountdownEmbedded flag.
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiDirectoryResolveResult::CountdownEmbeddedSet (
	EstiBool bCountdownEmbedded)
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::CountdownEmbeddedSet);

	m_bCountdownEmbedded = bCountdownEmbedded;

	return (estiOK);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::GreetingURLGet
//
// Description: Gets the GreetingURL
//
// Abstract:
//
// Returns:
//	const char *
//
const char *CstiDirectoryResolveResult::GreetingURLGet () const
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::GreetingURLGet);

	return (m_pszGreetingURL);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::GreetingURL2Get
//
// Description: Gets the second GreetingURL
//
// Abstract:
//
// Returns:
//	const char *
//
const char *CstiDirectoryResolveResult::GreetingURL2Get () const
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::GreetingURL2Get);
	
	return (m_pszGreetingURL2);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::GreetingTextGet
//
// Description: Gets the greeting text
//
// Abstract:
//
// Returns:
//	const char *
//
const char *CstiDirectoryResolveResult::GreetingTextGet () const
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::GreetingTextGet);
	
	return (m_pszGreetingText);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::MaxNumberOfRingsGet
//
// Description: Gets the max number of rings a phone will ring before
// 				the conferance manager will declare the phone as not answered.
//
// Abstract:
//
// Returns:
//	int
//
int CstiDirectoryResolveResult::MaxNumberOfRingsGet () const
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::MaxNumberOfRingsGet);

	return (m_nMaxNumbOfRings);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::MaxNumberOfRingsSet
//
// Description: Sets the max number of rings a phone will ring before
// 				the conferance manager will declare the phone as not answered.
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiDirectoryResolveResult::MaxNumberOfRingsSet (
	int nMaxNumbOfRings)
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::MaxNumberOfRingsSet);

	m_nMaxNumbOfRings = nMaxNumbOfRings;

	return (estiOK);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::GreetingPreferenceGet
//
// Description: Gets the greeting preference
//
// Abstract:
//
// Returns:
//	eSignMailGreetingType
//
eSignMailGreetingType CstiDirectoryResolveResult::GreetingPreferenceGet () const
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::GreetingPreferenceGet);
	
	return (m_eGreetingPreference);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::GreetingPreferenceSet
//
// Description: Sets the greeting preference
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiDirectoryResolveResult::GreetingPreferenceSet (
	eSignMailGreetingType eGreetingPreference)
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::GreetingPreferenceSet);
	
	m_eGreetingPreference = eGreetingPreference;
	
	return (estiOK);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::MaxRecordSecondsGet
//
// Description: Gets the max record time for a message in seconds.
//
// Abstract:
//
// Returns:
//	int
//
int CstiDirectoryResolveResult::MaxRecordSecondsGet () const
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::MaxRecordSecondsGet);

	return (m_nMaxRecordSeconds);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::MaxRecordSecondsSet
//
// Description: Sets the max record time for a messsage in seconds.
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiDirectoryResolveResult::MaxRecordSecondsSet (
	int nMaxRecordSeconds)
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::MaxRecordSecondsSet);

	m_nMaxRecordSeconds = nMaxRecordSeconds;

	return (estiOK);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::P2PMessageSupportedGet
//
// Description: Gets the P2PMessageSupported flag
//
// Abstract:
//
// Returns:
//	EstiBool
//
EstiBool CstiDirectoryResolveResult::P2PMessageSupportedGet () const
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::P2PMessageSupportedGet);

	return (m_bP2PMessageSupported);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::P2PMessageSupportedSet
//
// Description: Sets the P2PMessageSupported flag.
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiDirectoryResolveResult::P2PMessageSupportedSet (
	EstiBool bP2PMessageSupported)
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::P2PMessageSupportedSet);

	m_bP2PMessageSupported = bP2PMessageSupported;

	return (estiOK);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::RemoteDownloadSpeedGet
//
// Description: Gets the remote phone's download speed.
//
// Abstract:
//
// Returns:
//	int
//
int CstiDirectoryResolveResult::RemoteDownloadSpeedGet () const
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::RemoteDownloadSpeedGet);

	return (m_nRemoteDownloadSpeed);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::RemoteDownloadSpeedSet
//
// Description: Sets the download speed of the remote phone.
//
// Abstract:
//
// Returns:
//	EstiResult - estiOK on success, otherwise estiERROR
//
EstiResult CstiDirectoryResolveResult::RemoteDownloadSpeedSet (
	int nRemoteDownloadSpeed)
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::RemoteDownloadSpeedSet);

	m_nRemoteDownloadSpeed = nRemoteDownloadSpeed;

	return (estiOK);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiDirectoryResolveResult::SourceGet
//
// Description: Gets the Source
//
// Abstract:
//
// Returns:
//	const char *
//
const char *CstiDirectoryResolveResult::SourceGet () const
{
	stiLOG_ENTRY_NAME (CstiDirectoryResolveResult::SourceGet);

	return (m_pszSource);
}

