////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiContactListItem
//
//	File Name:	CstiContactListItem.cpp
//
//	Abstract:
//		Implement a single contact list item
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
#include "stiDefs.h"
#include "CstiContactListItem.h"

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
//; CstiContactListItem::NameGet
//
// Description: NameGet
//
// Abstract:
//		NameGet
//
// Returns:
//	CstiString
//
const char *CstiContactListItem::NameGet() const
{
	return m_Name;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::NameSet
//
// Description: NameSet
//
// Abstract:
//	NameSet
//
// Returns:
//	None
//
void CstiContactListItem::NameSet(const char *pszName)
{
	m_Name = pszName;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::CompanyNameGet
//
// Description: CompanyNameGet
//
// Abstract:
//		CompanyNameGet
//
// Returns:
//	CstiString
//
const char *CstiContactListItem::CompanyNameGet() const
{
	return m_CompanyName;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::CompanyNameSet
//
// Description: CompanyNameSet
//
// Abstract:
//	CompanyNameSet
//
// Returns:
//	None
//
void CstiContactListItem::CompanyNameSet(const char *pszCompany)
{
	m_CompanyName = pszCompany;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::DialStringGet
//
// Description: DialStringGet
//
// Abstract:
//	DialStringGet
//
// Returns:
//	CstiString
//
const char *CstiContactListItem::DialStringGet(unsigned int unIndex) const
{
	if (unIndex < m_NumberList.size())
	{
		return m_NumberList[unIndex].DialString;
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::DialStringSet
//
// Description: DialStringSet
//
// Abstract:
//	DialStringSet
//
// Returns:
//	None
//
void CstiContactListItem::DialStringSet(unsigned int unIndex, const char *pszDialString)
{
	if (unIndex < m_NumberList.size())
	{
		m_NumberList[unIndex].DialString = pszDialString;
	}
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::ItemIdGet
//
// Description: ItemIdGet
//
// Abstract:
//	ItemIdGet
//
// Returns:
//	ItemIdGet
//
const char * CstiContactListItem::ItemIdGet() const
{
	return m_ItemIdString;
}

void CstiContactListItem::ItemIdGet(CstiItemId *pId) const
{
	*pId = m_ItemId;
}

const char * CstiContactListItem::ItemPublicIdGet() const
{
	return m_ItemPublicIdString;
}

void CstiContactListItem::ItemPublicIdGet(CstiItemId *pPublicId) const
{
	*pPublicId = m_ItemPublicId;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::ItemIdSet
//
// Description: ItemIdSet
//
// Abstract:
//	ItemIdSet
//
// Returns:
//	None
//
void CstiContactListItem::ItemIdSet(const char *pszItemId)
{
	m_ItemId.ItemIdSet(pszItemId);
	m_ItemIdString = pszItemId;
}

void CstiContactListItem::ItemIdSet(const CstiItemId &ItemId)
{
	m_ItemId = ItemId;
	char idString[256];
	m_ItemId.ItemIdGet(idString, sizeof (idString));
	m_ItemIdString = idString;
}

void CstiContactListItem::ItemPublicIdSet(const char *pszItemPublicId)
{
	m_ItemPublicId.ItemIdSet(pszItemPublicId);
	m_ItemPublicIdString = pszItemPublicId;
}

void CstiContactListItem::ItemPublicIdSet(const CstiItemId &PublicId)
{
	m_ItemPublicId = PublicId;
	char idString[256];
	m_ItemPublicId.ItemIdGet(idString, sizeof (idString));
	m_ItemPublicIdString = idString;
}

#ifdef stiCONTACT_PHOTOS
////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::ImageIdGet
//
// Description: ImageIdGet
//
// Abstract:
//	ImageIdGet
//
// Returns:
//	ImageIdGet
//
const char *CstiContactListItem::ImageIdGet() const
{
	return m_ImageId;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::ImageIdSet
//
// Description: ImageIdSet
//
// Abstract:
//	ImageIdSet
//
// Returns:
//	None
//
void CstiContactListItem::ImageIdSet(const char *pszImageId)
{
	m_ImageId = pszImageId;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::ImageTimestampGet
//
// Description: ImageTimestampGet
//
// Abstract:
//	ImageTimestampGet
//
// Returns:
//	timestamp of when the image was last stored in core
//
const char *CstiContactListItem::ImageTimestampGet() const
{
	return m_ImageTimestamp;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::ImageTimestampSet
//
// Description: ImageTimestampSet
//
// Abstract:
//	ImageTimestampSet
//
// Returns:
//	None
//
void CstiContactListItem::ImageTimestampSet(const char *pszImageTimestamp)
{
	m_ImageTimestamp = pszImageTimestamp;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::ImageOverrideSet
//
// Description: Controls whether or not the image for this contact is overridden
// by the endpoint for this contact.
//
// Abstract:
//	ImageTimestampSet
//
// Returns:
//	None
//
void CstiContactListItem::ImageOverrideSet(bool overridden)
{
	m_ImageOverridden = overridden;
}
#endif

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::DialMethodGet
//
// Description: DialMethodGet
//
// Abstract:
//	DialMethodGet
//
// Returns:
//	EstiDialMethod
//
EstiDialMethod CstiContactListItem::DialMethodGet(unsigned int unIndex) const
{
	if (unIndex < m_NumberList.size())
	{
		return m_NumberList[unIndex].eDialMethod;
	}

	return estiDM_UNKNOWN;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::DialMethodSet
//
// Description: DialMethodSet
//
// Abstract:
//	DialMethodSet
//
// Returns:
//	None
//
void CstiContactListItem::DialMethodSet(unsigned int unIndex, EstiDialMethod eDialMethod)
{
	if (unIndex < m_NumberList.size())
	{
		m_NumberList[unIndex].eDialMethod = eDialMethod;
	}
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::RingToneGet
//
// Description: RingToneGet
//
// Abstract:
//	RingToneGet
//
// Returns:
//	int
//
int CstiContactListItem::RingToneGet() const
{
	return m_nRingTone;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::RingToneSet
//
// Description: RingToneSet
//
// Abstract:
//	RingToneSet
//
// Returns:
//	None
//
void CstiContactListItem::RingToneSet(int nRingTone)
{
	m_nRingTone = nRingTone;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::RingColorGet
//
// Description: RingColorGet
//
// Abstract:
//	RingColorGet
//
// Returns:
//	int
//
int CstiContactListItem::RingColorGet() const
{
    return m_nRingColor;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::RingColorSet
//
// Description: RingColorSet
//
// Abstract:
//	RingColorSet
//
// Returns:
//	None
//
void CstiContactListItem::RingColorSet(int nRingColor)
{
    m_nRingColor = nRingColor;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::RelayLanguageGet
//
// Description: RelayLanguageGet
//
// Abstract:
//	Get the language that the interpreter needs to speak to the hearing person.
//
// Returns:
//	int
//
const char *CstiContactListItem::RelayLanguageGet() const
{
	return m_RelayLanguage;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::RelayLanguageSet
//
// Description: RelayLanguageSet
//
// Abstract:
//	Set the language that the interpreter needs to speak to the hearing person.
//
// Returns:
//	None
//
void CstiContactListItem::RelayLanguageSet(const char *Language)
{
	m_RelayLanguage = Language;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::PhoneNumberTypeGet
//
// Description: PhoneNumberTypeGet
//
// Abstract:
//	PhoneNumberTypeGet
//
// Returns:
//	EPhoneNumberType
//
CstiContactListItem::EPhoneNumberType CstiContactListItem::PhoneNumberTypeGet(unsigned int unIndex) const
{
	if (unIndex < m_NumberList.size())
	{
		return m_NumberList[unIndex].ePhoneType;
	}

	return ePHONE_HOME;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::PhoneNumberTypeSet
//
// Description: PhoneNumberTypeSet
//
// Abstract:
//	PhoneNumberTypeSet
//
// Returns:
//	None
//
void CstiContactListItem::PhoneNumberTypeSet(unsigned int unIndex, EPhoneNumberType eType)
{
	if (unIndex < m_NumberList.size())
	{
		m_NumberList[unIndex].ePhoneType = eType;
	}
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::PhoneNumberIdGet
//
// Description: PhoneNumberIdGet
//
// Abstract:
//	PhoneNumberIdGet
//
// Returns:
//	unsigned int
//
const char *CstiContactListItem::PhoneNumberIdGet(unsigned int unIndex) const
{
	if (unIndex < m_NumberList.size())
	{
		return m_NumberList[unIndex].IdString;
	}

	return nullptr;
}

void CstiContactListItem::PhoneNumberIdGet(unsigned int unIndex, CstiItemId *pId) const
{
	if (unIndex < m_NumberList.size())
	{
		 *pId = m_NumberList[unIndex].Id;
	}
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::PhoneNumberIdSet
//
// Description: PhoneNumberIdSet
//
// Abstract:
//	PhoneNumberIdSet
//
// Returns:
//	None
//
void CstiContactListItem::PhoneNumberIdSet(unsigned int unIndex, const char *pId)
{
	if (unIndex < m_NumberList.size())
	{
		m_NumberList[unIndex].Id.ItemIdSet(pId);
		m_NumberList[unIndex].IdString=pId;
	}
}

void CstiContactListItem::PhoneNumberIdSet(unsigned int unIndex, const CstiItemId &Id)
{
	if (unIndex < m_NumberList.size())
	{
		m_NumberList[unIndex].Id = Id;

		std::string phoneNumberId;
		Id.ItemIdGet(&phoneNumberId);
		m_NumberList[unIndex].IdString=phoneNumberId.c_str();
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::PhoneNumberPublicIdGet
//
// Description: PhoneNumberPublicIdGet
//
// Abstract:
//	PhoneNumberPublicIdGet
//
// Returns:
//	const char *
//
const char *CstiContactListItem::PhoneNumberPublicIdGet(unsigned int unIndex) const
{
	if (unIndex < m_NumberList.size())
	{
		 return m_NumberList[unIndex].PublicIdString;
	}

	return nullptr;
}

void CstiContactListItem::PhoneNumberPublicIdGet(unsigned int unIndex, CstiItemId *pId) const
{
	if (unIndex < m_NumberList.size())
	{
		 *pId = m_NumberList[unIndex].PublicId;
	}
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::PhoneNumberPublicIdSet
//
// Description: PhoneNumberPublicIdSet
//
// Abstract:
//	PhoneNumberPublicIdSet
//
// Returns:
//	None
//
void CstiContactListItem::PhoneNumberPublicIdSet(unsigned int unIndex, const char *pPublicId)
{
	if (unIndex < m_NumberList.size())
	{
		m_NumberList[unIndex].PublicId.ItemIdSet(pPublicId);
		m_NumberList[unIndex].PublicId=pPublicId;
	}
}

void CstiContactListItem::PhoneNumberPublicIdSet(unsigned int unIndex, const CstiItemId &PublicId)
{
	if (unIndex < m_NumberList.size())
	{
		m_NumberList[unIndex].PublicId = PublicId;
		std::string publicId;
		PublicId.ItemIdGet(&publicId);
		m_NumberList[unIndex].PublicIdString=publicId.c_str();
	}
}

////////////////////////////////////////////////////////////////////////////////
//; CstiContactListItem::PhoneNumberAdd
//
// Description: PhoneNumberAdd
//
// Abstract:
//	PhoneNumberAdd
//
// Returns:
//	None
//
void CstiContactListItem::PhoneNumberAdd()
{
	CstiContactNumber ccn;
	m_NumberList.push_back(ccn);
}

// end file CstiContactListItem.cpp
