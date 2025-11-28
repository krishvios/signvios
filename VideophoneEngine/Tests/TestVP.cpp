// TestVP.cpp : Defines the entry point for the console application.
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved


#define stiDEBUGGING_TOOLS

#include "CstiCoreService.h"
#include "CstiMessageService.h"
#include "CstiStateNotifyService.h"
#include "PropertyManager.h"

#include <iostream>
#include <sstream>
#include <ctime>

#ifdef WIN32
#define BASE_DIR "C:"
#elif defined __APPLE__
#define BASE_DIR "/tmp"
#else
#define BASE_DIR PERSISTENT_DATA "/pm"
#endif

std::string name;
std::string phoneNumber;
std::string password;

stiHResult CoreServiceCallback(
	int32_t n32MessageParam, // holds the type of message
	size_t EventParam,	// holds data specific to the function to call
	size_t pParam)				// points to the instantiated CCI object
{
	EstiCmMessage eMessage = (EstiCmMessage)n32MessageParam;

	std::cout << "Message received... ";

	switch (eMessage)
	{
	case estiMSG_CB_ERROR_REPORT:
		{
			std::cout << " - Error report... ";
			CstiCoreResponse* poResponse = (CstiCoreResponse*)EventParam;
			if(poResponse)
			{
				std::cout << "Error report received: " << poResponse->ErrorStringGet() << "\n";
			}
			break;
		}
	case estiMSG_CORE_SERVICE_CONNECTING:
		{
			std::cout << "Core service connecting. "  << "\n";
			break;
		}
	case estiMSG_CORE_SERVICE_CONNECTED:
		{
			std::cout << "Core service connected. "  << "\n";
			break;
		}
	case estiMSG_STATE_NOTIFY_SERVICE_CONNECTING:
		{
			std::cout << "State notify service connecting. "  << "\n";
			break;
		}
	case estiMSG_STATE_NOTIFY_SERVICE_CONNECTED:
		{
			std::cout << "State notify service connected. "  << "\n";
			break;
		}
	case estiMSG_MESSAGE_SERVICE_CONNECTING:
		{
			std::cout << "Message service connecting. "  << "\n";
			break;
		}
	case estiMSG_MESSAGE_SERVICE_CONNECTED:
		{
			std::cout << "Message service connected. "  << "\n";
			break;
		}
	case estiMSG_CB_SERVICE_SHUTDOWN:
		{
			std::cout << "Service shutdown... "  << "\n";
			break;
		}
	case estiMSG_CB_CURRENT_TIME_SET:
		{
			time_t rawtime = (time_t)EventParam;
			time ( &rawtime );
			std::cout << "Current time set " << ctime (&rawtime) << "\n";
			break;
		}
	case estiMSG_STATE_NOTIFY_RESPONSE:
		{
			CstiStateNotifyResponse* poResponse = (CstiStateNotifyResponse*)EventParam;
			CstiStateNotifyResponse::EResponse eResponse = poResponse->ResponseGet();
			std::cout << "State notify response " << eResponse;
			switch(eResponse)
			{
			case CstiStateNotifyResponse::eHEARTBEAT_RESULT:
				std::cout << " heartbeat result";
				break;
			}
			std::cout << "\n";
			delete poResponse;
			break;
		}
	case estiMSG_MESSAGE_RESPONSE:
		{
			std::cout << "Message response received \n";
			CstiMessageResponse* poResponse = (CstiMessageResponse*)EventParam;
			
			if (NULL != poResponse)
			{
				CstiMessageResponse::EResponse eResponse = poResponse->ResponseGet();

				switch (eResponse)
				{
				case CstiMessageResponse::eMESSAGE_LIST_GET_RESULT:
					{
						CstiMessageList *messages = poResponse->MessageListGet();
						std::cout << "Message list get result - Count: " << messages->CountGet() << "\n";
						CstiMessageRequest *request = new CstiMessageRequest();
						for(unsigned int i=0; i<messages->CountGet(); i++)
						{
							CstiMessageListItem *message = messages->ItemGet(i);
							std::cout << "\nMessage - ID: " << message->ItemIdGet() 
								<< " Dial String: " << message->DialStringGet() 
								<< " Duration: " << message->DurationGet();
							request->RetrieveMessageKey (message->ItemIdGet(), message->NameGet(), message->DialStringGet());
						}
						g_oMessageService.RequestSend (request);
						break;
					}
				case CstiMessageResponse::eRETRIEVE_MESSAGE_KEY_RESULT:
					{
						std::cout << "\nMessage key get result: " << //poResponse->m_pszMessageResponseGUID;
							poResponse->ResponseStringGet(CstiMessageResponse::eRESPONSE_STRING_GUID);
						break;
					}
				case CstiMessageResponse::eRESPONSE_ERROR:
					{
						std::cout << "\nError: " << poResponse->ErrorStringGet();
						break;
					}
				}
			}
			delete poResponse;
			break;
		}
	case estiMSG_CORE_RESPONSE:
		{
			CstiCoreResponse* poResponse = (CstiCoreResponse*)EventParam;
			std::cout << "Core response received \n";

			if (NULL != poResponse)
			{
				switch (poResponse->ResponseGet ())
				{
				case CstiCoreResponse::eDIRECTORY_RESOLVE_RESULT:
					{
						std::cout << "Client authenticate result - ";
						if(poResponse->ErrorStringGet())
							std::cout << "Error: " << poResponse->ErrorStringGet() << "\n";
						else
						{
							CstiDirectoryResolveResult* result = poResponse->DirectoryResolveResultGet();
							std::cout << " Dial String: " << result->DialStringGet() << "\n";
						}
						break;
					}
				case CstiCoreResponse::eCLIENT_AUTHENTICATE_RESULT:
					{
						std::cout << "Client authenticate result - ";
						
						if(poResponse->ErrorStringGet())
							std::cout << "Error: " << poResponse->ErrorStringGet() << "\n";
						else
							std::cout << "Okay\n";
						break;
					}
				case CstiCoreResponse::eCALL_LIST_COUNT_GET_RESULT:
					{
						std::cout << "Call list count get result" << "\n";
						break;
					}
				case CstiCoreResponse::eCALL_LIST_GET_RESULT:
					{
						std::cout << "Call list get result" << "\n";
						CstiCallList *callList = poResponse->CallListGet();
						for(unsigned int i=0; callList&&(i<callList->CountGet()); i++)
						{
							CstiCallListItem *callListItem = callList->ItemGet(i);
							std::cout << "Call list item - Name: " << callListItem->NameGet() << " Number: " <<callListItem->DialStringGet() << "\n";
						}
						break;
					}
				case CstiCoreResponse::ePHONE_ACCOUNT_CREATE_RESULT:
					{
						std::cout << "Acount Create result" << "\n";
						if(poResponse->ErrorStringGet())
							std::cout << "Error: " << poResponse->ErrorStringGet() << "\n";
						break;
					}
				}
			}
			delete poResponse;
			break;
		}
	}

	return stiRESULT_SUCCESS;
}


