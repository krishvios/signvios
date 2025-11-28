/*!
 * \file FavoriteList.h
 * \brief Defines a list of Favorites
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2014 Sorenson Communications, Inc. -- All rights reserved
 *
 */

#pragma once

#include <vector>
#include "XMLManager.h"
#include "CstiFavorite.h"

class FavoriteList : public WillowPM::XMLManager
{
public:
	FavoriteList ();
	~FavoriteList() override;

	FavoriteList (const FavoriteList &other) = delete;
	FavoriteList (FavoriteList &&other) = delete;
	FavoriteList &operator= (const FavoriteList &other) = delete;
	FavoriteList &operator= (FavoriteList &&other) = delete;

	WillowPM::XMLListItemSharedPtr NodeToItemConvert (IXML_Node *node) override;
	
	stiHResult ItemAdd(const CstiFavoriteSharedPtr &poFavorite);
	CstiFavoriteSharedPtr ItemGet(int index) const;
	void ItemFree(const CstiFavoriteSharedPtr &pListItem);
	int ItemIndexGet(const CstiFavoriteSharedPtr &pFavorite);
	void ItemMove(int oldIndex, int newIndex);
	stiHResult ItemRemove (int index);
	CstiFavoriteSharedPtr ItemDetach (int index);
	void ListFree ();
	void purge ();

	int MaxCountGet() const { return m_maxCount; }
	void MaxCountSet(int count) { m_maxCount = count; }

	int CountGet () const;

private:
	void populate (IXML_Node *node);

	int m_maxCount {30};
};

