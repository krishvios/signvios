#include "UserAccountListItem.h"
#include "CstiString.h"
#include "CstiUserAccountInfo.h"

// User account info keys
static const std::string Name = "Name";
static const std::string TollFreeNumber = "TollFreeNumber";
static const std::string LocalNumber = "LocalNumber";
static const std::string PhoneNumber = "PhoneNumber";
static const std::string LocalNumberNewDate = "LocalNumberNewDate";
static const std::string NewTagValidDays = "NewTagValidDays";
static const std::string AssociatedPhone = "AssociatedPhone";
static const std::string CurrentPhone = "CurrentPhone";
static const std::string Email = "Email";
static const std::string EmailMain = "EmailMain";
static const std::string EmailPager = "EmailPager";
static const std::string SignMailEnabled = "SignMailEnabled";
static const std::string MustCallCIR = "MustCallCIR";
static const std::string CLAnsweredQuota = "CLAnsweredQuota";
static const std::string CLDialedQuota = "CLDialedQuota";
static const std::string CLMissedQuota = "CLMissedQuota";
static const std::string CLRedialQuota = "CLRedialQuota";
static const std::string CLBlockedQuota = "CLBlockedQuota";
static const std::string CLContactQuota = "CLContactQuota";
static const std::string CLFavoritesQuota = "CLFavoritesQuota";
static const std::string RingGroupLocalNumber = "RingGroupLocalNumber";
static const std::string RingGroupLocalNumberNewDate = "RingGroupLocalNumberNewDate";
static const std::string RingGroupNewTagValidDays = "RingGroupNewTagValidDays";
static const std::string RingGroupTollFreeNumber = "RingGroupTollFreeNumber";
static const std::string RingGroupName = "RingGroupName";
static const std::string ImageId = "ImageId";
static const std::string ImageDate = "ImageDate";
static const std::string UserRegistrationDataRequired = "UserRegistrationDataRequired";

// Emergency address keys
static const std::string Street = "Street";
static const std::string ApartmentNumber = "ApartmentNumber";
static const std::string City = "City";
static const std::string State = "State";
static const std::string ZipCode = "ZipCode";
static const std::string AddressStatus = "AddressStatus";

UserAccountListItem::UserAccountListItem (CstiUserAccountInfo& userAccountInfo, vpe::Address& emergencyAddress, EstiEmergencyAddressProvisionStatus& addressStatus) :
	m_userAccountInfo (userAccountInfo),
	m_emergencyAddress (emergencyAddress),
	m_addressStatus(addressStatus)
{
}