stiHResult HTTPServiceCallback (
	int32_t n32Param1,
	size_t n32Param2,
	size_t pParam)
{
	return stiRESULT_SUCCESS;
}

void addCallListItem()
{
	CstiCallListItem *callListItem = new CstiCallListItem ();
	callListItem->DialMethodSet(estiDM_BY_DIAL_STRING);
	callListItem->DialStringSet("18012879801");
	callListItem->NameSet("Test");
	
	time_t calLTime = TimeConvert("01/01/2010 1:15:00 PM");
	callListItem->CallTimeSet(calLTime);
	callListItem->DurationSet(30);
	callListItem->InSpeedDialSet(estiTRUE);
	CstiCoreRequest *request = new CstiCoreRequest ();
	request->callListItemAdd (callListItem, CstiCallList::eSPEED_DIAL);
	EstiResult hResult = g_oCoreService.RequestSend (request, 0);
}

void getCallListCount()
{
	CstiCoreRequest	*request = new CstiCoreRequest ();
	int result = request->callListCountGet (CstiCallList::eSPEED_DIAL);
	EstiResult hResult = g_oCoreService.RequestSend (request, 0);
}

void getCallList()
{
	CstiCoreRequest *callListRequest = new CstiCoreRequest ();
	int result = callListRequest->callListGet (CstiCallList::eSPEED_DIAL, 0, 0, 10, CstiCallList::eTIME, CstiList::SortDirection::DESCENDING, -1, false, true);
	EstiResult hResult = g_oCoreService.RequestSend (callListRequest, 0);
}

void getMessageList()
{
	std::cout <<"\nSending message request..";
	CstiMessageRequest *request = new CstiMessageRequest ();
	request->MessageListGet (0, 10);
	g_oMessageService.RequestSend (request);
}

void propertyManagerTest()
{
	WillowPM::PropertyManager *pm = WillowPM::PropertyManager::getInstance();
	pm->SetFilename(BASE_DIR "/FileName.xml");
	pm->SetDefaultFilename(BASE_DIR "/DefaultFileName.xml");
	pm->SetUpdateFilename(BASE_DIR "/UpdateFileName.xml");
	WillowPM::PropertyManager::getInstance ()->init ("test");

	char *ctemp = 0;
	pm->propertyGet("cmCoreServiceUrl1", &ctemp, "error");
	pm->propertyGet("cmCoreServiceUrl1", &ctemp);
	int result1 = pm->propertyDefaultSet("SomeName", 999, WillowPM::PropertyManager::Persistent);
	pm->propertySet("SomeOtherName", 666, WillowPM::PropertyManager::Persistent);
	pm->propertySet("StringName", "stringvalue", WillowPM::PropertyManager::Persistent);
}

