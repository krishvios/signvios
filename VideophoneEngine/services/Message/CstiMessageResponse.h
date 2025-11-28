/*!
 * \file CstiMessageResponse.h
 * \brief Definition of the CstiMessageResponse class.
 *
 * \date Wed Oct 18, 2006
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */


#ifndef CSTIMESSAGERESPONSE_H
#define CSTIMESSAGERESPONSE_H

//
// Includes
//
#include "stiSVX.h"
#include "CstiMessageList.h"
#include "CstiVPServiceResponse.h"
#include "CstiVPServiceRequest.h"
#include <vector>

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

class CstiMessageResponse : public CstiVPServiceResponse
{
public:
	enum EResponse
	{
		eRESPONSE_UNKNOWN,
		eRESPONSE_ERROR,
		eMESSAGE_INFO_GET_RESULT,
		eMESSAGE_VIEWED_RESULT,
		eMESSAGE_LIST_ITEM_EDIT_RESULT,
		eMESSAGE_LIST_GET_RESULT,
		eMESSAGE_CREATE_RESULT,
		eMESSAGE_DELETE_RESULT,
		eMESSAGE_DELETE_ALL_RESULT,
		eMESSAGE_SEND_RESULT,
		eMESSAGE_LIST_COUNT_GET_RESULT,
		eMESSAGE_DOWNLOAD_URL_GET_RESULT,
		eMESSAGE_DOWNLOAD_URL_LIST_GET_RESULT,
		eMESSAGE_UPLOAD_URL_GET_RESULT,
		eNEW_MESSAGE_COUNT_GET_RESULT,
		eRETRIEVE_MESSAGE_KEY_RESULT,
		eGREETING_INFO_GET_RESULT,
		eGREETING_UPLOAD_URL_GET_RESULT,
		eGREETING_DELETE_RESULT,
		eGREETING_ENABLED_STATE_SET_RESULT,
	}; // end EResponse


	enum EResponseError
	{
		// Errors occurring during message processing
		eNO_ERROR = 0,							// No Error
		eNOT_LOGGED_IN = 1,						// Not Logged In
		eMESSAGE_SERVER_NOT_REACHABLE = 14,
		eSQL_SERVER_NOT_REACHABLE = 39,

		// High-Level Server Errors (message not processed)
		eDATABASE_CONNECTION_ERROR = 0xFFFFFFFB,// Database connection error
		eXML_VALIDATION_ERROR = 0xFFFFFFFC,		// XML Validation Error
		eXML_FORMAT_ERROR = 0xFFFFFFFD,			// XML Format Error
		eGENERIC_SERVER_ERROR = 0xFFFFFFFE,		// General Server Error (exception, etc.)
		eUNKNOWN_ERROR = 0xFFFFFFFF
	}; // end EResponseError

	enum EResponseStringType
	{
		eRESPONSE_STRING_GUID,
		eRESPONSE_STRING_SERVER,
		eRESPONSE_STRING_RETRIEVAL_METHOD
	}; // end EResponseStringType
	
	enum EGreetingType
	{
		eGREETING_TYPE_DEFAULT,
		eGREETING_TYPE_PERSONAL,
		eGREETING_TYPE_UNKNOWN
	};
	
	///
	/// \brief Struct defining the GreetingInfo
	///
	struct SGreetingInfo
	{
		int nMaxSeconds {0};		        /*!< Max seconds for greeting */
		std::string Url1;		        /*!< URL to greeting */
		std::string Url2;		        /*!< URL to greeting */
		std::string GreetingImageUrl;	/*!< URL to greeting image*/
		std::string GreetingText;	    /*!< The greeting text */
		EGreetingType eGreetingType{eGREETING_TYPE_DEFAULT};	/*!< The greeting type */
	};

	class CstiMessageDownloadURLItem
	{
	public:
		std::string m_DownloadURL;
		int m_nFileSize{0};
		int m_nMaxBitRate{0};
		int m_nWidth{0};
		int m_nHeight{0};
		//	EstiVideoCodec m_eVideoCodec;
		std::string m_Codec;
		//	EstiVideoProfile m_eProfile;
		std::string m_Profile;
		//	EstiH264Level m_eLevel;
		std::string m_Level;
	};

	CstiMessageResponse (
		CstiVPServiceRequest *request,
		EResponse eResponse,
		EstiResult eResponseResult,
		EResponseError eError,
		const char *pszError);
	
	CstiMessageResponse (const CstiMessageResponse &) = delete;
	CstiMessageResponse (CstiMessageResponse &&) = delete;
	CstiMessageResponse &operator= (const CstiMessageResponse &) = delete;
	CstiMessageResponse &operator= (CstiMessageResponse &&) = delete;

	inline std::string clientTokenGet () const;
	inline void clientTokenSet (const std::string &clientToken);
	inline EResponse ResponseGet () const;
	inline EResponseError ErrorGet () const;

	inline std::string ResponseStringGet (
		EResponseStringType eStringType) const;
		
	inline void ResponseStringSet (
		EResponseStringType eStringType,
		const char *szResponseString);
	
	inline CstiMessageList * MessageListGet () const;
	
	inline void MessageListSet (
		CstiMessageList * poMessageList);