void UserAccountListItem::valueSet (IXML_Node* node)
{
	std::string elementName = ixmlNode_getNodeName (node);
	std::string elementValue;
	if (node->firstChild != nullptr && node->firstChild->nodeValue != nullptr)
	{
		elementValue = node->firstChild->nodeValue;
	}

	// Read user account info fields
	if (Name == elementName)
	{
		m_userAccountInfo.m_Name = elementValue;
	}
	else if (TollFreeNumber == elementName)
	{
		m_userAccountInfo.m_TollFreeNumber = elementValue;
	}
	else if (LocalNumber == elementName)
	{
		m_userAccountInfo.m_LocalNumber = elementValue;
	}
	else if (LocalNumberNewDate == elementName)
	{
		if (elementValue.empty())
		{
			m_userAccountInfo.m_localNumberNewDate = 0;
		}
		else
		{
			m_userAccountInfo.m_localNumberNewDate = std::stoull (elementValue);
		}
	}
	else if (PhoneNumber == elementName)
	{
		m_userAccountInfo.m_PhoneNumber = elementValue;
	}
	else if (NewTagValidDays == elementName)
	{
		m_userAccountInfo.m_newTagValidDays = elementValue;
	}
	else if (AssociatedPhone == elementName)
	{
		m_userAccountInfo.m_AssociatedPhone = elementValue;
	}
	else if (CurrentPhone == elementName)
	{
		m_userAccountInfo.m_CurrentPhone = elementValue;
	}
	else if (Email == elementName)
	{
		m_userAccountInfo.m_Email = elementValue;
	}
	else if (EmailMain == elementName)
	{
		m_userAccountInfo.m_EmailMain = elementValue;
	}
	else if (EmailPager == elementName)
	{
		m_userAccountInfo.m_EmailPager = elementValue;
	}
	else if (SignMailEnabled == elementName)
	{
		m_userAccountInfo.m_SignMailEnabled = elementValue;
	}
	else if (MustCallCIR == elementName)
	{
		m_userAccountInfo.m_MustCallCIR = elementValue;
	}
	else if (CLAnsweredQuota == elementName)
	{
		m_userAccountInfo.m_CLAnsweredQuota = elementValue;
	}
	else if (CLDialedQuota == elementName)
	{
		m_userAccountInfo.m_CLDialedQuota = elementValue;
	}
	else if (CLMissedQuota == elementName)
	{
		m_userAccountInfo.m_CLMissedQuota = elementValue;
	}
	else if (CLRedialQuota == elementName)
	{
		m_userAccountInfo.m_CLRedialQuota = elementValue;
	}
	else if (CLBlockedQuota == elementName)
	{
		m_userAccountInfo.m_CLBlockedQuota = elementValue;
	}
	else if (CLContactQuota == elementName)
	{
		m_userAccountInfo.m_CLContactQuota = elementValue;
	}
	else if (CLFavoritesQuota == elementName)
	{
		m_userAccountInfo.m_CLFavoritesQuota = elementValue;
	}
	else if (RingGroupLocalNumber == elementName)
	{
		m_userAccountInfo.m_RingGroupLocalNumber = elementValue;
	}
	else if (RingGroupLocalNumberNewDate == elementName)
	{
		if (elementValue.empty())
		{
			m_userAccountInfo.m_ringGroupLocalNumberNewDate = 0;
		}
		else
		{
			m_userAccountInfo.m_ringGroupLocalNumberNewDate = std::stoull (elementValue);
		}
	}
	else if (RingGroupNewTagValidDays == elementName)
	{
		m_userAccountInfo.m_ringGroupNewTagValidDays = elementValue;
	}
	else if (RingGroupTollFreeNumber == elementName)
	{
		m_userAccountInfo.m_RingGroupTollFreeNumber = elementValue;
	}
	else if (RingGroupName == elementName)
	{
		m_userAccountInfo.m_RingGroupName = elementValue;
	}
	else if (ImageId == elementName)
	{
		m_userAccountInfo.m_ImageId = elementValue;
	}
	else if (ImageDate == elementName)
	{
		m_userAccountInfo.m_ImageDate = elementValue;
	}
	else if (UserRegistrationDataRequired == elementName)
	{
		m_userAccountInfo.m_UserRegistrationDataRequired = elementValue;
	}


	// Read emergency address fields
	else if (Street == elementName)
	{
		m_emergencyAddress.street = elementValue;
	}
	else if (ApartmentNumber == elementName)
	{
		m_emergencyAddress.apartmentNumber = elementValue;
	}
	else if (City == elementName)
	{
		m_emergencyAddress.city = elementValue;
	}
	else if (State == elementName)
	{
		m_emergencyAddress.state = elementValue;
	}
	else if (ZipCode == elementName)
	{
		m_emergencyAddress.zipCode = elementValue;
	}
	else if (AddressStatus == elementName)
	{
		m_addressStatus = static_cast<EstiEmergencyAddressProvisionStatus>(std::stoi(elementValue));
		m_addressStatusLoaded = true;
	}
	else
	{
		stiTrace ("UserAccountListItem::valueSet: Unexpected element (%s)\n", elementName.c_str ());
	}
}

void UserAccountListItem::emergencyAddressSet (const ServiceResponseSP<vpe::Address>& response)
{
	if (response->responseSuccessful)
	{
		m_emergencyAddress.street = response->content.street;
		m_emergencyAddress.apartmentNumber = response->content.apartmentNumber;
		m_emergencyAddress.city = response->content.city;
		m_emergencyAddress.state = response->content.state;
		m_emergencyAddress.zipCode = response->content.zipCode;
	}
}

void UserAccountListItem::addressStatusSet (EstiEmergencyAddressProvisionStatus status)
{
	m_addressStatus = status;
}

