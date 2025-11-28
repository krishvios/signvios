/*!
 * \file CstiRegEx.cpp
 * \brief The CstiRegEx is a wrapper class for regex to support older compilers
 *
 * It provides RegEx functionality to the engine
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2016 Sorenson Communications, Inc. -- All rights reserved
 */
#include "CstiRegEx.h"

CstiRegEx::CstiRegEx (const std::string &pattern)
{
#ifdef stiUSE_STD_REGEX
	m_RegularExpression = new std::regex (pattern.c_str (), std::regex::extended);
#else
	regcomp (&m_RegularExpression, pattern.c_str (), REG_EXTENDED);
#endif
}

CstiRegEx::~CstiRegEx ()
{
#ifdef stiUSE_STD_REGEX
	delete m_RegularExpression;
#else
	regfree (&m_RegularExpression);
#endif
}

bool CstiRegEx::Match (std::string Input)
{
#ifdef stiUSE_STD_REGEX
	return std::regex_search (Input, (*m_RegularExpression));
#else
	return Match (Input.c_str ());
#endif
}

bool CstiRegEx::Match (const char* pInput)
{
#ifdef stiUSE_STD_REGEX
	std::string Input (pInput);
	return Match (Input);
#else
	return (regexec (&m_RegularExpression, pInput, 0, nullptr, 0) == 0);
#endif
}

bool CstiRegEx::Match (std::string Input, std::vector<std::string>& results)
{
#ifdef stiUSE_STD_REGEX
	std::smatch matches;
	if (std::regex_search (Input, matches, (*m_RegularExpression)))
	{
		for (auto iter = matches.cbegin (); iter != matches.cend (); iter++)
		{
			std::string string ((*iter).first, (*iter).second);
			results.push_back (string);
		}
		return true;
	}
#else
	const int MAX_MATCHES = 10;
	regmatch_t matches[MAX_MATCHES];
	if (regexec (&m_RegularExpression, Input.c_str (), MAX_MATCHES, matches, 0) == 0)
	{
		for (const auto &match: matches)
		{
			if (match.rm_so >= 0 && match.rm_eo >= 0)
			{
				results.push_back (Input.substr (match.rm_so, match.rm_eo - match.rm_so));
			}
			else
			{
				results.emplace_back ();
			}
		}
		return true;
	}
#endif
	return false;
}