bool m_bServicesStarted = false;

stiHResult ServicesStartup ()
{
	//stiDEBUG_TOOL (g_stiCCIDebug, stiTrace ("CstiCCI::ServicesStartup\n"););
	EstiInterfaceMode m_eLocalInterfaceMode = estiSTANDARD_MODE;

	stiHResult hResult = stiRESULT_TASK_ALREADY_STARTED;

	if (!m_bServicesStarted)
	{
		// If we are in ported mode then start some of the services in a paused state.
		bool bPause = false;

		if (estiPORTED_MODE == m_eLocalInterfaceMode)
		{
			bPause = true;
		}

		// Make sure the HTTP task is starting before starting the other services
		hResult = g_oHTTPTask.Startup (HTTPServiceCallback, 0);

		if (stiERROR_CODE (hResult) != stiRESULT_TASK_ALREADY_STARTED)
		{
			stiTESTRESULT ();
		}

		// Start up state notify
		hResult = g_oStateNotifyService.Startup (bPause, CoreServiceCallback, 0);

		if (stiERROR_CODE (hResult) != stiRESULT_TASK_ALREADY_STARTED)
		{
			stiTESTRESULT ();
		}
		// Start core services.
		hResult = g_oCoreService.Startup (bPause, CoreServiceCallback, 0);

		if (stiERROR_CODE (hResult) != stiRESULT_TASK_ALREADY_STARTED)
		{
			stiTESTRESULT ();
		}

		if (estiPORTED_MODE != m_eLocalInterfaceMode)
		{
			g_oCoreService.Connect ();
		}

		// Start up the message service
		hResult = g_oMessageService.Startup (bPause, CoreServiceCallback, 0);

		if (stiERROR_CODE (hResult) != stiRESULT_TASK_ALREADY_STARTED)
		{
			stiTESTRESULT ();
		}

		// Indicate that the services have been started.
		m_bServicesStarted = true;
	}

STI_BAIL:

	return hResult;
}

stiHResult ServicesInitialize (std::string uniqueId)
{
	// Initialize the core service subsystem
	stiHResult hResult = g_oCoreService.Initialize (uniqueId.c_str(), "VP-200");
	g_oCoreService.SSLFlagSet( eSSL_OFF );//eSSL_ON_WITHOUT_FAILOVER);//(eSSL_OFF);
	std::string serviceContacts[2] = {"http://10.20.130.13/VPServices/CoreRequest.aspx", "http://10.20.130.13/VPServices/CoreRequest.aspx"};
	g_oCoreService.ServiceContactsSet (serviceContacts);

	hResult = g_oStateNotifyService.Initialize (uniqueId.c_str());
	g_oStateNotifyService.SSLFlagSet( eSSL_OFF );
	std::string notifyContacts[2] = {"http://10.20.130.13/VPServices/StateNotifyRequest.aspx", "http://10.20.130.13/VPServices/StateNotifyRequest.aspx"};
	g_oStateNotifyService.ServiceContactsSet(notifyContacts);
	g_oStateNotifyService.HeartbeatDelaySet (30);

	hResult = g_oMessageService.Initialize (uniqueId.c_str());
	g_oMessageService.SSLFlagSet( eSSL_OFF );
	std::string messageContacts[2] = {"http://10.20.130.13/VPServices/MessageRequest.aspx", "http://10.20.130.13/VPServices/MessageRequest.aspx"};
	g_oMessageService.ServiceContactsSet (messageContacts);

	return hResult;
}

