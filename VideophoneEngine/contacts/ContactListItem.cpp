// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include <cstring>
#include "stiGUID.h"
#include "XMLManager.h"
#include "ContactListItem.h"
#include "IstiLightRing.h"

using namespace WillowPM;

ContactListItem::ContactListItem()
{
	Init();
}

ContactListItem::ContactListItem(IXML_Node *pNode)
{
	Init();
	PopulateWithIXML_Node(pNode);
}

ContactListItem::ContactListItem(const CstiContactListItemSharedPtr &cstiContactListItem)
{
	Init();
	CopyCstiContactListItem(cstiContactListItem.get ());
}

ContactListItem::ContactListItem(const IstiContactSharedPtr &other)
{
	Init();

	NameSet(other->NameGet());
	CompanyNameSet(other->CompanyNameGet());

	ItemIdSet(other->ItemIdGet());
	PublicIDSet(other->PublicIdGet());

	int pattern = 0;
	other->RingPatternGet(&pattern);
	RingPatternSet(pattern);

	int color = -1;
	other->RingPatternColorGet(&color);
	RingPatternColorSet(color);

	bool vco = false;
	other->VCOGet(&vco);
	VCOSet(vco);

	std::string value;
	if(XM_RESULT_SUCCESS == other->LanguageGet(&value))
	{
		LanguageSet(value);
	}

#ifdef stiCONTACT_PHOTOS
	if(XM_RESULT_SUCCESS == other->PhotoGet(&value))
	{
		PhotoSet(value);
	}

	if(XM_RESULT_SUCCESS == other->PhotoTimestampGet(&value))
	{
		PhotoTimestampSet(value);
	}
#endif

#ifdef stiCONTACT_LIST_MULTI_PHONE

	for (int i = 0; i < CstiContactListItem::ePHONE_MAX; i++)
	{
		auto numberType = static_cast<CstiContactListItem::EPhoneNumberType>(i);

		if (XM_RESULT_SUCCESS == other->DialStringGet(numberType, &value))
		{
			PhoneNumberAdd(
				numberType,
				other->PhoneNumberIdGet(numberType),
				other->PhoneNumberPublicIdGet(numberType),
				value);
		}
	}

#else

	other->HomeNumberGet(&value);
	HomeNumberSet(value);

	other->WorkNumberGet(&value);
	WorkNumberSet(value);

	other->CellNumberGet(&value);
	CellNumberSet(value);

#endif
}


std::string ContactListItem::PublicIDGet() const
{
	std::string publicID;

	StringGet(publicID, eCONTACT_PUBLIC_ID);

	return publicID;
}


CstiItemId ContactListItem::PublicIdGet() const
{
	std::string publicID;

	StringGet(publicID, eCONTACT_PUBLIC_ID);

	return {publicID.c_str ()};
}


std::string ContactListItem::NameGet() const
{
	std::string name;

	StringGet(name, eCONTACT_NAME);

	return name;
}


std::string ContactListItem::CompanyNameGet() const
{
	std::string company;

	StringGet(company, eCONTACT_COMPANY);

	return company;
}


#ifdef stiCONTACT_LIST_MULTI_PHONE
bool ContactListItem::DialStringMatch(const std::string &dialString)
{
	return (DialStringFind(dialString) != CstiContactListItem::ePHONE_MAX);
}

CstiContactListItem::EPhoneNumberType ContactListItem::DialStringFind(const std::string &dialString)
{
	int numberType = CstiContactListItem::ePHONE_MAX;

	if (!dialString.empty ())
	{
		for (numberType = 0; numberType < CstiContactListItem::ePHONE_MAX; numberType++)
		{
			std::string dialStringTemp;
			if (XM_RESULT_SUCCESS == DialStringGet((CstiContactListItem::EPhoneNumberType)numberType, &dialStringTemp))
			{
				if (dialString == dialStringTemp)
				{
					break;
				}
			}
		}
	}

	return (CstiContactListItem::EPhoneNumberType)numberType;
}

