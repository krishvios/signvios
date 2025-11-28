///
/// \file OfflineCore.cpp
/// \brief Definition of the OfflineCore class.
///
/// This class returns Core responses based on cached data the endpoint already
///    has or default values.  This is used by Core service when the endpoint
///    cannot connect to Core.
///
/// Sorenson Communications Inc. Confidential - Do not distribute
/// Copyright 2018-2018 by Sorenson Communications, Inc. All rights reserved.
///

#include "OfflineCore.h"
#include <sstream>
#include <boost/property_tree/xml_parser.hpp>
#include "stiCoreRequestDefines.h"
#include "CstiVPService.h"
#include "stiConfDefs.h"
#include "cmPropertyNameDef.h"
#include "ClientAuthenticateResult.h"

using namespace boost::property_tree;

std::map<std::string, int> nodeTypeMappings =
{
	{ CRQ_URIListSet, CstiCoreResponse::eURI_LIST_SET_RESULT },
	{ CRQ_CallListCountGet, CstiCoreResponse::eCALL_LIST_COUNT_GET_RESULT },
	{ CRQ_CallListGet, CstiCoreResponse::eCALL_LIST_GET_RESULT },
	{ CRQ_FavoritesListGet, CstiCoreResponse::eFAVORITE_LIST_GET_RESULT },
	{ CRQ_FavoritesListSet, CstiCoreResponse::eFAVORITE_LIST_SET_RESULT },
	{ CRQ_PhoneSettingsSet, CstiCoreResponse::ePHONE_SETTINGS_SET_RESULT },
	{ CRQ_UserInterfaceGroupGet, CstiCoreResponse::eUSER_INTERFACE_GROUP_GET_RESULT },
	{ CRQ_RingGroupInfoGet, CstiCoreResponse::eRING_GROUP_INFO_GET_RESULT },
	{ CRQ_RegistrationInfoGet, CstiCoreResponse::eREGISTRATION_INFO_GET_RESULT },
	{ CRQ_UserAccountInfoGet, CstiCoreResponse::eUSER_ACCOUNT_INFO_GET_RESULT },
	{ CRQ_UserSettingsGet, CstiCoreResponse::eUSER_SETTINGS_GET_RESULT },
	{ CRQ_UserSettingsSet, CstiCoreResponse::eUSER_SETTINGS_SET_RESULT },
	{ CRQ_EmergencyAddressGet, CstiCoreResponse::eEMERGENCY_ADDRESS_GET_RESULT },
	{ CRQ_EmergencyAddressProvisionStatusGet, CstiCoreResponse::eEMERGENCY_ADDRESS_PROVISION_STATUS_GET_RESULT },
	{ CRQ_AgreementStatusGet, CstiCoreResponse::eAGREEMENT_STATUS_GET_RESULT },
	{ CRQ_ImageDownloadURLGet, CstiCoreResponse::eIMAGE_DOWNLOAD_URL_GET_RESULT },
	{ CRQ_DirectoryResolve, CstiCoreResponse::eDIRECTORY_RESOLVE_RESULT },
	{ CRQ_CallListItemAdd, CstiCoreResponse::eCALL_LIST_ITEM_ADD_RESULT },
	{ CRQ_CallListItemEdit, CstiCoreResponse::eCALL_LIST_ITEM_EDIT_RESULT },
	{ CRQ_CallListItemRemove, CstiCoreResponse::eCALL_LIST_ITEM_REMOVE_RESULT },
	{ CRQ_FavoriteAdd, CstiCoreResponse::eFAVORITE_ITEM_ADD_RESULT },
	{ CRQ_FavoriteRemove, CstiCoreResponse::eFAVORITE_ITEM_REMOVE_RESULT },
	{ CRQ_CallListSet, CstiCoreResponse::eCALL_LIST_SET_RESULT },
	{ CRQ_ImageUploadURLGet , CstiCoreResponse::eIMAGE_UPLOAD_URL_GET_RESULT },
	{ CRQ_ClientLogout , CstiCoreResponse::eCLIENT_LOGOUT_RESULT },
	{ CRQ_UserLoginCheck , CstiCoreResponse::eUSER_LOGIN_CHECK_RESULT },
	{ CRQ_ClientAuthenticate, CstiCoreResponse::eCLIENT_AUTHENTICATE_RESULT },
	{ CRQ_ContactsImport, CstiCoreResponse::eCONTACTS_IMPORT_RESULT },
	{ CRQ_UpdateVersionCheck, CstiCoreResponse::eUPDATE_VERSION_CHECK_RESULT },
};


