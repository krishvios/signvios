/*!
*  \file CstiMessageInfo.cpp
*  \brief The MessageInfo class
*
*  Class implmentation for the Message Info class.
*  The MessageInfo is the object used to pass information between the Message Manager
*  the application.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*/
#include <cstring>

#include "stiSVX.h"
#include "stiError.h"
#include "stiDefs.h"
#include "stiTrace.h"
#include "CstiItemId.h"
#include "CstiMessageInfo.h"

CstiMessageInfo::CstiMessageInfo ()
{
	using namespace vpe::Serialization;
	m_serializationData.push_back (std::make_shared<IntegerProperty<EstiBool>> ("viewed", m_bViewed));
	m_serializationData.push_back (std::make_shared<IntegerProperty<uint32_t>> ("pause_point", m_un32PausePoint));
	m_serializationData.push_back (std::make_shared<IntegerProperty<uint32_t>> ("message_length", m_un32MessageLength));
	m_serializationData.push_back (std::make_shared<CustomIntegerProperty<CstiItemId>> ("id", m_ItemId,
		[] (CstiItemId& itemId) {
			std::string id;
			itemId.ItemIdGet (&id);
			return std::stoll(id);
		},
		[](int64_t value, CstiItemId& itemId) {
			itemId.ItemIdSet (std::to_string(value).c_str());
		}));
	m_serializationData.push_back (std::make_shared<StringProperty> ("message_name", m_messageName));
	m_serializationData.push_back (std::make_shared<IntegerProperty<time_t>> ("date_time", m_tDateTime));
	m_serializationData.push_back (std::make_shared<StringProperty> ("dial_string", m_dialString));
	m_serializationData.push_back (std::make_shared<StringProperty> ("interpreter_id", m_interpreterId));
	m_serializationData.push_back (std::make_shared<IntegerProperty<EstiMessageType>> ("message_type", m_messageTypeId));
	m_serializationData.push_back (std::make_shared<IntegerProperty<EstiBool>> ("dirty", m_bIsDirty));
	m_serializationData.push_back (std::make_shared<StringProperty> ("view_id", m_viewId));
	m_serializationData.push_back (std::make_shared<StringProperty> ("preview_image_url", m_previewImageURL));
	m_serializationData.push_back (std::make_shared<IntegerProperty<EstiBool>> ("featured", m_bFeaturedVideo));
	m_serializationData.push_back (std::make_shared<IntegerProperty<uint32_t>> ("featured_pos", m_un32FeaturedVideoPos));
	m_serializationData.push_back (std::make_shared<IntegerProperty<time_t>> ("air_date", m_tAirDate));
	m_serializationData.push_back (std::make_shared<IntegerProperty<time_t>> ("expiration_date", m_tExpirationDate));
	m_serializationData.push_back (std::make_shared<StringProperty> ("description", m_description));
}


/*!
 * \brief Sets the air date and time. 
 *  
 * \param tAirDate is a time_t. 
 *  
 * \return stiRESULT_SUCCESS 
 */
stiHResult CstiMessageInfo::AirDateSet(
	const time_t tAirDate)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_tAirDate = tAirDate;

	return hResult;
}


/*!
 * \brief Sets the date and time string for the message. 
 *  
 * \param pszDateTime is a const char pointer which points to the message's new date and time. This data will 
 *        be copied to an internal buffer. 
 *  
 * \return stiRESULT_SUCCESS 
 */
stiHResult CstiMessageInfo::DateTimeSet(
	const time_t tDateTime)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_tDateTime = tDateTime;

	return hResult;
}

/*!
 * \brief Sets the description string for the message. 
 *  
 * \param pszDescription is a const char pointer which points to the message's description. This data will 
 *        be copied to an internal buffer. 
 *  
 * \return stiRESULT_SUCCESS if the description string is stored successful, 
 *         stiRESULT_ERROR if pszDescription is NULL,
 *         stiRESULT_MEMORY_ALLOC_ERROR if allocating the internal buffer fails. 
 */
stiHResult CstiMessageInfo::DescriptionSet(
	const char *pszDescription)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Make sure the pszDescription is not null.
	stiTESTCOND (pszDescription, stiRESULT_ERROR);

	m_description = pszDescription;

STI_BAIL:

	return hResult;
}


