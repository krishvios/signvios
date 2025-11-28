// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef XMLLIST_H
#define XMLLIST_H

#include <vector>
#include <algorithm>
#include <cstring>
#include "XMLListItem.h"

namespace WillowPM
{

/*!
 * \brief List of items elements.
 *
 * This is essentially a wrapper class around the generic std::vector that
 * hides the STL-ness and adds a max size to the list.
 */
class XMLList
{
public:
	XMLList(unsigned short nMaxCount);

	~XMLList();

	XMLListItemSharedPtr ItemGet(int Index) { return m_List[Index]; }

	void ListFree();

	stiHResult ItemAdd(const XMLListItemSharedPtr &pListItem);
	stiHResult itemInsert(const XMLListItemSharedPtr &pListItem, unsigned int nIndex);
	void ItemDestroy(int nIndex);
	int ItemFind(const std::string &name);
	void DuplicatesCheck();

	size_t MaxCountGet() { return m_nMaxCount; }
	void MaxCountSet(size_t nMaxCount) {m_nMaxCount = nMaxCount;}
	size_t CountGet() { return m_List.size(); }

	void each (const std::function<void (XMLListItemSharedPtr)> &callback);

protected:

	std::vector<XMLListItemSharedPtr> m_List;
	size_t m_nMaxCount;                 //!< Maximum allowed number of list entries.

private:

};

}

#endif

