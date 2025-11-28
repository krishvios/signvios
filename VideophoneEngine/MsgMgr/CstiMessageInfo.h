/*!
*  \file CstiMessageInfo.h
*  \brief The Message Info Class
*
*  Class declaration for the Message Info class.  Provides storage
*  and access for persitent (both local and remote) and temporary data.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*/

#ifndef CSTIMESSAGEINFO_H
#define CSTIMESSAGEINFO_H

#include "stiSVX.h"
#include "CstiItemId.h"
#include "ISerializable.h"

/*!
* \ingroup VideophoneEngineLayer 
 * \brief Message Info Class
*/
class CstiMessageInfo : public vpe::Serialization::ISerializable
{
public:

	/*!
	 * \brief Constructor
	 */
	CstiMessageInfo ();

	/*!
	 * \brief Destructor
	 */
	~CstiMessageInfo () override = default;
	
	CstiMessageInfo (const CstiMessageInfo &) = delete;
	CstiMessageInfo (CstiMessageInfo&& other) = delete;
	CstiMessageInfo &operator= (const CstiMessageInfo &) = delete;
	CstiMessageInfo& operator= (CstiMessageInfo&& other) = delete;

	/*!
	 * \brief Returns the air date time string of the message. 
	 *  
	 * \return A const pointer to the air date time string of the message.  The data should be copied before use. 
	 */
	time_t AirDateGet()
	{
		return m_tAirDate;
	}

	/*!
	 * \brief Sets the air date. 
	 *  
	 * \param tAirDateTime is a time_t. 
	 *  
	 * \return stiRESULT_SUCCESS 
	 */
	stiHResult AirDateSet(
		time_t tAirDate);

	/*!
	 * \brief Returns the date time string of the message. 
	 *  
	 * \return A const pointer to the date time string of the message.  The data should be copied before use. 
	 */
	time_t DateTimeGet()
	{
		return m_tDateTime;
	}

	/*!
	 * \brief Sets the date and time. 
	 *  
	 * \param tDateTime is a time_t. 
	 *  
	 * \return stiRESULT_SUCCESS 
	 */
	stiHResult DateTimeSet(
		time_t tDateTime);

