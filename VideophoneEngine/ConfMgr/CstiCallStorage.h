/*!
* \file CstiCallStorage.h
* \brief See CstiCallStorage.cpp
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2000 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once

#include <list>
#include <memory>
#include <mutex>
#include "stiError.h"

class CstiCall;


const int nSTALE_OBJECT_CHECK_DELAY_SEC = 5;  // 5 seconds
const int nSTALE_OBJECT_CHECK_DELAY_MS = nSTALE_OBJECT_CHECK_DELAY_SEC * 1000; // Milliseconds

class CstiCallStorage
{
public:

	CstiCallStorage ();

	void lock () const;
	void unlock () const;

	std::list<CstiCallSP> listGet () const;
	
#ifdef stiHOLDSERVER
	CstiCallSP nthCallGet (uint32_t nthCallNumber);
	CstiCallSP CallObjectGetByLocalPhoneNbr (const char *phoneNumber);
	CstiCallSP CallObjectGetByHSCallId (__int64 n64CallID);
	void hangupAllCalls ();
#endif

	stiHResult remove (
	    CstiCall *call);

	void removeAll ();

	void store (
	    const CstiCallSP &call);

	stiHResult removeStaleObjects ();

	CstiCallSP headGet ();
	
	unsigned int countGet ();
	unsigned int countGet (uint32_t stateMask);
	unsigned int activeCallCountGet ();

	CstiCallSP get (
	    uint32_t stateMask);

	CstiCallSP lookup (
	    IstiCall *call) const;

	bool leavingMessageGet ();

	CstiCallSP getByAppData (
		size_t AppData);

	CstiCallSP incomingGet ();
	CstiCallSP outgoingGet ();
	CstiCallSP getByCallIndex (int callIndex);

	stiHResult collectStats ();

private:
	mutable std::recursive_mutex m_mutex;
	std::list<CstiCallSP> m_callList;

	CstiSignalConnection::Collection m_signalConnections;
};
