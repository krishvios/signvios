// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "XMLList.h"

using namespace WillowPM;

XMLList::XMLList(unsigned short nMaxCount)
	: m_nMaxCount(nMaxCount)
{
}

XMLList::~XMLList()
{
	ListFree();
}

void XMLList::ListFree()
{
	unsigned int size = CountGet();

	for (unsigned int i = 0; i < size; i++)
	{
		ItemDestroy(0);
	}

}

/*!
 * \brief Adds an item to the end of the list.  If we exceed max, delete the the head node
 * \param pListItem The item to be added
 * \retval The index of the new list entry
 */
stiHResult XMLList::ItemAdd(const XMLListItemSharedPtr &listItem)
{
	stiHResult hResult = stiRESULT_ERROR;
	if (m_List.size() < m_nMaxCount)
	{
		m_List.push_back(listItem);
		
		hResult = stiRESULT_SUCCESS;
	}
	
	return hResult;
}

stiHResult XMLList::itemInsert(const XMLListItemSharedPtr &listItem, unsigned int nIndex)
{
	stiHResult hResult = stiRESULT_ERROR;
	if (m_List.size () < m_nMaxCount)
	{
		m_List.insert(m_List.begin () + nIndex, listItem);
		
		hResult = stiRESULT_SUCCESS;
	}
	
	return hResult;
}

void XMLList::ItemDestroy(int nIndex)
{
	m_List.erase(m_List.begin() + nIndex);
}

int XMLList::ItemFind(const std::string &name)
{
	auto iter = std::find_if(m_List.begin(), m_List.end(),
				[name](const XMLListItemSharedPtr &item) { return item->NameMatch(name); });

	if (iter != m_List.end())
	{
		return std::distance(m_List.begin(), iter);
	}

	return -1;
}

void XMLList::DuplicatesCheck()
{
	for (auto &item : m_List)
	{
		int nCount = std::count_if(m_List.begin(), m_List.end(), [item](const XMLListItemSharedPtr &otherItem) { return item->DuplicateCheck(otherItem); });

		if (nCount > 1)
		{
			stiASSERTMSG(estiFALSE, "XMLList found duplicate entry for %s\n", item->NameGet().c_str ());
		}
	}
}


void XMLList::each (const std::function<void (XMLListItemSharedPtr)> &callback)
{
	for (auto &item : m_List)
	{
		callback(item);
	}
}
