// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "XMLListItem.h"
#include <map>

using namespace WillowPM;


namespace WillowPM
{

void XMLListItem::SetNumValues(int Num)
{
	m_Values.clear ();
	m_Values.resize (Num);
}

int XMLListItem::ValueCompare(const std::string &val, int Loc, bool &match)
{
	return m_Values[Loc].ValueCompare(val, match);
}

int XMLListItem::ValueCompareCaseInsensitive(const std::string &val, int Loc, bool &match)
{
	return m_Values[Loc].ValueCompareCaseInsensitive(val, match);
}

int XMLListItem::ValueCompare(const XMLListItemSharedPtr &item, int Loc, bool &match)
{
	return m_Values[Loc].ValueCompare(item->m_Values[Loc], match);
}

///\brief This method appropriately escapes the input string using xml escape notation.
///
void EscapeString(
	const std::string &input,
	std::string &output)
{
	output = input;
	static const std::map<char, std::string> escapes = {{'<', "&lt;"}, {'>', "&gt;"}, {'&', "&amp;"}, {'\'', "&apos;"}, {'\"', "&quot;"}};
	size_t pos = 0;

	for (;;)
	{
		pos = output.find_first_of ("<>&'\"", pos);

		if (pos == std::string::npos)
		{
			break;
		}

		auto escape = escapes.find(output[pos]);

		if (escape != escapes.end ())
		{
			output.replace(pos, 1, escape->second);

			pos += escape->second.length();
		}
		else
		{
			stiASSERTMSG (false, "No escape found for %c", output[pos]);
			++pos;
		}
	}
}

}

void XMLListItem::WriteField (
	FILE *pFile,
	const std::string &fieldValue,
	const std::string &fieldName)
{
	std::string escapedString;
	WillowPM::EscapeString (fieldValue, escapedString);
	fprintf (pFile, "<%s>%s</%s>\n", fieldName.c_str (), escapedString.c_str (), fieldName.c_str ());
}
