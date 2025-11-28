// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef XMLVALUE_H
#define XMLVALUE_H


#include "OptString.h"
#include <string>


namespace WillowPM
{

class XMLValue
{
public:
	XMLValue() = default;
	~XMLValue() = default;
	XMLValue (const XMLValue &other) = default;
	XMLValue &operator= (const XMLValue &other) = default;

	int StringGet(std::string &stringVal) const;
	int IntGet(int64_t &intVal) const;

	void StringSet(const std::string &stringVal);
	void IntSet(int64_t intVal);

	void valueRemove ()
	{
		m_value = nullptr;
	}

	int ValueCompare(const std::string &val, bool &match) const;
	int ValueCompareCaseInsensitive(const std::string &val, bool &match) const;
	int ValueCompare(const XMLValue &val, bool &match) const;

private:

	OptString m_value;
};

}


#endif
