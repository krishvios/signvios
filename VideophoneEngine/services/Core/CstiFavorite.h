/*!
 * \file CstiFavorite.h
 * \brief Defines an individual Favorite item
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2014 Sorenson Communications, Inc. -- All rights reserved
 *
 */

#ifndef CSTIFAVORITE_H
#define CSTIFAVORITE_H

#include "XMLListItem.h"
#include "CstiItemId.h"
#include "ixml.h"

class CstiFavorite : public WillowPM::XMLListItem
{
public:
	CstiFavorite() = default;
	CstiFavorite(const CstiFavorite &Rhs) = default;
	CstiFavorite (IXML_Node *item);
	~CstiFavorite() override = default;


	// XML List Item
	void Write (FILE *file) override;
	std::string NameGet () const override;
	void NameSet (const std::string &name) override;
	bool NameMatch (const std::string &name) override;

	CstiItemId PhoneNumberIdGet () const;
	void PhoneNumberIdSet(const CstiItemId &PhoneNumberId);

	bool IsContact () const;
	void IsContactSet(bool bContact);

	int PositionGet () const;
	void PositionSet(int nPos);

private:
	void populate (IXML_Node *node);

	CstiItemId m_PhoneNumberId;
	bool m_bIsContact{true};
	int m_nPosition{0};
};

using CstiFavoriteSharedPtr = std::shared_ptr<CstiFavorite>;

#endif // CSTIFAVORITE_H
