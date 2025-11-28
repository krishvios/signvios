// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef CSTIVPSERVICESREQUEST_H
#define CSTIVPSERVICESREQUEST_H

#include "stiSVX.h"
#include "VPServiceRequestContext.h"
#include "CstiVPServiceResponse.h"
#include "ServiceCallbackContext.h"
#include <list>

typedef struct _IXML_Node IXML_Node;
typedef struct _IXML_Document IXML_Document;
typedef struct _IXML_Element IXML_Element;


// Forward declarations
class CstiVPServiceRequest;

typedef stiHResult (*PstiRemainingResponseCallback) (
	CstiVPServiceRequest *pRequest,
	int nResponse,
	int nSystemicError,
	size_t CallbackParam);

enum RequestResults
{
	REQUEST_SUCCESS = 0,        // Request SUCCESS
	REQUEST_ERROR = 1           // GENERIC request error
};


void GetFormattedTime (
	time_t *pTime,
	char *pszFormattedTime,
	int nLength);

stiHResult addTimeAttributeToElement (
	IXML_Element *element,
	time_t time);

class CstiVPServiceRequest
{
public:

	CstiVPServiceRequest() = default;
	virtual ~CstiVPServiceRequest();

	CstiVPServiceRequest (const CstiVPServiceRequest &) = delete;
	CstiVPServiceRequest (CstiVPServiceRequest &&) = delete;
	CstiVPServiceRequest &operator= (const CstiVPServiceRequest &) = delete;
	CstiVPServiceRequest &operator= (CstiVPServiceRequest &&) = delete;

	EstiBool SSLEnabledGet() const;

	void SSLEnabledSet(EstiBool bSSLEnabled);

	/*!
	* \brief Returns the xml document as a character string
	*/
	int getXMLString(
		char **xmlString);

	/*!
	* \brief frees the character string returned by getxmlString
	*/
	void freeXMLString(
		const char *xmlString);

	/**
	* \brief Retrieves the ID of this Core Request
	* \return Error code if fails, otherwise SUCCESS (0)
	*/
	unsigned int requestIdGet();

	/**
	* \brief Sets the ID of this Core Request
	* \param unRequestNumber the request ID number
	* \return Error code if fails, otherwise SUCCESS (0)
	*/
	int requestIdSet(
		unsigned int unRequestNumber);

	std::string clientTokenGet();

	int PrepareRequestForSubmission(
		const std::string &cientToken,
		const std::string &uniqueID);

	/**
	* \brief set a parameter that comes back with the response.
	* \return none.
	*/
	template <typename ItemType>
	void contextItemSet(const ItemType &item)
	{
		m_context = std::make_shared<VPServiceRequestContextDerived<ItemType>>(item);
	}

	/**
	* \brief get the parameter that comes back with the response.
	* \return the contents of the context
	*/
	template <typename ItemType>
	inline ItemType contextItemGet () const
	{
		return ::contextItemGet<ItemType> (m_context);
	}

	/**
	* \brief get the parameter that comes back with the response.
	* \return the context parameter.
	*/
	inline VPServiceRequestContextSharedPtr contextGet () const
	{
		return m_context;
	}

	EstiBool &RemoveUponCommunicationError();

	void RepeatOnErrorSet (
		bool bRepeat)
	{
		m_bRepeatOnError = bRepeat;
	}

	bool RepeatOnErrorGet ()
	{
		return m_bRepeatOnError;
	}

	void ExpectedResultHandled (
		int nResponse);

	void ExpectedResultsReset ();

	void ProcessRemainingResponses (
		PstiRemainingResponseCallback pCallback,
		int nSystemicError,
		size_t CallbackParam);
		
	unsigned int AuthenticationAttemptsGet () const
	{
		return m_unAuthenticationAttempts;
	}
	
	void AuthenticationAttemptsSet (
		unsigned int nAttempts)
	{
		m_unAuthenticationAttempts = nAttempts;
	}

	void retryOfflineSet (bool value);
	bool retryOfflineGet ();

