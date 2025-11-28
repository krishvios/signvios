/*!
 * \file FavoriteList.h
 * \brief Defines a list of Favorites
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2014 Sorenson Communications, Inc. -- All rights reserved
 *
 */

#include "FavoriteList.h"
#include "XMLList.h"
#include <sstream>


FavoriteList::FavoriteList () 
	
{
	stiLOG_ENTRY_NAME(FavoriteList::FavoriteList);

	std::string dataFolder;
	stiOSDynamicDataFolderGet (&dataFolder);

	std::stringstream favoriteListFile;
	favoriteListFile << dataFolder << "Favorites.xml";
	SetFilename (favoriteListFile.str ());

	XMLManager::init ();
}


////////////////////////////////////////////////////////////////////////////////
//; FavoriteList::~FavoriteList
//
// Description: Default destructor
//
// Abstract:
//
// Returns:
//	None
//
FavoriteList::~FavoriteList()
{
	stiLOG_ENTRY_NAME(FavoriteList::~FavoriteList);
	m_pdoc->ListFree ();
}

////////////////////////////////////////////////////////////////////////////////
//; FavoriteList::ItemGet
//
// Description: Returns the indicated item, or NULL
//
// Abstract:
//
// Returns:
//	CstiFavorite * - Pointer to requested item, or NULL on error
//
CstiFavoriteSharedPtr FavoriteList::ItemGet(int index) const
{
	stiLOG_ENTRY_NAME(FavoriteList::ItemGet);

	CstiFavoriteSharedPtr favorite;

	// Was a valid index requested?
	if (index < CountGet())
	{
		// Yes! Return the item.
		favorite = std::static_pointer_cast<CstiFavorite> (m_pdoc->ItemGet (index));
	} // end if

	return (favorite);
} // end FavoriteList::ItemGet

////////////////////////////////////////////////////////////////////////////////
//; FavoriteList::ItemAdd
//
// Description: Adds an item to the array of CallListItems
//
// Abstract:
//
// Returns:
//	stiRESULT_SUCCESS if successful, an error if not.
//
stiHResult FavoriteList::ItemAdd(const CstiFavoriteSharedPtr &favoriteItem)
{
	stiLOG_ENTRY_NAME(FavoriteList::ItemAdd);

	m_pdoc->ItemAdd (favoriteItem);

	startSaveDataTimer ();

	return stiRESULT_SUCCESS;

} // end FavoriteList::ItemAdd

////////////////////////////////////////////////////////////////////////////////
//; FavoriteList::ItemRemove
//
// Description: Removes an item from the list
//
// Abstract:
//
// Returns:
//	stiRESULT_SUCCESS if successful, an error if not.
//
stiHResult FavoriteList::ItemRemove (
	int index)
{
	stiLOG_ENTRY_NAME (FavoriteList::ItemRemove);
	CstiFavoriteSharedPtr favorite = ItemGet(index);
	// Update favorite positions
	if (favorite)
	{
		int nFavoritePosition = favorite->PositionGet();
		for (int i = 0; i < CountGet(); i++)
		{
			auto item = std::static_pointer_cast<CstiFavorite> (m_pdoc->ItemGet (i));
			if (nFavoritePosition < item->PositionGet())
			{
				item->PositionSet(item->PositionGet() - 1);
			}
		}
	}
	m_pdoc->ItemDestroy (index);

	startSaveDataTimer ();

	return stiRESULT_SUCCESS;

} // end FavoriteList::ItemRemove

////////////////////////////////////////////////////////////////////////////////
//; FavoriteList::ItemDetach
//
// Description: Combination of ItemGet and ItemRemove.
//
// Abstract:
//
// Returns:
//	CstiFavorite * - Pointer to requested item, or NULL on error
//
CstiFavoriteSharedPtr FavoriteList::ItemDetach (
	int index)
{
	stiLOG_ENTRY_NAME (FavoriteList::ItemDetach);

	// Can we get this item?
	auto item = ItemGet (index);
	
	if (item)
	{
		// Yes!
		ItemRemove (index);
	}

	// Update favorite positions
	for (int i = index; i < CountGet (); i++)
	{
		auto item2 = std::static_pointer_cast<CstiFavorite> (m_pdoc->ItemGet (i));
		item2->PositionSet (item2->PositionGet () - 1);
	}

	return item;

} // end FavoriteList::ItemDetach

int FavoriteList::ItemIndexGet(const CstiFavoriteSharedPtr &favorite)
{
	int nIndex = -1;

	for (int i = 0; i < CountGet(); i++)
	{
		auto item = std::static_pointer_cast<CstiFavorite> (m_pdoc->ItemGet (i));
		if (item->PhoneNumberIdGet() == favorite->PhoneNumberIdGet())
		{
			nIndex = i;
			break;
		}
	}

	return nIndex;
}

void FavoriteList::ItemMove(int oldIndex, int newIndex)
{
	auto favorite = ItemDetach (oldIndex);

	if (newIndex > oldIndex)
	{
		newIndex--;
	}

	m_pdoc->itemInsert(favorite, newIndex);
	
	for (int i = 0; m_pdoc->CountGet (); i++)
	{
		auto item = std::static_pointer_cast<CstiFavorite> (m_pdoc->ItemGet (i));
		item->PositionSet (i);
	}
	startSaveDataTimer ();
}

void FavoriteList::purge ()
{
	m_pdoc->ListFree ();
	startSaveDataTimer ();
}


int FavoriteList::CountGet () const
{
	return static_cast<int>(m_pdoc->CountGet ());
}


WillowPM::XMLListItemSharedPtr FavoriteList::NodeToItemConvert (IXML_Node *node)
{
	CstiFavoriteSharedPtr item = std::make_shared<CstiFavorite> (node);
	return item;
}


void FavoriteList::populate (IXML_Node *node)
{
	node = ixmlNode_getFirstChild (node);   // the first subtag

	while (node)
	{
		const char *tagName = ixmlNode_getNodeName (node);
		IXML_Node *childNode = ixmlNode_getFirstChild (node);

		if (strcmp (tagName, "Favorite") == 0)
		{
			ItemAdd (std::make_shared<CstiFavorite> (childNode));
		}

		node = ixmlNode_getNextSibling (node);
	}
}
