///
/// \file CstiRegistrationList.h
/// \brief Implements a list of registration items
///
/// Sorenson Communications Inc. Confidential - Do not distribute
/// Copyright 2016 by Sorenson Communications, Inc. All rights reserved.
///

#ifndef CSTIREGISTRATIONLISTITEM_H
#define CSTIREGISTRATIONLISTITEM_H

#include "stiTools.h"
#include "ixml.h"
#include "XMLListItem.h"
#include "stiConfDefs.h"
#include <vector>

const char XMLSIP[] = "SIP";
const char XMLUNKNOWN[] = "UNKNOWN";
const char XMLRegistrationListItem[] = "Registration";
const char XMLPublicDomain[] = "PublicDomain";
const char XMLPrivateDomain[] = "PrivateDomain";
const char XMLCredentialList[] = "CredentialList";
const char XMLCredentialListItem[] = "CredentialListItem";
const char XMLUsername[] = "Username";
const char XMLPassword[] = "Password";

///
/// \brief Struct defining the registration information for SIP proxy, etc.
///
struct SRegistrationCredentials
{
	std::string UserName;
	std::string Password;
};

class CstiRegistrationListItem : public WillowPM::XMLListItem
{
public:
	CstiRegistrationListItem (EstiProtocol eType);
	~CstiRegistrationListItem () override = default;

	void Write (FILE *pFile) override;
	std::string NameGet () const override;
	void NameSet (const std::string &name) override;
	bool NameMatch (const std::string &name) override;
	EstiProtocol TypeGet ();
	SstiRegistrationInfo RegistrationInfoGet () const;
	bool RegistrationMatch (SstiRegistrationInfo * pRegistrationData);
	void RegistrationUpdate (SstiRegistrationInfo * pRegisgrationData);

	bool CredentialsCheck (const char& Username, const char& Pin);
	static EstiProtocol RegistrationType (IXML_Node * pNode);
	stiHResult XMLNodeProcess (IXML_Node * pNode, EstiProtocol eType);
	void RegistrationDataClear ();

private:
	EstiProtocol m_eType;
	SstiRegistrationInfo m_sRegInfo;

};

#endif