bool UserAccountListItem::loadedRequiredValues ()
{
	return (!m_userAccountInfo.m_LocalNumber.empty() || !m_userAccountInfo.m_PhoneNumber.empty())
		&& !m_emergencyAddress.street.empty()
		&& m_addressStatusLoaded;
}

void UserAccountListItem::Write (FILE* file)
{
	// Write user account info fields
	WriteField (file, m_userAccountInfo.m_Name, Name);
	WriteField (file, m_userAccountInfo.m_TollFreeNumber, TollFreeNumber);
	WriteField (file, m_userAccountInfo.m_LocalNumber, LocalNumber);
	WriteField (file, std::to_string (m_userAccountInfo.m_localNumberNewDate), LocalNumberNewDate);
	WriteField (file, m_userAccountInfo.m_PhoneNumber, PhoneNumber);
	WriteField (file, m_userAccountInfo.m_newTagValidDays, NewTagValidDays);
	WriteField (file, m_userAccountInfo.m_AssociatedPhone, AssociatedPhone);
	WriteField (file, m_userAccountInfo.m_CurrentPhone, CurrentPhone);
	WriteField (file, m_userAccountInfo.m_Email, Email);
	WriteField (file, m_userAccountInfo.m_EmailMain, EmailMain);
	WriteField (file, m_userAccountInfo.m_EmailPager, EmailPager);
	WriteField (file, m_userAccountInfo.m_SignMailEnabled, SignMailEnabled);
	WriteField (file, m_userAccountInfo.m_MustCallCIR, MustCallCIR);
	WriteField (file, m_userAccountInfo.m_CLAnsweredQuota, CLAnsweredQuota);
	WriteField (file, m_userAccountInfo.m_CLDialedQuota, CLDialedQuota);
	WriteField (file, m_userAccountInfo.m_CLMissedQuota, CLMissedQuota);
	WriteField (file, m_userAccountInfo.m_CLRedialQuota, CLRedialQuota);
	WriteField (file, m_userAccountInfo.m_CLBlockedQuota, CLBlockedQuota);
	WriteField (file, m_userAccountInfo.m_CLContactQuota, CLContactQuota);
	WriteField (file, m_userAccountInfo.m_CLFavoritesQuota, CLFavoritesQuota);
	WriteField (file, m_userAccountInfo.m_RingGroupLocalNumber, RingGroupLocalNumber);
	WriteField (file, std::to_string (m_userAccountInfo.m_ringGroupLocalNumberNewDate), RingGroupLocalNumberNewDate);
	WriteField (file, m_userAccountInfo.m_ringGroupNewTagValidDays, RingGroupNewTagValidDays);
	WriteField (file, m_userAccountInfo.m_RingGroupTollFreeNumber, RingGroupTollFreeNumber);
	WriteField (file, m_userAccountInfo.m_RingGroupName, RingGroupName);
	WriteField (file, m_userAccountInfo.m_ImageId, ImageId);
	WriteField (file, m_userAccountInfo.m_ImageDate, ImageDate);
	WriteField (file, m_userAccountInfo.m_UserRegistrationDataRequired, UserRegistrationDataRequired);

	// Write emergency address fields
	WriteField (file, m_emergencyAddress.street, Street);
	WriteField (file, m_emergencyAddress.apartmentNumber, ApartmentNumber);
	WriteField (file, m_emergencyAddress.city, City);
	WriteField (file, m_emergencyAddress.state, State);
	WriteField (file, m_emergencyAddress.zipCode, ZipCode);
	WriteField (file, std::to_string (m_addressStatus), AddressStatus);
}

void UserAccountListItem::WriteField (FILE* file, const CstiString& value, const std::string& name)
{
	if (value != nullptr)
	{
		WillowPM::XMLListItem::WriteField (file, value.toStdString (), name);
	}
}

/*! \brief NameGet - This method is not used by the UserAccountManager
 *
 * \return an empty string
 */
std::string UserAccountListItem::NameGet () const
{
	return std::string ();
}

/*! \brief NameSet - This method is not used by the UserAccountManager
 *
 * \param name ignored
 */
void UserAccountListItem::NameSet (const std::string& name)
{
}

/*! \brief NameMatch - This method is not used by the UserAccountManager
 *
 * \param name ignored
 * \returns false
 */
bool UserAccountListItem::NameMatch (const std::string& name)
{
	return false;
}
