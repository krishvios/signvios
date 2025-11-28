// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "XMLValue.h"
#include "XMLManager.h"
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <cctype>

using namespace WillowPM;


int XMLValue::IntGet(int64_t &intVal) const
{
	int errCode = XM_RESULT_SUCCESS;

	if (m_value)
	{
		int64_t int64Value;
		// Use this instead of lexical_cast to avoid throwing exception
		// Previously atoi was used, but that doesn't support 64bit integers...
		if (boost::conversion::try_lexical_convert(m_value.value (), int64Value))
		{
			intVal = int64Value;
		}
		else
		{
			// Assert to be able to track how often this happens
			stiASSERTMSG (false, "Unable to convert XMLValue to integer: %s", m_value.value ().c_str ());

			// Set to zero for now to mimic some old atoi behavior
			// TODO: Consider returning XM_RESULT_TYPE_MISMATCH
			intVal = 0;
		}
	}
	else
	{
		errCode = XM_RESULT_NOT_FOUND;
	}

	return errCode;
}


int XMLValue::StringGet(std::string &stringVal) const
{
	int errCode = XM_RESULT_SUCCESS;

	if (m_value)
	{
		stringVal = m_value.value ();
	}
	else
	{
		errCode = XM_RESULT_NOT_FOUND;
	}

	return errCode;
}


void XMLValue::IntSet(int64_t intVal)
{
	m_value = std::to_string(intVal);
}


void XMLValue::StringSet(const std::string &stringVal)
{
	m_value = stringVal;
}


int XMLValue::ValueCompare(const std::string &val, bool &match) const
{
	int errCode = XM_RESULT_SUCCESS;

	if (m_value)
	{
		match = m_value.value () == val;
	}
	else
	{
		errCode = XM_RESULT_NOT_FOUND;
	}

	return errCode;
}

int XMLValue::ValueCompareCaseInsensitive(const std::string &val, bool &match) const
{
	int errCode = XM_RESULT_SUCCESS;

	if (m_value)
	{
		match = std::equal (m_value.value ().begin (), m_value.value ().end (),
							val.begin (),
							[](char c1, char c2) { return std::tolower(c1) == std::tolower(c2);});
	}
	else
	{
		errCode = XM_RESULT_NOT_FOUND;
	}

	return errCode;
}

int XMLValue::ValueCompare(const XMLValue &val, bool &match) const
{
	int errCode = XM_RESULT_SUCCESS;

	if (m_value)
	{
		if (val.m_value)
		{
			match = m_value.value () == val.m_value.value ();
		}
		else
		{
			match = false;
		}
	}
	else
	{
		errCode = XM_RESULT_NOT_FOUND;
	}

	return errCode;
}

