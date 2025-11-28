// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef CONTACTLISTITEM_H
#define CONTACTLISTITEM_H

#include "ixml.h"
#include "XMLListItem.h"
#include "CstiContactListItem.h"
#include "IstiContact.h"


class ContactListItem : public WillowPM::XMLListItem, public IstiContact
{
public:
	ContactListItem();
	ContactListItem(IXML_Node *pNode);
	ContactListItem(const CstiContactListItemSharedPtr &cstiContactListItem);
	ContactListItem(const IstiContactSharedPtr &other);
	~ContactListItem() override = default;


#ifdef stiCONTACT_LIST_MULTI_PHONE
	enum ENumberFieldType
	{
		eNUMBERFIELD_ID = 0,
		eNUMBERFIELD_PUBLIC_ID,
		eNUMBERFIELD_DIALSTRING,

		eNUMBERFIELD_MAX
	};
#endif

	enum EContactField
	{
		eCONTACT_NAME = 0,
		eCONTACT_COMPANY,
		eCONTACT_PUBLIC_ID,
		eCONTACT_LIGHTRING,
		eCONTACT_LIGHTRING_COLOR,
		eCONTACT_LANGUAGE,
		eCONTACT_VCO,
		eCONTACT_PHOTO,
		eCONTACT_PHOTO_TIMESTAMP,
		eCONTACT_PHOTO_NUMBER_TYPE,

#ifndef stiCONTACT_LIST_MULTI_PHONE
		eCONTACT_NUMBER_HOME,
		eCONTACT_NUMBER_CELL,
		eCONTACT_NUMBER_WORK,
#endif

		eCONTACT_MAX
	};

	std::string PublicIDGet() const override;
	CstiItemId PublicIdGet() const override;
	std::string NameGet() const override;
	std::string CompanyNameGet() const override;

#ifdef stiCONTACT_LIST_MULTI_PHONE
	int PhoneNumberIDGet(CstiContactListItem::EPhoneNumberType type, std::string  *id) const override;
	CstiItemId PhoneNumberIdGet(CstiContactListItem::EPhoneNumberType type) const override;
	virtual int PhoneNumberPublicIDGet(CstiContactListItem::EPhoneNumberType type, std::string *publicID) const;
	CstiItemId PhoneNumberPublicIdGet(CstiContactListItem::EPhoneNumberType type) const override;
	int DialStringGet(CstiContactListItem::EPhoneNumberType type, std::string *dialString) const override;
	int DialStringGetById(const CstiItemId &NumberId, std::string *pDialString) override;
	int PhoneNumberTypeGet(const std::string &dialString, CstiContactListItem::EPhoneNumberType *pType) override;
#else
	virtual int HomeNumberGet(std::string *number) const;
	virtual int CellNumberGet(std::string *number) const;
	virtual int WorkNumberGet(std::string *number) const;
#endif

	int RingPatternGet(int *pPattern) const override;
	int RingPatternColorGet(int *pnColor) const override;
	int LanguageGet(std::string *language) const override;
	int VCOGet(bool *pbVCO) const override;
	int PhotoGet(std::string *photoID) const override;
	int PhotoTimestampGet(std::string *photoTimestamp) const override;

	virtual void PublicIDSet(const std::string &publicID);
	virtual void PublicIDSet(const CstiItemId &PublicID);
	void NameSet(const std::string &name) override;
	void CompanyNameSet(const std::string &company) override;

#ifdef stiCONTACT_LIST_MULTI_PHONE
	bool DialStringMatch(const std::string &dialString) override;
	CstiContactListItem::EPhoneNumberType DialStringFind(const std::string &dialString) override;
	CstiContactListItem::EPhoneNumberType PhoneNumberPublicIdFind(CstiItemId &PublicId) override;
	void PhoneNumberPublicIdSet(CstiContactListItem::EPhoneNumberType type, const CstiItemId &PublicId) override;
	void ClearPhoneNumbers() override;
	void PhoneNumberAdd(CstiContactListItem::EPhoneNumberType type, const std::string &dialString) override;
	void PhoneNumberAdd(CstiContactListItem::EPhoneNumberType type, const CstiItemId &ID, const CstiItemId &PublicID, const std::string &dialString);
	void PhoneNumberSet(CstiContactListItem::EPhoneNumberType type, const std::string &dialString) override;
#else
	virtual void HomeNumberSet(const std::string &number);
	virtual void CellNumberSet(const std::string &number);
	virtual void WorkNumberSet(const std::string &number);
#endif

	void RingPatternSet(int Pattern) override;
	void RingPatternColorSet(int nColor) override;
	void LanguageSet(const std::string &language) override;
	void VCOSet(bool bVCO) override;
	void PhotoSet(const std::string &photoID) override;
	void PhotoTimestampSet(const std::string &photoTimestamp) override;
	bool NameMatch(const std::string &name) override;
	void Write(FILE *pFile) override;
	void HomeFavoriteOnSaveSet(bool homeFavoriteOnSave) override;
	virtual bool HomeFavoriteOnSaveGet() {return m_homeFavorite;}
	void WorkFavoriteOnSaveSet(bool workFavoriteOnSave) override;
	virtual bool WorkFavoriteOnSaveGet() {return m_workFavorite;}
	void MobileFavoriteOnSaveSet(bool mobileFavoriteOnSave) override;
	virtual bool MobileFavoriteOnSaveGet() {return m_mobileFavorite;}

	void IDSet(const std::string &id) override;
	void ItemIdSet(const CstiItemId &Id) override;
	std::string IDGet() const override { return m_contactIDStr; }
	CstiItemId ItemIdGet() const override { return m_contactId; }

	void CopyCstiContactListItem(const CstiContactListItem *cstiContactListItem);
	CstiContactListItemSharedPtr GetCstiContactListItem() override;

protected:
	void Init();
	void PopulateWithIXML_Node(IXML_Node *pNode);
	void WriteField(FILE *pFile, int Field, const std::string &fieldName);

	std::string m_contactIDStr;
	CstiItemId m_contactId;

	bool m_homeFavorite = false;
	bool m_workFavorite = false;
	bool m_mobileFavorite = false;
};

using ContactListItemSharedPtr = std::shared_ptr<ContactListItem>;

#endif
