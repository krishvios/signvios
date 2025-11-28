// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include <cstring>
#include "CstiBlockListItem.h"


using namespace WillowPM;


namespace WillowPM
{

const char g_szIdTag[] = "Id";
const char g_szDescriptionTag[] = "Description";
const char g_szDialStringTag[] = "DialString";
const char g_szBlockedItemTag[] = "BlockedItem";


CstiBlockListItem::CstiBlockListItem ()
{
	SetNumValues (eFIELD_MAX);
}

std::string CstiBlockListItem::DescriptionGet () const
{
	std::string name;

	StringGet (name, eDESCRIPTION);

	return name;
}


std::string CstiBlockListItem::PhoneNumberGet () const
{
	std::string number;

	StringGet (number, eDIAL_STRING);

	return number;
}


void CstiBlockListItem::DescriptionSet (const std::string &name)
{
	StringSet (name, eDESCRIPTION);
}


void CstiBlockListItem::PhoneNumberSet (const std::string &number)
{
	StringSet (number, eDIAL_STRING);
}


void CstiBlockListItem::IDSet (const std::string &id)
{
	m_contactID = id;
}


void CstiBlockListItem::WriteField (
	FILE *pFile,
	int Field,
	const std::string &fieldName)
{
	std::string value;

	m_Values[Field].StringGet (value);
	if (!value.empty ())
	{
		XMLListItem::WriteField(pFile, value, fieldName);
	}
}


void CstiBlockListItem::Write (FILE *pFile)
{
	fprintf (pFile, "<%s>\n", g_szBlockedItemTag);

	fprintf (pFile, "  <%s>%s</%s>\n", g_szIdTag, m_contactID.c_str (), g_szIdTag);
	WriteField (pFile, eDESCRIPTION, g_szDescriptionTag);
	WriteField (pFile, eDIAL_STRING, g_szDialStringTag);

	fprintf (pFile, "</%s>\n", g_szBlockedItemTag);
}

}; // namespace WillowPM
