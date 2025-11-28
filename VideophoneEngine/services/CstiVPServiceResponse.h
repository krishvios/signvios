////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiVPServiceResponse
//
//	File Name:	CstiVPServiceResponse.h
//
//	Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTIVPSERVICERESPONSE_H
#define CSTIVPSERVICERESPONSE_H


#include "stiSVX.h"
#include "VPServiceRequestContext.h"
#include "ServiceCallbackContext.h"
#include <functional>

//
// Constants
//
#define MAX_RESPONSE_STRINGS	5

// Forward declarations
class CstiVPServiceRequest;

class CstiVPServiceResponse
{
public:

	CstiVPServiceResponse (
		CstiVPServiceRequest *request,
		EstiResult eResponseResult,
		const char *pszError);

	CstiVPServiceResponse (const CstiVPServiceResponse &) = delete;
	CstiVPServiceResponse (CstiVPServiceResponse &&) = delete;
	CstiVPServiceResponse &operator= (const CstiVPServiceResponse &) = delete;
	CstiVPServiceResponse &operator= (CstiVPServiceResponse &&) = delete;

	void destroy ();

	inline unsigned int RequestIDGet () const;

	inline EstiResult ResponseResultGet () const;
	inline void ResponseResultSet (EstiResult eResponseResult);

	template <typename ItemType>
	inline ItemType contextItemGet () const
	{
		return ::contextItemGet<ItemType> (m_pContext);
	}

	VPServiceRequestContextSharedPtr contextGet () const
	{
		return m_pContext;
	}

	inline void ErrorStringSet (const char *pszError);
	inline std::string ErrorStringGet () const;

	std::string ResponseStringGet (
		int nIndex) const;

	stiHResult ResponseStringSet (
		int nIndex,
		const char *pszResponseString);

	inline unsigned int ResponseValueGet () const;
	inline void ResponseValueSet (unsigned int unResponseValue);

	bool hasCallbackType (const std::type_info& t);

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

	std::shared_ptr<CstiVPServiceResponse> m_shared_ptr;

protected:

	virtual ~CstiVPServiceResponse () = default;


private:

	unsigned int m_unRequestID{0};
	EstiResult m_eResponseResult;
	std::string m_error;
	std::string m_responseString[MAX_RESPONSE_STRINGS];
	unsigned int m_unResponseValue;
	VPServiceRequestContextSharedPtr m_pContext;
	std::vector<std::shared_ptr<ServiceCallbackContextBase>> m_callbacks;
};


////////////////////////////////////////////////////////////////////////////////
//; CstiVPServiceResponse::ErrorStringGet
//
// Description:
//
// Abstract:
//
// Returns:
//
std::string CstiVPServiceResponse::ErrorStringGet () const
{
	stiLOG_ENTRY_NAME (CstiVPServiceResponse::ErrorStringGet);

	return m_error;
} // end CstiVPServiceResponse::ErrorStringGet


////////////////////////////////////////////////////////////////////////////////
//; CstiVPServiceResponse::ErrorStringSet
//
// Description:
//
// Abstract:
//
// Returns:
//
void CstiVPServiceResponse::ErrorStringSet (
	const char *pszError)
{
	stiLOG_ENTRY_NAME (CstiVPServiceResponse::ErrorStringSet);

	m_error = pszError;
} // end CstiVPServiceResponse::ErrorStringSet


////////////////////////////////////////////////////////////////////////////////
//; CstiVPServiceResponse::RequestIDGet
//
// Description:
//
// Abstract:
//
// Returns:
//
unsigned int CstiVPServiceResponse::RequestIDGet () const
{
	stiLOG_ENTRY_NAME (CstiVPServiceResponse::RequestIDGet);

	return (m_unRequestID);
} // end CstiVPServiceResponse::RequestIDGet

////////////////////////////////////////////////////////////////////////////////
//; CstiVPServiceResponse::ResponseResultGet
//
// Description:
//
// Abstract:
//
// Returns:
//
EstiResult CstiVPServiceResponse::ResponseResultGet () const
{
	stiLOG_ENTRY_NAME (CstiVPServiceResponse::ResponseResultGet);

	return (m_eResponseResult);
} // end CstiVPServiceResponse::ResponseResultGet


////////////////////////////////////////////////////////////////////////////////
//; CstiVPServiceResponse::ResponseResultSet
//
// Description:
//
// Abstract:
//
// Returns:
//
void CstiVPServiceResponse::ResponseResultSet (EstiResult eResponseResult)
{
	stiLOG_ENTRY_NAME (CstiVPServiceResponse::ResponseResultSet);

	m_eResponseResult = eResponseResult;
} // end CstiVPServiceResponse::ResponseResultSet


////////////////////////////////////////////////////////////////////////////////
//; CstiVPServiceResponse::ResponseValueGet
//
// Description:
//
// Abstract:
//
// Returns:
//
unsigned int CstiVPServiceResponse::ResponseValueGet () const
{
	stiLOG_ENTRY_NAME (CstiVPServiceResponse::ResponseValueGet);

	return (m_unResponseValue);
} // end CstiVPServiceResponse::ResponseValueGet

////////////////////////////////////////////////////////////////////////////////
//; CstiVPServiceResponse::ResponseValueSet
//
// Description:
//
// Abstract:
//
// Returns:
//
void CstiVPServiceResponse::ResponseValueSet (
	unsigned int unResponseValue)
{
	stiLOG_ENTRY_NAME (CstiVPServiceResponse::ResponseValueSet);

	m_unResponseValue = unResponseValue;
} // end CstiVPServiceResponse::ResponseValueSet


#endif // CSTIVPSERVICERESPONSE_H
