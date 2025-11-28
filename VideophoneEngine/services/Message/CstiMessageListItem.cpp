/*!
 * \file CstiMessgeListItem.cpp
 * \brief Implementation of the CstiMessageListItem class.
 *
 * Implements a single CstiMessageListItem contained by the CstiList
 *
 * \date Tue Oct 31, 2006
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

//
// Includes
//

#include "stiDefs.h"
#include "CstiMessageListItem.h"

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
// Locals
//

//
// Function Declarations
//


////////////////////////////////////////////////////////////////////////////////
//; CstiMessageListItem::ViewedGet
//
// Description: ViewedGet
//
// Abstract:
//	ViewedGet
//
// Returns:
//	EstiBool
//
EstiBool CstiMessageListItem::ViewedGet ()
{
	return m_bViewed;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiMessageListItem::ViewedSet
//
// Description: ViewedSet
//
// Abstract:
//	ViewedSet
//
// Returns:
//	None
//
void CstiMessageListItem::ViewedSet (EstiBool bViewed)
{
	m_bViewed = bViewed;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiMessageListItem::PausePointGet
//
// Description: Get the pause point	in seconds
//
// Abstract:
//	
//
// Returns:
//	m_un16PausePoint
//
uint32_t CstiMessageListItem::PausePointGet()
{
	return m_un32PausePoint;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiMessageListItem::PausePointSet
//
// Description: Sets the pause point in seconds
//
// Abstract:
//	
//
// Returns:
//	None
//
void CstiMessageListItem::PausePointSet(uint32_t un32PausePoint)
{
	m_un32PausePoint = un32PausePoint;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiMessageListItem::FileSizeGet
//
// Description: Get the file size in bytes
//
// Abstract:
//	
//
// Returns:
//	m_un32FileSize
//
uint32_t CstiMessageListItem::FileSizeGet()
{
	return m_un32FileSize;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiMessageListItem::FileSizeSet
//
// Description: Sets the file size in bytes
//
// Abstract:
//	
//
// Returns:
//	None
//
void CstiMessageListItem::FileSizeSet(uint32_t un32FileSize)
{
	m_un32FileSize = un32FileSize;
}

/*!
 * \brief Gets the CstiMessageListItem MessageTypeId value
 * \return The MessageTypeId enumb value
 */
EstiMessageType CstiMessageListItem::MessageTypeIdGet () const
{
	return m_messageTypeId;
}

/*!
 * \brief Sets the CstiMessageListItem MessageTypeId value
 * \param messageTypeId The MessageTypeId value
 */
void CstiMessageListItem::MessageTypeIdSet (EstiMessageType messageTypeId)
{
	m_messageTypeId = messageTypeId;
}

/*!
 * \brief Gets the CstiMessageListItem InterpreterId value
 * \return The InterpreterId string value
 */
const char * CstiMessageListItem::InterpreterIdGet () const
{
	return m_InterpreterId;
}

/*!
 * \brief Sets the CstiMessageListItem InterpreterId value
 * \param pszInterpreterId The InterpreterId value
 */
void CstiMessageListItem::InterpreterIdSet (const char *pszInterpreterId)
{
	m_InterpreterId = pszInterpreterId;
}

/*!
 * \brief Gets the CstiMessageListItem PreviewImageURL value
 * \return The PreviewImageULR string value
 */
const char * CstiMessageListItem::PreviewImageURLGet () const
{
	return m_PreviewImageURL;
}

/*!
 * \brief Sets the CstiMessageListItem preivew image URL value
 * \param pszPreviewImageURL The PreviewImageURL value
 */
void CstiMessageListItem::PreviewImageURLSet (const char *pszPreviewImageURL)
{
	m_PreviewImageURL = pszPreviewImageURL;
}

/*!
 * \brief Gets the CstiMessageListItem IsFeatured value
 * \return The IsFeatured value
 */
EstiBool CstiMessageListItem::FeaturedVideoGet () const
{
	return m_bFeaturedVideo;
}

/*!
 * \brief Sets the CstiMessageListItem ivew image URL value
 * \param pszPreviewImageURL The PreviewImageURL value
 */
void CstiMessageListItem::FeaturedVideoSet (EstiBool bFeaturedVideo)
{
	m_bFeaturedVideo = bFeaturedVideo;
}

/*!
 * \brief Gets the CstiMessageListItem featured video position
 * \return The featured video postion value
 */
uint32_t CstiMessageListItem::FeaturedVideoPosGet() const
{
	return m_un32FeaturedPos;
}

/*!
 * \brief Sets the CstiMessageListItem featured video position
 * \param pszPreviewImageURL The featured video's position in the list.
 */
void CstiMessageListItem::FeaturedVideoPosSet(uint32_t un32FeaturedPos)
{
	m_un32FeaturedPos = un32FeaturedPos;
}

/*!
 * \brief Gets the MessageListItem call air date value
 * \return The ttAirDate
 */
time_t CstiMessageListItem::AirDateGet() const
{
	return m_ttAirDate;
}

/*!
 * \brief Sets the MessageListItem air date value
 * \param ttAirDate The air date value
 */
void CstiMessageListItem::AirDateSet(time_t ttAirDate)
{
	m_ttAirDate = ttAirDate;
}

/*!
 * \brief Gets the MessageListItem call expiration date value
 * \return The ttExpirationDate
 */
time_t CstiMessageListItem::ExpirationDateGet() const
{
	return m_ttExpirationDate;
}

/*!
 * \brief Sets the MessageListItem Expiration date value
 * \param ttExpirationDate The Expiration date value
 */
void CstiMessageListItem::ExpirationDateSet(time_t ttExpirationDate)
{
	m_ttExpirationDate = ttExpirationDate;
}

/*!
 * \brief Gets the CstiMessageListItem description value
 * \return The description string value
 */
const char * CstiMessageListItem::DescriptionGet() const
{
	return m_Description;
}

/*!
 * \brief Sets the CstiMessageListItem description value
 * \param pszDescription The description value
 */
void CstiMessageListItem::DescriptionSet(const char *pszDescription)
{
	m_Description = pszDescription;
}

// end file CstiMessageListItem.cpp