CstiContactListItem::EPhoneNumberType ContactListItem::PhoneNumberPublicIdFind(CstiItemId &PublicId)
{
	int nNumberType;

	for (nNumberType = 0; nNumberType < CstiContactListItem::ePHONE_MAX; nNumberType++)
	{
		CstiItemId NumberId = PhoneNumberPublicIdGet((CstiContactListItem::EPhoneNumberType)nNumberType);
		if (NumberId.IsValid())
		{
			if (PublicId == NumberId)
			{
				break;
			}
		}
	}

	return (CstiContactListItem::EPhoneNumberType)nNumberType;
}

void ContactListItem::PhoneNumberPublicIdSet(CstiContactListItem::EPhoneNumberType type, const CstiItemId &PublicId)
{
	int loc = eCONTACT_MAX + type * eNUMBERFIELD_MAX + eNUMBERFIELD_PUBLIC_ID;
	std::string Id;
	PublicId.ItemIdGet(&Id);
	StringSet(Id, loc);
}

void ContactListItem::ClearPhoneNumbers()
{
	for (size_t loc = eCONTACT_MAX; loc < this->m_Values.size (); loc++)
	{
		m_Values[loc].valueRemove ();
	}
}

int ContactListItem::PhoneNumberIDGet(CstiContactListItem::EPhoneNumberType type, std::string *id) const
{
	int loc = eCONTACT_MAX + type * eNUMBERFIELD_MAX + eNUMBERFIELD_ID;
	return StringGet(*id, loc);
}

CstiItemId ContactListItem::PhoneNumberIdGet(CstiContactListItem::EPhoneNumberType type) const
{
	int loc = eCONTACT_MAX + type * eNUMBERFIELD_MAX + eNUMBERFIELD_ID;

	std::string tempId;

	StringGet(tempId, loc);

	return {tempId.c_str ()};
}

int ContactListItem::PhoneNumberPublicIDGet(CstiContactListItem::EPhoneNumberType type, std::string *publicID) const
{
	int loc = eCONTACT_MAX + type * eNUMBERFIELD_MAX + eNUMBERFIELD_PUBLIC_ID;
	return StringGet(*publicID, loc);
}

CstiItemId ContactListItem::PhoneNumberPublicIdGet(CstiContactListItem::EPhoneNumberType type) const
{
	int loc = eCONTACT_MAX + type * eNUMBERFIELD_MAX + eNUMBERFIELD_PUBLIC_ID;

	std::string tempId;

	StringGet(tempId, loc);

	return {tempId.c_str ()};
}

int ContactListItem::DialStringGet(CstiContactListItem::EPhoneNumberType type, std::string *dialString) const
{
	int loc = eCONTACT_MAX + type * eNUMBERFIELD_MAX + eNUMBERFIELD_DIALSTRING;
	return StringGet(*dialString, loc);
}


int ContactListItem::DialStringGetById(const CstiItemId &NumberId, std::string *dialString)
{
	for (int i = 0; i < CstiContactListItem::ePHONE_MAX; i++)
	{
		int loc = eCONTACT_MAX + i * eNUMBERFIELD_MAX + eNUMBERFIELD_PUBLIC_ID;
		bool bMatch = false;
		std::string Id;
		NumberId.ItemIdGet(&Id);
		ValueCompareCaseInsensitive(Id, loc, bMatch);

		if (bMatch)
		{
			return DialStringGet((CstiContactListItem::EPhoneNumberType)i, dialString);
		}
	}

	return 0;

}

int ContactListItem::PhoneNumberTypeGet(const std::string &dialString, CstiContactListItem::EPhoneNumberType *pType)
{
	for (int i = 0; i < CstiContactListItem::ePHONE_MAX; i++)
	{
		int loc = eCONTACT_MAX + i * eNUMBERFIELD_MAX + eNUMBERFIELD_DIALSTRING;
		bool bMatch = false;
		ValueCompare(dialString, loc, bMatch);

		if (bMatch)
		{
			*pType = (CstiContactListItem::EPhoneNumberType)i;
			break;
		}
	}

	return 0;
}

