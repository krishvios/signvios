// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "XMLListItem.h"
#include "PropertyListItem.h"
#include <string>


using namespace WillowPM;



PropertyListItem::PropertyListItem()
{
	SetNumValues(eLOC_MAX);
}


std::string PropertyListItem::NameGet() const
{
	std::string name;

	XMLListItem::StringGet(name, eLOC_NAME);

	return name;
}

void PropertyListItem::NameGet(std::string &Name) const
{
	XMLListItem::StringGet(Name, eLOC_NAME);
}

std::string PropertyListItem::TypeGet() const
{
	std::string type;

	XMLListItem::StringGet(type, eLOC_TYPE);

	return type;
}


void PropertyListItem::TypeGet(std::string &Type) const
{
	XMLListItem::StringGet(Type, eLOC_TYPE);
}


int PropertyListItem::StringGet(std::string &val, int Loc) const
{
	int errCode = XM_RESULT_SUCCESS;

	if ((Loc == eLOC_TEMPORARY) && !m_bHasTemp)
	{
		errCode = m_Values[eLOC_PERSISTENT].StringGet(val);
	}
	else
	{
		errCode = m_Values[Loc].StringGet(val);
	}

	if (Loc == eLOC_PERSISTENT)
	{
		m_bHasTemp = false;
	}

	// If the value was not found then use the default.
	if (errCode == XM_RESULT_NOT_FOUND)
	{
		val = m_defaultValue;
	}

	return errCode;
}


int PropertyListItem::IntGet(int64_t &val, int Loc) const
{
	int errCode = XM_RESULT_SUCCESS;

	if ((Loc == eLOC_TEMPORARY) && !m_bHasTemp)
	{
		errCode = m_Values[eLOC_PERSISTENT].IntGet(val);
	}
	else
	{
		errCode = m_Values[Loc].IntGet(val);
	}

	if (Loc == eLOC_PERSISTENT)
	{
		m_bHasTemp = false;
	}

	//
	// If the value was not found then use the default.
	if (errCode == XM_RESULT_NOT_FOUND)
	{
		if (m_defaultValue.empty ())
		{
			val = 0;
		}
		else
		{
			try
			{
				val = std::stoll(m_defaultValue);
			}
			catch (...)
			{
				val = 0;
			}
		}
	}

	return errCode;
}


void PropertyListItem::ValueSet(const std::string &val, int Loc)
{
	XMLListItem::StringSet(val, Loc);
	m_bHasTemp = (Loc == eLOC_TEMPORARY);
}

void PropertyListItem::StringSet(const std::string &val, int Loc)
{
	XMLListItem::StringSet(val, Loc);
	TypeSet(XT_STRING);
	m_bHasTemp = (Loc == eLOC_TEMPORARY);
}

void PropertyListItem::IntSet(int64_t Val, int Loc)
{
	XMLListItem::IntSet(Val, Loc);
	TypeSet(XT_INT);
	m_bHasTemp = (Loc == eLOC_TEMPORARY);
}

bool PropertyListItem::NameMatch(const std::string &name)
{
	bool match = false;
	m_Values[eLOC_NAME].ValueCompare(name, match);
	return match;
}

bool PropertyListItem::DuplicateCheck(const XMLListItemSharedPtr &item)
{
	bool match = false;
	ValueCompare(item, eLOC_NAME, match);
	return match;
}


void PropertyListItem::Write(FILE *pFile)
{
	auto name = NameGet();
	if (!name.empty ())
	{
		auto type = TypeGet();
		if (!type.empty ())
		{
			std::string value;
			auto result = m_Values[eLOC_PERSISTENT].StringGet(value);

			if (result != XM_RESULT_NOT_FOUND)
			{
				std::string escapedString;

				EscapeString (value, escapedString);

				fprintf(pFile, "<%s type=\"%s\">%s</%s>\n", name.c_str (), type.c_str (), escapedString.c_str (), name.c_str ());
			}
		}
	}
}


