// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef XMLLISTITEM_H
#define XMLLISTITEM_H


#include <cstdio>
#include "XMLValue.h"
#include <string>
#include "stiSVX.h"
#include <memory>
#include <vector>


namespace WillowPM
{

class XMLListItem;
using XMLListItemSharedPtr = std::shared_ptr<XMLListItem>;

void EscapeString(
	const std::string &input,
	std::string &output);


class XMLListItem
{
public:
	XMLListItem() = default;
	virtual ~XMLListItem() = default;

	virtual void Write(FILE *pFile) = 0;

	void SetNumValues(int Num);

	virtual std::string NameGet() const = 0;
	virtual int StringGet(std::string &stringVal, int Loc) const { return m_Values[Loc].StringGet(stringVal); }
	virtual int IntGet(int64_t &intVal, int Loc) const { return m_Values[Loc].IntGet(intVal); }

	virtual void NameSet(const std::string &name) = 0;
	virtual void StringSet(const std::string &stringVal, int Loc) { m_Values[Loc].StringSet(stringVal); }
	virtual void IntSet(int64_t intVal, int Loc) { m_Values[Loc].IntSet(intVal); }

	virtual bool NameMatch(const std::string &name) = 0;
	virtual bool DuplicateCheck(const XMLListItemSharedPtr &) { return false; }

	int ValueCompare(const std::string &val, int Loc, bool &match);
	int ValueCompareCaseInsensitive(const std::string &val, int Loc, bool &match);
	int ValueCompare(const XMLListItemSharedPtr &pItem, int Loc, bool &match);

protected:

	void WriteField (
		FILE *pFile,
		const std::string &fieldValue,
		const std::string &fieldName);

	std::vector<XMLValue> m_Values;
};

}


#endif