#else
int ContactListItem::HomeNumberGet(char **pNumber) const
{
	return StringGet(pNumber, eCONTACT_NUMBER_HOME);
}

int ContactListItem::CellNumberGet(char **pNumber) const
{
	return StringGet(pNumber, eCONTACT_NUMBER_CELL);
}

int ContactListItem::WorkNumberGet(char **pNumber) const
{
	return StringGet(pNumber, eCONTACT_NUMBER_WORK);
}
#endif

int ContactListItem::RingPatternGet(int *pPattern) const
{
	auto pattern = static_cast<int64_t>(IstiLightRing::Pattern::Flash);
	int code = IntGet(pattern, eCONTACT_LIGHTRING);
	*pPattern = static_cast<int>(pattern);

	return code;
}

int ContactListItem::RingPatternColorGet(int *pnColor) const
{
	int64_t color = IstiLightRing::estiLED_COLOR_WHITE;
	int code = IntGet(color, eCONTACT_LIGHTRING_COLOR);
	*pnColor = static_cast<int>(color);

	return code;
}

int ContactListItem::LanguageGet(std::string *language) const
{
	return StringGet(*language, eCONTACT_LANGUAGE);
}

int ContactListItem::VCOGet(bool *pbVCO) const
{
	int64_t nVal = 0;
	int retval = IntGet(nVal, eCONTACT_VCO);
	*pbVCO = (nVal == 1);
	return retval;
}

int ContactListItem::PhotoGet(std::string *photoID) const
{
	return StringGet(*photoID, eCONTACT_PHOTO);
}

int ContactListItem::PhotoTimestampGet(std::string *photoTimestamp) const
{
	return StringGet(*photoTimestamp, eCONTACT_PHOTO_TIMESTAMP);
}

void ContactListItem::PublicIDSet(const std::string &publicID)
{
	StringSet(publicID, eCONTACT_PUBLIC_ID);
}

void ContactListItem::PublicIDSet(const CstiItemId &PublicID)
{
	std::string Id;

	if (PublicID.IsValid())
	{
		PublicID.ItemIdGet(&Id);
	}

	StringSet(Id, eCONTACT_PUBLIC_ID);
}

void ContactListItem::NameSet(const std::string &name)
{
	StringSet(name, eCONTACT_NAME);
}


void ContactListItem::CompanyNameSet (
	const std::string &company)
{
	StringSet(company, eCONTACT_COMPANY);
}

void ContactListItem::HomeFavoriteOnSaveSet(bool homeFavoriteOnSave)
{
	m_homeFavorite = homeFavoriteOnSave;
}

void ContactListItem::WorkFavoriteOnSaveSet(bool workFavoriteOnSave)
{
	m_workFavorite = workFavoriteOnSave;
}

void ContactListItem::MobileFavoriteOnSaveSet(bool mobileFavoriteOnSave)
{
	m_mobileFavorite = mobileFavoriteOnSave;
}


#ifdef stiCONTACT_LIST_MULTI_PHONE

void ContactListItem::PhoneNumberAdd(CstiContactListItem::EPhoneNumberType type, const std::string &dialString)
{
	int locBase = eCONTACT_MAX + type * eNUMBERFIELD_MAX;
	auto publicID = stiGUIDGenerate();
	
	IntSet(0, locBase + eNUMBERFIELD_ID);
	StringSet(publicID, locBase + eNUMBERFIELD_PUBLIC_ID);
	StringSet(dialString, locBase + eNUMBERFIELD_DIALSTRING);
}

void ContactListItem::PhoneNumberAdd(
	CstiContactListItem::EPhoneNumberType type,
	const CstiItemId &Id,
	const CstiItemId &PublicID,
	const std::string &dialString)
{
	int locBase = eCONTACT_MAX + type * eNUMBERFIELD_MAX;
	std::string IdString;
	Id.ItemIdGet(&IdString);
	StringSet(IdString, locBase + eNUMBERFIELD_ID);
	PublicID.ItemIdGet(&IdString);
	StringSet(IdString, locBase + eNUMBERFIELD_PUBLIC_ID);
	StringSet(dialString, locBase + eNUMBERFIELD_DIALSTRING);
}

