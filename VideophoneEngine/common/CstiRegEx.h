/*!
 * \file CstiRegEx.h
 * \brief See CstiRegEx.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2016 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef CstiRegEx_h
#define CstiRegEx_h

// NOTE: std::regex is broken in gcc until 4.9: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53631
#if APPLICATION != APP_NTOUCH_VP2  && APPLICATION != APP_NTOUCH_VP3 && \
	APPLICATION != APP_NTOUCH_VP4 && APPLICATION != APP_REGISTRAR_LOAD_TEST_TOOL// ntouch VP does not support C++11 compiler
#define stiUSE_STD_REGEX
#endif
#ifdef stiUSE_STD_REGEX
#include <regex>
#else
#include "regex.h"
#endif

#include <string>
#include <vector>

class CstiRegEx
{
public:
	CstiRegEx (const std::string &pattern);
	~CstiRegEx ();

	CstiRegEx (const CstiRegEx &other) = delete;
	CstiRegEx (CstiRegEx &&other) = delete;
	CstiRegEx &operator = (const CstiRegEx &other) = delete;
	CstiRegEx &operator = (CstiRegEx &&other) = delete;

	bool Match (std::string Input);
	bool Match (const char * pInput);
	bool Match (std::string Input, std::vector<std::string>& results);
private:
#ifdef stiUSE_STD_REGEX
	std::regex* m_RegularExpression;
#else
	regex_t m_RegularExpression{};
#endif
};

#endif
