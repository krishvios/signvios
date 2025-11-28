/*!
* \file CstiString.h
* \brief See CstiString.cpp
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once

#include <string>


class CstiString
{
public:
	CstiString () = default;
	CstiString (const CstiString &) = default;
	CstiString (const std::string &value);

	CstiString &operator= (const CstiString &value);
	CstiString &operator= (const char *value);
	CstiString &operator= (const std::string &value);

	bool IsNullOrEmpty () const;
	bool isNull () const;
	bool isEmpty () const;
	int length () const;
	std::string toStdString () const;

	operator const char *() const
	{
		return (m_hasValue ? m_value.c_str() : nullptr);
	}

private:
	std::string m_value;
	bool m_hasValue = false;
};
