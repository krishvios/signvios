///
/// \file CstiRegistrationManager.cpp
/// \brief Implements a manager for Registration data
///
/// Sorenson Communications Inc. Confidential - Do not distribute
/// Copyright 2016 by Sorenson Communications, Inc. All rights reserved.
///


#ifndef CSTIREGISTRATIONLIST_H
#define CSTIREGISTRATIONLIST_H

#include "stiTools.h"
#include "ixml.h"
#include "XMLListItem.h"
#include "CstiRegistrationListItem.h"
#include <memory>

const char XMLRegistrationList[] = "RegistrationList";
const char XMLRegistration[] = "Registration";
const char XMLUserNumbers[] = "UserNumbers";
const char XMLLocalPhoneNumber[] = "LocalPhoneNumber";
const char XMLTollFreePhoneNumber[] = "TollFreePhoneNumber";
const char XMLRingGroupLocalPhoneNumber[] = "RingGroupLocalPhoneNumber";
const char XMLRingGroupTollFreePhoneNumber[] = "RingGroupTollFreePhoneNumber";
const char XMLSorensonPhoneNumber[] = "SorensonPhoneNumber";

class CstiRegistrationList : public WillowPM::XMLListItem
{
public:
	CstiRegistrationList ();
	~CstiRegistrationList () override = default;

	void Write (FILE *pFile) override;
	std::string NameGet () const override;
	void NameSet (const std::string &name) override;
	bool NameMatch (const std::string &name) override;
	SstiRegistrationInfo SipRegistrationInfoGet () const;
	bool RegistrationsMatch (SstiRegistrationInfo* registrationList);
	void RegistrationsUpdate (SstiRegistrationInfo* registrationList);
	bool CredentialsCheck (const char& Username, const char& Pin);
	stiHResult PhoneNumbersSet (const SstiUserPhoneNumbers * pPhoneNumbers);
	stiHResult PhoneNumbersGet (SstiUserPhoneNumbers * pPhoneNumbers) const;
	stiHResult XMLListProcess (IXML_NodeList *pNodeList);
	void ClearRegistrations ();

private:
	CstiRegistrationListItem m_SipRegistration;
	SstiUserPhoneNumbers m_sPhoneNumbers{};
};

using CstiRegistrationListSharedPtr = std::shared_ptr<CstiRegistrationList>;

#endif

