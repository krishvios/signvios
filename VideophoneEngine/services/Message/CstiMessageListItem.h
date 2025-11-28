/*!
 * \file CstiMessageListItem.h
 * \brief Definition of the CstiMessageListItem class.
 *
 * \date Tue Oct 31, 2006
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
 
#ifndef CSTI_MESSAGE_LIST_ITEM_H
#define CSTI_MESSAGE_LIST_ITEM_H

//
// Includes
//
#include <ctime>
#include "CstiCallListItem.h"
#include "stiDefs.h"
#include "stiSVX.h"
#include <memory>

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//

//
// Class Declaration
//

class CstiMessageListItem : public CstiCallListItem
{
public:
	
	CstiMessageListItem () = default;
	~CstiMessageListItem () override = default;

	EstiBool ViewedGet ();
	void ViewedSet (EstiBool bViewed);

	uint32_t PausePointGet();
	void PausePointSet(uint32_t un32PausePoint);

	uint32_t FileSizeGet();
	void FileSizeSet(uint32_t unFileSize);

	EstiMessageType MessageTypeIdGet () const;
	void MessageTypeIdSet (EstiMessageType messageTypeId);

	const char *InterpreterIdGet() const;
	void InterpreterIdSet(const char *pszInterpreterId);

	const char *PreviewImageURLGet() const;
	void PreviewImageURLSet(const char *pszPreviewImageURL);

	EstiBool FeaturedVideoGet () const;
	void FeaturedVideoSet (EstiBool bFeaturedVideo);

	uint32_t FeaturedVideoPosGet() const;
	void FeaturedVideoPosSet(uint32_t unFeaturedPos);

	time_t AirDateGet () const;
	void AirDateSet (time_t ttAirDate);

	time_t ExpirationDateGet () const;
	void ExpirationDateSet (time_t ttExpirationDate);

	const char *DescriptionGet() const;
	void DescriptionSet(const char *pszDescription);

private:
	
	EstiMessageType m_messageTypeId = estiMT_NONE;

	EstiBool m_bViewed{estiFALSE};
	uint32_t m_un32PausePoint{0};
	uint32_t m_un32FileSize{0};
	CstiString m_InterpreterId;

	CstiString m_PreviewImageURL;
	EstiBool m_bFeaturedVideo{estiFALSE};
	uint32_t m_un32FeaturedPos{0};
	time_t m_ttAirDate{0};
	time_t m_ttExpirationDate{0};
	CstiString m_Description;

}; // end class CstiMessgeListItem

using CstiMessageListItemSharedPtr = std::shared_ptr<CstiMessageListItem>;

#endif // CSTI_MESSAGE_LIST_ITEM_H

// end file CstiMessgeListItem.h