void ContactListItem::PhoneNumberSet(CstiContactListItem::EPhoneNumberType type, const std::string &dialString)
{
	int locBase = eCONTACT_MAX + type * eNUMBERFIELD_MAX;

	std::string tempDialString;

	if (XM_RESULT_SUCCESS != DialStringGet(type, &tempDialString))
	{
		// If the phone number was already deleted, don't try to add an empty dial string
		PhoneNumberAdd(type, dialString);
	}
	else
	{
		StringSet(dialString, locBase + eNUMBERFIELD_DIALSTRING);
	}
}

#else

void ContactListItem::HomeNumberSet(const std::string &number)
{
	StringSet(number, eCONTACT_NUMBER_HOME);
}

void ContactListItem::CellNumberSet(const std::string &number)
{
	StringSet(number, eCONTACT_NUMBER_CELL);
}

void ContactListItem::WorkNumberSet(const std::string &number)
{
	StringSet(number, eCONTACT_NUMBER_WORK);
}

#endif

void ContactListItem::RingPatternSet(int Pattern)
{
	IntSet(Pattern, eCONTACT_LIGHTRING);
}

void ContactListItem::RingPatternColorSet(int nColor)
{
    IntSet(nColor, eCONTACT_LIGHTRING_COLOR);
}

void ContactListItem::LanguageSet(const std::string &language)
{
	StringSet(language, eCONTACT_LANGUAGE);
}

void ContactListItem::VCOSet(bool bVCO)
{
	IntSet(bVCO, eCONTACT_VCO);
}

void ContactListItem::PhotoSet(const std::string &photoID)
{
	StringSet(photoID, eCONTACT_PHOTO);
}

void ContactListItem::PhotoTimestampSet(const std::string &photoTimestamp)
{
	StringSet(photoTimestamp, eCONTACT_PHOTO_TIMESTAMP);
}

bool ContactListItem::NameMatch(const std::string &name)
{
	bool bMatch = false;
	m_Values[eCONTACT_NAME].ValueCompare(name, bMatch);
	return bMatch;
}

void ContactListItem::IDSet(const std::string &id)
{
	m_contactIDStr = id;
}

void ContactListItem::ItemIdSet(const CstiItemId &Id)
{
	m_contactId = Id;
	
	std::string idStr;

	if (Id.IsValid())
	{
		m_contactId.ItemIdGet(&idStr);
	}

	IDSet(idStr);
}

void ContactListItem::Init()
{
#ifdef stiCONTACT_LIST_MULTI_PHONE
	SetNumValues(eCONTACT_MAX + CstiContactListItem::ePHONE_MAX * eNUMBERFIELD_MAX); //This is necessary to allocate enough space for each phone number type and all of its values.
#else
	SetNumValues(eCONTACT_MAX);
#endif
}

void ContactListItem::WriteField(
	FILE *pFile,
	int Field,
	const std::string &fieldName)
{
	std::string value;

	m_Values[Field].StringGet(value);
	if (!value.empty ())
	{
		XMLListItem::WriteField(pFile, value, fieldName);
	}
}

