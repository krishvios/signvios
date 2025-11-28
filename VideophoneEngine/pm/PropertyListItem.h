// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef PROPERTYLISTITEM_H
#define PROPERTYLISTITEM_H


#include "XMLListItem.h"
#include "XMLManager.h"
#include "XMLValue.h"

namespace WillowPM
{

class PropertyListItem : public XMLListItem
{
public:
	PropertyListItem();
	~PropertyListItem() override = default;

	enum EStorageLocation
	{
		eLOC_PERSISTENT = 0,
		eLOC_TEMPORARY,
		eLOC_NAME,
		eLOC_TYPE,

		eLOC_MAX
	};

	std::string NameGet() const override;
	void NameGet(std::string &Name) const;
	std::string TypeGet() const;
	void TypeGet(std::string &Type) const;
	int StringGet(std::string &Val, int Loc) const override;

	int IntGet(int64_t &ntVal, int Loc) const override;

	void NameSet(const std::string &name) override { XMLListItem::StringSet(name, eLOC_NAME); }
	void TypeSet(const std::string &type) { XMLListItem::StringSet(type, eLOC_TYPE); }
	void ValueSet(const std::string &val, int Loc);
	void StringSet(const std::string &stringVal, int Loc) override;
	void IntSet(int64_t IntVal, int Loc) override;

	bool NameMatch(const std::string &name) override;
	bool DuplicateCheck(const XMLListItemSharedPtr &item) override;

	void RemoveTemp() { m_bHasTemp = false; }
	void RemoveValues ()
	{
		m_Values[eLOC_PERSISTENT].valueRemove ();
		m_Values[eLOC_TEMPORARY].valueRemove ();
		m_bHasTemp = false;
	}

	void Write(FILE *pFile) override;

	void defaultSet (int64_t defaultValue)
	{
		m_defaultValue = std::to_string (defaultValue);
	}

	void defaultSet (const std::string &defaultValue)
	{
		m_defaultValue = defaultValue;
	}

private:

	mutable bool m_bHasTemp{false};

	std::string m_defaultValue;
};

using PropertyListItemSharedPtr = std::shared_ptr<PropertyListItem>;

}






#endif
