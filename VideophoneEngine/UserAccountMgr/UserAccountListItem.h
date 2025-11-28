#pragma once
#include "XMLListItem.h"
#include "ixml.h"
#include "ServiceResponse.h"
#include "Address.h"
#include <string>

class CstiString;
class CstiUserAccountInfo;

class UserAccountListItem : public WillowPM::XMLListItem
{
public:

	UserAccountListItem (
		CstiUserAccountInfo& userAccountInfo,
		vpe::Address& emergencyAddress,
		EstiEmergencyAddressProvisionStatus& addressStatus);

	void valueSet (IXML_Node* node);
	void emergencyAddressSet (const ServiceResponseSP<vpe::Address>& response);
	void addressStatusSet (EstiEmergencyAddressProvisionStatus status);
	bool loadedRequiredValues ();

private:

	void Write (FILE* pFile) override;
	std::string NameGet () const override;
	void NameSet (const std::string& name) override;
	bool NameMatch (const std::string& name) override;
	void WriteField (FILE* file, const CstiString& value, const std::string& name);

	CstiUserAccountInfo& m_userAccountInfo;
	vpe::Address& m_emergencyAddress;
	EstiEmergencyAddressProvisionStatus& m_addressStatus;
	bool m_addressStatusLoaded = false;
};