void ContactListItem::Write(FILE *pFile)
{
	fprintf(pFile, "<contact>\n");

	fprintf(pFile, "  <id>%s</id>\n", m_contactIDStr.length() ? m_contactIDStr.c_str() : "");
	WriteField(pFile, eCONTACT_NAME, "name");
	WriteField(pFile, eCONTACT_COMPANY, "company");
	WriteField(pFile, eCONTACT_PUBLIC_ID, "public_id");
	WriteField(pFile, eCONTACT_LIGHTRING, "lightring");
	WriteField(pFile, eCONTACT_LANGUAGE, "relaylang");
	WriteField(pFile, eCONTACT_VCO, "vco");
	WriteField(pFile, eCONTACT_PHOTO, "photoid");
	WriteField(pFile, eCONTACT_PHOTO_TIMESTAMP, "photo_timestamp");
	WriteField(pFile, eCONTACT_PHOTO_NUMBER_TYPE, "photo_num_type");
#ifndef stiCONTACT_LIST_MULTI_PHONE
	WriteField(pFile, eCONTACT_NUMBER_HOME, "homenum");
	WriteField(pFile, eCONTACT_NUMBER_CELL, "cellnum");
	WriteField(pFile, eCONTACT_NUMBER_WORK, "worknum");
#else
	char pNumberType[20];
	char pFieldName[40];

	for (int number = 0;number<CstiContactListItem::ePHONE_MAX;number++)
	{
		std::string tempDialString;

		if (XM_RESULT_SUCCESS != DialStringGet(static_cast<CstiContactListItem::EPhoneNumberType>(number), &tempDialString))
		{
			continue;
		}
		
		switch (number)
		{
			case CstiContactListItem::ePHONE_HOME:
				sprintf(pNumberType, "homenum");
				break;
			case CstiContactListItem::ePHONE_CELL:
				sprintf(pNumberType, "cellnum");
				break;
			case CstiContactListItem::ePHONE_WORK:
				sprintf(pNumberType, "worknum");
				break;
			default:
				continue; //We don't want to process this one, it's an invalid value...
		}

		for (int numberField = 0;numberField<eNUMBERFIELD_MAX; numberField++)
		{
			switch (numberField)
			{
				case eNUMBERFIELD_ID:
					sprintf(pFieldName, "%s_id", pNumberType);
					break;
				case eNUMBERFIELD_PUBLIC_ID:
					sprintf(pFieldName, "%s_public_id", pNumberType);
					break;
				case eNUMBERFIELD_DIALSTRING:
					sprintf(pFieldName, "%s_dialstring", pNumberType);
					break;
				default:
				continue; //We don't want to process this one, it's an invalid value...
			}

			WriteField(pFile, eCONTACT_MAX + number * eNUMBERFIELD_MAX + numberField, pFieldName);
		}
	}
#endif

	fprintf(pFile, "</contact>\n");
}