	/*!
	 * \brief Returns the description of the message. 
	 *  
	 * \return A const pointer to the description string of the message.  The data should be copied before use. 
	 */
	const char *DescriptionGet()
	{
		return m_description.c_str();
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
	stiHResult DescriptionSet(
		const char *pszDescription);

	/*!
	 * \brief Returns the unformated dial string of the message. 
	 *  
	 * \return A const pointer to the unformated dial string of the message.  The data should be copied before use. 
	 */
	const char *DialStringGet()
	{
		return m_dialString.c_str();
	}

	/*!
	 * \brief Sets the unformated dial string for the message. 
	 *  
	 * \param pszDialString is a const char pointer which points to the message's new phone number. This data will 
	 *        be copied to an internal buffer. 
	 *  
	 * \return stiRESULT_SUCCESS if the dial string is stored successful, 
	 *         stiRESULT_ERROR if pszDialString is NULL,
	 *         stiRESULT_MEMORY_ALLOC_ERROR if allocating the internal buffer fails. 
	 */
	stiHResult DialStringSet(
		const char *pszDialString);

	/*!
	 * \brief Returns the dirty status of the message. 
	 * 		  Note: Should only be used by the engine. 
	 *  
	 * \return estiTRUE if the message is dirty. 
	 * 		   estiFALSE if the message is clean.  
	 */
	EstiBool DirtyGet()
	{
		return m_bIsDirty;
	}

	/*!
	 * \brief Sets the dirty status of the message. 
	 *  	  Note: Shoud only be used by the engine.
	 *  
	 * \param bIsDirty is an EstiBool and indicates weather the message is in a dirty state. 
	 *  
	 */
	void DirtySet(
		EstiBool bIsDirty)
	{
		m_bIsDirty = bIsDirty;
	}

	/*!
	 * \brief Returns the experation date time string of the message. 
	 *  
	 * \return A const pointer to the experation date time string of the message.  The data should be copied before use. 
	 */
	time_t ExpirationDateGet()
	{
		return m_tExpirationDate;
	}

	/*!
	 * \brief Sets the experation date and time. 
	 *  
	 * \param tExperationDateTime is a time_t. 
	 *  
	 * \return stiRESULT_SUCCESS 
	 */
	stiHResult ExpirationDateSet(
		time_t tExperationDate);

	/*!
	 * \brief Returns the viewed state of a message. 
	 *  
	 * \return true if the message has been previously viewed, otherwise returns false. 
	 */
	EstiBool FeaturedVideoGet()
	{
		return m_bFeaturedVideo;
	}

	/*!
	 * \brief Sets the viewed state of a message. 
	 *  
	 * \param bNewViewed is the new viewed state. 
	 */
	void FeaturedVideoSet(
		EstiBool bFeaturedVideo)
	{
		m_bFeaturedVideo = bFeaturedVideo;
	}

	/*!
	 * \brief Returns the index of the featured video. 
	 *  
	 * \return An 32 bit unsigned interger specifying the index of the featured video. 
	 */
	uint32_t FeaturedVideoPosGet()
	{
		return m_un32FeaturedVideoPos;
	}

	/*!
	 * \brief Sets the desired index of the featured video. 
	 *  
	 * \param un32FeaturedVideoPos is the featured video's index. 
	 */
	void FeaturedVideoPosSet (
		uint32_t un32FeaturedVideoPos)
	{
		m_un32FeaturedVideoPos = un32FeaturedVideoPos;
	}

	/*!
	 * \brief Returns the unformated InterpreterId string of the message.
	 *
	 * \return A const pointer to the unformated InterpreterId string of the message.  The data should be copied before use.
	 */
	const char *InterpreterIdGet()
	{
		return m_interpreterId.c_str();
	}

	/*!
	 * \brief Sets the unformated InterpreterId string for the message.
	 *
	 * \param pszInterpreterId is a const char pointer which points to the message's InterpreterId string. This data will
	 *        be copied to an internal buffer.
	 *
	 * \return stiRESULT_SUCCESS if the InterpreterId is stored successful,
	 *         stiRESULT_ERROR if pszInterpreterId is NULL,
	 *         stiRESULT_MEMORY_ALLOC_ERROR if allocating the internal buffer fails.
	 */
	stiHResult InterpreterIdSet(
		const char *pszInterpreterId);

	/*!
	 * \brief Returns the unique item ID for the message. 
	 *  
	 * \return A CstiItemId that contains the unique item ID for the message. 
	 */
	CstiItemId ItemIdGet()
	{
		return m_ItemId;
	}

	/*!
	 * \brief Sets a new CstiItemId for the message. 
	 *  
	 * \param pNewItemId is a CstiItemId that should be set on the message. 
	 */
	void ItemIdSet(
		const CstiItemId &pItemId)
	{
		m_ItemId = pItemId;
	}

	/*!
	 * \brief Returns the message length in seconds. 
	 *  
	 * \return An 32 bit unsigned interger specifying the message length in seconds. 
	 */

	uint32_t MessageLengthGet()
	{
		return m_un32MessageLength;
	}

	/*!
	 * \brief Sets the message length in seconds. 
	 *  
	 * \param un32PausePoint is the message length in seconds. 
	 */
	void MessageLengthSet (
		uint32_t un32MessageLength)
	{
		m_un32MessageLength = un32MessageLength;
	}


	/*!
	* \brief Returns the message type ID of a message.
	*
	* \return An enumeration specifying the type ID of this message.
	*/
	EstiMessageType MessageTypeIdGet () const
	{
		return m_messageTypeId;
	}

	/*!
	* \brief Sets the message type ID of the message.
	*
	* \param messageTypeId is the new message type ID.
	*/
	void MessageTypeIdSet (
		EstiMessageType messageTypeId)
	{
		m_messageTypeId = messageTypeId;
	}


	/*!
	 * \brief Returns the name of the message. 
	 *  
	 * \return A const pointer to the name of the message.  The data should be copied before use. 
	 */
	const char *NameGet()
	{
		return m_messageName.c_str();
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
	stiHResult NameSet(
		const char *pszName);

	/*!
	 * \brief Returns the point, in seconds, that the messages was paused at during the last viewing. 
	 *  
	 * \return An 32 bit unsigned interger specifying the last pause point of the message. 
	 */
	uint32_t PausePointGet()
	{
		return m_un32PausePoint;
	}

	/*!
	 * \brief Sets the desired pause point of the message. 
	 *  
	 * \param un32PausePoint is the new pause point in seconds. 
	 */
	void PausePointSet (
		uint32_t un32PausePoint)
	{
		m_un32PausePoint = un32PausePoint;
	}

	/*!
	 * \brief Returns the unique image ID for the message. 
	 *  
	 * \return A const pointer to the video's preview imaage.  The data should be copied before use. 
	 */
	const char *PreviewImageURLGet()
	{
		return m_previewImageURL.c_str();
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
	stiHResult PreviewImageURLSet(
		const char *pszPreviewImageURL);

	/*!
	 * \brief Returns the viewed state of a message. 
	 *  
	 * \return true if the message has been previously viewed, otherwise returns false. 
	 */
	EstiBool ViewedGet()
	{
		return m_bViewed;
	}

	/*!
	 * \brief Sets the viewed state of a message. 
	 *  
	 * \param bNewViewed is the new viewed state. 
	 */
	void ViewedSet(EstiBool bViewed)
	{
		m_bViewed = bViewed;
	}

	/*!
	 * \brief Returns the viewId of the message being played. This is not maintained by the item, but 
	 *  	  is only used when storing a pause point.  It should be set on a message info that is being
	 *  	  passed into the message manager's MessageInfoSet() function when saving a pause point.
	 *  
	 * \return A const pointer to the objects view ID.  The data should be copied before use. 
	 */
	std::string ViewIdGet()
	{
		return m_viewId;
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
	stiHResult ViewIdSet(
		const std::string &viewId);


private:
	std::string typeNameGet () override;
	vpe::Serialization::MetaData& propertiesGet () override;
	vpe::Serialization::ISerializable* instanceCreate (const std::string& typeName) override;

	EstiBool m_bViewed{estiFALSE};
	uint32_t m_un32PausePoint{0};
	uint32_t m_un32MessageLength{0};
	CstiItemId m_ItemId;
	std::string m_messageName;
	time_t m_tDateTime{};
	std::string m_dialString;
	std::string m_interpreterId;
	EstiMessageType m_messageTypeId{estiMT_NONE};
	EstiBool m_bIsDirty{estiFALSE};
	std::string m_viewId;
	std::string m_previewImageURL;
	EstiBool m_bFeaturedVideo{estiFALSE};
	uint32_t m_un32FeaturedVideoPos{0};
	time_t m_tAirDate{0};
	time_t m_tExpirationDate{0};
	std::string m_description;

	vpe::Serialization::MetaData m_serializationData;
};

#endif // ISTIMESSAGEINFO_H