int main(int argc, char* argv[])
{

	if (argc < 4) return 1;
	name = argv[1];
	phoneNumber = argv[2];
	password = argv[3];

	EstiResult eResult = estiOK;
	stiHResult hResult = stiRESULT_SUCCESS;

	eResult = stiOSInit();
	
	//EstiResult eResult = stiErrorLogSystemInit ();

	int pnNSCount = 0;
	stiOSGetDNSCount(&pnNSCount);
	printf("\nDNS Count: %d", pnNSCount);

	char szDNSAddress[16];
	stiGetDNSAddress(0, szDNSAddress, sizeof(szDNSAddress));
	printf("\nDNS Address: %s", szDNSAddress);

	for (int useWireless = 0; useWireless <= 1; useWireless++)
	{
	stiOSSetNIC(useWireless);

	char acIPAddress[16]; acIPAddress[0] = '\0';
	stiGetLocalIp(acIPAddress, sizeof(acIPAddress));
	printf("\nIP Address: %s", acIPAddress);

	char macAddress[32];
	eResult = stiMACAddressGet (macAddress);
	printf("\nMac Address: %s", macAddress);

	char acNetworkMask[32]; acNetworkMask[0] = '\0';
	stiGetNetSubnetMaskAddress(acNetworkMask, sizeof(acNetworkMask));
	printf("\nNetwork Mask: %s", acNetworkMask);

	char acGatewayAddres[32]; acGatewayAddres[0] = '\0';
	stiGetDefaultGatewayAddress (acGatewayAddres, sizeof(acGatewayAddres));
	printf("\nGateway Address: %s", acGatewayAddres);
	}

	EstiBool bValid = stiMACAddressValid ();
	printf("\nMac Address Valid?: %s", (bValid) ? "YES" : "NO");

	char macaddress[32];
	eResult = stiOSGetUniqueID (macaddress);
	printf("\nUnique ID: %s", macaddress);

	std::string uniqueId = macaddress;
	hResult = ServicesInitialize (uniqueId);
	hResult = ServicesStartup ();
	g_oCoreService.Login (phoneNumber.c_str(), password.c_str(), NULL, NULL);

	char inputChar = 0;
	
	do{
		std::cout << "\n\nCore Services Test\n";
		std::cout << "\t a: Add call list item\n";
		std::cout << "\t c: Get call list item\n";
		std::cout << "\t d: Directory resolve\n";
		std::cout << "\t l: Get message list\n";
		std::cout << "\t p: Property manager test\n";
		std::cout << "\t u: User associate\n";
		std::cout << "\t x: Exit\n";

		std::cin >> inputChar;
		switch(inputChar)
		{
		case 'a':
			{
				addCallListItem ();
				break;
			}
		case 'c':
			{
				getCallList ();
				break;
			}
		case 'd':
			{
				CstiCoreRequest *request = new CstiCoreRequest ();
				EstiDialMethod dialMethod = estiDM_BY_DS_PHONE_NUMBER;//estiDM_BY_DIAL_STRING;
				request->directoryResolve("18017771005", dialMethod);
				eResult = g_oCoreService.RequestSend ( request, 0);
				break;
			}
		case 'l':
			{
				getMessageList ();
				break;
			}
		case 'p':
			{
				propertyManagerTest ();
				break;
			}
		case 'u':
			{
				CstiCoreRequest *request = new CstiCoreRequest ();
				request->userAccountAssociate (name.c_str(), phoneNumber.c_str(), password.c_str(), CstiUserAccountInfo::eVP200_PLAN, estiFALSE, estiFALSE);
				eResult = g_oCoreService.RequestSend (request, 0);
				break;
			}
		case '1':
			{
				g_oCoreService.ClientLogout(NULL, NULL);
				phoneNumber = "18012879801";
				//g_oCoreService.Login(phoneNumber.c_str(), password.c_str(), 0);
				CstiCoreRequest *request = new CstiCoreRequest ();
				request->userAccountAssociate (name.c_str(), phoneNumber.c_str(), password.c_str(), CstiUserAccountInfo::eVP200_PLAN, estiFALSE, estiFALSE);
				eResult = g_oCoreService.RequestSend (request, 0);
				g_oCoreService.Login(phoneNumber.c_str(), password.c_str(), NULL, NULL);
				stiOSTaskDelay(1000);
				getMessageList ();
				break;
			}
		case '2':
			{
				g_oCoreService.ClientLogout(NULL, NULL);
				phoneNumber = "18012879800";
				//g_oCoreService.Login(phoneNumber.c_str(), password.c_str(), 0);
				CstiCoreRequest *request = new CstiCoreRequest ();
				request->userAccountAssociate (name.c_str(), phoneNumber.c_str(), password.c_str(), CstiUserAccountInfo::eVP200_PLAN, estiFALSE, estiFALSE);
				eResult = g_oCoreService.RequestSend (request, 0);
				g_oCoreService.Login(phoneNumber.c_str(), password.c_str(), NULL, NULL);
				stiOSTaskDelay(1000);
				getMessageList ();
				break;
			}
		}
	}while(inputChar!='x');

	CstiCoreRequest *logoutRequest = new CstiCoreRequest ();
	logoutRequest->clientLogout();
	g_oCoreService.RequestSend(logoutRequest, 0);

	stiHResult hCurrent = g_oMessageService.Shutdown ();
	hCurrent = g_oStateNotifyService.Shutdown ();
	hCurrent = g_oCoreService.Shutdown ();
	hCurrent = g_oHTTPTask.Shutdown ();

	std::cout << "\nWaiting for message service shutdown...";
	g_oMessageService.WaitForShutdown ();
	std::cout << "\nWaiting for state notify service shutdown...";
	g_oStateNotifyService.WaitForShutdown ();
	std::cout << "\nWaiting for core service shutdown...";
	g_oCoreService.WaitForShutdown ();
	std::cout << "\nWaiting for HTTPTask service shutdown...";
	g_oHTTPTask.WaitForShutdown ();

	return 0;
}