void ContactListItem::PopulateWithIXML_Node(IXML_Node *pNode)
{// pNode should point to "<contact>" initially

	const char *pValue = nullptr;

	pNode = ixmlNode_getFirstChild(pNode);   // the first subtag

	while (pNode)
	{
		const char *pTagName = ixmlNode_getNodeName(pNode);
		pValue = (const char *)ixmlNode_getNodeValue(ixmlNode_getFirstChild(pNode));

		if (pValue)
		{
			if (strcmp(pTagName, "id") == 0)
			{
				CstiItemId id(pValue);
				ItemIdSet(id);
			}
			else if (strcmp(pTagName, "public_id") == 0)
			{
				StringSet(pValue, eCONTACT_PUBLIC_ID);
			}
			else if (strcmp(pTagName, "name") == 0)
			{
				StringSet(pValue, eCONTACT_NAME);
			}
			else if (strcmp(pTagName, "company") == 0)
			{
				StringSet(pValue, eCONTACT_COMPANY);
			}
			else if (strcmp(pTagName, "lightring") == 0)
			{
				StringSet(pValue, eCONTACT_LIGHTRING);
			}
			else if (strcmp(pTagName, "relaylang") == 0)
			{
				StringSet(pValue, eCONTACT_LANGUAGE);
			}
			else if (strcmp(pTagName, "vco") == 0)
			{
				StringSet(pValue, eCONTACT_VCO);
			}
			else if (strcmp(pTagName, "photoid") == 0)
			{
				StringSet(pValue, eCONTACT_PHOTO);
			}
			else if (strcmp(pTagName, "photo_timestamp") == 0)
			{
				StringSet(pValue, eCONTACT_PHOTO_TIMESTAMP);
			}
			else if (strcmp(pTagName, "photo_num_type") == 0)
			{
				StringSet(pValue, eCONTACT_PHOTO_NUMBER_TYPE);
			}
	#ifndef stiCONTACT_LIST_MULTI_PHONE
			else if (strcmp(pTagName, "homenum") == 0)
			{
				StringSet(pValue, eCONTACT_NUMBER_HOME);
			}
			else if (strcmp(pTagName, "worknum") == 0)
			{
				StringSet(pValue, eCONTACT_NUMBER_WORK);
			}
			else if (strcmp(pTagName, "cellnum") == 0)
			{
				StringSet(pValue, eCONTACT_NUMBER_CELL);
			}
	#else
			else //It must be phone number information
			{
				int number = -1;
				int numberField = -1;

				if (strstr(pTagName, "homenum"))
				{
					number = CstiContactListItem::ePHONE_HOME;
				}
				else if (strstr(pTagName, "worknum"))
				{
					number = CstiContactListItem::ePHONE_WORK;
				}
				else if (strstr(pTagName, "cellnum"))
				{
					number = CstiContactListItem::ePHONE_CELL;
				}

				if (strstr(pTagName, "_public_id"))
				{
					numberField = eNUMBERFIELD_PUBLIC_ID;
				}
				else if (strstr(pTagName, "_id"))
				{
					numberField = eNUMBERFIELD_ID;
				}
				else if (strstr(pTagName, "_dialstring"))
				{
					numberField = eNUMBERFIELD_DIALSTRING;
				}

				if (number>=0 && numberField>=0)
				{//Only set the value if it's valid
					StringSet(pValue, eCONTACT_MAX + number * eNUMBERFIELD_MAX + numberField);
				}
			}
	#endif
		}

		pNode = ixmlNode_getNextSibling(pNode);
	}
}

void ContactListItem::CopyCstiContactListItem(const CstiContactListItem *pCstiContactListItem)
{
	CstiItemId Id, PublicId;
	pCstiContactListItem->ItemIdGet(&Id);
	ItemIdSet(Id);

	NameSet(pCstiContactListItem->NameGet());
	CompanyNameSet(pCstiContactListItem->CompanyNameGet());

	pCstiContactListItem->ItemPublicIdGet(&PublicId);
	PublicIDSet(PublicId);

#ifdef stiCONTACT_PHOTOS
	PhotoSet(pCstiContactListItem->ImageIdGet());
	PhotoTimestampSet(pCstiContactListItem->ImageTimestampGet());
#endif

    int nPattern = pCstiContactListItem->RingToneGet();
    if (nPattern & 0xffffff00)
    {
        // The old method saved color in upper bits of the pattern,
        //  so strip that off it exists
        RingPatternSet(nPattern & 0xff);
        // And set the Color from the upper bits
        RingPatternColorSet(nPattern >> 8);
    }
    else
    {
        RingPatternColorSet(pCstiContactListItem->RingColorGet());
		RingPatternSet(pCstiContactListItem->RingToneGet());
    }

	VCOSet(pCstiContactListItem->DialMethodGet(0) == estiDM_UNKNOWN_WITH_VCO ||
		   pCstiContactListItem->DialMethodGet(0) == estiDM_BY_VRS_WITH_VCO);
	LanguageSet(pCstiContactListItem->RelayLanguageGet());

#ifdef stiCONTACT_LIST_MULTI_PHONE

	ClearPhoneNumbers(); //wipe out all the old phone numbers first..

	for (int i = 0; i < pCstiContactListItem->PhoneNumberCountGet(); i++)
	{
		pCstiContactListItem->PhoneNumberIdGet(i, &Id);
		pCstiContactListItem->PhoneNumberPublicIdGet(i, &PublicId);

		PhoneNumberAdd(static_cast<CstiContactListItem::EPhoneNumberType>(
			pCstiContactListItem->PhoneNumberTypeGet(i)),
			Id,
			PublicId,
			pCstiContactListItem->DialStringGet(i));
	}

#else

	HomeNumberSet(pCstiContactListItem->DialStringGet(0));

#endif
}

