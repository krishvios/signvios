///
/// \file CstiRegistrationManager.cpp
/// \brief Implements a manager for Registration data
///
/// Sorenson Communications Inc. Confidential - Do not distribute
/// Copyright 2016 by Sorenson Communications, Inc. All rights reserved.
///


#ifndef CSTIREGISTRATIONMANAGER_H
#define CSTIREGISTRATIONMANAGER_H

#include "XMLManager.h"
#include "XMLList.h"
#include "CstiCoreResponse.h"
#include "CstiRegistrationList.h"
#include "openssl/md5.h"
#include <sstream>

const char XMLRegistrationFileName[] = "Registrations.xml";

class CstiRegistrationManager : public WillowPM::XMLManager
{
public:
	CstiRegistrationManager ();
	~CstiRegistrationManager () override;
	stiHResult Initialize ();

	WillowPM::XMLListItemSharedPtr NodeToItemConvert (IXML_Node *pNode) override;

	stiHResult PhoneNumbersSet (SstiUserPhoneNumbers * pPhoneNumbers);

	stiHResult PhoneNumbersGet (SstiUserPhoneNumbers * pPhoneNumbers);

	stiHResult ClearRegistrations ();

	SstiRegistrationInfo SipRegistrationInfoGet ();

	bool RegistrationsUpdate (CstiCoreResponse*  coreResponse);


	bool CredentialsCheck (const char& Username, const char& Pin);

private:
	CstiRegistrationListSharedPtr m_Registrations;
};

#endif //#ifndef CSTIREGISTRATIONMANAGER_H


