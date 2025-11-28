////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiContactListItem
//
//	File Name:	CstiContactListItem.h
//
//	Abstract:
//		See CstiContactListItem.cpp
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTI_CONTACT_LIST_ITEM_H
#define CSTI_CONTACT_LIST_ITEM_H

//
// Includes
//
#include <vector>
#include "stiDefs.h"
#include "stiSVX.h"
#include "CstiListItem.h"
#include "CstiString.h"
#include "CstiItemId.h"

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

class CstiContactListItem : public CstiListItem
{
public:

	CstiContactListItem() = default;
	~CstiContactListItem() override = default;

	CstiContactListItem (const CstiContactListItem &) = delete;
	CstiContactListItem &operator= (const CstiContactListItem &) = delete;

	const char *NameGet() const;
	void NameSet (const char *pszName);
	const char *CompanyNameGet() const;
	void CompanyNameSet (const char *pszCompany);
	const char *ItemIdGet() const;
	void ItemIdGet(CstiItemId *pId) const;
	void ItemIdSet(const char *pszItemId);
	void ItemIdSet(const CstiItemId &ItemId);
	const char *ItemPublicIdGet() const;
	void ItemPublicIdGet(CstiItemId *pPublicId) const;
	void ItemPublicIdSet(const char *pszItemPublicId);
	void ItemPublicIdSet(const CstiItemId &PublicId);
	int RingToneGet() const;
	void RingToneSet(int nRingTone);
    int RingColorGet() const;
    void RingColorSet(int nRingColor);
    void RelayLanguageSet(const char *Language);
	const char *RelayLanguageGet() const;

#ifdef stiCONTACT_PHOTOS
	const char *ImageIdGet() const;
	void ImageIdSet(const char *pszImageId);
	const char *ImageTimestampGet() const;
	void ImageTimestampSet(const char *pszImageTimestamp);
	bool ImageIsOverridden() const { return m_ImageOverridden; }
	void ImageOverrideSet(bool override);
#endif

	const char *DialStringGet(unsigned int unIndex) const;
	void DialStringSet(unsigned int unIndex, const char *pszDialString);
	EstiDialMethod DialMethodGet(unsigned int unIndex) const;
	void DialMethodSet(unsigned int unIndex, EstiDialMethod eDialMethod);

	enum EPhoneNumberType
	{
		ePHONE_HOME = 0,
		ePHONE_WORK,
		ePHONE_CELL,

		ePHONE_MAX
	};

	EPhoneNumberType PhoneNumberTypeGet(unsigned int unIndex) const;
	void PhoneNumberTypeSet(unsigned int unIndex, EPhoneNumberType eType);
	const char *PhoneNumberIdGet(unsigned int unIndex) const;
	void PhoneNumberIdGet(unsigned int unIndex, CstiItemId *pId) const;
	void PhoneNumberIdSet(unsigned int unIndex, const char *pszId);
	void PhoneNumberIdSet(unsigned int unIndex, const CstiItemId &Id);
	const char *PhoneNumberPublicIdGet(unsigned int unIndex) const;
	void PhoneNumberPublicIdGet(unsigned int unIndex, CstiItemId *pId) const;
	void PhoneNumberPublicIdSet(unsigned int unIndex, const char *pszPublicId);
	void PhoneNumberPublicIdSet(unsigned int unIndex, const CstiItemId &PublicId);

	int PhoneNumberCountGet() const { return m_NumberList.size(); }

	void PhoneNumberAdd();

private:

	struct CstiContactNumber
	{
		CstiItemId Id;
		CstiString IdString;
		EstiDialMethod eDialMethod;
		EPhoneNumberType ePhoneType;
		CstiString DialString;
		CstiItemId PublicId;
		CstiString PublicIdString;
	};

	CstiString m_Name;
	CstiString m_CompanyName;
	CstiItemId m_ItemId;
	CstiString m_ItemIdString;
	CstiItemId m_ItemPublicId;
	CstiString m_ItemPublicIdString;

#ifdef stiCONTACT_PHOTOS
	CstiString m_ImageId;
	CstiString m_ImageTimestamp;
	bool m_ImageOverridden{false};
#endif

	std::vector<CstiContactNumber> m_NumberList;

	int m_nRingTone{0};
    int m_nRingColor{-1};

	CstiString m_RelayLanguage;

}; // end class CstiContactListItem

using CstiContactListItemSharedPtr = std::shared_ptr<CstiContactListItem>;

#endif // CSTI_CONTACT_LIST_ITEM_H

// end file CstiContactListItem.h