	std::vector <SGreetingInfo> *GreetingInfoListGet();
	void GreetingInfoListSet (const std::vector <SGreetingInfo> &greetingInfoList);

	std::vector<CstiMessageDownloadURLItem> *MessageDownloadUrlListGet ();
	void MessageDownloadUrlListSet (const std::vector<CstiMessageDownloadURLItem> & messageDownloadUrlList);

protected:

	~CstiMessageResponse () override;

private:
	
	EResponse m_eResponse {eRESPONSE_UNKNOWN};
	EResponseError m_eError {eNO_ERROR};

	std::string m_clientToken;
	std::string m_messageResponseGUID;
	std::string m_messageResponseServer;
	std::string m_messageResponseRetrievalMethod;

	CstiMessageList *m_poMessageList {nullptr};

	std::vector <SGreetingInfo> m_GreetingInfoList;
	std::vector <CstiMessageDownloadURLItem> m_MessageDownloadUrlList;

}; // end class CstiMessageResponse

////////////////////////////////////////////////////////////////////////////////
//; CstiMessageResponse::clientTokenGet
//
// Description: 
//
// Abstract:
//
// Returns:
//
std::string CstiMessageResponse::clientTokenGet () const
{
	stiLOG_ENTRY_NAME (CstiMessageResponse::clientTokenGet);

	return m_clientToken;
} 

////////////////////////////////////////////////////////////////////////////////
//; CstiMessageResponse::clientTokenSet
//
// Description: 
//
// Abstract:
//
// Returns:
//
void CstiMessageResponse::clientTokenSet (
	const std::string &clientToken)
{
	stiLOG_ENTRY_NAME (CstiMessageResponse::clientTokenSet);

	m_clientToken = clientToken;
} 

////////////////////////////////////////////////////////////////////////////////
//; CstiMessageResponse::ResponseGet
//
// Description: 
//
// Abstract:
//
// Returns:
//
CstiMessageResponse::EResponse CstiMessageResponse::ResponseGet () const
{
	stiLOG_ENTRY_NAME (CstiMessageResponse::ResponseGet);

	return (m_eResponse);
} // end CstiMessageResponse::ResponseGet

////////////////////////////////////////////////////////////////////////////////
//; CstiMessageResponse::ErrorGet
//
// Description: 
//
// Abstract:
//
// Returns:
//
CstiMessageResponse::EResponseError CstiMessageResponse::ErrorGet () const
{
	stiLOG_ENTRY_NAME (CstiMessageResponse::ErrorGet);

	return (m_eError);
} // end CstiMessageResponse::ErrorGet


/*!
*\brief Retrieves the response member string of the specified String Type
* 
* This function is specific to the CstiMessageResponse class; it is used to 
* retrieve the member variable strings that are particular to this type of response.
* \param eStringType Enumerated value represting the type of member string to retrieve.
* \return const char * to the string value
*/
std::string CstiMessageResponse::ResponseStringGet (
	EResponseStringType eStringType) const
{
	stiLOG_ENTRY_NAME (CstiMessageResponse::ResponseStringGet);

	// Determine which member variable string to get
	//
	switch (eStringType)
	{
		case eRESPONSE_STRING_GUID:
			return m_messageResponseGUID;
		case eRESPONSE_STRING_SERVER:
			return m_messageResponseServer;
		case eRESPONSE_STRING_RETRIEVAL_METHOD:
			return m_messageResponseRetrievalMethod;
	}
		
	return {};
} // end CstiMessageResponse::ResponseStringGet

/*!
*\brief Sets the response member string of the specified String Type
* 
* This function is specific to the CstiMessageResponse class; it is used to 
* set the member variable strings that are particular to this type of response.
* \return OK or ERROR
*/
void CstiMessageResponse::ResponseStringSet (
	EResponseStringType eStringType,				//!< Enumerated value representing the type of member string to set
	const char * pszResponseString)					//!< The response string
{
	stiLOG_ENTRY_NAME (CstiMessageResponse::ResponseStringSet);

	// Determine which member variable string to set
	//
	switch (eStringType)
	{
		case eRESPONSE_STRING_SERVER:
			m_messageResponseServer = pszResponseString ? pszResponseString : "";
			break;
		case eRESPONSE_STRING_RETRIEVAL_METHOD:
			m_messageResponseRetrievalMethod = pszResponseString ? pszResponseString : "";
			break;
		case eRESPONSE_STRING_GUID:
			m_messageResponseGUID = pszResponseString ? pszResponseString : "";
			break;
		default:
			stiASSERT(false);
	}
} // end CstiMessageResponse::ResponseStringSet

/*!
 *\brief Gets the message list saved in the response 
 */
CstiMessageList * CstiMessageResponse::MessageListGet () const
{
	stiLOG_ENTRY_NAME (CstiMessageResponse::MessageListGet);

	return (m_poMessageList);
} // end CstiMessageResponse::MessageListGet

/*!
 *\brief Sets the message list saved in the response 
 */
void CstiMessageResponse::MessageListSet (
	CstiMessageList * poMessageList)
{
	stiLOG_ENTRY_NAME (CstiMessageResponse::MessageListSet);

	m_poMessageList = poMessageList;
} // end CstiMessageResponse::MessageListSet

#endif // CSTIMESSAGERESPONSE_H
// end file CstiMessageResponse.h
