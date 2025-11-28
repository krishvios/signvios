// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef BLOCKLISTITEM_H
#define BLOCKLISTITEM_H


#include "XMLListItem.h"


namespace WillowPM
{

extern const char g_szIdTag[];
extern const char g_szDescriptionTag[];
extern const char g_szDialStringTag[];

class CstiBlockListItem : public XMLListItem
{
public:
	CstiBlockListItem ();
	~CstiBlockListItem () override = default;

	CstiBlockListItem (const CstiBlockListItem &) = delete;
	CstiBlockListItem (CstiBlockListItem &&other) = delete;
	CstiBlockListItem &operator= (CstiBlockListItem &) = delete;
	CstiBlockListItem &operator= (CstiBlockListItem &&other) = delete;

	enum EContactField
	{
		eDESCRIPTION = 0,
		eDIAL_STRING,

		eFIELD_MAX
	};

	std::string DescriptionGet () const;
	std::string PhoneNumberGet () const;

	void DescriptionSet (const std::string &name);
	void PhoneNumberSet (const std::string &number);

	void Write (FILE *pFile) override;

	void IDSet (const std::string &id);
	std::string IDGet () const { return m_contactID; }

	std::string NameGet() const override { return DescriptionGet (); }
	void NameSet(const std::string &name) override { DescriptionSet (name); }
	bool NameMatch(const std::string &/*name*/) override { return false; }

private:

	void WriteField (FILE *pFile, int Field, const std::string &fieldName);

	std::string m_contactID;

};

using CstiBlockListItemSharedPtr = std::shared_ptr<CstiBlockListItem>;

} // end namespace

#endif // BLOCKLISTITEM_H