OfflineCore::OfflineCore () :
	CstiEventQueue("OfflineCore")
{
}

void OfflineCore::login (const std::string &phoneNumber, const std::string &pin, unsigned int *punRequestId)
{
	unsigned int requestId = nextRequestIdGet();

	if (punRequestId)
	{
		*punRequestId = requestId;
	}

	auto request = new CstiCoreRequest ();
	request->requestIdSet (requestId);
	request->userLoginCheck (phoneNumber.c_str (), pin.c_str ());
	request->clientAuthenticate ("", phoneNumber.c_str (), pin.c_str ());
	handleRequest (request, [request]() {
		delete request;
	});
}


void OfflineCore::handleRequest (CstiCoreRequest *request, const std::function<void ()>& callback)
{
	PostEvent ([this, request, callback]() {
		char* pszData = nullptr;
		request->getXMLString (&pszData);
		requestParse (request, pszData);
		if (callback != nullptr)
		{
			callback ();
		}
	});
}


void OfflineCore::enable ()
{
	if (!m_enabledState)
	{
		m_enabledState = true;
		StartEventLoop ();
		remoteLog ("EventType=OfflineCore Reason=Enabling");
	}
}


void OfflineCore::disable ()
{
	if (m_enabledState)
	{
		m_enabledState = false;
		StopEventLoop ();
		remoteLog ("EventType=OfflineCore Reason=Disabling");
	}
}

bool OfflineCore::enabledGet ()
{
	return m_enabledState;
}


void OfflineCore::requestParse (CstiCoreRequest *request, const std::string &responseString)
{
	ptree tree;
	std::stringstream ss;
	ss.str (responseString);
	read_xml (ss, tree);

	auto requestElements = tree.get_child (CRQ_COREREQUEST);
	for (auto requestElement : requestElements)
	{
		nodeProcess (request, requestElement);
	}
}