	template<typename T>
	void callbackAdd (std::function<void (std::shared_ptr<T>)> successCallback, std::function<void (std::shared_ptr<T>)> errorCallback)
	{
		auto context = std::make_shared<ServiceCallbackContext<T>> ();
		context->successCallback = successCallback;
		context->errorCallback = errorCallback;
		m_callbacks.push_back (context);
	}

	template<typename T>
	void callbackRaise (const std::shared_ptr<T>& content, bool error)
	{
		auto hash = typeid(T).hash_code ();

		auto removeItems = std::remove_if (m_callbacks.begin (), m_callbacks.end (),
			[hash, content, error](const std::shared_ptr<ServiceCallbackContextBase>& c) {
				if (c->contentTypeIdGet () == hash)
				{
					auto context = std::static_pointer_cast<ServiceCallbackContext<T>>(c);
					if (error)
					{
						if (context->errorCallback)
						{
							context->errorCallback (content);
						}
					}
					else
					{
						if (context->successCallback)
						{
							context->successCallback (content);
						}
					}
					return true;
				}
				return false;
			});

		m_callbacks.erase (removeItems, m_callbacks.end ());
	}
	
	std::vector<std::shared_ptr<ServiceCallbackContextBase>> m_callbacks;

protected:

	virtual const char *RequestTypeGet() = 0;

	stiHResult AddRequest(
		IN const char *szRequest,
		OUT IXML_Element **p_newElement = nullptr,
		IN const char *attr1 = nullptr,
		IN const char *attr1Val = nullptr,
		IN const char *attr2 = nullptr,
		IN const char *attr2Val = nullptr);

	stiHResult AddSubElementToRequest(
		IN const char *szElement,
		IN const char *content = nullptr,
		OUT IXML_Element **p_newElement = nullptr,
		IN const char *attr1 = nullptr,
		IN const char *attr1Val = nullptr,
		IN const char *attr2 = nullptr,
		IN const char *attr2Val = nullptr);

	/*!
	* \brief Adds a sub-element to the passed-in element
	*/
	stiHResult AddSubElementToThisElement(
		IN const IXML_Element *parentElement,
		IN const char *szElement,
		IN const char *content = nullptr,
		OUT IXML_Element **p_newElement = nullptr,
		IN const char *attr1 = nullptr,
		IN const char *attr1Val = nullptr,
		IN const char *attr2 = nullptr,
		IN const char *attr2Val = nullptr
		);

	stiHResult addTimeToSubElement (
		IXML_Element *element,
		time_t time);

	/*!
	 * \brief Specialized function to see if a request by name of tagName exists,
	 *   and if so, set the m_pMainReq to that existing request
	 */
	bool ObtainExistingRequest(
		const char *tagName);

	/*!
	 * \brief Gets the pointer to the IXML_Document for this request
	 */
	int GetIXMLDocumentPointers(
		IXML_Document **pxDoc,
		IXML_Node **pMainNode);

	IXML_Document *m_pxDoc{nullptr};        //<! points to the IXML_Document request document
	IXML_Node *m_pMainReq{nullptr};         //<! points to the main request tag in the document

	void ExpectedResultAdd (
		int nResponse);

	int requestResultGet (stiHResult hResult)
	{
		return stiIS_ERROR(hResult) ? REQUEST_ERROR : REQUEST_SUCCESS;
	}

private:

	const char* XMLPrefixGet ();

	stiHResult Initialize();

	virtual bool UseUniqueIDAsAlternate ();

	IXML_Node *m_pRootNode{nullptr};        //<! points to the root node

	std::string m_clientToken;				//<! local client token

	EstiBool m_bSSLEnabled{estiTRUE};			//<! SSL mode for this request

	unsigned int m_unId{0};
	VPServiceRequestContextSharedPtr m_context;	//<! a parameter that is sent back with the response
	EstiBool m_bRemoveUponCommunicationError{estiFALSE};	//<! If communication failure, remove this request from queue
	bool m_bRepeatOnError{false};
	unsigned int m_unAuthenticationAttempts{0};

	struct SResponse
	{
		int nResponse;
		bool bHandled;
	};

	std::list <SResponse> m_ExpectedResponses;

	bool m_retryOffline = false;
};


#endif // CSTIVPSERVICESREQUEST_H