/*!
 * \brief Sets the dial string for the message. 
 *  
 * \param pszDialString is a const char pointer which points to the message's new phone number. This data will 
 *        be copied to an internal buffer. 
 *  
 * \return stiRESULT_SUCCESS if the phone number string is stored successful, 
 *         stiRESULT_ERROR if pszDialString is NULL,
 *         stiRESULT_MEMORY_ALLOC_ERROR if allocating the internal buffer fails. 
 */
stiHResult CstiMessageInfo::DialStringSet(
	const char *pszDialString)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Make sure the pszDialString is not null.
	stiTESTCOND (pszDialString, stiRESULT_ERROR);

	m_dialString = pszDialString;

STI_BAIL:

	return hResult;
}

/*!
 * \brief Sets the experation date and time. 
 *  
 * \param tExperationDateTime is a time_t. 
 *  
 * \return stiRESULT_SUCCESS 
 */
stiHResult CstiMessageInfo::ExpirationDateSet(
	const time_t tExpirationDate)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_tExpirationDate = tExpirationDate;
	return hResult;
}

/*!
 * \brief Sets the InterpreterId string for the message.
 *
 * \param pszInterpreterId is a const char pointer which points to the message's InterpreterId. This data will
 *        be copied to an internal buffer.
 *
 * \return stiRESULT_SUCCESS if the InterpreterId string is stored successful,
 *         stiRESULT_ERROR if pszInterpreterId is NULL,
 *         stiRESULT_MEMORY_ALLOC_ERROR if allocating the internal buffer fails.
 */
stiHResult CstiMessageInfo::InterpreterIdSet(const char *pszInterpreterId)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Make sure the pszInterpreterId is not null.
	stiTESTCOND (pszInterpreterId, stiRESULT_ERROR);

	m_interpreterId = pszInterpreterId;

STI_BAIL:

	return hResult;
}


/*!
 * \brief Sets the name for the message. 
 *  
 * \param pszName is a const char pointer which points to the message's new name. This data will 
 *        be copied to an internal buffer. 
 *  
 * \return stiRESULT_SUCCESS if the name string is stored successful, 
 *         stiRESULT_ERROR if pszName is NULL,
 *         stiRESULT_MEMORY_ALLOC_ERROR if allocating the internal buffer fails. 
 */
stiHResult CstiMessageInfo::NameSet(
	const char *pszName)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Make sure the pszName is not null.
	stiTESTCOND (pszName, stiRESULT_ERROR);

	m_messageName = pszName;

STI_BAIL:

	return hResult;
}

/*!
 * \brief Sets a URL for the message's image. 
 *  
 * \param pszDescription is a const char pointer which points to the video's preview image. This data will 
 *        be copied to an internal buffer. 
 *  
 * \return stiRESULT_SUCCESS if the description string is stored successful, 
 *         stiRESULT_ERROR if pszDescription is NULL,
 *         stiRESULT_MEMORY_ALLOC_ERROR if allocating the internal buffer fails. 
 */
stiHResult CstiMessageInfo::PreviewImageURLSet(
	const char *pszPreviewImageURL)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Make sure the pszPreviewImageURL is not null.
	stiTESTCOND (pszPreviewImageURL, stiRESULT_ERROR);

	m_previewImageURL = pszPreviewImageURL;

STI_BAIL:

	return hResult;
}

/*!
 * \brief Returns the viewId of the message being played. This is not maintained by the item, but 
 *  	  is only used when storing a pause point.  It should be set on a message info that is being
 *  	  passed into the message manager's MessageInfoSet() function when saving a pause point.
 *  
 * \param pszViewId is a const char pointer which points to the message's view ID. This data will 
 *        be copied to an internal buffer. 
 *  
 * \return stiRESULT_SUCCESS if the dial string is stored successful, 
 *         stiRESULT_ERROR if pszViewId is NULL,
 *         stiRESULT_MEMORY_ALLOC_ERROR if allocating the internal buffer fails. 
 */
stiHResult CstiMessageInfo::ViewIdSet(
	const std::string &viewId)
{
	m_viewId = viewId;

	return stiRESULT_SUCCESS;
}

std::string CstiMessageInfo::typeNameGet ()
{
	return typeid(CstiMessageInfo).name ();
}

vpe::Serialization::MetaData& CstiMessageInfo::propertiesGet ()
{
	return m_serializationData;
}

vpe::Serialization::ISerializable* CstiMessageInfo::instanceCreate (const std::string& typeName)
{
	return nullptr;
}