void OfflineCore::nodeProcess (CstiCoreRequest *request, ptree::value_type &node)
{
	std::string nodeName = node.first;
	auto responseType = responseTypeDetermine (nodeName);
	CstiCoreResponse* response = nullptr;
	switch (responseType)
	{
		case CstiCoreResponse::eRESPONSE_UNKNOWN:
		{
			break;
		}
		case CstiCoreResponse::eUSER_LOGIN_CHECK_RESULT:
		{
			constexpr unsigned int USER_NAME_OR_PASSWORD_INVALID = 0;
			constexpr unsigned int USER_LOGGED_IN_ON_CURRENT_PHONE = 2;

			auto userIdentity = userIdentityParse (node);

			bool result = authenticate (userIdentity.phoneNumber, userIdentity.pin);
			response = new CstiCoreResponse (request, 
											 CstiCoreResponse::eUSER_LOGIN_CHECK_RESULT, 
											 estiOK, 
											 CstiCoreResponse::eNO_ERROR, 
											 "");
			if (result)
			{
				response->ResponseValueSet (USER_LOGGED_IN_ON_CURRENT_PHONE);
			}
			else
			{
				response->ResponseValueSet (USER_NAME_OR_PASSWORD_INVALID);
			}
			break;
		}
		case CstiCoreResponse::eCLIENT_AUTHENTICATE_RESULT:
		{
			auto userIdentity = userIdentityParse (node);
			
			bool result = authenticate (userIdentity.phoneNumber, userIdentity.pin);
			auto response = std::make_shared<vpe::ServiceResponse<ClientAuthenticateResult>> ();
			response->requestId = request->requestIdGet ();
			response->responseSuccessful = result;
			response->origin = vpe::ResponseOrigin::VPServices;

			if (result)
			{
				response->content.userId = propertyStringGet (PHONE_USER_ID);
				response->content.groupUserId = propertyStringGet (GROUP_USER_ID);
				
				credentialsUpdate.Emit (userIdentity.phoneNumber, userIdentity.pin);

				// Update the phone number for logging
				remoteLoggerPhoneNumberSet (userIdentity.phoneNumber);

				// Send a remote log event
				remoteLog ("EventType=OfflineCore Reason=LoggedIn");
			}
			else
			{
				response->coreErrorNumber = CstiCoreResponse::eERROR_OFFLINE_ACTION_NOT_ALLOWED;
			}
			clientAuthenticateSignal.Emit (response);
			request->callbackRaise (response, response->responseSuccessful);
			break;
		}
		case CstiCoreResponse::eCALL_LIST_ITEM_ADD_RESULT:
		{
			auto callListType = callListTypeParse (node);
			if (callListType == VAL_Blocked || callListType == VAL_Contact)
			{
				response = new CstiCoreResponse (request, 
												 responseType, 
												 estiERROR, 
												 CstiCoreResponse::eERROR_OFFLINE_ACTION_NOT_ALLOWED, 
												 "");
			}
			break;
		}
		case CstiCoreResponse::eCALL_LIST_ITEM_EDIT_RESULT:
		case CstiCoreResponse::eCALL_LIST_ITEM_REMOVE_RESULT:
		case CstiCoreResponse::eFAVORITE_ITEM_ADD_RESULT:
		case CstiCoreResponse::eFAVORITE_ITEM_REMOVE_RESULT:
		case CstiCoreResponse::eCALL_LIST_SET_RESULT:
		case CstiCoreResponse::eIMAGE_UPLOAD_URL_GET_RESULT:
		case CstiCoreResponse::eCONTACTS_IMPORT_RESULT:
		{
			response = new CstiCoreResponse (request, 
											 responseType, 
											 estiERROR, 
											 CstiCoreResponse::eERROR_OFFLINE_ACTION_NOT_ALLOWED, 
											 "");
			break;
		}
		case CstiCoreResponse::eCALL_LIST_GET_RESULT:
		{
			auto callListType = callListTypeParse (node);

			response = new CstiCoreResponse (request, 
											 responseType, 
											 estiOK, 
											 CstiCoreResponse::eNO_ERROR, 
											 nullptr);
			if (callListType == VAL_Contact)
			{
				auto contactList = new CstiContactList ();
				contactListGet(contactList);
				response->ContactListSet (contactList);
			}
			else if (callListType == VAL_Dialed)
			{
				auto callList = new CstiCallList ();
				callListGet (callList, CstiCallList::eDIALED);
				response->CallListSet (callList);
			}
			else if (callListType == VAL_Answered)
			{
				auto callList = new CstiCallList ();
				callListGet (callList, CstiCallList::eANSWERED);
				response->CallListSet (callList);
			}
			else if (callListType == VAL_Missed)
			{
				auto callList = new CstiCallList ();
				callListGet (callList, CstiCallList::eMISSED);
				response->CallListSet (callList);
			}
			else if (callListType == VAL_Blocked)
			{
				auto callList = new CstiCallList ();
				blockListGet (callList);
				response->CallListSet (callList);
			}
			else
			{
				auto callList = new CstiCallList ();
				callList->CountSet (0);
				response->CallListSet (callList);
			}
			break;
		}
		case CstiCoreResponse::eCALL_LIST_COUNT_GET_RESULT:
		{
			response = new CstiCoreResponse (request, 
											 responseType, 
											 estiOK, 
											 CstiCoreResponse::eNO_ERROR, 
											 nullptr);
			auto callList = new CstiCallList ();
			callList->CountSet (0);
			response->CallListSet (callList);
			break;
		}
		case CstiCoreResponse::eFAVORITE_LIST_GET_RESULT:
		{
			response = new CstiCoreResponse (request, 
											 responseType, 
											 estiOK, 
											 CstiCoreResponse::eNO_ERROR, 
											 nullptr);
			auto favorites = new SstiFavoriteList ();
			favoriteListGet (favorites);
			response->FavoriteListSet (favorites);
			break;
		}
		case CstiCoreResponse::eFAVORITE_LIST_SET_RESULT:
		{
			response = new CstiCoreResponse (request, 
											 responseType, 
											 estiERROR, 
											 CstiCoreResponse::eERROR_OFFLINE_ACTION_NOT_ALLOWED, 
											 nullptr);
			break;
		}
		case CstiCoreResponse::eUSER_INTERFACE_GROUP_GET_RESULT:
		{
			response = new CstiCoreResponse (request, 
											 responseType, 
											 estiOK, 
											 CstiCoreResponse::eNO_ERROR, 
											 nullptr);
			int localInterfaceMode = propertyIntegerGet (cmLOCAL_INTERFACE_MODE);
			response->ResponseStringSet (0, std::to_string(localInterfaceMode).c_str());
			break;
		}
		case CstiCoreResponse::eREGISTRATION_INFO_GET_RESULT:
		{
			SstiRegistrationInfo info;
			registrationInfoGet (info);
			response = new CstiCoreResponse (request, 
											 responseType, 
											 estiOK, 
											 CstiCoreResponse::eNO_ERROR, 
											 nullptr);
			CstiCoreResponse::SRegistrationInfo responseInfo;
			std::vector <CstiCoreResponse::SRegistrationInfo> responseList;
			for (auto i = info.PasswordMap.begin(); i != info.PasswordMap.end(); i++)
			{
				CstiCoreResponse::SRegistrationCredentials cred;
				cred.UserName = i->first;
				cred.Password = i->second;

				responseInfo.Credentials.push_back (cred);
			}
			responseInfo.eType = estiSIPS;
			responseInfo.PrivateDomain = info.PrivateDomain;
			responseInfo.PublicDomain = info.PublicDomain;
			responseList.push_back (responseInfo);
			response->RegistrationInfoListSet (&responseList);
			break;
		}
		case CstiCoreResponse::eUSER_ACCOUNT_INFO_GET_RESULT:
		{
			auto response = std::make_shared<vpe::ServiceResponse<CstiUserAccountInfo>> ();
			response->requestId = request->requestIdGet ();
			response->origin = vpe::ResponseOrigin::VPServices;
			response->responseSuccessful = userAccountInfoGet (response->content);		
			userAccountInfoReceivedSignal.Emit (response);
			request->callbackRaise (response, response->responseSuccessful);
			break;
		}
		case CstiCoreResponse::eRING_GROUP_INFO_GET_RESULT:
		{
			SstiUserPhoneNumbers phoneNumbers;
			userPhoneNumbersGet (&phoneNumbers);

			std::string userName = propertyStringGet (DefUserName);

			std::string ringGroupLocalNumber = phoneNumbers.szRingGroupLocalPhoneNumber;

			if (ringGroupLocalNumber.empty())
			{
				response = new CstiCoreResponse (request, 
												 responseType, 
												 estiERROR, 
												 CstiCoreResponse::eNOT_MEMBER_OF_RING_GROUP, 
												 "You are not a member of a ring group");
			}
			else
			{
				std::vector<CstiCoreResponse::SRingGroupInfo> ringGroupList;
				myPhoneGroupListGet (ringGroupList);

				response = new CstiCoreResponse (request, 
												 responseType, 
												 estiOK, 
												 CstiCoreResponse::eNO_ERROR, 
												 nullptr);
				response->RingGroupInfoListSet (&ringGroupList);
			}
			break;
		}
		case CstiCoreResponse::eAGREEMENT_STATUS_GET_RESULT:
		{
			response = new CstiCoreResponse (request, 
											 responseType, 
											 estiOK, 
											 CstiCoreResponse::eNO_ERROR, 
											 nullptr);
			std::vector <CstiCoreResponse::SAgreement> agreementList;
			response->AgreementListSet (&agreementList);
			break;
		}
		case CstiCoreResponse::eIMAGE_DOWNLOAD_URL_GET_RESULT:
		{
			response = new CstiCoreResponse (request, 
											 responseType, 
											 estiERROR, 
											 CstiCoreResponse::eERROR_IMAGE_NOT_FOUND, 
											 nullptr);
			break;
		}
		case CstiCoreResponse::eDIRECTORY_RESOLVE_RESULT:
		{			
			response = new CstiCoreResponse (request, 
											 responseType, 
											 estiOK, 
											 CstiCoreResponse::eNO_ERROR, 
											 nullptr);
			auto result = new CstiDirectoryResolveResult ();
			result->DialStringSet (nullptr);
			result->MaxNumberOfRingsSet (15);

			std::ostringstream uriForList;
			uriForList << "sip:\\1@" << stiSIP_UNRESOLVED_DOMAIN;
			std::list<SstiURIInfo> uriList;
			SstiURIInfo uri;
			uri.eNetwork = (SstiURIInfo::EstiNetwork)(SstiURIInfo::estiSorenson | SstiURIInfo::estiITRS);
			uri.URI = uriForList.str();
			uriList.push_back (uri);
			result->UriListSet (uriList);

			response->DirectoryResolveResultSet (result);
			break;
		}
		case CstiCoreResponse::eUPDATE_VERSION_CHECK_RESULT:
		{
			response = new CstiCoreResponse (request, 
											 responseType, 
											 estiERROR, 
											 CstiCoreResponse::eERROR_OFFLINE_ACTION_NOT_ALLOWED, 
											 nullptr);
			break;
		}
		case CstiCoreResponse::eEMERGENCY_ADDRESS_GET_RESULT:
		{
			auto response = std::make_shared<vpe::ServiceResponse<vpe::Address>> ();
			response->requestId = request->requestIdGet ();
			response->origin = vpe::ResponseOrigin::VPServices;
			response->responseSuccessful = emergencyAddressGet (response->content);
			if (!response->responseSuccessful)
			{
				response->coreErrorNumber = CstiCoreResponse::eERROR_OFFLINE_ACTION_NOT_ALLOWED;
			}
			
			emergencyAddressReceivedSignal.Emit (response);
			request->callbackRaise (response, response->responseSuccessful);
			break;
		}
		case CstiCoreResponse::eEMERGENCY_ADDRESS_PROVISION_STATUS_GET_RESULT:
		{
			auto response = std::make_shared<vpe::ServiceResponse<EstiEmergencyAddressProvisionStatus>> ();
			response->requestId = request->requestIdGet ();
			response->origin = vpe::ResponseOrigin::VPServices;
			response->responseSuccessful = emergencyAddressStatusGet (response->content);
			if (!response->responseSuccessful)
			{
				response->coreErrorNumber = CstiCoreResponse::eERROR_OFFLINE_ACTION_NOT_ALLOWED;
			}

			emergencyAddressStatusReceivedSignal.Emit (response);
			request->callbackRaise (response, response->responseSuccessful);
			break;
		}

		default:
		{
			response = new CstiCoreResponse (request, 
											 responseType, 
											 estiOK, 
											 CstiCoreResponse::eNO_ERROR, 
											 nullptr);
			break;
		}
	}
	if (response)
	{
		sendCoreResponse.Emit (response);
	}
}


std::string OfflineCore::callListTypeParse (ptree::value_type &node)
{
	auto path = (std::string)TAG_CallList;
	path.append (".<xmlattr>.");
	path.append (ATT_Type);
	return node.second.get<std::string> (path);
}


OfflineCore::UserIdentity OfflineCore::userIdentityParse (boost::property_tree::ptree::value_type &node)
{
	auto userIdentity = node.second.get_child (TAG_UserIdentity);
	UserIdentity result;
	result.phoneNumber = userIdentity.get<std::string> (TAG_PhoneNumber);
	result.pin = userIdentity.get<std::string> (TAG_Pin);
	return result;
}


CstiCoreResponse::EResponse OfflineCore::responseTypeDetermine (const std::string& nodeName)
{
	auto iter = nodeTypeMappings.find(nodeName);

	if(iter != nodeTypeMappings.end())
	{
		return static_cast<CstiCoreResponse::EResponse>(iter->second);
	}

	return CstiCoreResponse::eRESPONSE_UNKNOWN;
}