CstiContactListItemSharedPtr ContactListItem::GetCstiContactListItem()
{
	auto pCstiItem = std::make_shared<CstiContactListItem> ();

	auto value = NameGet();
	pCstiItem->NameSet(value.c_str ());

	value = CompanyNameGet();
	pCstiItem->CompanyNameSet(value.c_str ());

	pCstiItem->ItemIdSet(ItemIdGet());
	pCstiItem->ItemPublicIdSet(PublicIdGet());

	int Pattern = 0;
	RingPatternGet(&Pattern);
    pCstiItem->RingToneSet(Pattern);

    int nColor = -1;
    int nTmpColor;
    if (RingPatternColorGet(&nTmpColor) != XM_RESULT_NOT_FOUND)
	{
        nColor = nTmpColor;
    }
    pCstiItem->RingColorSet(nColor);

	bool bVCO = false;
	VCOGet(&bVCO);

	LanguageGet(&value);
	pCstiItem->RelayLanguageSet(value.c_str ());

#ifdef stiCONTACT_PHOTOS
	PhotoGet(&value);
	pCstiItem->ImageIdSet(value.c_str ());

	PhotoTimestampGet(&value);
	pCstiItem->ImageTimestampSet(value.c_str ());
#endif

#ifdef stiCONTACT_LIST_MULTI_PHONE
	
	for (int i = 0; i < CstiContactListItem::ePHONE_MAX; i++)
	{
		std::string dialString;
		auto  numberType = static_cast<CstiContactListItem::EPhoneNumberType>(i);

		if (XM_RESULT_SUCCESS == DialStringGet(numberType, &dialString))
		{
			pCstiItem->PhoneNumberAdd();
			unsigned int unIndex = pCstiItem->PhoneNumberCountGet() - 1;
			pCstiItem->PhoneNumberIdSet(unIndex, PhoneNumberIdGet(numberType));
			pCstiItem->PhoneNumberPublicIdSet(unIndex, PhoneNumberPublicIdGet(numberType));
			pCstiItem->DialStringSet(unIndex, dialString.c_str ());
			pCstiItem->PhoneNumberTypeSet(unIndex, numberType);

			if (bVCO)
			{
				pCstiItem->DialMethodSet(unIndex, estiDM_UNKNOWN_WITH_VCO);
			}
			else
			{
				pCstiItem->DialMethodSet(unIndex, estiDM_UNKNOWN);
			}
		}
	}

#else
	char *pszNumbers[3];
	pszNumbers[0] = NULL;
	pszNumbers[1] = NULL;
	pszNumbers[2] = NULL;

	HomeNumberGet(&pszNumbers[0]);
	WorkNumberGet(&pszNumbers[1]);
	CellNumberGet(&pszNumbers[2]);

	int count = 0;

	if (pszNumbers[0])
	{
		count++;
	}
	if (pszNumbers[1])
	{
		count++;
	}
	if (pszNumbers[2])
	{
		count++;
	}

	for (int i = 0; i < count; i++)
	{
		pCstiItem->PhoneNumberAdd();

		if (bVCO)
		{
			pCstiItem->DialMethodSet(i, estiDM_UNKNOWN_WITH_VCO);
		}
		else
		{
			pCstiItem->DialMethodSet(i, estiDM_UNKNOWN);
		}

		pCstiItem->DialStringSet(i, pszNumbers[i]);

		pCstiItem->PhoneNumberTypeSet(i, CstiContactListItem::ePHONE_HOME);

		const char *pGUID = stiGUIDGenerate();
		pCstiItem->PhoneNumberPublicIdSet(i, pGUID);
		if (pGUID)
		{
			delete [] pGUID;
			pGUID = NULL;
		}
	}

	for (int i = 0; i< 3;i++)
	{
		if (pszNumbers[i])
		{
			delete [] pszNumbers[i];
			pszNumbers[i] = NULL;
		}
	}

#endif

	return pCstiItem;
}
