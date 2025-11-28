// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "IstiVideophoneEngine.h"
#include "IstiCall.h"
#include "stiError.h"
#include "PropertyManager.h"
#include "cmPropertyNameDef.h"
#include "CstiCoreRequest.h"
#include "CstiCoreResponse.h"
#include "CstiCallListItem.h"
#include "CstiMessageRequest.h"
#include "CstiMessageResponse.h"
#include "IstiMessageViewer.h"
#include "IstiBlockListManager.h"
#include "CstiBlockListManager.h"
#include "CstiCCI.h"
#include "CstiVRCLClient.h"
#include "IVideoInputVP2.h"
#include "IstiContactManager.h"
#include "IstiContact.h"
#include "ContactListItem.h"
#include "IstiVideoOutput.h"
//#include "stiTrace.h"

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Message.h>
#include <regex.h>
#include <list>
#include <ctime>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <fstream>
#include <condition_variable>
#include <mutex>
#include <chrono>

using namespace WillowPM;
using namespace std;


// These configuration values can be easily over-ridden by creating the file Test.cfg
// in the directory where this application runs.  It should look like:
/*

# Phone number and IP address for the remote device to call and/or make VRCL connections with
UNIT_TEST_VIDEOPHONE_NUMBER 18012874456
UNIT_TEST_VIDEOPHONE_ADDRESS 10.20.136.102

# Account phone number and password to connect to core with (soft-phone)
ACCOUNT_PHONE_NUMBER 18011114444
ACCOUNT_PASSWORD 1234

# Hold server address (can be the same address as the UNIT_TEST_VIDEOPHONE_NUMBER
HOLDSERVER_ADDRESS 10.20.136.102

# Email addresses to use for testing.  Give two distinct addresses.
EMAIL_ADDRESS1 test1@company.com
EMAIL_ADDRESS2 test2@company.com

*/


//
//  Blank lines and lines beginning with '#' are ignored.
//  Those lines that begin with the keywords above will replace the default values
//  with the value in the file.
//
std::string g_UnitTestVideophoneNumber = "18012874456";
std::string g_UnitTestVideophoneAddress = "10.20.136.102";
std::string g_AccountPhoneNumber = "18011114444";
std::string g_AccountPassword = "1234";
std::string g_HoldserverAddress = "209.169.226.175";
std::string g_EmailAddress1 = "test1@company.com";
std::string g_EmailAddress2 = "test2@company.com";
bool g_bHaveConfiguration = false;

#define TEST_TIMEOUT			1 * 60	// Allow a test up to one minute to succeed.

#define SPECIAL_CALLER_ID_CHARS "\"'<>&#!*[]()%,_.@"

#define VALID_GUID 				"SignMailGreeting/viewGreeting.aspx?greetingID=00000000-0000-0000-0000-000000000100"
#define INVALID_GUID 			"SignMailGreeting/viewGreeting.aspx?greetingID=00000000-0000-0000-0000-000000000000"
#define MESSAGE_SERVER_IP		"engv-vms-dwn.sorensoncomm.sorenson.com"

#define BLOCKED_USER_NAME		"Blocked Person"
#define BLOCKED_USER_NUMBER		"18006661212"

#define NEW_BLOCKED_USER_NUMBER	"18005551212"
#define NEW_BLOCKED_USER_NAME	"New Blocked Person"

#define CONTACT_USER_NAME       "Richard Shields" SPECIAL_CALLER_ID_CHARS
#define CONTACT_USER_NUMBER     "12345678901"

#define LOCAL_NAME 				"Unit Tester"
#define VRS_HEARING_NUMBER		"18009998877"

#define TEST_PROPERTY_TABLE						"PropertyTables/TestPropertyTable.xml"
#define TEST_PROPERTY_TABLE_NAT					"PropertyTables/TestPropertyTable-NAT.xml"
#define TEST_PROPERTY_TABLE_NO_PLAYER_CONTROLS	"PropertyTables/TestPropertyTable-PlayerControlsDisabled.xml"
#define TEST_PROPERTY_TABLE_INTERCEPTOR			"PropertyTables/TestPropertyTable-Interceptor.xml"
#define TEST_PROPERTY_TABLE_SIGNMAIL			"PropertyTables/TestPropertyTable-SignMail.xml"
#define ACTUAL_PROPERTY_TABLE	"./data/pm/PropertyTable.xml"
#define TEST_BACKUP_EXTENSION	".tstbak"

// Interceptor rules files and destination.
#define INTERCEPTOR_RULES_DESTINATION	"/mnt/InterceptorRules/interceptorRules.txt"
#define INTERCEPTOR_PASS_THROUGH "InterceptorRules/PassThrough.txt"
#define INTERCEPTOR_EMPTY_MESSAGE_LIST "InterceptorRules/EmptyMessageList.txt"
#define INTERCEPTOR_NOTIFY_NEW_MESSAGE "InterceptorRules/NotifyNewMessage.txt"
#define INTERCEPTOR_DELETE_MESSAGE "InterceptorRules/DeleteMessage.txt"
#define INTERCEPTOR_MESSAGE_LIST_CHANGED "InterceptorRules/MessageListChanged.txt"
#define INTERCEPTOR_LOAD_MESSAGE_LIST_CNT11 "InterceptorRules/LoadMessageListCnt11.txt"
#define INTERCEPTOR_LOAD_MESSAGE_LIST_NEW_MESSAGES "InterceptorRules/LoadMessageListNewMessages.txt"
#define INTERCEPTOR_MODIFY_MESSAGE_VIEWED_PAUSE_POINT "InterceptorRules/ModifyMsg-Viewed-PausePoint.txt"
#define INTERCEPTOR_COPY_DELAY 2
		
#define FILE_COPY(f,t) CopyFile(f, t)

#define UPDATE_SERVER			"eng-autobuild.sorenson.com"
#define UPDATE_MPU_COMBO_FILE	"ntouch/1.0.5/latest/ntouch_mpu.combo"
#define UPDATE_RCU_FLIMG_FILE	"ntouch/1.0.5/latest/ntouch_rcu.flimg"

#define LOCAL_NAME_TEST_STRING "Local Name Test"

#define MESSAGE_NAME_BUFFER_SIZE 256

#define LOCAL_VRCL_PORT	15326	// The local port has been changed so it does not conflict with softphones running on the same machine

const uint16_t ShareTextString[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', '\0'};

class IstiVideophoneEngineTest;

CPPUNIT_TEST_SUITE_REGISTRATION (IstiVideophoneEngineTest);

struct SstiMessage
{
	int32_t n32Message;
	size_t Param;
};

struct SstiMessageManagerItem
{
	CstiItemId itemId;
	int nCategoryType;
	char szMessageName[MESSAGE_NAME_BUFFER_SIZE];
	int nMessagType;
	std::list<SstiMessageManagerItem *> lItemList;
};

struct ProtocolInfo
{
	const char *pszProtocolName;
	int nPort;
};

enum EMessageType
{
	MSGTYPE_CATEGORY,
	MSGTYPE_SUBCATEGORY,
	MSGTYPE_MESSAGE
};

enum EFilePlayerTestState
{
	FPTS_PLAY,
	FPTS_PAUSE,
	FPTS_PAUSED_PLAY,
	FPTS_SKIPBEGIN,
	FPTS_SKIPFORWARD,
	FPTS_SKIPBACK,
	FPTS_SKIPEND,
	FPTS_TEST_FINISHED
};

void CopyFile (
	const char *pszSrc,
	const char *pszDst)
{
	int nResult = 0;
	char szBuffer[512];

	mkdir ("./data", 0700);
	mkdir ("./data/pm", 0700);

	sprintf (szBuffer, "cp -fr %s %s", pszSrc, pszDst);

	nResult = system (szBuffer);

	CPPUNIT_ASSERT_MESSAGE (szBuffer, 0 == nResult);
}

struct Test
{
	Test (
		const char *pszTestName,
		const char *pszPropertyTable)
	:
		Name(pszTestName),
		PropertyTable(pszPropertyTable)
	{
	}

	Test (const Test &Other)
	{
		Name = Other.Name;
		PropertyTable = Other.PropertyTable;
	}

	std::string Name;
	std::string PropertyTable;
};

#define LOAD_TEST(TestName, PropTable) \
	m_Tests.push_back(Test(#TestName, PropTable)); \
	CPPUNIT_TEST(TestName);

class IstiVideophoneEngineTest : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( IstiVideophoneEngineTest );
	
	LOAD_TEST (CreateVideophoneEngine, TEST_PROPERTY_TABLE);
	LOAD_TEST (CallDial, TEST_PROPERTY_TABLE);
#if 0
	LOAD_TEST (userAccountAssociate, TEST_PROPERTY_TABLE);
	LOAD_TEST (CoreLogin, TEST_PROPERTY_TABLE);
	LOAD_TEST (callListCountGet, TEST_PROPERTY_TABLE);
	LOAD_TEST (BlockListAdd, TEST_PROPERTY_TABLE);
	LOAD_TEST (BlockListGetByNumber, TEST_PROPERTY_TABLE);
	LOAD_TEST (BlockListGetByIndex, TEST_PROPERTY_TABLE);
	LOAD_TEST (BlockListGetById, TEST_PROPERTY_TABLE);
	LOAD_TEST (BlockListItemEdit, TEST_PROPERTY_TABLE);
	LOAD_TEST (BlockListDelete, TEST_PROPERTY_TABLE);
	LOAD_TEST (BlockListDial, TEST_PROPERTY_TABLE);
#endif

#if 0
	LOAD_TEST (ContactListGet, TEST_PROPERTY_TABLE);
	LOAD_TEST (ContactListClear, TEST_PROPERTY_TABLE);
#endif

	LOAD_TEST (ContactManagerListItemAdd, TEST_PROPERTY_TABLE);
	LOAD_TEST (ContactManagerListItemGetByName, TEST_PROPERTY_TABLE);
	LOAD_TEST (ContactManagerListItemGetByPhoneNumber, TEST_PROPERTY_TABLE);
	LOAD_TEST (ContactManagerListItemGetById, TEST_PROPERTY_TABLE);
	LOAD_TEST (ContactManagerListItemRemove, TEST_PROPERTY_TABLE);
	LOAD_TEST (ContactManagerListClear, TEST_PROPERTY_TABLE);
	LOAD_TEST (ContactManagerImport, TEST_PROPERTY_TABLE);

#if 0
	LOAD_TEST (MissedCallListClear, TEST_PROPERTY_TABLE);
	LOAD_TEST (DialedCallListClear, TEST_PROPERTY_TABLE);
	LOAD_TEST (AnsweredCallListClear, TEST_PROPERTY_TABLE);
	LOAD_TEST (MissedCallListGet, TEST_PROPERTY_TABLE);
	LOAD_TEST (DialedCallListGet, TEST_PROPERTY_TABLE);
	LOAD_TEST (AnsweredCallListGet, TEST_PROPERTY_TABLE);
	LOAD_TEST (MessageListGet, TEST_PROPERTY_TABLE);
	LOAD_TEST (userAccountInfoGet, TEST_PROPERTY_TABLE);
	LOAD_TEST (signMailEmailPreferencesSet, TEST_PROPERTY_TABLE);
	LOAD_TEST (getUserDefaults, TEST_PROPERTY_TABLE);
	LOAD_TEST (primaryUserExists, TEST_PROPERTY_TABLE);
	LOAD_TEST (userSettingsGet, TEST_PROPERTY_TABLE);
	LOAD_TEST (updateVersionCheck, TEST_PROPERTY_TABLE);
#endif
	LOAD_TEST (CallAnswer, TEST_PROPERTY_TABLE);
	LOAD_TEST (CallReject, TEST_PROPERTY_TABLE);
//	LOAD_TEST (RemoteAnswerWithPrivacy, TEST_PROPERTY_TABLE);
//	LOAD_TEST (RemoteCallReject, TEST_PROPERTY_TABLE);
#if 0
	LOAD_TEST (RequestVideo, TEST_PROPERTY_TABLE);
	LOAD_TEST (RequestVideoFail, TEST_PROPERTY_TABLE);
	LOAD_TEST (PlayVideo, TEST_PROPERTY_TABLE);
	LOAD_TEST (PlayVideoNoPlayerControls, TEST_PROPERTY_TABLE_NO_PLAYER_CONTROLS);
	LOAD_TEST (RecordSignMail, TEST_PROPERTY_TABLE_SIGNMAIL);
	LOAD_TEST (RelayLanguageListGet, TEST_PROPERTY_TABLE);
#endif
//	LOAD_TEST (UpdateDetermine, TEST_PROPERTY_TABLE);
//	LOAD_TEST (UpdateFirmware);
//	LOAD_TEST (VRCL, TEST_PROPERTY_TABLE);
#if 0
	LOAD_TEST (VRCLDial, TEST_PROPERTY_TABLE);
	LOAD_TEST (VRCLDialVRS, TEST_PROPERTY_TABLE);
	LOAD_TEST (VRCLDialVRSVCO, TEST_PROPERTY_TABLE);
	LOAD_TEST (VRCLDialRemoteReject, TEST_PROPERTY_TABLE);
	LOAD_TEST (VRCLAnswer, TEST_PROPERTY_TABLE);
#endif
//	LOAD_TEST (AutoReject, TEST_PROPERTY_TABLE);
#if 0
	LOAD_TEST (LocalNameSet, TEST_PROPERTY_TABLE);
	LOAD_TEST (UserSettingsSet, TEST_PROPERTY_TABLE);
	LOAD_TEST (PhoneSettingsSet, TEST_PROPERTY_TABLE);
	LOAD_TEST (RingsBeforeGreeting, TEST_PROPERTY_TABLE);
#ifdef stiMESSAGE_MANAGER
//	LOAD_TEST (LoadCategoriesNoMessages, TEST_PROPERTY_TABLE_INTERCEPTOR);
//	LOAD_TEST (LoadCategoriesMessages, TEST_PROPERTY_TABLE_INTERCEPTOR);
//	LOAD_TEST (UpdateMessageItems, TEST_PROPERTY_TABLE_INTERCEPTOR);
//	LOAD_TEST (DeleteMessageItems, TEST_PROPERTY_TABLE_INTERCEPTOR);
	LOAD_TEST (MessageInfoCoverage, TEST_PROPERTY_TABLE);
#endif
#endif
	CPPUNIT_TEST_SUITE_END();

public:
	IstiVideophoneEngineTest ();

	void setUp();
	void tearDown();

	void ContactManagerImport ();
	void CreateVideophoneEngine();
	void userAccountAssociate ();
	void CoreLogin ();
	void callListCountGet ();
	void BlockListAdd ();
	void BlockListDelete ();
	void BlockListDial ();
	void BlockListGetByNumber ();
	void BlockListGetByIndex ();
	void BlockListGetById ();
	void BlockListItemEdit ();
	void ContactListGet ();
	void ContactListClear ();

	void ContactManagerListItemAdd ();
	void ContactManagerListItemGetByName();
	void ContactManagerListItemGetByPhoneNumber();
	void ContactManagerListItemGetById();
	void ContactManagerListItemRemove();
	void ContactManagerListClear();
	void MissedCallListClear ();
	void DialedCallListClear ();
	void AnsweredCallListClear ();
	void MissedCallListGet ();
	void DialedCallListGet ();
	void AnsweredCallListGet ();
	void MessageListGet ();
	void userAccountInfoGet ();
	void signMailEmailPreferencesSet ();
	void getUserDefaults ();
	void primaryUserExists ();
	void userSettingsGet ();
	void updateVersionCheck ();
	void CallDial ();
	void CallAnswer ();
	void CallReject ();
	void RemoteAnswerWithPrivacy ();
	void RemoteCallReject ();
	void RequestVideo();
	void RequestVideoFail();
	void PlayVideo();
	void PlayVideoNoPlayerControls();
	void RecordSignMail();
	void RelayLanguageListGet();
	void UpdateDetermine ();
	void UpdateFirmware ();
	void VRCL ();
	void VRCLDial ();
	void VRCLDialVRS ();
	void VRCLDialVRSVCO ();
	void VRCLDialRemoteReject ();
	void VRCLAnswer ();
	void AutoReject ();
	void LocalNameSet ();
	void UserSettingsSet ();
	void PhoneSettingsSet ();
	void RingsBeforeGreeting ();
#ifdef stiMESSAGE_MANAGER
	void LoadCategoriesNoMessages ();
	void LoadCategoriesMessages ();
	void UpdateMessageItems ();
	void DeleteMessageItems ();
	void MessageInfoCoverage ();
#endif

private:

	IstiVideophoneEngine *m_pVE;
	bool m_bStarted;
	std::mutex m_EngineCallbackMutex;
	std::condition_variable m_MessageCond;
	std::string m_LocalIPv4Address;
	std::string m_LocalIPv6Address;
	std::list<SstiMessage> m_MessageList;
	static std::list<Test> m_Tests;
	std::string m_TestName;

	struct VRCLMessageQueue
	{
		std::list<SstiMessage> m_VrclMessageList;
		std::condition_variable m_VrclMessageCond;
		std::mutex m_VRCLCallbackMutex;
	};
	
	VRCLMessageQueue m_LocalVRCLQueue;
	VRCLMessageQueue m_RemoteVRCLQueue;
	
	CstiVRCLClient m_oLocalClient;
	CstiVRCLClient m_oRemoteClient;

	void LocalVRCLClientConnect ();
	void RemoteVRCLClientConnect ();

	stiHResult WaitForMessage(
		int32_t n32TimeOut,
		int32_t n32Message,
		size_t *pParam,
		bool *pbTimedOut);

	stiHResult WaitForMessage(
		int32_t n32Message,
		size_t *pParam,
		bool *pbTimedOut);

	stiHResult WaitForMessage(
		int32_t n32Message,
		bool *pbTimedOut);

	stiHResult ConsumeMessages (
		int32_t n32TimeOut);

	stiHResult WaitForVrclMessage(
		VRCLMessageQueue *pVRCLQueue,
		std::vector<int32_t> messages,
		int32_t *message,
		size_t *pParam,
		bool *pbTimedOut);

	stiHResult WaitForLocalVrclMessage(
		int32_t message,
		size_t *pParam,
		bool *pbTimedOut);

	stiHResult WaitForLocalVrclMessage(
		std::vector<int32_t> messages,
		int32_t *message,
		size_t *pParam,
		bool *pbTimedOut);

	stiHResult WaitForRemoteVrclMessage(
		int32_t message,
		size_t *pParam,
		bool *pbTimedOut);

	stiHResult WaitForRemoteVrclMessage(
		std::vector<int32_t> messages,
		int32_t *message,
		size_t *pParam,
		bool *pbTimedOut);

	bool WaitForCoreResponse (
		unsigned int unCoreRequestId,
		CstiCoreResponse **ppResponse);

	bool WaitForMessageResponse (
		unsigned int unMessageRequestId,
		CstiMessageResponse **ppResponse);

	static stiHResult EngineCallBack(
		int32_t n32Message,
		size_t MessageParam,
		size_t CallbackParam);

	stiHResult VRCLCallBack (
		VRCLMessageQueue *pVRCLQueue,
		int32_t n32Message,
		size_t MessageParam);

	static stiHResult LocalVRCLCallBack(
		int32_t n32Message,
		size_t MessageParam,
		size_t CallbackParam,
		size_t FromId);

	static stiHResult RemoteVRCLCallBack(
		int32_t n32Message,
		size_t MessageParam,
		size_t CallbackParam,
		size_t FromId);

	void ContactManagerListItemRemove (
		const std::string &name);

	void CallListGet (
		CstiCallList::EType eCallListType,
		CstiCoreResponse **ppCoreResponse);

	void CallListClear (
		CstiCallList::EType eCallListType);

	void UserAccountInfoGet (
		CstiUserAccountInfo *pUserAccountInfo);

	void SignMailEmailPreferencesSet (
		bool enableNotifications,
		const std::string &email1,
		const std::string &email2);

	void CallDial (
		EstiDialMethod eDialMethod,
		const char *pszDialString,
		bool bUsesDirectoryResolve,
		bool bVrsCall);

	void LocalCameraMove (
		uint8_t un8CameraMovement);
	
#ifdef stiMESSAGE_MANAGER
	void CleanupMessageList(
		std::list<SstiMessageManagerItem *> *plMessageManagerList);

	void FindMessageItem(
		const CstiItemId &itemId,
		const std::list<SstiMessageManagerItem *> *plMessageManagerList,
		SstiMessageManagerItem **ppMessageManagerItem);

	void ProcessMsgMgrEvents(
		std::list<SstiMessageManagerItem *> *plMessageManagerList);

	bool RemoveMessageItem(
		const CstiItemId &itemId,
		std::list<SstiMessageManagerItem *> *plMessageManagerList);

	void UpdateMessageList (
		const CstiItemId &itemId,
		std::list<SstiMessageManagerItem *> *plMessageManagerList);
#endif

	void ContactListCountGet (
		int *pnContactListCount);
	
	void ViewPositionsSet ();

	void prepareForInboundCall ();
	IstiCall *remoteDial ();

	bool waitForConferencingReady ();

	bool m_conferencingReady {false};
	std::condition_variable m_conferencingReadyCondition;
	std::mutex m_conferencingReadyMutex;
};

std::list<Test> IstiVideophoneEngineTest::m_Tests;


/*!
 * \brief
 *
 */
IstiVideophoneEngineTest::IstiVideophoneEngineTest ()
	:
	m_pVE (nullptr),
	m_bStarted (false),
	m_oLocalClient (LocalVRCLCallBack, (size_t)this, false),
	m_oRemoteClient (RemoteVRCLCallBack, (size_t)this, false)
{
}


/*!
 * \brief Read in the configuration data
 *
 */
void ReadConfigurationData ()
{
	if (!g_bHaveConfiguration)
	{
		
		ifstream infile;
		infile.open ("Test.cfg", ifstream::in);

		while (infile.good ())
		{
			// Read in each line of the configuration file and set values accordingly.
			std::string Line;
			getline (infile, Line);

			// If the line is blank or if the line begins with a '#' character,
			// skip it.
			if (!Line.empty () &&
				 Line[0] != '#')
			{
				int nSeparator1 = Line.find_first_of (" \t");
				std::string Parameter = Line.substr (0, nSeparator1);
				int nSeparator2 = Line.find_last_of (" \t");
				std::string Value = Line.substr (nSeparator2 + 1);

				if (Parameter == "UNIT_TEST_VIDEOPHONE_NUMBER")
				{
					g_UnitTestVideophoneNumber = Value;
				}
				else if (Parameter == "UNIT_TEST_VIDEOPHONE_ADDRESS")
				{
					g_UnitTestVideophoneAddress = Value;
				}
				else if (Parameter == "ACCOUNT_PHONE_NUMBER")
				{
					g_AccountPhoneNumber = Value;
				}
				else if (Parameter == "ACCOUNT_PASSWORD")
				{
					g_AccountPassword = Value;
				}
				else if (Parameter == "HOLDSERVER_ADDRESS")
				{
					g_HoldserverAddress = Value;
				}
				else if (Parameter == "EMAIL_ADDRESS1")
				{
					g_EmailAddress1 = Value;
				}
				else if (Parameter == "EMAIL_ADDRESS2")
				{
					g_EmailAddress2 = Value;
				}
			}
		}
		
		infile.close ();

		// Since we have set up the configuration, we don't need to do so on the rest
		// of the tests.  Set the bool to indicate this.
		g_bHaveConfiguration = true;

		cout << "\nTestVideophoneNumber = " << g_UnitTestVideophoneNumber << "\n";
		cout << "TestVideophoneAddress = " << g_UnitTestVideophoneAddress << "\n";
		cout << "AccountPhoneNumber = " << g_AccountPhoneNumber << "\n";
		cout << "AccountPassword = " << g_AccountPassword << "\n";
		cout << "HoldserverAddress = " << g_HoldserverAddress << "\n";
		cout << "Email 1 = " << g_EmailAddress1 << "\n";
		cout << "Email 2 = " << g_EmailAddress2 << "\n";
	}
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::setUp()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bTestTimedOut = false;
	std::string PropertyTable;

	m_TestName = "No Name!";
	if (!m_Tests.empty())
	{
		m_TestName = m_Tests.front().Name;
		PropertyTable = m_Tests.front().PropertyTable;
		m_Tests.pop_front ();
	}
	
	stiTrace("********** Setting up EngineTest: %s **********\n", m_TestName.c_str());

	m_bStarted = false;

	// Read in personal configuration (if present) for numbers, addresses, accounts, etc. to use.
	ReadConfigurationData ();

	FILE_COPY (PropertyTable.c_str(), ACTUAL_PROPERTY_TABLE);

	m_conferencingReady = false;

	stiTrace("setUp - Create Videophone Engine\n");
	m_pVE = ::videophoneEngineCreate (ProductType::VP2, "1.0.0",
								  false, true, "./data", STATIC_DATA, EngineCallBack, (size_t)this);
	CPPUNIT_ASSERT_MESSAGE ("CreateVideophoneEngine", m_pVE != nullptr);

	stiTrace("setUp - Videophone Engine created!\n");

	// Set the VRSServer to be the IP address of the g_HoldserverAddress
	WillowPM::PropertyManager::getInstance ()->propertySet (cmVRS_SERVER, g_HoldserverAddress);

	if (m_pVE)
	{
		stiTrace("setUp - Call NetworkStartup()\n");
		hResult = m_pVE->NetworkStartup ();

		CPPUNIT_ASSERT_MESSAGE ("NetworkStartup", !stiIS_ERROR (hResult));

		stiTrace("setUp - Call ServicesStartup()\n");
		hResult = m_pVE->ServicesStartup ();

		CPPUNIT_ASSERT_MESSAGE ("ServicesStartup", !stiIS_ERROR (hResult));

		//
		// Wait on condition before proceeding.
		//
		WaitForMessage (estiMSG_CORE_SERVICE_CONNECTED, &bTestTimedOut);

		CPPUNIT_ASSERT_MESSAGE ("Services timed out", !bTestTimedOut);

		//
		// Start up the conferencing engine (along with everything else).
		//
		stiTrace("setUp - Videophone Engine Startup()\n");
		hResult = m_pVE->Startup ();

		CPPUNIT_ASSERT_MESSAGE ("Videophone Engine failed to start", !stiIS_ERROR (hResult));

		//
		// Wait on condition before proceeding.
		//
		WaitForMessage (estiMSG_CONFERENCE_MANAGER_STARTUP_COMPLETE, &bTestTimedOut);
		CPPUNIT_ASSERT_MESSAGE ("Conf Mgr timed out", !bTestTimedOut);

		//
		// Get the local IP address for use in some of the tests.
		//
		stiHResult hResult = stiGetLocalIp (&m_LocalIPv4Address, estiTYPE_IPV4);
		CPPUNIT_ASSERT_MESSAGE (std::string ("stiGetLocalIp: "), !stiIS_ERROR (hResult));
#ifdef IPV6_ENABLED
		hResult = stiGetLocalIp (&m_LocalIPv6Address, estiTYPE_IPV6);
		CPPUNIT_ASSERT_MESSAGE (std::string ("stiGetLocalIp: "), !stiIS_ERROR (hResult));
#endif // IPV6_ENABLED

		m_bStarted = true;
	}

	m_oLocalClient.Startup();
	m_oRemoteClient.Startup();
	
	stiTrace("********** Starting EngineTest: %s **********\n", m_TestName.c_str());
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::tearDown()
{
	stiTrace("********** Tearing down EngineTest: %s **********\n", m_TestName.c_str());
	
	m_oLocalClient.Shutdown();
	m_oRemoteClient.Shutdown();
	
	m_oLocalClient.WaitForShutdown();
	m_oRemoteClient.WaitForShutdown();
	
	//
	// Shutdown
	//
	if (m_pVE)
	{
		m_pVE->ServicesShutdown ();

		m_pVE->NetworkShutdown ();

		m_pVE->Shutdown ();

		DestroyVideophoneEngine (m_pVE);

		m_pVE = nullptr;
	}

	//
	// Empty Queues
	//
	while (!m_LocalVRCLQueue.m_VrclMessageList.empty ())
	{
		m_LocalVRCLQueue.m_VrclMessageList.pop_front ();
	}
	
	while (!m_RemoteVRCLQueue.m_VrclMessageList.empty ())
	{
		m_RemoteVRCLQueue.m_VrclMessageList.pop_front ();
	}
	
	while (!m_MessageList.empty ())
	{
		m_MessageList.pop_front ();
	}
	
	//
	// All call objects should be freed at this point.  Check
	// by inspecting the call count data member in the CstiCall class.
	//
	if (CstiCall::m_nCallCount != 0)
	{
		CstiCall::m_nCallCount = 0;
		CPPUNIT_ASSERT_MESSAGE ("Not all call objects were freed.\n", false);
	}

	stiTrace("********** Finished EngineTest: %s **********\n", m_TestName.c_str());
}


void IstiVideophoneEngineTest::LocalVRCLClientConnect()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bTestTimedOut;
	size_t MessageParam;

	//
	// Make sure the local VRCL port is open.
	//
	m_pVE->VRCLPortSet (LOCAL_VRCL_PORT);

	hResult = m_oLocalClient.Connect (m_LocalIPv4Address.c_str (), LOCAL_VRCL_PORT);
	CPPUNIT_ASSERT_MESSAGE ("Could not establish a VRCL connection with the local videophone.", !stiIS_ERROR (hResult));
//	hResult = m_oLocalClient.Connect (m_LocalIPv6Address.c_str (), LOCAL_VRCL_PORT);
//	CPPUNIT_ASSERT_MESSAGE ("Could not establish a VRCL connection with the local videophone.", !stiIS_ERROR (hResult));

	WaitForLocalVrclMessage (estiMSG_VRCL_CLIENT_CONNECTED, &MessageParam, &bTestTimedOut);

	CPPUNIT_ASSERT_MESSAGE (std::string ("VRCL Local Connect timed out: ") + std::string (m_LocalIPv4Address) + " "+ std::string (m_LocalIPv6Address),
									!bTestTimedOut);
}


void IstiVideophoneEngineTest::RemoteVRCLClientConnect ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bTestTimedOut;
	size_t MessageParam;

	hResult = m_oRemoteClient.Connect (g_UnitTestVideophoneAddress.c_str (), 0);
	CPPUNIT_ASSERT_MESSAGE ("Could not establish a VRCL connection with the remote videohone.", !stiIS_ERROR (hResult));

	WaitForRemoteVrclMessage (estiMSG_VRCL_CLIENT_CONNECTED, &MessageParam, &bTestTimedOut);

	CPPUNIT_ASSERT_MESSAGE ("Did not receive a VRCL connected message from the remote videophone.", !bTestTimedOut);
}


/*!
 * \brief
 *
 */
stiHResult IstiVideophoneEngineTest::WaitForMessage(
	int32_t n32TimeOut,
	int32_t n32Message,
	size_t *pParam,
	bool *pbTimedOut)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiMessage Message;
	struct timeval sStartTime;
	struct timeval sEndTime;
	gettimeofday (&sStartTime, nullptr);

	sEndTime.tv_sec = sStartTime.tv_sec + n32TimeOut;
	sEndTime.tv_usec = sStartTime.tv_usec;

//	stiTrace ("Waiting for message %s (%d): StartTime = %d, %d\n", m_pVE->MessageStringGet(n32Message).c_str (), n32Message, sStartTime.tv_sec, sStartTime.tv_usec);

	for (;;)
	{
		std::unique_lock<std::mutex> lock(m_EngineCallbackMutex);

		*pbTimedOut = false;

		if (m_MessageList.empty ())
		{
			timeval TimeToEnd;
			gettimeofday (&sStartTime, nullptr);
			timersub(&sEndTime, &sStartTime, &TimeToEnd);

//			stiTrace ("Amount to wait: %d, %d\n", TimeToEnd.tv_sec, TimeToEnd.tv_usec);

			if (TimeToEnd.tv_sec <= 0 ||
//					estiOK != stiOSCondWait2 (m_MessageCond, lock, TimeToEnd.tv_sec, TimeToEnd.tv_usec * 1000))
				std::cv_status::timeout == m_MessageCond.wait_for(lock, std::chrono::seconds(TimeToEnd.tv_sec)))
			{
				gettimeofday (&sStartTime, nullptr);

				stiTrace ("Time is now = %d, %d\n", sStartTime.tv_sec, sStartTime.tv_usec);
				stiTrace ("Message %s did not arrive in time. EndTime = %d, %d\n", m_pVE->MessageStringGet (n32Message).c_str (), sEndTime.tv_sec, sEndTime.tv_usec);

				*pbTimedOut = true;
				break;
			}
		}

		//
		// Double check that the condition was met (there is something in the queue).
		//
		if (!m_MessageList.empty ())
		{
			Message = m_MessageList.front ();
			m_MessageList.pop_front ();

			if (Message.n32Message == n32Message)
			{
				*pParam = Message.Param;
				break;
			}

			m_pVE->MessageCleanup (Message.n32Message, Message.Param);
		}
	}

	return hResult;
}


stiHResult IstiVideophoneEngineTest::WaitForMessage(
	int32_t n32Message,
	size_t *pParam,
	bool *pbTimedOut)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = WaitForMessage (TEST_TIMEOUT, n32Message, pParam, pbTimedOut);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


/*!
 * \brief Notifies the caller when the message arrives and does the cleanup
 *
 */
stiHResult IstiVideophoneEngineTest::WaitForMessage (
	int32_t n32Message,
	bool *pbTimedOut)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	size_t Param;
	
	hResult = WaitForMessage(n32Message, &Param, pbTimedOut);
	
	if (!stiIS_ERROR (hResult) && !pbTimedOut)
	{
		m_pVE->MessageCleanup (n32Message, Param);
	}

	return hResult;
}


stiHResult IstiVideophoneEngineTest::ConsumeMessages (
	int32_t n32TimeOut)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	struct timeval sStartTime;
	struct timeval sEndTime;

	stiTrace ("Consuming messages for %d seconds\n", n32TimeOut);

	gettimeofday (&sStartTime, nullptr);

	sEndTime.tv_sec = sStartTime.tv_sec + n32TimeOut;
	sEndTime.tv_usec = sStartTime.tv_usec;

	stiTrace ("Time to End: %d, %d\n", sEndTime.tv_sec, sEndTime.tv_usec);

	for (;;)
	{
		std::unique_lock<std::mutex> lock(m_EngineCallbackMutex);

		//
		// Determine if it is time to return
		//
		timeval TimeToEnd;

		gettimeofday (&sStartTime, nullptr);
		timersub(&sEndTime, &sStartTime, &TimeToEnd);

		if (TimeToEnd.tv_sec <= 0)
		{
			stiTrace ("Ending: CurrentTime: %d, %d, EndTime: %d, %d\n",
					sStartTime.tv_sec, sStartTime.tv_usec,
					sEndTime.tv_sec, sEndTime.tv_usec);

			break;
		}

		//
		// If the list is empty then wait until a message arrives
		// but not past the amount of time alloted.
		//
		if (m_MessageList.empty ())
		{
			m_MessageCond.wait_for(lock, std::chrono::milliseconds(TimeToEnd.tv_sec * 1000 + (TimeToEnd.tv_usec / 1000)));
		}

		//
		// Double check that the condition was met (there is something in the queue).
		//
		while (!m_MessageList.empty ())
		{
			SstiMessage Message = m_MessageList.front ();
			m_MessageList.pop_front ();

			m_pVE->MessageCleanup (Message.n32Message, Message.Param);
		}
	}

	return hResult;
}


/*!
 * \brief
 *
 */
stiHResult IstiVideophoneEngineTest::WaitForVrclMessage(
	VRCLMessageQueue *pVRCLQueue,
	std::vector<int32_t> messages,
	int32_t *message,
	size_t *pParam,
	bool *pbTimedOut)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiMessage Message;
	struct timeval sCurrentTime;
	struct timeval sStartTime;
	struct timeval sTimeDiff;
	
	gettimeofday (&sStartTime, nullptr);
	
	for (;;)
	{
		std::unique_lock<std::mutex> lock(pVRCLQueue->m_VRCLCallbackMutex);
		
		*pbTimedOut = false;
		
		if (pVRCLQueue->m_VrclMessageList.empty ())
		{
			gettimeofday (&sCurrentTime, nullptr);
			timersub(&sCurrentTime, &sStartTime, &sTimeDiff);
			if (sTimeDiff.tv_sec > TEST_TIMEOUT ||
				std::cv_status::timeout == pVRCLQueue->m_VrclMessageCond.wait_for(lock, std::chrono::seconds(TEST_TIMEOUT)))
			{
				for (auto &message: messages)
				{
					stiTrace ("VRCL message (%d) did not arrive in time.\n", message);
				}

				*pbTimedOut = true;
				break;
			}
		}

		//
		// Double check that the condition was met (there is something in the queue).
		//
		if (!pVRCLQueue->m_VrclMessageList.empty ())
		{
			Message = pVRCLQueue->m_VrclMessageList.front ();
			pVRCLQueue->m_VrclMessageList.pop_front ();
			if (std::find(messages.begin(), messages.end(), Message.n32Message) != messages.end ())
			{
				if (message)
				{
					*message = Message.n32Message;
				}

				*pParam = Message.Param;
				break;
			}
		}
	}
	return hResult;
}


stiHResult IstiVideophoneEngineTest::WaitForLocalVrclMessage(
	int32_t message,
	size_t *pParam,
	bool *pbTimedOut)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = WaitForLocalVrclMessage ({message}, pParam, pbTimedOut);

	return hResult;
}


stiHResult IstiVideophoneEngineTest::WaitForLocalVrclMessage(
	std::vector<int32_t> messages,
	int32_t *message,
	size_t *pParam,
	bool *pbTimedOut)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	hResult = WaitForVrclMessage (&m_LocalVRCLQueue, messages, message, pParam, pbTimedOut);
	
	return hResult;
}


stiHResult IstiVideophoneEngineTest::WaitForRemoteVrclMessage(
	int32_t message,
	size_t *pParam,
	bool *pbTimedOut)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = WaitForRemoteVrclMessage ({message}, nullptr, pParam, pbTimedOut);

	return hResult;
}


stiHResult IstiVideophoneEngineTest::WaitForRemoteVrclMessage(
	std::vector<int32_t> messages,
	int32_t *message,
	size_t *pParam,
	bool *pbTimedOut)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	hResult = WaitForVrclMessage (&m_RemoteVRCLQueue, messages, message, pParam, pbTimedOut);
	
	return hResult;
}


/*!
 * \brief
 *
 */
stiHResult IstiVideophoneEngineTest::EngineCallBack(
	int32_t n32Message,
	size_t MessageParam,
	size_t CallbackParam)
{
	auto pThis = (IstiVideophoneEngineTest *)CallbackParam;
	SstiMessage Message;

	if (n32Message == estiMSG_CONFERENCING_READY_STATE)
	{
		if (MessageParam == estiConferencingStateReady)
		{
			std::unique_lock<std::mutex> lock (pThis->m_conferencingReadyMutex);
			pThis->m_conferencingReady = true;
			pThis->m_conferencingReadyCondition.notify_all ();
		}
		else
		{
			std::unique_lock<std::mutex> lock (pThis->m_conferencingReadyMutex);
			pThis->m_conferencingReady = false;
		}
	}

	Message.n32Message = n32Message;
	Message.Param = MessageParam;

	std::unique_lock<std::mutex> lock(pThis->m_EngineCallbackMutex);

	pThis->m_MessageList.push_back (Message);

	pThis->m_MessageCond.notify_all();

	return stiRESULT_SUCCESS;
}


stiHResult IstiVideophoneEngineTest::VRCLCallBack (
	VRCLMessageQueue *pVRCLQueue,
	int32_t n32Message,
	size_t MessageParam)
{
	SstiMessage Message;
	
	Message.n32Message = n32Message;
	Message.Param = MessageParam;
	
	std::unique_lock<std::mutex> lock(pVRCLQueue->m_VRCLCallbackMutex);
	pVRCLQueue->m_VrclMessageList.push_back (Message);
	pVRCLQueue->m_VrclMessageCond.notify_all();
	
	return stiRESULT_SUCCESS;
}


/*!
 * \brief
 *
 */
stiHResult IstiVideophoneEngineTest::LocalVRCLCallBack(
	int32_t n32Message,
	size_t MessageParam,
	size_t CallbackParam,
	size_t FromId)
{
	auto pThis = (IstiVideophoneEngineTest *)CallbackParam;
	
	pThis->VRCLCallBack (&pThis->m_LocalVRCLQueue, n32Message, MessageParam);
	
	return stiRESULT_SUCCESS;
}


/*!
 * \brief
 *
 */
stiHResult IstiVideophoneEngineTest::RemoteVRCLCallBack(
	int32_t n32Message,
	size_t MessageParam,
	size_t CallbackParam,
	size_t FromId)
{
	auto pThis = (IstiVideophoneEngineTest *)CallbackParam;
	
	pThis->VRCLCallBack (&pThis->m_RemoteVRCLQueue, n32Message, MessageParam);
	
	return stiRESULT_SUCCESS;
}


/*!
 * \brief
 *
 */
bool IstiVideophoneEngineTest::WaitForCoreResponse (
	unsigned int unCoreRequestId,
	CstiCoreResponse **ppResponse)
{
	bool bTestTimedOut = false;
	size_t Param;

	//
	// Wait on condition before proceeding.
	//
	time_t StartTime;
	time_t Now;

	time (&StartTime);

	for (;;)
	{
		WaitForMessage (estiMSG_CORE_RESPONSE, &Param, &bTestTimedOut);

		if (bTestTimedOut)
		{
			break;
		}

		auto  poResponse = (CstiCoreResponse*)Param;

		if (poResponse->RequestIDGet () == unCoreRequestId)
		{
			*ppResponse = poResponse;

			break;
		}

		m_pVE->MessageCleanup(estiMSG_CORE_RESPONSE, Param);

		time (&Now);

		if (Now - StartTime > TEST_TIMEOUT)
		{
			bTestTimedOut = true;

			break;
		}
	}

	return bTestTimedOut;
}


/*!
 * \brief
 *
 */
bool IstiVideophoneEngineTest::WaitForMessageResponse (
	unsigned int unMessageRequestId,
	CstiMessageResponse **ppResponse)
{
	bool bTestTimedOut = false;
	size_t Param;

	//
	// Wait on condition before proceeding.
	//
	for (;;)
	{
		WaitForMessage (estiMSG_MESSAGE_RESPONSE, &Param, &bTestTimedOut);

		if (bTestTimedOut)
		{
			break;
		}

		auto  poResponse = (CstiMessageResponse*)Param;

		if (poResponse->RequestIDGet () == unMessageRequestId)
		{
			*ppResponse = poResponse;

			break;
		}

		m_pVE->MessageCleanup(estiMSG_MESSAGE_RESPONSE, Param);
	}

	return bTestTimedOut;
}


bool IstiVideophoneEngineTest::waitForConferencingReady ()
{
	std::unique_lock<std::mutex> lock (m_conferencingReadyMutex);

	auto waitUntil = std::chrono::system_clock::now() + std::chrono::seconds (5);
	while (!m_conferencingReady && std::chrono::system_clock::now() < waitUntil)
	{
		m_conferencingReadyCondition.wait_until(lock, waitUntil);
	}

	return m_conferencingReady;
}


void IstiVideophoneEngineTest::ViewPositionsSet ()
{
	IstiVideoOutput::SstiDisplaySettings DisplaySettings;

	DisplaySettings.un32Mask = IstiVideoOutput::eDS_SELFVIEW_VISIBILITY
							 | IstiVideoOutput::eDS_SELFVIEW_SIZE
							 | IstiVideoOutput::eDS_SELFVIEW_POSITION
							 | IstiVideoOutput::eDS_REMOTEVIEW_VISIBILITY
							 | IstiVideoOutput::eDS_REMOTEVIEW_SIZE
							 | IstiVideoOutput::eDS_REMOTEVIEW_POSITION;
	DisplaySettings.stSelfView.bVisible = 1;
	DisplaySettings.stSelfView.unWidth = 640;
	DisplaySettings.stSelfView.unHeight = 360;
	DisplaySettings.stSelfView.unPosX = 0;
	DisplaySettings.stSelfView.unPosY = 0;

	DisplaySettings.stRemoteView.bVisible = 1;
	DisplaySettings.stRemoteView.unWidth = 1280;
	DisplaySettings.stRemoteView.unHeight = 720;
	DisplaySettings.stRemoteView.unPosX = 0;
	DisplaySettings.stRemoteView.unPosY = 0;

	IstiVideoOutput::InstanceGet()->DisplaySettingsSet (&DisplaySettings);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::CreateVideophoneEngine()
{
	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	ViewPositionsSet ();

//	ConsumeMessages (24 * 60 * 60);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::userAccountAssociate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *pCoreRequest = nullptr;
	unsigned int unCoreRequestId;
	bool bTestTimedOut = true;
	CstiCoreResponse *pResponse = nullptr;

	stiTrace("userAccountAssociate - Start\n");
	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	//
	// Test userAccountAssociate
	//
	pCoreRequest = new CstiCoreRequest;

	stiTrace("userAccountAssociate - Create userAccountAssociate core request.\n");
	pCoreRequest->userAccountAssociate(
		nullptr,
		g_AccountPhoneNumber.c_str (),
		g_AccountPassword.c_str (),
		estiTRUE, // true for primary user on this phone
		estiFALSE // true for "match current primary user"
		);

	stiTrace("userAccountAssociate - Send core request.\n");
	hResult = m_pVE->CoreRequestSend(pCoreRequest, &unCoreRequestId);

	CPPUNIT_ASSERT_MESSAGE ("CoreRequestSend failed", !stiIS_ERROR (hResult));

	stiTrace("userAccountAssociate - Wait for core response.\n");
	bTestTimedOut = WaitForCoreResponse (unCoreRequestId, &pResponse);

	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);

	m_pVE->MessageCleanup(estiMSG_CORE_RESPONSE, (size_t)pResponse);
	stiTrace("userAccountAssociate - Succeeded!\n");
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::CoreLogin ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	unsigned int unCoreRequestId;
	bool bTestTimedOut = true;
	CstiCoreResponse *pResponse = nullptr;

	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	stiTrace("CoreLogin - Call userAccountAssociate()\n");
	
	//
	// Make sure that the user is the primary user on the videophone.
	//
	userAccountAssociate ();

	//
	// Test CoreLogin
	//
	stiTrace("CoreLogin - Call Engine->CoreLogin()\n");
	hResult = m_pVE->CoreLogin (g_AccountPhoneNumber, g_AccountPassword, nullptr, &unCoreRequestId);

	CPPUNIT_ASSERT_MESSAGE ("CoreLogin failed", !stiIS_ERROR (hResult));

	stiTrace("CoreLogin - Wait for Engine response\n");
	bTestTimedOut = WaitForCoreResponse (unCoreRequestId, &pResponse);

	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);

	bool bSuccessful = (estiOK == pResponse->ResponseResultGet ());

	m_pVE->MessageCleanup(estiMSG_CORE_RESPONSE, (size_t)pResponse);

	CPPUNIT_ASSERT_MESSAGE ("Authentication failed\n", bSuccessful);
	stiTrace("CoreLogin - Succeeded!\n");
}


void IstiVideophoneEngineTest::ContactManagerImport ()
{
	bool bTestTimedOut = false;
	size_t Param;

	// Now log in.
	CoreLogin ();

	auto contactManager = m_pVE->ContactManagerGet ();
	CPPUNIT_ASSERT_MESSAGE ("Contact manager unavailable.", contactManager != NULL);

	contactManager->clearContacts ();

	WaitForMessage (estiMSG_CONTACT_LIST_DELETED, &Param, &bTestTimedOut);

	CPPUNIT_ASSERT_MESSAGE ("Contacts Clear timed out", !bTestTimedOut);

	// Test to make sure the item can be retreived
	contactManager->ImportContacts ("18012116670", "3142");

	WaitForMessage (estiMSG_CONTACT_LIST_IMPORT_COMPLETE, &Param, &bTestTimedOut);

	CPPUNIT_ASSERT_MESSAGE ("Contact import timed out", !bTestTimedOut);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::callListCountGet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *pCoreRequest = nullptr;
	unsigned int unCoreRequestId;
	bool bTestTimedOut = true;
	CstiCoreResponse *pResponse = nullptr;

	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	//
	// Test callListCountGet
	//
	pCoreRequest = new CstiCoreRequest;

	pCoreRequest->callListCountGet (CstiCallList::eDIALED);

	hResult = m_pVE->CoreRequestSend(pCoreRequest, &unCoreRequestId);

	CPPUNIT_ASSERT_MESSAGE ("CoreRequestSend failed", !stiIS_ERROR (hResult));

	bTestTimedOut = WaitForCoreResponse (unCoreRequestId, &pResponse);

	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);

	m_pVE->MessageCleanup(estiMSG_CORE_RESPONSE, (size_t)pResponse);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::CallListGet (
	CstiCallList::EType eCallListType,
	CstiCoreResponse **ppCoreResponse)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *pCoreRequest = nullptr;
	unsigned int unCoreRequestId;
	bool bTestTimedOut = true;
	CstiCoreResponse *pResponse = nullptr;

	//
	// Test callListGet
	//
	pCoreRequest = new CstiCoreRequest;

	if (eCallListType == CstiCallList::eCONTACT)
	{
		pCoreRequest->contactListGet();
	}
	else
	{
		pCoreRequest->callListGet (eCallListType);
	}

	hResult = m_pVE->CoreRequestSend(pCoreRequest, &unCoreRequestId);
	CPPUNIT_ASSERT_MESSAGE ("CoreRequestSend failed", !stiIS_ERROR (hResult));

	bTestTimedOut = WaitForCoreResponse (unCoreRequestId, &pResponse);
	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Core request failed", pResponse->ResponseResultGet() == estiOK);

	*ppCoreResponse = pResponse;
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::ContactListGet ()
{
	CstiCoreResponse *pResponse = nullptr;

	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	//
	// Test CallListGet
	//
	CallListGet (CstiCallList::eCONTACT, &pResponse);

	m_pVE->MessageCleanup(estiMSG_CORE_RESPONSE, (size_t)pResponse);
}

/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::ContactManagerListItemGetByName ()
{
	auto contactManager = m_pVE->ContactManagerGet ();
	CPPUNIT_ASSERT_MESSAGE ("Contact manager unavailable.", contactManager != NULL);

	// Now log in.
	CoreLogin ();

	// Add an item (so there is something to get)
	ContactManagerListItemAdd();

	// Test to make sure the item can be retreived
	auto contact = contactManager->contactByPhoneNumberGet (CONTACT_USER_NUMBER);

	CPPUNIT_ASSERT_MESSAGE ("Contact list item not retreived", contact != nullptr);

	auto contactName = contact->NameGet ();

	CPPUNIT_ASSERT_MESSAGE ("Contact user name mismatch", contactName == CONTACT_USER_NAME);
}

/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::ContactManagerListItemGetByPhoneNumber ()
{
	auto contactManager = m_pVE->ContactManagerGet ();
	CPPUNIT_ASSERT_MESSAGE ("Contact manager unavailable.", contactManager != NULL);

	// Now log in.
	CoreLogin ();

	// Add an item (so there is something to get)
	ContactManagerListItemAdd();

	// Test to make sure the item can be retreived
	auto contact = contactManager->contactByPhoneNumberGet (CONTACT_USER_NUMBER);
	CPPUNIT_ASSERT_MESSAGE ("Contact list item not retreived", contact != nullptr);

	std::string number;
	contact->DialStringGet(CstiContactListItem::ePHONE_HOME, &number);
	CPPUNIT_ASSERT_MESSAGE ("Contact phone number mismatch", number == CONTACT_USER_NUMBER);
}

/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::ContactManagerListItemGetById ()
{
	auto contactManager = m_pVE->ContactManagerGet ();
	CPPUNIT_ASSERT_MESSAGE ("Contact manager unavailable.", contactManager != NULL);

	// Now log in.
	CoreLogin ();

	// Clear the contact list so we know we're starting clean
	contactManager->clearContacts();

	bool bTimedOut;
	WaitForMessage (estiMSG_CONTACT_LIST_DELETED, &bTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("estiMSG_CONTACT_LIST_DELETED did not arrive in time",
							!bTimedOut);
	
	// Add an item (so there is something to get)
	ContactManagerListItemAdd();

	auto contact1 = contactManager->contactByPhoneNumberGet (CONTACT_USER_NUMBER);
	CPPUNIT_ASSERT_MESSAGE ("Contact list item not retreived", contact1 != nullptr);

	// Test to make sure the item can be retrieved
	auto contact2 = contactManager->contactByIDGet(contact1->ItemIdGet());
	CPPUNIT_ASSERT_MESSAGE ("Contact list item not retreived", contact2 != nullptr);

	CPPUNIT_ASSERT_MESSAGE ("Contact user mismatch", contact1->ItemIdGet () == contact2->ItemIdGet ());
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::CallListClear (
	CstiCallList::EType eCallListType)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *pCoreRequest = nullptr;
	unsigned int unCoreRequestId;
	bool bTestTimedOut = true;
	CstiCoreResponse *pResponse = nullptr;

	//
	// Create the call list item
	//
	CstiList *pList = nullptr;
	pCoreRequest = new CstiCoreRequest;

	if (eCallListType == CstiCallList::eCONTACT)
	{
		pList = new CstiContactList;
		pCoreRequest->contactListSet((CstiContactList *)pList);
	}
	else
	{
		pList = new CstiCallList;
		((CstiCallList *)pList)->TypeSet (eCallListType);
		pCoreRequest->callListSet ((CstiCallList *)pList);
	}

	//
	// Test callListSet
	//
	hResult = m_pVE->CoreRequestSend(pCoreRequest, &unCoreRequestId);
	CPPUNIT_ASSERT_MESSAGE ("CoreRequestSend failed", !stiIS_ERROR (hResult));

	delete pList;
	pList = nullptr;

	bTestTimedOut = WaitForCoreResponse (unCoreRequestId, &pResponse);
	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Core request failed", pResponse->ResponseResultGet() == estiOK);

	m_pVE->MessageCleanup(estiMSG_CORE_RESPONSE, (size_t)pResponse);

	//
	// Get the speed dial list and check to make sure it is empty.
	//
	CallListGet (eCallListType, &pResponse);

	if (eCallListType == CstiCallList::eCONTACT)
	{
		pList = pResponse->ContactListGet();
	}
	else
	{
		pList = pResponse->CallListGet();
	}

	if (pList)
	{
		CPPUNIT_ASSERT_MESSAGE ("List is not empty", pList->CountGet () == 0);
	}

	m_pVE->MessageCleanup(estiMSG_CORE_RESPONSE, (size_t)pResponse);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::ContactListClear ()
{
	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	CallListClear (CstiCallList::eCONTACT);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::ContactManagerListClear ()
{
	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	auto contactManager = m_pVE->ContactManagerGet();

	// Add an item (so there is something to clear)
	ContactManagerListItemAdd();

	contactManager->clearContacts();

	bool bTimedOut;
	WaitForMessage (estiMSG_CONTACT_LIST_DELETED, &bTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("estiMSG_CONTACT_LIST_DELETED did not arrive in time",
							!bTimedOut);

	// How many contacts are left?
	auto count = contactManager->getNumContacts();
	CPPUNIT_ASSERT_MESSAGE("ContactList is still not empty", count == 0);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::ContactManagerListItemRemove ()
{
	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	auto contactManager = m_pVE->ContactManagerGet();

	// Start from a clean slate.
	contactManager->clearContacts();

	bool bTimedOut;
	WaitForMessage (estiMSG_CONTACT_LIST_DELETED, &bTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("estiMSG_CONTACT_LIST_DELETED did not arrive in time",
							!bTimedOut);
	
	// First add a contact
	ContactManagerListItemAdd();

	// Then try to remove it
	ContactManagerListItemRemove(CONTACT_USER_NUMBER);

	// Then see if it's still there
	auto contact = contactManager->contactByPhoneNumberGet (CONTACT_USER_NUMBER);

	CPPUNIT_ASSERT_MESSAGE("Contact was not deleted", contact == nullptr);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::MissedCallListClear ()
{
	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	CallListClear (CstiCallList::eMISSED);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::DialedCallListClear ()
{
	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	CallListClear (CstiCallList::eDIALED);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::AnsweredCallListClear ()
{
	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	CallListClear (CstiCallList::eANSWERED);
}


void IstiVideophoneEngineTest::ContactListCountGet (
	int *pnCount)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *pCoreRequest = nullptr;
	unsigned int unCoreRequestId;
	bool bTestTimedOut = true;
	CstiCoreResponse *pResponse = nullptr;
	
	//
	// Count the number of items in the contact list.
	//
	pCoreRequest = new CstiCoreRequest;
	
	pCoreRequest->contactListCountGet ();
	
	hResult = m_pVE->CoreRequestSend (pCoreRequest, &unCoreRequestId);
	CPPUNIT_ASSERT_MESSAGE ("CoreRequestSend failed", !stiIS_ERROR (hResult));
	
	bTestTimedOut = WaitForCoreResponse (unCoreRequestId, &pResponse);
	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Core request failed.", pResponse->ResponseResultGet() == estiOK);
	
	CstiContactList *pContactList = pResponse->ContactListGet ();
	
	*pnCount = pContactList->CountGet ();

	m_pVE->MessageCleanup(estiMSG_CORE_RESPONSE, (size_t)pResponse);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::ContactManagerListItemAdd ()
{
	int nContactListCount1 = 0;
	int nContactListCount2 = 0;

	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	ContactListCountGet (&nContactListCount1);
	
	//
	// Get the contact manager
	//
	auto contactManager = m_pVE->ContactManagerGet();

	//
	// Create a contact object
	//
	auto contact = contactManager->ContactCreate();

	//
	// Edit the contact
	//
	contact->NameSet(CONTACT_USER_NAME);

	contact->PhoneNumberAdd (CstiContactListItem::ePHONE_HOME, CONTACT_USER_NUMBER);

	contactManager->addContact(contact);
	
	bool bTimedOut = false;
	WaitForMessage (estiMSG_CONTACT_LIST_ITEM_ADDED, &bTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("estiMSG_CONTACT_LIST_ITEM_ADDED did not arrive in time",
							!bTimedOut);

	ContactListCountGet (&nContactListCount2);
	CPPUNIT_ASSERT_MESSAGE ("Contact list count is not correct.", nContactListCount1 + 1 == nContactListCount2);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::ContactManagerListItemRemove (
	const std::string &name)
{
	auto contactManager = m_pVE->ContactManagerGet();

	auto contact = contactManager->contactByPhoneNumberGet (name);
	CPPUNIT_ASSERT_MESSAGE ("Contact to delete was not found.", contact != nullptr);
	
	contactManager->removeContact (contact);

	// Wait for the core response
	bool bTimedOut;
	size_t Param;
	WaitForMessage (estiMSG_CONTACT_LIST_ITEM_DELETED, &Param, &bTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("estiMSG_CONTACT_LIST_ITEM_DELETED did not arrive in time",
							!bTimedOut);

	CstiItemId *pId = (CstiItemId *)Param;
	delete pId;
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::MissedCallListGet ()
{
	CstiCoreResponse *pResponse = nullptr;

	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	//
	// Test CallListGet
	//
	CallListGet (CstiCallList::eMISSED, &pResponse);

	m_pVE->MessageCleanup(estiMSG_CORE_RESPONSE, (size_t)pResponse);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::DialedCallListGet ()
{
	CstiCoreResponse *pResponse = nullptr;

	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	//
	// Test CallListGet
	//
	CallListGet (CstiCallList::eDIALED, &pResponse);

	m_pVE->MessageCleanup(estiMSG_CORE_RESPONSE, (size_t)pResponse);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::AnsweredCallListGet ()
{
	CstiCoreResponse *pResponse = nullptr;

	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	//
	// Test CallListGet
	//
	CallListGet (CstiCallList::eANSWERED, &pResponse);

	m_pVE->MessageCleanup(estiMSG_CORE_RESPONSE, (size_t)pResponse);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::MessageListGet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiMessageRequest *pMessageRequest = nullptr;
	unsigned int unMessageRequestId;
	bool bTestTimedOut = true;
	CstiMessageResponse *pResponse = nullptr;

	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	//
	// Test messageListGet
	//
	pMessageRequest = new CstiMessageRequest;

	pMessageRequest->MessageListGet ();

	hResult = m_pVE->MessageRequestSend(pMessageRequest, &unMessageRequestId);

	CPPUNIT_ASSERT_MESSAGE ("MessageRequestSend failed", !stiIS_ERROR (hResult));

	bTestTimedOut = WaitForMessageResponse (unMessageRequestId, &pResponse);

	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);

	m_pVE->MessageCleanup(estiMSG_MESSAGE_RESPONSE, (size_t)pResponse);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::UserAccountInfoGet (
	CstiUserAccountInfo *pUserAccountInfo)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *pCoreRequest = nullptr;
	unsigned int unCoreRequestId;
	bool bTestTimedOut = true;
	CstiCoreResponse *pResponse = nullptr;

	//
	// Test userAccountInfoGet
	//
	pCoreRequest = new CstiCoreRequest;

	pCoreRequest->userAccountInfoGet ();

	hResult = m_pVE->CoreRequestSend(pCoreRequest, &unCoreRequestId);

	CPPUNIT_ASSERT_MESSAGE ("CoreRequestSend failed", !stiIS_ERROR (hResult));

	bTestTimedOut = WaitForCoreResponse (unCoreRequestId, &pResponse);

	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);

	*pUserAccountInfo = *pResponse->UserAccountInfoGet ();

	m_pVE->MessageCleanup(estiMSG_CORE_RESPONSE, (size_t)pResponse);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::userAccountInfoGet ()
{
	CstiUserAccountInfo UserAccountInfo;

	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	UserAccountInfoGet (&UserAccountInfo);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::SignMailEmailPreferencesSet (
	bool enableNotifications,
	const std::string &email1,
	const std::string &email2)
{
	//
	// Test signMailEmailPreferencesSet
	//
	m_pVE->userAccountManagerGet()->signMailEmailPreferencesSet(enableNotifications, email1, email2, [this](std::shared_ptr<CstiCoreResponse> poResponse)
	{
		CPPUNIT_ASSERT_MESSAGE("SignMail update failed", poResponse->ResponseResultGet() != estiERROR);
	});
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::signMailEmailPreferencesSet ()
{
	CstiUserAccountInfo UserAccountInfo;
	std::string address;

	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	UserAccountInfoGet (&UserAccountInfo);

	if (UserAccountInfo.m_Email != g_EmailAddress1)
	{
		address = g_EmailAddress1;
	}
	else
	{
		address = g_EmailAddress2;
	}

	UserAccountInfo.m_EmailMain = address;
	bool enabled = stiStrICmp(UserAccountInfo.m_SignMailEnabled.c_str(), "true");

	SignMailEmailPreferencesSet (enabled, UserAccountInfo.m_EmailMain, UserAccountInfo.m_EmailPager);

	UserAccountInfoGet (&UserAccountInfo);

	CPPUNIT_ASSERT_MESSAGE ("Email address was not saved", UserAccountInfo.m_EmailMain == address);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::getUserDefaults ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *pCoreRequest = nullptr;
	unsigned int unCoreRequestId;
	bool bTestTimedOut = true;
	CstiCoreResponse *pResponse = nullptr;

	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	//
	// Test getUserDefaults
	//
	pCoreRequest = new CstiCoreRequest;

	pCoreRequest->getUserDefaults ();

	hResult = m_pVE->CoreRequestSend(pCoreRequest, &unCoreRequestId);

	CPPUNIT_ASSERT_MESSAGE ("CoreRequestSend failed", !stiIS_ERROR (hResult));

	bTestTimedOut = WaitForCoreResponse (unCoreRequestId, &pResponse);

	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);

	m_pVE->MessageCleanup(estiMSG_CORE_RESPONSE, (size_t)pResponse);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::primaryUserExists ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *pCoreRequest = nullptr;
	unsigned int unCoreRequestId;
	bool bTestTimedOut = true;
	CstiCoreResponse *pResponse = nullptr;

	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	//
	// Test primaryUserExists
	//
	pCoreRequest = new CstiCoreRequest;

	pCoreRequest->primaryUserExists ();

	hResult = m_pVE->CoreRequestSend(pCoreRequest, &unCoreRequestId);

	CPPUNIT_ASSERT_MESSAGE ("CoreRequestSend failed", !stiIS_ERROR (hResult));

	bTestTimedOut = WaitForCoreResponse (unCoreRequestId, &pResponse);

	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);

	m_pVE->MessageCleanup(estiMSG_CORE_RESPONSE, (size_t)pResponse);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::userSettingsGet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *pCoreRequest = nullptr;
	unsigned int unCoreRequestId;
	bool bTestTimedOut = true;
	CstiCoreResponse *pResponse = nullptr;

	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	//
	// Test userSettingsGet
	//
	pCoreRequest = new CstiCoreRequest;

	pCoreRequest->userSettingsGet (nullptr);

	hResult = m_pVE->CoreRequestSend(pCoreRequest, &unCoreRequestId);

	CPPUNIT_ASSERT_MESSAGE ("CoreRequestSend failed", !stiIS_ERROR (hResult));

	bTestTimedOut = WaitForCoreResponse (unCoreRequestId, &pResponse);

	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);

	m_pVE->MessageCleanup(estiMSG_CORE_RESPONSE, (size_t)pResponse);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::updateVersionCheck ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *pCoreRequest = nullptr;
	unsigned int unCoreRequestId;
	bool bTestTimedOut = true;
	CstiCoreResponse *pResponse = nullptr;

	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	//
	// Test updateVersionCheck
	//
	pCoreRequest = new CstiCoreRequest;

	pCoreRequest->updateVersionCheck ("A.B.C", "D.E.F");

	hResult = m_pVE->CoreRequestSend(pCoreRequest, &unCoreRequestId);

	CPPUNIT_ASSERT_MESSAGE ("CoreRequestSend failed", !stiIS_ERROR (hResult));

	bTestTimedOut = WaitForCoreResponse (unCoreRequestId, &pResponse);

	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);

	m_pVE->MessageCleanup(estiMSG_CORE_RESPONSE, (size_t)pResponse);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::LocalCameraMove (
	uint8_t un8CameraMovement)
{
	for (int i = 0; i < 10; i++)
	{
		dynamic_cast<IVideoInputVP2 *>(IstiVideoInput::InstanceGet())->cameraMove (un8CameraMovement);
		usleep (250000);
	}
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::CallDial (
	EstiDialMethod eDialMethod,
	const char *pszDialString,
	bool bUsesDirectoryResolve,
	bool bVrsCall)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bTestTimedOut = true;
	bool bDialingTimedOut = false;
	bool bRingingTimedOut = false;
	bool bRemoteRingingTimedOut = false;
	bool bDirectoryResolveTimedOut = false;
	bool bConferencingTimedOut = false;
	bool bHoldTimedOut = false;
	bool bResumeTimedOut = false;
	IstiCall *pCall;
	std::shared_ptr<IstiCall> call;
	size_t MessageParam;

	stiTrace ("<CallDial> Dialing %s, DirectoryResolve = %s\n", pszDialString, bUsesDirectoryResolve ? "Yes" : "No");
	hResult = m_pVE->CallDial (eDialMethod, pszDialString, nullptr, nullptr, estiDS_UNKNOWN, &pCall);

	call = pCall->sharedPointerGet ();

	CPPUNIT_ASSERT_MESSAGE (std::string("CallDial failed: ") + std::string (pszDialString), !stiIS_ERROR (hResult));

	if (bUsesDirectoryResolve)
	{
#if 0
		WaitForMessage (estiMSG_RESOLVING_NAME, &MessageParam, &bTestTimedOut);

		CPPUNIT_ASSERT_MESSAGE (std::string("Resolving timed out: ") + std::string (pszDialString),
								!stiIS_ERROR (hResult));
#endif
		WaitForMessage (estiMSG_DIRECTORY_RESOLVE_RESULT, &MessageParam, &bDirectoryResolveTimedOut);

		if (!bDirectoryResolveTimedOut)
		{
			auto pCallResolution = (SstiCallResolution *)MessageParam;

			hResult = pCallResolution->pCall->ContinueDial ();

			CPPUNIT_ASSERT_MESSAGE (std::string("ContinueDial failed: ") + std::string (pszDialString),
									!stiIS_ERROR (hResult));

			delete pCallResolution;
			pCallResolution = nullptr;
			//m_pVE->MessageCleanup (estiMSG_DIRECTORY_RESOLVE_RESULT, MessageParam);
		}
	}

	//
	// Wait on condition before proceeding.
	//
	if (!bVrsCall)
	{
		WaitForMessage (estiMSG_DIALING, &bDialingTimedOut);
	}

	if (!bDialingTimedOut)
	{
		//
		// Wait on condition before proceeding.
		//
		if (!bVrsCall)
		{
			WaitForMessage (estiMSG_RINGING, &bRingingTimedOut);
		}
		
		if (!bRingingTimedOut)
		{
			//
			// Wait on condition before proceeding.
			//
			if (!bVrsCall)
			{
				WaitForRemoteVrclMessage (estiMSG_VRCL_CLIENT_STATUS_INCOMING, &MessageParam, &bRemoteRingingTimedOut);
			}
			
			if (!bRemoteRingingTimedOut)
			{
				if (!bVrsCall)
				{
					//
					// Tell the remote system to answer the call.
					//
					m_oRemoteClient.Answer ();
				}

				//
				// Wait on condition before proceeding.
				//
				WaitForMessage (estiMSG_CONFERENCING, &bConferencingTimedOut);

				sleep (10);

				if (!bConferencingTimedOut)
				{

					//
					// Send and receive Text
					//
					for (int i = 0; ShareTextString[i]; i++)
					{
						uint16_t un16TmpText[2];

						un16TmpText[0] = ShareTextString[i];
						un16TmpText[1] = '\0';

						call->TextSend(un16TmpText, estiSTS_KEYBOARD);

						char szTmpText[2];

						szTmpText[0] = (char)ShareTextString[i];
						szTmpText[1] = '\0';

						m_oRemoteClient.TextMessageSend (szTmpText);

						usleep(250000);
					}

					sleep (5);

					//
					// Local Pan, tilt, zoom
					//
					if (call->RemotePtzCapsGet ())
					{
						LocalCameraMove (estiPAN_LEFT);
						LocalCameraMove (estiPAN_RIGHT);
						LocalCameraMove (estiTILT_UP);
						LocalCameraMove (estiTILT_DOWN);
						LocalCameraMove (estiZOOM_IN);
						LocalCameraMove (estiZOOM_OUT);
					}

					call->RemoteLightRingFlash ();

					if (!bVrsCall)
					{
						if (call->IsHoldableGet ())
						{
							//
							// Place call on hold
							//
							call->Hold ();

							WaitForMessage (estiMSG_HELD_CALL_LOCAL, &bHoldTimedOut);

							sleep (10);

							if (!bHoldTimedOut)
							{
								call->Resume ();

								WaitForMessage (estiMSG_RESUMED_CALL_LOCAL, &bResumeTimedOut);

								sleep (10);
							}
						}
					}
					
					if (!bVrsCall)
					{
						//
						// Have remote enable privacy
						//
						m_oRemoteClient.VideoPrivacySet (estiON);
						
						sleep (2);
						
						m_oRemoteClient.VideoPrivacySet (estiOFF);

						//
						// Turn off and on audio privacy on remote videophone.
						//
						m_oRemoteClient.AudioPrivacySet(estiOFF);
						WaitForRemoteVrclMessage (estiMSG_VRCL_AUDIO_PRIVACY_SET_SUCCESS, &MessageParam, &bTestTimedOut);
						CPPUNIT_ASSERT_MESSAGE ("Set Audio Privacy timed out: ", !bTestTimedOut);
						
						sleep (5);
						
						m_oRemoteClient.AudioPrivacySet(estiON);
						WaitForRemoteVrclMessage (estiMSG_VRCL_AUDIO_PRIVACY_SET_SUCCESS, &MessageParam, &bTestTimedOut);
						CPPUNIT_ASSERT_MESSAGE ("Set Audio Privacy timed out: ", !bTestTimedOut);
					}
					
					//
					// Get information from the call object.
					//
					std::string RemoteName;
					std::string RemoteAlternateName;
					EstiInterfaceMode eInterfaceMode;  stiUNUSED_ARG (eInterfaceMode);
					uint32_t un32Width, un32Height;
					SstiCallStatistics Stats;

					call->RemoteNameGet (&RemoteName);

					call->RemoteAlternateNameGet (&RemoteAlternateName);

					eInterfaceMode = call->RemoteInterfaceModeGet ();

					call->VideoPlaybackSizeGet (&un32Width, &un32Height);

					call->StatisticsGet (&Stats);

					call->IsInContactsGet ();
				}
			}
		}
	}

	//
	// Hangup the call.
	//
	hResult = call->HangUp ();

	//
	// Now check the "timed out" values
	//
	CPPUNIT_ASSERT_MESSAGE (std::string ("Directory Resolve timed out: ") + std::string (pszDialString),
							!bDirectoryResolveTimedOut);

	CPPUNIT_ASSERT_MESSAGE (std::string ("Dialing timed out: ") + std::string (pszDialString),
							!bDialingTimedOut);

	CPPUNIT_ASSERT_MESSAGE (std::string ("Ringing timed out: ") + std::string (pszDialString),
							!bRingingTimedOut);

	CPPUNIT_ASSERT_MESSAGE (std::string ("Remote Ringing timed out: ") + std::string (pszDialString),
							!bRemoteRingingTimedOut);
	CPPUNIT_ASSERT_MESSAGE (std::string ("Conferencing timed out: ") + std::string (pszDialString),
							!bConferencingTimedOut);

	CPPUNIT_ASSERT_MESSAGE (std::string ("Hold timed out: ") + std::string (pszDialString),
							!bHoldTimedOut);

	CPPUNIT_ASSERT_MESSAGE (std::string ("Resume timed out: ") + std::string (pszDialString),
							!bResumeTimedOut);

	CPPUNIT_ASSERT_MESSAGE (std::string ("Hangup failed: ") + std::string (pszDialString),
							!stiIS_ERROR (hResult));

	//
	// Wait on condition before proceeding.
	//
	WaitForMessage (estiMSG_DISCONNECTED, &MessageParam, &bTestTimedOut);

	hResult = m_pVE->CallObjectRemove(call.get ());

	CPPUNIT_ASSERT_MESSAGE (std::string ("Disconnected timed out: ")
							+ std::string (pszDialString),
							!stiIS_ERROR (hResult));

	CPPUNIT_ASSERT_MESSAGE (std::string ("CallObjectRemove failed: ") + std::string (pszDialString),
							!stiIS_ERROR (hResult));
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::CallDial ()
{
	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	const char *pszUnitTestPhoneNumber = g_UnitTestVideophoneNumber.c_str ();
	const char *pszUnitTestAddress = g_UnitTestVideophoneAddress.c_str ();
	auto pszSipUri = new char[4 + strlen (pszUnitTestPhoneNumber) + 1 + strlen (pszUnitTestAddress) + 1];
	sprintf (pszSipUri, "SIP:%s@%s", pszUnitTestPhoneNumber, pszUnitTestAddress);
	struct SDialStrings
	{
		EstiDialMethod eDialMethod;
		const char *pszDialString;
		bool bUsesDirectoryResolve;
		bool bVrsCall;
	} DialStrings[] =
	{
		{estiDM_UNKNOWN, pszUnitTestPhoneNumber, true, false},
//		{estiDM_UNKNOWN, pszUnitTestAddress, false},
//		{estiDM_UNKNOWN, "911", false, true},
//		{estiDM_UNKNOWN, "18017853722", true, true},
//		{estiDM_UNKNOWN, "011123456789", false, true},
//		{estiDM_UNKNOWN_WITH_VCO, "18017853722", true, true},
//		{estiDM_UNKNOWN_WITH_VCO, "011123456789", false, true},
//		{estiDM_UNKNOWN, pszSipUri, false},
	};
//	const int nMaxDialStrings = sizeof (DialStrings) / sizeof (DialStrings[0]);
	bool bTestTimedOut = false;
	size_t MessageParam;

	CoreLogin ();

	((CstiCCI*)m_pVE)->LocalNamesSet ("Automated Test System " SPECIAL_CALLER_ID_CHARS, "Automated Call Dial" SPECIAL_CALLER_ID_CHARS);

	m_pVE->VCOCallbackNumberSet ("18012879491");

	m_pVE->VCOUseSet (estiTRUE);

	ViewPositionsSet ();

	//
	// Set the user phone number.
	//
	SstiUserPhoneNumbers PhoneNumbers;

	memset (&PhoneNumbers, 0, sizeof (PhoneNumbers));

	strcpy (PhoneNumbers.szPreferredPhoneNumber, g_AccountPhoneNumber.c_str ());
	strcpy (PhoneNumbers.szLocalPhoneNumber, g_AccountPhoneNumber.c_str ());

	m_pVE->UserPhoneNumbersSet(&PhoneNumbers);

	RemoteVRCLClientConnect ();

	//
	// Make sure the remote phone is set so it doesn't use authorized number checking.
	//
	m_oRemoteClient.SettingSet("Number", "cmCheckForAuthorizedNumber", "0");
	WaitForRemoteVrclMessage (estiMSG_VRCL_SETTING_SET_SUCCESS, &MessageParam, &bTestTimedOut);

	CPPUNIT_ASSERT_MESSAGE (std::string ("Set Remote Setting timed out: ") + g_UnitTestVideophoneAddress,
								!bTestTimedOut);

	//
	// Set audio privacy on before making the calls.
	//
	m_oRemoteClient.AudioPrivacySet(estiON);
	WaitForRemoteVrclMessage (estiMSG_VRCL_AUDIO_PRIVACY_SET_SUCCESS, &MessageParam, &bTestTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Set Audio Privacy timed out: ", !bTestTimedOut);
	
	//
	// Make sure NAT Traversal is enabled on remote vidoephone
	//
//	m_oRemoteClient.SettingSet("Number", "NATTraversalSIP", "2");
	
	sleep (10);
	
//	for (int i = 0; i < nMaxDialStrings; i++)
	for (int i = 0, j = 0; j < 1; j++)
	{
		//
		// Test dialing by phone number
		//
		CallDial (DialStrings[i].eDialMethod, DialStrings[i].pszDialString,
				DialStrings[i].bUsesDirectoryResolve, DialStrings[i].bVrsCall);
		sleep (10);
	}
	
	//
	// Disable NAT Traversal on remote videophone
	//
//	m_oRemoteClient.SettingSet("Number", "NATTraversalSIP", "0");
	
	sleep (10);
	
	if (pszSipUri)
	{
		delete [] pszSipUri;
		pszSipUri = nullptr;
	}
}


void IstiVideophoneEngineTest::prepareForInboundCall ()
{
	CoreLogin ();
	((CstiCCI*)m_pVE)->LocalNamesSet ("Automated Test System " SPECIAL_CALLER_ID_CHARS, "Automated Call Dial" SPECIAL_CALLER_ID_CHARS);

	//
	// Set the user phone number.
	//
	SstiUserPhoneNumbers PhoneNumbers {};
	strcpy (PhoneNumbers.szPreferredPhoneNumber, g_AccountPhoneNumber.c_str ());
	strcpy (PhoneNumbers.szLocalPhoneNumber, g_AccountPhoneNumber.c_str ());
	m_pVE->UserPhoneNumbersSet(&PhoneNumbers);

	bool success = waitForConferencingReady ();
	CPPUNIT_ASSERT_MESSAGE ("Conference ready timed out", success);

	RemoteVRCLClientConnect ();

	// Sleep for five seconds before placing inbond call to ensure
	// registration process completes on the backend.
	sleep (5);
}


IstiCall *IstiVideophoneEngineTest::remoteDial ()
{
	bool bIncomingCallTimedOut = false;
	const int maxInboundAttempts {3};
	size_t MessageParam {0};

	prepareForInboundCall ();

	// Dial remotely
	for (int i = 0; i < maxInboundAttempts; i++)
	{
		m_oRemoteClient.DialExx (g_AccountPhoneNumber.c_str (), true);
		WaitForMessage (estiMSG_INCOMING_CALL, &MessageParam, &bIncomingCallTimedOut);

		if (!bIncomingCallTimedOut)
		{
			break;
		}

		// Make sure the remote call is not dialing.
		m_oRemoteClient.Hangup ();
	}

	CPPUNIT_ASSERT_MESSAGE (std::string ("Incoming timed out: ") + g_AccountPhoneNumber,
						!bIncomingCallTimedOut);

	return reinterpret_cast<IstiCall*>(MessageParam);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::CallAnswer ()
{
	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);
	bool bDisconnectTimedOut = false;
	bool bRemoteDisconnectedTimeOut = false;
	bool bConferencingTimedOut = false;
	size_t MessageParam {0};

	auto pCall = remoteDial ();

	pCall->Answer ();

	//
	// Wait on condition before proceeding.
	//
	WaitForMessage (estiMSG_CONFERENCING, &bConferencingTimedOut);

	if (!bConferencingTimedOut)
	{
		sleep (10);
	}

	// Hangup remotely
	m_oRemoteClient.Hangup ();

	WaitForMessage (estiMSG_DISCONNECTED, &bDisconnectTimedOut);

	m_pVE->CallObjectRemove(pCall);

	//
	// Wait for the remote to disconnect before moving on.
	//
	WaitForRemoteVrclMessage(estiMSG_DISCONNECTED, &MessageParam, &bRemoteDisconnectedTimeOut);

	CPPUNIT_ASSERT_MESSAGE (std::string ("Conferencing timed out: ") + g_AccountPhoneNumber,
							!bConferencingTimedOut);

	CPPUNIT_ASSERT_MESSAGE (std::string ("Disconnect timed out: ") + g_AccountPhoneNumber,
							!bDisconnectTimedOut);

	CPPUNIT_ASSERT_MESSAGE (std::string ("Remote Disconnect timed out: ") + g_AccountPhoneNumber,
							!bRemoteDisconnectedTimeOut);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::CallReject ()
{
	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);
	bool bDisconnectTimedOut = false;
	bool bRemoteDisconnectedTimeOut = false;
	int32_t message {0};
	size_t MessageParam {0};

	auto pCall = remoteDial ();

	pCall->Reject (estiRESULT_LOCAL_SYSTEM_BUSY);

	WaitForMessage (estiMSG_DISCONNECTED, &bDisconnectTimedOut);

	m_pVE->CallObjectRemove(pCall);

	//
	// Wait for the signmail greeting started message.
	// If the greeting has started then simply hangup.
	//
	WaitForRemoteVrclMessage (
		{estiMSG_VRCL_SIGNMAIL_GREETING_STARTED, estiMSG_VRCL_CLIENT_CALL_TERMINATED},
		&message, &MessageParam,
		&bDisconnectTimedOut);

	if (!bDisconnectTimedOut)
	{
		if (message == estiMSG_VRCL_SIGNMAIL_GREETING_STARTED)
		{
			m_oRemoteClient.Hangup ();

			//
			// Wait for the remote's call to end before moving on.
			//
			WaitForRemoteVrclMessage(estiMSG_VRCL_CLIENT_CALL_TERMINATED, &MessageParam, &bDisconnectTimedOut);
		}
	}

	CPPUNIT_ASSERT_MESSAGE (std::string ("Disconnect timed out: ") + g_AccountPhoneNumber,
							!bDisconnectTimedOut);

	CPPUNIT_ASSERT_MESSAGE (std::string ("Remote call terminated timed out: ") + g_AccountPhoneNumber,
							!bRemoteDisconnectedTimeOut);
	
	// Sleep for a bit to let the shutdown complete.
	sleep (5);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::RemoteCallReject ()
{
	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);
	bool bDisconnectTimedOut = false;
	bool bRemoteDisconnectedTimeOut = false;
	bool bRemoteRingingTimedOut = false;
	size_t MessageParam;
	IstiCall *pCall = nullptr;
	stiHResult hResult = stiRESULT_SUCCESS;

	CoreLogin ();

	((CstiCCI*)m_pVE)->LocalNamesSet ("Automated Test System " SPECIAL_CALLER_ID_CHARS, "Automated Call Dial" SPECIAL_CALLER_ID_CHARS);
	//
	// Set the user phone number.
	//
	SstiUserPhoneNumbers PhoneNumbers;
	memset (&PhoneNumbers, 0, sizeof (PhoneNumbers));
	strcpy (PhoneNumbers.szPreferredPhoneNumber, g_AccountPhoneNumber.c_str());
	strcpy (PhoneNumbers.szLocalPhoneNumber, g_AccountPhoneNumber.c_str());
	m_pVE->UserPhoneNumbersSet(&PhoneNumbers);

	RemoteVRCLClientConnect ();

	//
	// Dial the unit test vp
	//
	hResult = m_pVE->CallDial (estiDM_UNKNOWN, g_UnitTestVideophoneAddress.c_str (), nullptr, nullptr, estiDS_UNKNOWN, &pCall);
	CPPUNIT_ASSERT_MESSAGE (std::string("CallDial failed: ") + g_UnitTestVideophoneAddress, !stiIS_ERROR (hResult));

	//
	// Wait on condition before proceeding.
	//
	WaitForRemoteVrclMessage (estiMSG_VRCL_CLIENT_STATUS_INCOMING, &MessageParam, &bRemoteRingingTimedOut);

	if (!bRemoteRingingTimedOut)
	{
		//
		// Tell the remote system to reject the call.
		//
		m_oRemoteClient.Reject ();

		//
		// Wait to see if there is a disconnect message.
		//
		WaitForMessage (estiMSG_DISCONNECTED, &bDisconnectTimedOut);

		m_pVE->CallObjectRemove(pCall);
		pCall = nullptr;

		//
		// Wait for the remote to disconnect before moving on.
		//
		WaitForRemoteVrclMessage(estiMSG_DISCONNECTED, &MessageParam, &bRemoteDisconnectedTimeOut);

		CPPUNIT_ASSERT_MESSAGE (std::string ("Disconnect timed out: ") + g_UnitTestVideophoneAddress,
								!bDisconnectTimedOut);

		CPPUNIT_ASSERT_MESSAGE (std::string ("Remote Disconnect timed out: ") + g_UnitTestVideophoneAddress,
								!bRemoteDisconnectedTimeOut);
	}

	CPPUNIT_ASSERT_MESSAGE (std::string ("Remote ringing timed out: ") + g_UnitTestVideophoneAddress,
							!bRemoteRingingTimedOut);

	// Sleep for a bit to let the shutdown complete.
	sleep (5);
}


/*!
 * \brief Test remote answer with privacy.
 *
 * In this test we want to call a remote device and have it answer with privacy.
 */
void IstiVideophoneEngineTest::RemoteAnswerWithPrivacy ()
{
	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);
	bool bDisconnectTimedOut = false;
	bool bPrematureDisconnectTimedOut = false;
	bool bRemoteDisconnectedTimeOut = false;
	bool bConferencingTimedOut = false;
	bool bRemoteRingingTimedOut = false;
	size_t MessageParam;
	IstiCall *pCall = nullptr;
	stiHResult hResult = stiRESULT_SUCCESS;

	CoreLogin ();

	((CstiCCI*)m_pVE)->LocalNamesSet ("Automated Test System " SPECIAL_CALLER_ID_CHARS, "Automated Call Dial" SPECIAL_CALLER_ID_CHARS);
	
	//
	// Set the user phone number.
	//
	SstiUserPhoneNumbers PhoneNumbers;
	memset (&PhoneNumbers, 0, sizeof (PhoneNumbers));
	strcpy (PhoneNumbers.szPreferredPhoneNumber, g_AccountPhoneNumber.c_str());
	strcpy (PhoneNumbers.szLocalPhoneNumber, g_AccountPhoneNumber.c_str());
	m_pVE->UserPhoneNumbersSet(&PhoneNumbers);

	RemoteVRCLClientConnect ();

		// Get the local IP address.
		char szDialString[un8stiDIAL_STRING_LENGTH + 1];

		const char *Protocols[] =
		{
			"SIP"
		};
		const int nNumProtocols = sizeof (Protocols) / sizeof (Protocols[0]);

		for (int i = 0; i < nNumProtocols; i++)
		{
			//
			// Build the dial string.
			//
			sprintf (szDialString, "%s:%s", Protocols[i], g_UnitTestVideophoneAddress.c_str ());

			//
			// Turn on privacy before answering the call.
			//
			IstiVideoInput::InstanceGet ()->PrivacySet (estiTRUE);

			//
			// Dial the unit test vp
			//
			hResult = m_pVE->CallDial (estiDM_UNKNOWN, szDialString, nullptr, nullptr, estiDS_UNKNOWN, &pCall);
			CPPUNIT_ASSERT_MESSAGE (std::string("CallDial failed: ") + std::string (szDialString), !stiIS_ERROR (hResult));

			//
			// Wait on condition before proceeding.
			//
			WaitForRemoteVrclMessage (estiMSG_VRCL_CLIENT_STATUS_INCOMING, &MessageParam, &bRemoteRingingTimedOut);
		
			if (!bRemoteRingingTimedOut)
			{
				//
				// Tell the remote system to answer the call.
				//
			m_oRemoteClient.Answer ();

				//
				// Wait on condition before proceeding.
				//
			WaitForMessage (estiMSG_CONFERENCING, &MessageParam, &bConferencingTimedOut);

				if (!bConferencingTimedOut)
				{
					m_pVE->MessageCleanup(estiMSG_CONFERENCING, MessageParam);

					//
					// Wait to see if there is a disconnect message.  If there is then a problem occured.
					//
					WaitForMessage (estiMSG_DISCONNECTED, &MessageParam, &bPrematureDisconnectTimedOut);

					if (bPrematureDisconnectTimedOut)
					{
						//
						// Turn off privacy before ending the call
						//
						IstiVideoInput::InstanceGet ()->PrivacySet (estiFALSE);

						sleep (5); // Without this sleep, a deadlock situation is exposed (see bug 11097).
						
						// Hangup remotely
//						m_oRemoteClient.Hangup ();
						hResult = pCall->HangUp ();

						WaitForMessage (estiMSG_DISCONNECTED, &MessageParam, &bDisconnectTimedOut);

						if (!bDisconnectTimedOut)
						{
							m_pVE->MessageCleanup(estiMSG_DISCONNECTED, MessageParam);
						}
					}
					else
					{
					m_pVE->MessageCleanup(estiMSG_DISCONNECTED, MessageParam);
					}
				}

				//
				// Wait for the remote to disconnect before moving on.
				//
				WaitForRemoteVrclMessage(estiMSG_DISCONNECTED, &MessageParam, &bRemoteDisconnectedTimeOut);

				CPPUNIT_ASSERT_MESSAGE (std::string ("Disconnected prematurely: ") + std::string (szDialString),
										bPrematureDisconnectTimedOut);

				CPPUNIT_ASSERT_MESSAGE (std::string ("Conferencing timed out: ") + std::string (szDialString),
										!bConferencingTimedOut);

				CPPUNIT_ASSERT_MESSAGE (std::string ("Disconnect timed out: ") + std::string (szDialString),
										!bDisconnectTimedOut);

				CPPUNIT_ASSERT_MESSAGE (std::string ("Remote Disconnect timed out: ") + std::string (szDialString),
										!bRemoteDisconnectedTimeOut);
			}

		m_pVE->CallObjectRemove(pCall);
		pCall = nullptr;

		CPPUNIT_ASSERT_MESSAGE (std::string ("Remote ringing timed out: ") + std::string (szDialString),
								!bRemoteRingingTimedOut);
		}

	// Sleep for a bit to let the shutdown complete.
	sleep (5);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::RequestVideo ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bTestTimedOut = false;
	size_t MessageParam;
	bool bWaitingForPlayWhenReady = true;
	bool bWaitingForClosed = true;
	CstiMessageRequest *pMessageRequest = nullptr;
	unsigned int unMessageRequestId;
	CstiMessageResponse *pResponse = nullptr;

	CoreLogin ();

	// Get the message list so we can get a valid message to request.
	pMessageRequest = new CstiMessageRequest;
	pMessageRequest->MessageListGet (0, 100);

	hResult = m_pVE->MessageRequestSend(pMessageRequest, &unMessageRequestId);
	CPPUNIT_ASSERT_MESSAGE ("MessageRequestSend failed", !stiIS_ERROR (hResult));

	bTestTimedOut = WaitForMessageResponse (unMessageRequestId, &pResponse);
	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);

	// Get the message ID.
	CstiMessageList *pMessageList = pResponse->MessageListGet();
	int nListCount = pMessageList->CountGet();
	CPPUNIT_ASSERT_MESSAGE ("Message count is 0. No messages to view!", nListCount > 0);

	// Get the fist message in the list.
	CstiMessageListItemSharedPtr message = pMessageList->ItemGet(0);

	IstiMessageViewer *pMessageViewer = IstiMessageViewer::InstanceGet();

	CPPUNIT_ASSERT_MESSAGE ("MessageViewer is missing.", pMessageViewer);

	CstiItemId itemId = message->ItemIdGet();
	pMessageViewer->Open(itemId,
						 message->NameGet(),
						 message->DialStringGet());

	while (bWaitingForPlayWhenReady)
	{
		WaitForMessage (estiMSG_FILE_PLAY_STATE, &MessageParam, &bTestTimedOut);

		CPPUNIT_ASSERT_MESSAGE ("FilePlayer failed to play Video\n", !bTestTimedOut);

		CPPUNIT_ASSERT_MESSAGE ("FilePlayer encountered an error\n", MessageParam != IstiMessageViewer::estiERROR);

		switch(MessageParam)
		{
			case IstiMessageViewer::estiVIDEO_END:
				break;

			case IstiMessageViewer::estiOPENING:
				break;

			case IstiMessageViewer::estiPLAY_WHEN_READY:
				bWaitingForPlayWhenReady = false;
				break;

			case IstiMessageViewer::estiPLAYING:
				break;

			default:
				break;
		}
		
		m_pVE->MessageCleanup(estiMSG_FILE_PLAY_STATE, MessageParam);
	}
	
	pMessageViewer->Close();

	while (bWaitingForClosed)
	{
		WaitForMessage (estiMSG_FILE_PLAY_STATE, &MessageParam, &bTestTimedOut);

		CPPUNIT_ASSERT_MESSAGE ("FilePlayer failed to play Video\n", !bTestTimedOut);

		CPPUNIT_ASSERT_MESSAGE ("FilePlayer encountered an error\n", MessageParam != IstiMessageViewer::estiERROR);

		switch(MessageParam)
		{
			case IstiMessageViewer::estiCLOSED:
				bWaitingForClosed = false;
				break;

			case IstiMessageViewer::estiCLOSING:
				break;

			default:
				break;
		}
		
		m_pVE->MessageCleanup(estiMSG_FILE_PLAY_STATE, MessageParam);
	}

	// Cleanup the request result.
	m_pVE->MessageCleanup(estiMSG_MESSAGE_RESPONSE, (size_t)pResponse);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::RequestVideoFail ()
{
	bool bTestTimedOut = false;
	bool bWaitingForError = true;
	size_t MessageParam;

	CoreLogin ();

	IstiMessageViewer *pMessageViewer = IstiMessageViewer::InstanceGet();
	CPPUNIT_ASSERT_MESSAGE ("MessageViewer is missing.", pMessageViewer);

	CstiItemId itemId("0");
	pMessageViewer->Open(itemId,
						 "0",
						 "0");

	while (bWaitingForError)
	{
		WaitForMessage (estiMSG_FILE_PLAY_STATE, &MessageParam, &bTestTimedOut);

		CPPUNIT_ASSERT_MESSAGE ("FilePlayer failed to play Video\n", !bTestTimedOut);

		CPPUNIT_ASSERT_MESSAGE ("FilePlayer GUID is valid?\n", MessageParam != IstiMessageViewer::estiPLAYING);

		switch(MessageParam)
		{
			case IstiMessageViewer::estiOPENING:
				break;

			case IstiMessageViewer::estiRELOADING:
				// We failed to load the video (BAD guid) so now we are trying again.
				break;

			case IstiMessageViewer::estiREQUESTING_GUID:
				break;

			case IstiMessageViewer::estiERROR:
				{
					bWaitingForError = false;
					IstiMessageViewer::EError msgError = pMessageViewer->ErrorGet();
					CPPUNIT_ASSERT_MESSAGE ("FilePlayer GUID is valid?\n", msgError == IstiMessageViewer::estiERROR_REQUESTING_GUID);
				}
				break;

			default:
				break;
		}
	}

}

/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::PlayVideo ()
{
	bool bTestTimedOut = false;
	size_t MessageParam;
	bool bWaitingForClosed = true;
	uint32_t un32PausePoint = 0;  stiUNUSED_ARG (un32PausePoint);

	CoreLogin ();

	IstiMessageViewer *pMessageViewer = IstiMessageViewer::InstanceGet(); 
	CPPUNIT_ASSERT_MESSAGE ("MessageViewer is missing.", pMessageViewer);

#ifndef stiMESSAGE_MANAGER 
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiMessageRequest *pMessageRequest = NULL;
	CstiMessageResponse *pResponse = NULL;
	unsigned int unMessageRequestId;
	CstiItemId *pItemId = NULL;

	// Get the message list so we can get a valid message to request.
	pMessageRequest = new CstiMessageRequest;
	pMessageRequest->MessageListGet (0, 100);

	hResult = m_pVE->MessageRequestSend(pMessageRequest, &unMessageRequestId);
	CPPUNIT_ASSERT_MESSAGE ("MessageRequestSend failed", !stiIS_ERROR (hResult));

	bTestTimedOut = WaitForMessageResponse (unMessageRequestId, &pResponse);
	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);

	// Get the message ID.
	CstiMessageList *pMessageList = pResponse->MessageListGet();
	int nListCount = pMessageList->CountGet();
	CPPUNIT_ASSERT_MESSAGE ("Message count is 0. No messages to view!", nListCount > 0);

	// Get the fist message in the list.
	CstiMessageListItem *pMessage = pMessageList->ItemGet(0);

	// Check the start position on the message (we don't care what it is but check it).
	un32PausePoint = pMessage->PausePointGet();

	// Set the start position at 1 second.
	pMessageViewer->SetStartPosition(1);

	pItemId = new CstiItemId(pMessage->ItemIdGet());
	pMessageViewer->Open(pItemId,
						 pMessage->NameGet(),
						 pMessage->DialStringGet());
#else
	unsigned int unCategoryCount = 0;
	CstiItemId *pCategoryId = nullptr;
	CstiMessageInfo messageInfo;

	// sleep for a time to give the Message Manager time to get populated.
	sleep(2);

	// Find a message to play.
	IstiMessageManager *pMessageManager = m_pVE->MessageManagerGet ();

	// Get the number of messages in the sub category.
	pMessageManager->Lock();
	pMessageManager->CategoryCountGet(&unCategoryCount);
	for (unsigned int index = 0;
		 index < unCategoryCount;
		 index++)
	{
		pMessageManager->CategoryByIndexGet(index, &pCategoryId);

		unsigned int unMessageCount = 0;
		unsigned int unSubCategoryCount = 0;
		pMessageManager->MessageCountGet(*pCategoryId, &unMessageCount);
		pMessageManager->SubCategoryCountGet(*pCategoryId, &unSubCategoryCount);
		if (unMessageCount > 0)
		{
			pMessageManager->MessageByIndexGet(*pCategoryId, 0, &messageInfo);
			break;
		}

		if (unSubCategoryCount > 0)
		{
			CstiItemId *pSubCategory = nullptr;
			pMessageManager->SubCategoryByIndexGet(*pCategoryId, 0, &pSubCategory);
			
			// If we have a sub category it has a message so just load the first message.
			pMessageManager->MessageByIndexGet(*pSubCategory, 0, &messageInfo);
			break;
		}
	}
	pMessageManager->Unlock();

	pMessageViewer->Open(messageInfo.ItemIdGet(),
						 messageInfo.NameGet(),
						 messageInfo.DialStringGet());
#endif
	//******** Note: This test is expecting video 357.H264 (CATS) or a video of 
	//********  	 similar length 3:12 otherwise this test will fail. 
	//
	// We want to walk the player thourgh varies test states to test different pieces
	// of the code.  We should follow the this path.
	// 	Play for 5 sec. Pause player (FPTS_PAUSE)
	//	Pause for 5 sec begin playing to end (FPTS_PAUSED_PLAY)
	// 	Play to end of video Restart (FPTS_SKIPBEGIN)
	//  Play 1 sec. Skip forward twice play to end (FPTS_SKIPFORWARD)
	//	Skip back 4 times (FPTS_SKIPBACK)
	// 	Skip to end (FPTS_SKIPEND)
	// 	Close file.
	//

	int nFPTState = FPTS_PLAY;
	while (nFPTState != FPTS_TEST_FINISHED)
	{
		WaitForMessage (estiMSG_FILE_PLAY_STATE, &MessageParam, &bTestTimedOut);

		CPPUNIT_ASSERT_MESSAGE ("FilePlayer failed to play Video\n", !bTestTimedOut);

		CPPUNIT_ASSERT_MESSAGE ("FilePlayer encountered an error\n", MessageParam != IstiMessageViewer::estiERROR);

		switch(MessageParam)
		{
			case IstiMessageViewer::estiVIDEO_END:
			{
				switch(nFPTState)
				{
				case FPTS_SKIPEND:
				{
					nFPTState = FPTS_TEST_FINISHED;
						
						break;
				}

				case FPTS_PAUSED_PLAY:
				{
					nFPTState = FPTS_SKIPBEGIN;
					pMessageViewer->Restart();

						break;
				}

				case FPTS_SKIPFORWARD:
				{
					nFPTState = FPTS_SKIPBACK;

					pMessageViewer->SkipBack(5);

						break;
				}

				case FPTS_SKIPBACK:
				{
					nFPTState = FPTS_SKIPEND;
					pMessageViewer->Restart();

						break;
				}

				default:
					break;
				}

				break;
			}

			case IstiMessageViewer::estiOPENING:
				break;

			case IstiMessageViewer::estiPLAY_WHEN_READY:
				break;

			case IstiMessageViewer::estiPLAYING:
			{
				switch(nFPTState)
				{
				case FPTS_PLAY:
				{
					sleep (5);
					nFPTState = FPTS_PAUSE;
					pMessageViewer->Pause();

						break;
				}

				case FPTS_PAUSED_PLAY:
					// Skip near the end of the file.
					pMessageViewer->SkipForward(170);
					sleep(1);
					break;

				case FPTS_SKIPBEGIN:
				{
					sleep(1);
					nFPTState = FPTS_SKIPFORWARD;
					pMessageViewer->SkipForward(90);
					sleep(1);
					pMessageViewer->SkipForward(80);

						break;
				}

				case FPTS_SKIPBACK:
				{
					sleep(1);
					pMessageViewer->SkipBack(5);
					sleep(1);
					pMessageViewer->SkipBack(5);
					sleep(1);
					pMessageViewer->SkipBack(5);

					// Test fast forward
					pMessageViewer->FastForward();

						break;
				}

				case FPTS_SKIPEND:
				{
					sleep(1);
					pMessageViewer->SkipEnd();

						break;
				}

				default:
					break;
				}

				break;
			}

			case IstiMessageViewer::estiPAUSED:
				{
					if (nFPTState == FPTS_PAUSE)
					{
						nFPTState = FPTS_PAUSED_PLAY;
						pMessageViewer->Play();
					}

				break;
				}

			default:
				break;
		}
		
		m_pVE->MessageCleanup(estiMSG_FILE_PLAY_STATE, MessageParam);
	}

	// Store the pause point.
#ifndef stiMESSAGE_MANAGER 
	un32PausePoint = pMessageViewer->StopPositionGet();
	pMessageRequest = new CstiMessageRequest;
	if (pMessageRequest)
	{
		pMessageRequest->MessagePausePointSave(pItemId,
											   pMessageViewer->GetViewId(), 
											   un32PausePoint);

		m_pVE->MessageRequestSend(pMessageRequest, NULL);
	}
#endif

	// Check the DownloadBitRate() and AverageFrameRate().
	un32PausePoint = pMessageViewer->GetDownloadBitRate();
	un32PausePoint =  pMessageViewer->GetAverageFrameRate();

	// Close the viewer.
	pMessageViewer->Close();

	while (bWaitingForClosed)
	{
		WaitForMessage (estiMSG_FILE_PLAY_STATE, &MessageParam, &bTestTimedOut);

		CPPUNIT_ASSERT_MESSAGE ("FilePlayer failed to play Video\n", !bTestTimedOut);

		CPPUNIT_ASSERT_MESSAGE ("FilePlayer encountered an error\n", MessageParam != IstiMessageViewer::estiERROR);

		switch(MessageParam)
		{
			case IstiMessageViewer::estiCLOSED:
				bWaitingForClosed = false;
				break;

			case IstiMessageViewer::estiCLOSING:
				break;

			default:
				break;
		}
		
		m_pVE->MessageCleanup(estiMSG_FILE_PLAY_STATE, MessageParam);
	}

#ifndef stiMESSAGE_MANAGER 
	// Mark the message as viwed.
	pMessageRequest = new CstiMessageRequest;
	if (pMessageRequest)
	{
		pMessageRequest->MessageViewed (pItemId, 
										pMessage->NameGet(),
										pMessage->DialStringGet());
		m_pVE->MessageRequestSend (pMessageRequest, NULL);
	}

	// Cleanup the request result.
	m_pVE->MessageCleanup(estiMSG_MESSAGE_RESPONSE, (size_t)pResponse);

	// Cleanup the item ID.
	if (pItemId)
	{
		delete pItemId;
		pItemId = NULL;
	}   			   
#endif

}

void IstiVideophoneEngineTest::PlayVideoNoPlayerControls ()
{                            
	bool bTestTimedOut = false;
	size_t MessageParam;
	bool bWaitingForPlayWhenReady = true;
	bool bPlayPass1 = false; 

	CoreLogin ();

	IstiMessageViewer *pMessageViewer = IstiMessageViewer::InstanceGet(); 
	CPPUNIT_ASSERT_MESSAGE ("MessageViewer is missing.", pMessageViewer);

#ifndef stiMESSAGE_MANAGER 
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiMessageRequest *pMessageRequest = NULL;
	CstiMessageResponse *pResponse = NULL;
	unsigned int unMessageRequestId;
	CstiItemId *pItemId = NULL;

	// Get the message list so we can get a valid message to request.
	pMessageRequest = new CstiMessageRequest;
	pMessageRequest->MessageListGet (0, 100);

	hResult = m_pVE->MessageRequestSend(pMessageRequest, &unMessageRequestId);
	CPPUNIT_ASSERT_MESSAGE ("MessageRequestSend failed", !stiIS_ERROR (hResult));

	bTestTimedOut = WaitForMessageResponse (unMessageRequestId, &pResponse);
	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);

	// Get the message ID.
	CstiMessageList *pMessageList = pResponse->MessageListGet();
	int nListCount = pMessageList->CountGet();
	CPPUNIT_ASSERT_MESSAGE ("Message count is 0. No messages to view!", nListCount > 0);

	// Get the fist message in the list.
	CstiMessageListItem *pMessage = pMessageList->ItemGet(0);

	pItemId = new CstiItemId(pMessage->ItemIdGet());
	pMessageViewer->Open(pItemId,
						 pMessage->NameGet(),
						 pMessage->DialStringGet());
#else
	unsigned int unCategoryCount = 0;
	CstiItemId *pCategoryId = nullptr;
	CstiMessageInfo messageInfo;

	// sleep for a time to give the Message Manager time to get populated.
	sleep(2);

	// Find a message to play.
	IstiMessageManager *pMessageManager = m_pVE->MessageManagerGet ();

	// Get the number of messages in the sub category.
	pMessageManager->Lock();
	pMessageManager->CategoryCountGet(&unCategoryCount);
	for (unsigned int index = 0;
		 index < unCategoryCount;
		 index++)
	{
		pMessageManager->CategoryByIndexGet(index, &pCategoryId);

		unsigned int unMessageCount = 0;
		unsigned int unSubCategoryCount = 0;
		pMessageManager->MessageCountGet(*pCategoryId, &unMessageCount);
		pMessageManager->SubCategoryCountGet(*pCategoryId, &unSubCategoryCount);
		if (unMessageCount > 0)
		{
			pMessageManager->MessageByIndexGet(*pCategoryId, 0, &messageInfo);
			break;
		}

		if (unSubCategoryCount > 0)
		{
			CstiItemId *pSubCategory = nullptr;
			pMessageManager->SubCategoryByIndexGet(*pCategoryId, 0, &pSubCategory);
			
			// If we have a sub category it has a message so just load the first message.
			pMessageManager->MessageByIndexGet(*pSubCategory, 0, &messageInfo);
			break;
		}
	}
	pMessageManager->Unlock();

	pMessageViewer->Open(messageInfo.ItemIdGet(),
						 messageInfo.NameGet(),
						 messageInfo.DialStringGet());
#endif

	while (bWaitingForPlayWhenReady)
	{
		WaitForMessage (estiMSG_FILE_PLAY_STATE, &MessageParam, &bTestTimedOut);

		CPPUNIT_ASSERT_MESSAGE ("FilePlayer failed to play Video\n", !bTestTimedOut);

		CPPUNIT_ASSERT_MESSAGE ("FilePlayer encountered an error\n", MessageParam != IstiMessageViewer::estiERROR);

		switch(MessageParam)
		{
			case IstiMessageViewer::estiVIDEO_END:
				break;

			case IstiMessageViewer::estiOPENING:
				break;

			case IstiMessageViewer::estiPLAY_WHEN_READY:
				break;

			case IstiMessageViewer::estiPLAYING:
			{
				sleep (5);
				if (!bPlayPass1)
				{
					pMessageViewer->Pause();
					bPlayPass1 = true;
				}
				else
				{
					bWaitingForPlayWhenReady = false;
				}
			}
				break;

			case IstiMessageViewer::estiPAUSED:
				sleep (5);
				pMessageViewer->Restart();
				break;

			default:
				break;
		}
	}
	
	pMessageViewer->Close();

	bool bWaitingForClosed = true;
	
	while (bWaitingForClosed)
	{
	WaitForMessage (estiMSG_FILE_PLAY_STATE, &MessageParam, &bTestTimedOut);

		CPPUNIT_ASSERT_MESSAGE ("FilePlayer failed to play Video\n", !bTestTimedOut);

		CPPUNIT_ASSERT_MESSAGE ("FilePlayer encountered an error\n", MessageParam != IstiMessageViewer::estiERROR);

		switch(MessageParam)
		{
			case IstiMessageViewer::estiCLOSED:
				bWaitingForClosed = false;
				break;

			case IstiMessageViewer::estiCLOSING:
				break;

			default:
				break;
		}
		
		m_pVE->MessageCleanup(estiMSG_FILE_PLAY_STATE, MessageParam);
	}
	
#ifndef stiMESSAGE_MANAGER 
	// Cleanup the request result.
	m_pVE->MessageCleanup(estiMSG_MESSAGE_RESPONSE, (size_t)pResponse);

	// Cleanup the item ID.
	if (pItemId)
	{
		delete pItemId;
		pItemId = NULL;
	}   			   
#endif
}


void IstiVideophoneEngineTest::RecordSignMail ()
{
	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);
	bool bRemoteRingingTimedOut = false;
	bool bTestTimedOut = false;
	bool bDirectoryResolveTimedOut = false;
	bool bWaitingForRecord = true;
	size_t MessageParam;
	IstiCall *pCall = nullptr;
	stiHResult hResult = stiRESULT_SUCCESS;

	ViewPositionsSet ();

	CoreLogin ();

	// Get the message viewer.
	IstiMessageViewer *pMessageViewer = IstiMessageViewer::InstanceGet(); 
	CPPUNIT_ASSERT_MESSAGE ("MessageViewer is missing.", pMessageViewer);

	((CstiCCI*)m_pVE)->LocalNamesSet ("Automated Test System " SPECIAL_CALLER_ID_CHARS, "Automated Call Dial" SPECIAL_CALLER_ID_CHARS);

	//
	// Set the user phone number.
	//
	SstiUserPhoneNumbers PhoneNumbers;
	memset (&PhoneNumbers, 0, sizeof (PhoneNumbers));
	strcpy (PhoneNumbers.szPreferredPhoneNumber, g_AccountPhoneNumber.c_str());
	strcpy (PhoneNumbers.szLocalPhoneNumber, g_AccountPhoneNumber.c_str());
	m_pVE->UserPhoneNumbersSet(&PhoneNumbers);

	RemoteVRCLClientConnect ();

	stiTrace ("*************************** Dialing the remote videophone: %s.\n", g_UnitTestVideophoneNumber.c_str());
	
	//
	// Dial the unit test vp
	//
	hResult = m_pVE->CallDial (estiDM_UNKNOWN, g_UnitTestVideophoneNumber.c_str (), nullptr, nullptr, estiDS_UNKNOWN, &pCall);
	CPPUNIT_ASSERT_MESSAGE (std::string("CallDial failed: ") + g_UnitTestVideophoneNumber, !stiIS_ERROR (hResult));

	{
		WaitForMessage (estiMSG_DIRECTORY_RESOLVE_RESULT, &MessageParam, &bDirectoryResolveTimedOut);

		if (!bDirectoryResolveTimedOut)
		{
			auto pCallResolution = (SstiCallResolution *)MessageParam;

			hResult = pCallResolution->pCall->ContinueDial ();

			delete pCallResolution;
			pCallResolution = nullptr;
			//m_pVE->MessageCleanup (estiMSG_DIRECTORY_RESOLVE_RESULT, MessageParam);

			CPPUNIT_ASSERT_MESSAGE (std::string("RecordSignMail failed to call: ") + g_UnitTestVideophoneNumber,
									!stiIS_ERROR (hResult));
		}

		stiTrace ("*************************** Waiting for remote videophone to see incoming call.\n");
		
		//
		// Wait on condition before proceeding.
		//
		WaitForRemoteVrclMessage (estiMSG_VRCL_CLIENT_STATUS_INCOMING, &MessageParam, &bRemoteRingingTimedOut);
	
		CPPUNIT_ASSERT_MESSAGE ("remote ringing timed out\n", !bRemoteRingingTimedOut);
		
			//
			// Tell the remote system to reject the call.
			//
		m_oRemoteClient.Reject ();

		//
		// Wait for call to be in the "leave message" state.
		//
		WaitForMessage (estiMSG_LEAVE_MESSAGE, &MessageParam, &bTestTimedOut);
		CPPUNIT_ASSERT_MESSAGE ("Failed to leave message.\n", !bTestTimedOut);

		stiTrace ("*************************** Waiting for greeting to play.\n");
		
		//
		// Wait for the greeting to begin playing and then skip it
		// and then wait for the closed state.
		//
		bool bWaitingForGreeting = true;
		
		while (bWaitingForGreeting)
		{
			WaitForMessage (estiMSG_FILE_PLAY_STATE, &MessageParam, &bTestTimedOut);
			
			CPPUNIT_ASSERT_MESSAGE ("Did not receive estiMSG_FILE_PLAY_STATE message.\n", !bTestTimedOut);
		
			CPPUNIT_ASSERT_MESSAGE ("FilePlayer encountered an error\n", MessageParam != IstiMessageViewer::estiERROR);
			
			if (MessageParam == IstiMessageViewer::estiPLAYING)
			{
				sleep (5);
				pMessageViewer->SkipSignMailGreeting();
			}
			else if (MessageParam == IstiMessageViewer::estiCLOSED)
			{
				bWaitingForGreeting = false;
			}

			m_pVE->MessageCleanup (estiMSG_FILE_PLAY_STATE, MessageParam);
		}
		
		stiTrace ("*************************** Waiting for recording.\n");
		
		//
		// Now wait for the recording, stop the recording and then wait for the closed state.
		//
		while (bWaitingForRecord)
		{
			WaitForMessage (estiMSG_FILE_PLAY_STATE, &MessageParam, &bTestTimedOut);

			CPPUNIT_ASSERT_MESSAGE ("Did not receive estiMSG_FILE_PLAY_STATE message.\n", !bTestTimedOut);

			CPPUNIT_ASSERT_MESSAGE ("FilePlayer encountered an error\n", MessageParam != IstiMessageViewer::estiERROR);

			switch(MessageParam)
			{
				case IstiMessageViewer::estiOPENING:
				stiTrace ("IstiMessageViewer::estiOPENING\n");
					break;

				case IstiMessageViewer::estiPLAY_WHEN_READY:
				stiTrace ("IstiMessageViewer::estiPLAY_WHEN_READY\n");
					break;

				case IstiMessageViewer::estiPLAYING:
				stiTrace ("IstiMessageViewer::estiPLAYING\n");
					break;

				case IstiMessageViewer::estiRECORDING:
				{
					sleep (3);
				
//					CPPUNIT_ASSERT_MESSAGE ("Test", false);
				
					// We can't record yet so stop recording and exit.
					pMessageViewer->RecordStop();
				
					sleep (3);
				
					break;
				}

				case IstiMessageViewer::estiCLOSED:
					
					stiTrace ("IstiMessageViewer::estiCLOSED\n");
					
					bWaitingForRecord = false;
					
					break;
					
				case IstiMessageViewer::estiUPLOADING:
					
					stiTrace ("IstiMessageViewer::estiUPLOADING\n");
					
					break;
					
				case IstiMessageViewer::estiUPLOAD_COMPLETE:
					
					stiTrace ("IstiMessageViewer::estiUPLOAD_COMPLETE\n");
					
					break;

				case IstiMessageViewer::estiRELOADING:
					
					stiTrace ("IstiMessageViewer::estiRELOADING\n");
					
					break;
					
				case IstiMessageViewer::estiPAUSED:
					
					stiTrace ("IstiMessageViewer::estiPAUSED\n");
					
					break;
					
				case IstiMessageViewer::estiCLOSING:
					
					stiTrace ("IstiMessageViewer::estiCLOSING\n");
					
					break;
					
				case IstiMessageViewer::estiERROR:
					
					stiTrace ("IstiMessageViewer::estiERROR\n");
					
					break;
					
				case IstiMessageViewer::estiVIDEO_END:
					
					stiTrace ("IstiMessageViewer::estiVIDEO_END\n");
					
					break;
					
				case IstiMessageViewer::estiREQUESTING_GUID:
					
					stiTrace ("IstiMessageViewer::estiREQUESTING_GUID\n");
					
					break;
					
				case IstiMessageViewer::estiRECORD_CONFIGURE:
					
					stiTrace ("IstiMessageViewer::estiRECORD_CONFIGURE\n");
					
					break;
					
				case IstiMessageViewer::estiWAITING_TO_RECORD:
					
					stiTrace ("IstiMessageViewer::estiWAITING_TO_RECORD\n");
					
					break;
					
				case IstiMessageViewer::estiRECORD_FINISHED:
					
					stiTrace ("IstiMessageViewer::estiRECORD_FINISHED\n");
					
					break;
					
				case IstiMessageViewer::estiPLAYER_IDLE:
					
					stiTrace ("IstiMessageViewer::estiPLAYER_IDLE\n");
					
					break;

				default:
				
					stiTrace ("Some state change.\n");
				
					break;
			}
		
			m_pVE->MessageCleanup (estiMSG_FILE_PLAY_STATE, MessageParam);
		}

		stiTrace ("*************************** Deleting the recording.\n");
	
		// Don't send the recorded message.
		pMessageViewer->DeleteRecordedMessage();
	}

	// Cleanup
	m_pVE->CallObjectRemove(pCall);
	pCall = nullptr;

	// Sleep for a bit to let the shutdown complete.
	sleep (5);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::RelayLanguageListGet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	// Request the relay language list from videophone engine (with NULL parameter)
	hResult = m_pVE->RelayLanguageListGet (nullptr);
	CPPUNIT_ASSERT_MESSAGE ("RelayLanguageListGet did not fail (as it was supposed to)", stiIS_ERROR (hResult));

	std::vector<std::string> RelayLanguage;
	// Request the relay language list from videophone engine (with valid parameter)
	hResult = m_pVE->RelayLanguageListGet (&RelayLanguage);
	CPPUNIT_ASSERT_MESSAGE ("RelayLanguageListGet failed", !stiIS_ERROR (hResult));

}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::UpdateDetermine ()
{
#if 0
	stiHResult hResult = stiRESULT_SUCCESS;
	size_t MessageParam;
	bool bTestTimedOut = false;
#if 0
	std::list<SstiUpdateSource> *pUpdateSourceList;
#endif
	
	CoreLogin ();

	hResult = m_pVE->UpdateDetermine ();

	CPPUNIT_ASSERT_MESSAGE ("UpdateDetermine failed", !stiIS_ERROR (hResult));

	WaitForMessage (estiMSG_UPDATE_CHECKING, &MessageParam, &bTestTimedOut);

	CPPUNIT_ASSERT_MESSAGE ("estiMSG_UPDATE_CHECKING not received", !bTestTimedOut);

#if 0
	WaitForMessage (estiMSG_UPDATE_NEEDED, &MessageParam, &bTestTimedOut);

	CPPUNIT_ASSERT_MESSAGE ("estiMSG_UPDATE_NEEDED not received", !bTestTimedOut);

	pUpdateSourceList = (std::list<SstiUpdateSource> *)MessageParam;
	
	m_pVE->Update (pUpdateSourceList);
#else
	for (;;)
	{
		WaitForMessage (estiMSG_UPDATE_DOWNLOADING, &MessageParam, &bTestTimedOut);
		m_pVE->MessageCleanup(estiMSG_UPDATE_DOWNLOADING, MessageParam);

		CPPUNIT_ASSERT_MESSAGE ("estiMSG_UPDATE_DOWNLOADING not received", !bTestTimedOut);

		if (MessageParam >= 99)
		{
			break;
		}
	}

	WaitForMessage (60 * 3, estiMSG_UPDATE_PROGRAMMING, &MessageParam, &bTestTimedOut);
	m_pVE->MessageCleanup(estiMSG_UPDATE_PROGRAMMING, MessageParam);

	CPPUNIT_ASSERT_MESSAGE ("estiMSG_UPDATE_PROGRAMMING not received", !bTestTimedOut);

	for (;;)
	{
		WaitForMessage (estiMSG_UPDATE_PROGRAMMING, &MessageParam, &bTestTimedOut);
		m_pVE->MessageCleanup(estiMSG_UPDATE_PROGRAMMING, MessageParam);

		CPPUNIT_ASSERT_MESSAGE ("estiMSG_UPDATE_PROGRAMMING not received", !bTestTimedOut);

		if (MessageParam >= 99)
		{
			break;
		}
	}
#endif

	WaitForMessage (estiMSG_UPDATE_SUCCESSFUL, &MessageParam, &bTestTimedOut);

	CPPUNIT_ASSERT_MESSAGE ("estiMSG_UPDATE_SUCCESSFUL not received", !bTestTimedOut);
#endif

}


#if 0
/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::UpdateFirmware ()
{
	size_t MessageParam;
	bool bTestTimedOut = false;

	CoreLogin ();

	std::list<SstiUpdateSource> *pUpdateSourceList = new std::list<SstiUpdateSource>;
	SstiUpdateSource UpdateSource;

	UpdateSource.eUpdateType = SstiUpdateSource::eFIRMWARE;
	UpdateSource.strServer = UPDATE_SERVER;
	UpdateSource.strFile = UPDATE_MPU_COMBO_FILE;
	UpdateSource.strUser = "anonymous";
	// Unique id used for the update server password
	char szUniqueID[nMAX_UNIQUE_ID_LENGTH];
	stiOSGetUniqueID (szUniqueID);
	UpdateSource.strPassword = szUniqueID;

	pUpdateSourceList->push_back(UpdateSource);


	// Add apploader to update?
	UpdateSource.eUpdateType = SstiUpdateSource::eAPPLOADER;
	UpdateSource.strFile = UPDATE_RCU_FLIMG_FILE;

	pUpdateSourceList->push_front(UpdateSource);

	m_pVE->Update (pUpdateSourceList);

	WaitForMessage (estiMSG_UPDATE_SUCCESSFUL, &MessageParam, &bTestTimedOut);

	CPPUNIT_ASSERT_MESSAGE ("estiMSG_UPDATE_SUCCESSFUL not received", !bTestTimedOut);

}
#endif


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::BlockListAdd ()
{
	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	std::string Description;
	stiHResult hResult;
	unsigned int unRequestId;
	bool bTimedOut;
	size_t Param;

	IstiBlockListManager *pBLManager = m_pVE->BlockListManagerGet ();
	CPPUNIT_ASSERT_MESSAGE ("Block list manager unavailable. (Did you forget to set the BlockListEnabled property?)", pBLManager != nullptr);
	((WillowPM::CstiBlockListManager*)pBLManager)->EnabledSet (true);

	// Test to make sure we cannot add the item when the user is not logged in.
	hResult = pBLManager->ItemAdd (BLOCKED_USER_NUMBER, BLOCKED_USER_NAME, &unRequestId);

	CPPUNIT_ASSERT_MESSAGE ("Block list item added without being logged in!",
							stiIS_ERROR (hResult));

	// Now log in.
	CoreLogin ();

	// Try and remove if exising (this is to get to a known state, not a test)
	hResult = pBLManager->ItemDelete (BLOCKED_USER_NUMBER, &unRequestId);
	if (!stiIS_ERROR (hResult))
	{
		WaitForMessage(estiMSG_BLOCK_LIST_ITEM_DELETED, &Param, &bTimedOut);
		CPPUNIT_ASSERT_MESSAGE ("estiMSG_BLOCK_LIST_ITEM_DELETED did not arrive in time",
								!bTimedOut);
	}

	//
	// Test to make sure we cannot add the item when the block list is disabled.
	//
	((WillowPM::CstiBlockListManager*)pBLManager)->EnabledSet (false);

	hResult = pBLManager->ItemAdd (BLOCKED_USER_NUMBER, BLOCKED_USER_NAME, &unRequestId);

	CPPUNIT_ASSERT_MESSAGE ("Block list item added when list was disabled", stiIS_ERROR (hResult));

	((WillowPM::CstiBlockListManager*)pBLManager)->EnabledSet (true);

	//
	// Test to make sure we can add the item.
	//
	hResult = pBLManager->ItemAdd (BLOCKED_USER_NUMBER, BLOCKED_USER_NAME, &unRequestId);

	CPPUNIT_ASSERT_MESSAGE ("Block list item not added", !stiIS_ERROR (hResult));

	WaitForMessage(estiMSG_BLOCK_LIST_ITEM_ADDED, &Param, &bTimedOut);

	CPPUNIT_ASSERT_MESSAGE ("estiMSG_BLOCK_LIST_ITEM_ADDED did not arrive in time", !bTimedOut);

	// Test to make sure we cannot add two of the same item.
	hResult = pBLManager->ItemAdd (BLOCKED_USER_NUMBER, BLOCKED_USER_NAME, &unRequestId);

	CPPUNIT_ASSERT_MESSAGE ("Block list item added twice", stiIS_ERROR (hResult));

	//
	// Remove the item to get back to a known state.
	//
	hResult = pBLManager->ItemDelete (BLOCKED_USER_NUMBER, &unRequestId);
	if (!stiIS_ERROR (hResult))
	{
		WaitForMessage(estiMSG_BLOCK_LIST_ITEM_DELETED, &Param, &bTimedOut);
		CPPUNIT_ASSERT_MESSAGE ("estiMSG_BLOCK_LIST_ITEM_DELETED did not arrive in time",
								!bTimedOut);
	}
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::BlockListGetByNumber ()
{
	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	IstiBlockListManager *pBLManager = m_pVE->BlockListManagerGet ();
	CPPUNIT_ASSERT_MESSAGE ("Block list manager unavailable. (Did you forget to set the BlockListEnabled property?)", pBLManager != nullptr);

	bool bWasBlocked;
	std::string Description;
	std::string ItemId;
	stiHResult hResult;
	unsigned int unRequestId;
	size_t Param;
	bool bTimedOut;

	// Now log in.
	CoreLogin ();

	// Try and add item (this is to get to a known state, not a test)
	hResult = pBLManager->ItemAdd (BLOCKED_USER_NUMBER, BLOCKED_USER_NAME, &unRequestId);
	if (stiRESULT_SUCCESS == hResult)
	{
		hResult = WaitForMessage (estiMSG_BLOCK_LIST_ITEM_ADDED, &Param, &bTimedOut);
	}

	// Test to make sure the item can be retreived
	bWasBlocked = pBLManager->ItemGetByNumber (BLOCKED_USER_NUMBER, &ItemId, &Description);

	CPPUNIT_ASSERT_MESSAGE ("Block list item not retreived", bWasBlocked == estiTRUE);
	CPPUNIT_ASSERT_MESSAGE ("Blocked user name mismatch", 0 == strcmp(Description.c_str (), BLOCKED_USER_NAME));

	//
	// Remove the item to get back to a known state.
	//
	hResult = pBLManager->ItemDelete (BLOCKED_USER_NUMBER, &unRequestId);
	if (!stiIS_ERROR (hResult))
	{
		WaitForMessage(estiMSG_BLOCK_LIST_ITEM_DELETED, &Param, &bTimedOut);
		CPPUNIT_ASSERT_MESSAGE ("estiMSG_BLOCK_LIST_ITEM_DELETED did not arrive in time",
								!bTimedOut);
	}
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::BlockListGetByIndex ()
{
	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	IstiBlockListManager *pBLManager = m_pVE->BlockListManagerGet ();
	CPPUNIT_ASSERT_MESSAGE ("Block list manager unavailable. (Did you forget to set the BlockListEnabled property?)", pBLManager != nullptr);

	std::string Number;
	std::string Description;
	std::string ItemId;
	stiHResult hResult;
	unsigned int unRequestId;
	size_t Param;
		bool bTimedOut;

	// Now log in.
	CoreLogin ();

	//
	// Try and add an item to make sure the list is not empty.
	//
	hResult = pBLManager->ItemAdd (BLOCKED_USER_NUMBER, BLOCKED_USER_NAME, &unRequestId);
	if (stiRESULT_SUCCESS == hResult)
	{
		hResult = WaitForMessage (estiMSG_BLOCK_LIST_ITEM_ADDED, &Param, &bTimedOut);
	}

	//
	// Test to make sure an item can be retreived
	//
	hResult = pBLManager->ItemGetByIndex (0, &ItemId, &Number, &Description);

	CPPUNIT_ASSERT_MESSAGE ("Block list item not retreived", !stiIS_ERROR (hResult));

	//
	// Remove the item to get back to a known state.
	//
	hResult = pBLManager->ItemDelete (BLOCKED_USER_NUMBER, &unRequestId);
	if (!stiIS_ERROR (hResult))
	{
		WaitForMessage(estiMSG_BLOCK_LIST_ITEM_DELETED, &Param, &bTimedOut);
		CPPUNIT_ASSERT_MESSAGE ("estiMSG_BLOCK_LIST_ITEM_DELETED did not arrive in time",
								!bTimedOut);
	}
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::BlockListGetById ()
{
	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	IstiBlockListManager *pBLManager = m_pVE->BlockListManagerGet ();
	CPPUNIT_ASSERT_MESSAGE ("Block list manager unavailable. (Did you forget to set the BlockListEnabled property?)", pBLManager != nullptr);

	std::string Number;
	std::string Description;
	std::string Number2;
	std::string Description2;
	std::string ItemId;
	stiHResult hResult;
	unsigned int unRequestId;
		size_t Param;
	bool bTimedOut;
	bool bResult;

	// Now log in.
	CoreLogin ();

	//
	// Try and add an item to make sure the list is not empty.
	//
	hResult = pBLManager->ItemAdd (BLOCKED_USER_NUMBER, BLOCKED_USER_NAME, &unRequestId);
	if (stiRESULT_SUCCESS == hResult)
	{
		hResult = WaitForMessage (estiMSG_BLOCK_LIST_ITEM_ADDED, &Param, &bTimedOut);
	}

	//
	// Test to make sure an item can be retreived
	//
	hResult = pBLManager->ItemGetByIndex (0, &ItemId, &Number, &Description);

	CPPUNIT_ASSERT_MESSAGE ("Block list item not retreived", !stiIS_ERROR (hResult));

	//
	// Retrieve the item by id
	//
	bResult = pBLManager->ItemGetById (ItemId.c_str (), &Number2, &Description2);

	CPPUNIT_ASSERT_MESSAGE ("Block list item not retreived", bResult);
	CPPUNIT_ASSERT_MESSAGE ("Blocked user name mismatch", Number == Number2 && Description == Description2);
	
	//
	// Remove the item to get back to a known state.
	//
	hResult = pBLManager->ItemDelete (BLOCKED_USER_NUMBER, &unRequestId);
	if (!stiIS_ERROR (hResult))
	{
		WaitForMessage(estiMSG_BLOCK_LIST_ITEM_DELETED, &Param, &bTimedOut);
		CPPUNIT_ASSERT_MESSAGE ("estiMSG_BLOCK_LIST_ITEM_DELETED did not arrive in time",
								!bTimedOut);
	}
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::BlockListItemEdit ()
{
	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	IstiBlockListManager *pBLManager = m_pVE->BlockListManagerGet ();
	CPPUNIT_ASSERT_MESSAGE ("Block list manager unavailable. (Did you forget to set the BlockListEnabled property?)", pBLManager != nullptr);

	bool bWasBlocked;
	std::string Number;
	std::string Description;
	std::string ItemId;
	stiHResult hResult;
	unsigned int unRequestId;
	size_t Param;
	bool bTimedOut;
	bool bResult;

	// Now log in.
	CoreLogin ();

	// Try and add item (this is to get to a known state, not a test)
	hResult = pBLManager->ItemAdd (BLOCKED_USER_NUMBER, BLOCKED_USER_NAME, &unRequestId);
	if (stiRESULT_SUCCESS == hResult)
	{
		hResult = WaitForMessage (estiMSG_BLOCK_LIST_ITEM_ADDED, &Param, &bTimedOut);
	}

	// Test to make sure the item can be retreived
	bWasBlocked = pBLManager->ItemGetByNumber (BLOCKED_USER_NUMBER, &ItemId, &Description);

	CPPUNIT_ASSERT_MESSAGE ("Block list item not retreived", bWasBlocked == estiTRUE);
	CPPUNIT_ASSERT_MESSAGE ("Blocked user name mismatch", 0 == strcmp(Description.c_str (), BLOCKED_USER_NAME));

	//
	// Edit the item.
	//
	hResult = pBLManager->ItemEdit (ItemId.c_str (), NEW_BLOCKED_USER_NUMBER, NEW_BLOCKED_USER_NAME, &unRequestId);

	CPPUNIT_ASSERT_MESSAGE ("Failed to edit the block list item", !stiIS_ERROR (hResult));
	
	hResult = WaitForMessage (estiMSG_BLOCK_LIST_ITEM_EDITED, &Param, &bTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("estiMSG_BLOCK_LIST_ITEM_EDITED did not arrive in time",
							!bTimedOut);
	
	bResult = pBLManager->ItemGetById (ItemId.c_str (), &Number, &Description);

	CPPUNIT_ASSERT_MESSAGE ("Block list item not retreived", bResult);
	CPPUNIT_ASSERT_MESSAGE ("Blocked user name mismatch",
							strcmp (Number.c_str (), NEW_BLOCKED_USER_NUMBER) == 0
							&& strcmp (Description.c_str (), NEW_BLOCKED_USER_NAME) == 0);
	
	//
	// Remove the item to get back to a known state.
	//
	hResult = pBLManager->ItemDelete (NEW_BLOCKED_USER_NUMBER, &unRequestId);
	if (!stiIS_ERROR (hResult))
	{
		WaitForMessage(estiMSG_BLOCK_LIST_ITEM_DELETED, &Param, &bTimedOut);
		CPPUNIT_ASSERT_MESSAGE ("estiMSG_BLOCK_LIST_ITEM_DELETED did not arrive in time",
								!bTimedOut);
	}
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::BlockListDial ()
{
	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	IstiBlockListManager *pBLManager = m_pVE->BlockListManagerGet ();
	CPPUNIT_ASSERT_MESSAGE ("Block list manager unavailable. (Did you forget to set the BlockListEnabled property?)", pBLManager != nullptr);

	stiHResult hResult;
	unsigned int unRequestId;

	CoreLogin ();

	// Try and add item (this is to get to a known state, not a test)
	hResult = pBLManager->ItemAdd (BLOCKED_USER_NUMBER, BLOCKED_USER_NAME, &unRequestId);
	if (stiRESULT_SUCCESS == hResult)
	{
		bool bTimedOut;
		size_t Param;
		hResult = WaitForMessage (estiMSG_BLOCK_LIST_ITEM_ADDED, &Param, &bTimedOut);

		CPPUNIT_ASSERT_MESSAGE ("Not able to add block list item.",
								!bTimedOut);
	}


	// TODO: Attempt to dial call when blocked


	// Try and remove if exising (this is to get to a known state, not a test)
	hResult = pBLManager->ItemDelete (BLOCKED_USER_NUMBER, &unRequestId);
	if (stiRESULT_SUCCESS == hResult)
	{
		bool bTimedOut;
		size_t Param;
		hResult = WaitForMessage (estiMSG_BLOCK_LIST_ITEM_DELETED, &Param, &bTimedOut);

		CPPUNIT_ASSERT_MESSAGE ("Not able to delete block list item.",
								!bTimedOut);
	}


	// TODO: Attempt to dial call when not blocked
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::BlockListDelete ()
{
	stiHResult hResult;
	std::string Description;
	bool bTimedOut;
	size_t Param;
	unsigned int unRequestId;
	std::string ItemId;

	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	IstiBlockListManager *pBLManager = m_pVE->BlockListManagerGet ();
	CPPUNIT_ASSERT_MESSAGE ("Block list manager unavailable. (Did you forget to set the BlockListEnabled property?)", pBLManager != nullptr);

	// Test to make sure we cannot delete when not logged in.
	hResult = pBLManager->ItemDelete (BLOCKED_USER_NUMBER, &unRequestId);

	CPPUNIT_ASSERT_MESSAGE ("Block list item deleted without being logged in!",
							stiIS_ERROR (hResult));

	// Now log in.
	CoreLogin ();

	//
	// Make sure a block list item is available for deletion
	//
	bool bResult = pBLManager->ItemGetByNumber (BLOCKED_USER_NUMBER, &ItemId, &Description);

	if (!bResult)
	{
		// Try and add item (this is to get to a known state, not a test)
		hResult = pBLManager->ItemAdd (BLOCKED_USER_NUMBER, BLOCKED_USER_NAME, &unRequestId);

		CPPUNIT_ASSERT_MESSAGE ("Unable to add block list item.", !stiIS_ERROR (hResult));

		hResult = WaitForMessage (estiMSG_BLOCK_LIST_ITEM_ADDED, &Param, &bTimedOut);

		CPPUNIT_ASSERT_MESSAGE ("Not able to add block list item.",
								!bTimedOut);
	}

	// Test to make sure the item can be deleted
	hResult = pBLManager->ItemDelete (BLOCKED_USER_NUMBER, &unRequestId);

	CPPUNIT_ASSERT_MESSAGE ("Block list item not deleted", !stiIS_ERROR (hResult));

	hResult = WaitForMessage (estiMSG_BLOCK_LIST_ITEM_DELETED, &Param, &bTimedOut);

	CPPUNIT_ASSERT_MESSAGE ("Block list item not deleted", !bTimedOut);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::VRCL ()
{
	bool bTestTimedOut = false;
	size_t MessageParam;
	char *pszData = nullptr;
	stiHResult hResult = stiRESULT_SUCCESS;

	// log in to core
	CoreLogin ();

	//
	// Open up the engine's VRCL port
	//
	LocalVRCLClientConnect();

	//
	// Get the application version
	//
	m_oLocalClient.ApplicationVersionGet();
	WaitForLocalVrclMessage (estiMSG_VRCL_CLIENT_APPLICATION_VERSION, &MessageParam, &bTestTimedOut);

	CPPUNIT_ASSERT_MESSAGE ("Get Application Version timed out: ", !bTestTimedOut);

	pszData = (char*)MessageParam;
	if (pszData)
	{
		stiTrace ("<Test::VRCL> VRCL Server Application Version is %s\n", pszData);
		delete [] pszData;
		pszData = nullptr;
	}


	//
	// Get the device type of the server
	//
	m_oLocalClient.DeviceTypeGet();
	WaitForLocalVrclMessage (estiMSG_VRCL_DEVICE_TYPE, &MessageParam, &bTestTimedOut);

	CPPUNIT_ASSERT_MESSAGE ("Get Remote Device Type timed out: ", !bTestTimedOut);

	pszData = (char*)MessageParam;
	if (pszData)
	{
		stiTrace ("<Test::VRCL> Remote VRCL Server is running on device \"%s\"\n", pszData);
		delete [] pszData;
		pszData = nullptr;
	}


	//
	// Set the AudioPrivacy to on then off
	//
	m_oLocalClient.AudioPrivacySet(estiON);
	WaitForLocalVrclMessage (estiMSG_VRCL_AUDIO_PRIVACY_SET_SUCCESS, &MessageParam, &bTestTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Set Audio Privacy timed out: ", !bTestTimedOut);

	stiTrace ("<Test::VRCL> Remote Audio Privacy Enabled\n");

	m_oLocalClient.AudioPrivacySet(estiOFF);
	WaitForLocalVrclMessage (estiMSG_VRCL_AUDIO_PRIVACY_SET_SUCCESS, &MessageParam, &bTestTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Set Audio Privacy timed out: ", !bTestTimedOut);

	stiTrace ("<Test::VRCL> Remote Audio Privacy Disabled\n");


	//
	// Set the VideoPrivacy to on then off
	//
	m_oLocalClient.VideoPrivacySet(estiON);
	WaitForLocalVrclMessage (estiMSG_VRCL_VIDEO_PRIVACY_SET_SUCCESS, &MessageParam, &bTestTimedOut);

	CPPUNIT_ASSERT_MESSAGE ("Set Video Privacy timed out: ", !bTestTimedOut);

	stiTrace ("<Test::VRCL> Remote Video Privacy Enabled\n");

	m_oLocalClient.VideoPrivacySet(estiOFF);
	WaitForLocalVrclMessage (estiMSG_VRCL_VIDEO_PRIVACY_SET_SUCCESS, &MessageParam, &bTestTimedOut);

	CPPUNIT_ASSERT_MESSAGE ("Set Video Privacy timed out: ", !bTestTimedOut);

	stiTrace ("<Test::VRCL> Remote Video Privacy Disabled\n");


	//
	// Set the LocalName
	//
	m_oLocalClient.LocalNameSet(LOCAL_NAME);
	WaitForLocalVrclMessage (estiMSG_VRCL_LOCAL_NAME_SET_SUCCESS, &MessageParam, &bTestTimedOut);

	CPPUNIT_ASSERT_MESSAGE ("Set Local Name timed out: ", !bTestTimedOut);

	stiTrace ("<Test::VRCL> Remote Local Name Successfully Set to \"%s\"\n", LOCAL_NAME);


	//
	// Set the VRS Hearing Number
	//
	hResult = m_oLocalClient.VRSHearingPhoneNumberSet(VRS_HEARING_NUMBER);

	CPPUNIT_ASSERT_MESSAGE ("Set VRS Hearing Number failed: ", !stiIS_ERROR (hResult));

	stiTrace ("<Test::VRCL> VRS Hearing Number \"%s\" successfully sent to remote server.\n", VRS_HEARING_NUMBER);


	//
	// Get a setting value from the remote device
	//
	m_oLocalClient.SettingGet("Number", "cmCallMaxSendSpeed");
	WaitForLocalVrclMessage (estiMSG_VRCL_SETTING_GET_SUCCESS, &MessageParam, &bTestTimedOut);

	CPPUNIT_ASSERT_MESSAGE ("Get Remote Setting timed out: ", !bTestTimedOut);

	pszData = (char*)MessageParam;
	if (pszData)
	{
		stiTrace ("<Test::VRCL> Remote VRCL Server is set for MaxSendSpeed of %s\n", pszData);
		delete [] pszData;
		pszData = nullptr;
	}

	//
	// Set a setting value on the remote device
	//
	m_oLocalClient.SettingSet("Number", "uiAutoAnswer", "0");
	WaitForLocalVrclMessage (estiMSG_VRCL_SETTING_SET_SUCCESS, &MessageParam, &bTestTimedOut);

	CPPUNIT_ASSERT_MESSAGE ("Set Remote Setting timed out: ", !bTestTimedOut);

	stiTrace ("<Test::VRCL> SettingSet returned successfully\n");
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::VRCLDial ()
{
	bool bRemoteRingingTimedOut;
	bool bDialingTimedOut = false;
	bool bDisconnectTimedOut = false;
	size_t MessageParam;
	
	// log in to core
	CoreLogin ();

	//
	// Connect VRCL clients to the local and remote videophones.
	//
	LocalVRCLClientConnect ();

	RemoteVRCLClientConnect ();
	
	m_oLocalClient.Dial(g_UnitTestVideophoneNumber.c_str ());

	WaitForRemoteVrclMessage (estiMSG_VRCL_CLIENT_STATUS_INCOMING, &MessageParam, &bRemoteRingingTimedOut);
	
	if (!bRemoteRingingTimedOut)
	{
		WaitForMessage (estiMSG_DIALING, &bDialingTimedOut);
		
		m_oRemoteClient.Answer ();
		
		sleep (5);
	}

	m_oLocalClient.Hangup ();
	
	//
	// Wait on condition before proceeding.
	//
	WaitForMessage (estiMSG_DISCONNECTED, &MessageParam, &bDisconnectTimedOut);
	
	if (!bDisconnectTimedOut)
	{
		auto poCall = (IstiCall *)MessageParam;
		
		m_pVE->CallObjectRemove(poCall);
		
		m_pVE->MessageCleanup(estiMSG_DISCONNECTED, MessageParam);
	}
	
	sleep (5);
	
	CPPUNIT_ASSERT_MESSAGE ("Remote videophone did not ring", !bRemoteRingingTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Did not receive dialing message", !bDialingTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Did not receive disconnected message.\n", !bDisconnectTimedOut);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::VRCLDialVRS ()
{
	bool bConferencingTimedOut = false;
	bool bDisconnectTimedOut = false;
	size_t MessageParam;
	
	// log in to core
	CoreLogin ();

	//
	// Connect VRCL clients to the local and remote videophones.
	//
	LocalVRCLClientConnect ();

	RemoteVRCLClientConnect ();
	
	m_oLocalClient.VRSDial(VRS_HEARING_NUMBER);

	WaitForMessage (estiMSG_CONFERENCING, &bConferencingTimedOut);
		
	sleep (10);

	m_oLocalClient.Hangup ();
	
	//
	// Wait on condition before proceeding.
	//
	WaitForMessage (estiMSG_DISCONNECTED, &MessageParam, &bDisconnectTimedOut);
	
	if (!bDisconnectTimedOut)
	{
		auto poCall = (IstiCall *)MessageParam;
		
		m_pVE->CallObjectRemove(poCall);
		
		m_pVE->MessageCleanup(estiMSG_DISCONNECTED, MessageParam);
	}
	
	CPPUNIT_ASSERT_MESSAGE ("Did not receive conferencing message", !bConferencingTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Did not receive disconnected message.\n", !bDisconnectTimedOut);
	
	sleep(5);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::VRCLDialVRSVCO ()
{
	bool bConferencingTimedOut = false;
	bool bDisconnectTimedOut = false;
	size_t MessageParam;
	
	// log in to core
	CoreLogin ();

	//
	// Connect VRCL clients to the local and remote videophones.
	//
	LocalVRCLClientConnect ();

	RemoteVRCLClientConnect ();
	
	m_pVE->VCOCallbackNumberSet ("18012879491");
	m_pVE->VCOUseSet (estiTRUE);

	m_oLocalClient.VRSVCODial(VRS_HEARING_NUMBER);

	WaitForMessage (estiMSG_CONFERENCING, &bConferencingTimedOut);
	
	sleep (10);

	m_oLocalClient.Hangup ();
	
	//
	// Wait on condition before proceeding.
	//
	WaitForMessage (estiMSG_DISCONNECTED, &MessageParam, &bDisconnectTimedOut);
	
	if (!bDisconnectTimedOut)
	{
		auto poCall = (IstiCall *)MessageParam;
		
		m_pVE->CallObjectRemove(poCall);
		
		m_pVE->MessageCleanup(estiMSG_DISCONNECTED, MessageParam);
	}
	
	CPPUNIT_ASSERT_MESSAGE ("Did not receive conferencing message", !bConferencingTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Did not receive disconnected message.\n", !bDisconnectTimedOut);
	
	sleep(5);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::VRCLDialRemoteReject ()
{
	bool bRemoteRingingTimedOut;
	bool bDialingTimedOut = false;
	bool bDisconnectTimedOut = false;
	size_t MessageParam;
	
	// log in to core
	CoreLogin ();

	//
	// Connect VRCL clients to the local and remote videophones.
	//
	LocalVRCLClientConnect ();

	RemoteVRCLClientConnect ();
	
	m_oLocalClient.Dial(g_UnitTestVideophoneNumber.c_str ());

	WaitForRemoteVrclMessage (estiMSG_VRCL_CLIENT_STATUS_INCOMING, &MessageParam, &bRemoteRingingTimedOut);
	
	if (!bRemoteRingingTimedOut)
	{
		WaitForMessage (estiMSG_DIALING, &bDialingTimedOut);
		
		m_oRemoteClient.Reject ();
	}
	else
	{
		m_oLocalClient.Hangup ();
	}
	
	//
	// Wait on condition before proceeding.
	//
	WaitForMessage (estiMSG_DISCONNECTED, &MessageParam, &bDisconnectTimedOut);
	
	if (!bDisconnectTimedOut)
	{
		auto poCall = (IstiCall *)MessageParam;
		
		m_pVE->CallObjectRemove(poCall);
		
		m_pVE->MessageCleanup(estiMSG_DISCONNECTED, MessageParam);
	}
	
	CPPUNIT_ASSERT_MESSAGE ("Remote videophone did not ring", !bRemoteRingingTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Did not receive dialing message", !bDialingTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Did not receive disconnected message.\n", !bDisconnectTimedOut);
}


/*!
 * \brief
 *
 */
void IstiVideophoneEngineTest::VRCLAnswer ()
{
	bool bIncomingCallTimedOut = false;
	bool bDisconnectTimedOut = false;
	size_t MessageParam {0};
	const int maxInboundAttempts {3};

	prepareForInboundCall ();

	//
	// Connect VRCL clients to the local and remote videophones.
	//
	LocalVRCLClientConnect ();
	
	//
	// Have the remote videophone call us.
	//
	for (int i = 0; i < maxInboundAttempts; i++)
	{
		m_oRemoteClient.Dial (m_LocalIPv4Address.c_str ());
		WaitForLocalVrclMessage (estiMSG_VRCL_CLIENT_STATUS_INCOMING, &MessageParam, &bIncomingCallTimedOut);

		if (!bIncomingCallTimedOut)
		{
			break;
		}

		// Make sure the remote call is not dialing.
		m_oRemoteClient.Hangup ();
	}
	
	if (!bIncomingCallTimedOut)
	{
		m_oLocalClient.Answer ();
		
		sleep (5);
	}

	m_oLocalClient.Hangup ();
	
	//
	// Wait on condition before proceeding.
	//
	WaitForMessage (estiMSG_DISCONNECTED, &MessageParam, &bDisconnectTimedOut);
	
	if (!bDisconnectTimedOut)
	{
		auto poCall = (IstiCall *)MessageParam;
		
		m_pVE->CallObjectRemove(poCall);
		
		m_pVE->MessageCleanup(estiMSG_DISCONNECTED, MessageParam);
	}
	
	CPPUNIT_ASSERT_MESSAGE ("Remote videophone did not ring", !bIncomingCallTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Did not receive disconnected message.\n", !bDisconnectTimedOut);
}


void IstiVideophoneEngineTest::AutoReject()
{
	size_t MessageParam {0};
	bool bTerminatedTimedOut = false;
	bool bIncomingCallTimedOut = false;

	remoteDial ();

	//
	// Wait for the signmail greeting started message.
	// If the greeting has started then simply hangup.
	//
	bool signMailGreetingStartedTimedOut = false;

	WaitForRemoteVrclMessage (estiMSG_VRCL_SIGNMAIL_GREETING_STARTED, &MessageParam, &signMailGreetingStartedTimedOut);

	if (!signMailGreetingStartedTimedOut)
	{
		m_oRemoteClient.Hangup ();
	}

	//
	// Wait for a client call terminated message.
	//
	WaitForRemoteVrclMessage(estiMSG_VRCL_CLIENT_CALL_TERMINATED, &MessageParam, &bTerminatedTimedOut);

	CPPUNIT_ASSERT_MESSAGE ("Call was not rejected: " + g_AccountPhoneNumber, !bTerminatedTimedOut);

	//
	// Wait to see if we get an incoming call message.  If everything is working
	// correctly, we shouldn't.
	//
	WaitForMessage (estiMSG_INCOMING_CALL, &MessageParam, &bIncomingCallTimedOut);

	if (!bIncomingCallTimedOut)
	{
		m_pVE->MessageCleanup (estiMSG_INCOMING_CALL, MessageParam);

		CPPUNIT_ASSERT_MESSAGE ("Recieved an incoming call message", false);
	}

	m_pVE->AutoRejectSet (estiOFF);

	CPPUNIT_ASSERT_MESSAGE ("Auto reject set to OFF failed.", estiOFF == m_pVE->AutoRejectGet ());
}


void IstiVideophoneEngineTest::LocalNameSet()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string localName;
	
	hResult = m_pVE->LocalNameSet (LOCAL_NAME_TEST_STRING);
	CPPUNIT_ASSERT_MESSAGE ("Local Name Set failed.", !stiIS_ERROR (hResult));
	
	hResult = m_pVE->LocalNameGet (&localName);
	CPPUNIT_ASSERT_MESSAGE ("Local Name Get failed.", !stiIS_ERROR (hResult));
	
	CPPUNIT_ASSERT_MESSAGE ("Local name did not match.", localName == LOCAL_NAME_TEST_STRING);
}


void IstiVideophoneEngineTest::UserSettingsSet()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *pCoreRequest = nullptr;
	unsigned int unCoreRequestId;
	bool bTestTimedOut = true;
	CstiCoreResponse *pResponse = nullptr;

	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	//
	// Test userSettingsSet
	//
	pCoreRequest = new CstiCoreRequest;

	pCoreRequest->userSettingsSet ("UserSettingTest1", "TestValue");
	
	pCoreRequest->userSettingsSet ("UserSettingTest2", 256);
	
	pCoreRequest->userSettingsSet ("UserSettingTest3", estiTRUE);

	hResult = m_pVE->CoreRequestSend(pCoreRequest, &unCoreRequestId);
	CPPUNIT_ASSERT_MESSAGE ("CoreRequestSend failed", !stiIS_ERROR (hResult));

	bTestTimedOut = WaitForCoreResponse (unCoreRequestId, &pResponse);
	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Core request failed.", pResponse->ResponseResultGet () == estiOK);

	m_pVE->MessageCleanup(estiMSG_CORE_RESPONSE, (size_t)pResponse);
	
	pCoreRequest = new CstiCoreRequest;
	
	pCoreRequest->userSettingsGet("UserSettingTest1");
	
	pCoreRequest->userSettingsGet("UserSettingTest2");
	
	pCoreRequest->userSettingsGet("UserSettingTest3");

	hResult = m_pVE->CoreRequestSend(pCoreRequest, &unCoreRequestId);
	CPPUNIT_ASSERT_MESSAGE ("CoreRequestSend failed", !stiIS_ERROR (hResult));

	bTestTimedOut = WaitForCoreResponse (unCoreRequestId, &pResponse);
	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Core request failed.", pResponse->ResponseResultGet () == estiOK);

	m_pVE->MessageCleanup(estiMSG_CORE_RESPONSE, (size_t)pResponse);
}


void IstiVideophoneEngineTest::PhoneSettingsSet()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCoreRequest *pCoreRequest = nullptr;
	unsigned int unCoreRequestId;
	bool bTestTimedOut = true;
	CstiCoreResponse *pResponse = nullptr;

	CPPUNIT_ASSERT_MESSAGE ("Services not started", m_bStarted);

	CoreLogin ();

	//
	// Test phoneSettingsSet
	//
	pCoreRequest = new CstiCoreRequest;

	pCoreRequest->phoneSettingsSet ("UserSettingTest1", "TestValue");
	
	pCoreRequest->phoneSettingsSet ("UserSettingTest2", 256);
	
	pCoreRequest->phoneSettingsSet ("UserSettingTest3", estiTRUE);

	hResult = m_pVE->CoreRequestSend(pCoreRequest, &unCoreRequestId);
	CPPUNIT_ASSERT_MESSAGE ("CoreRequestSend failed", !stiIS_ERROR (hResult));

	bTestTimedOut = WaitForCoreResponse (unCoreRequestId, &pResponse);
	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Core request failed.", pResponse->ResponseResultGet () == estiOK);

	m_pVE->MessageCleanup(estiMSG_CORE_RESPONSE, (size_t)pResponse);
	
	pCoreRequest = new CstiCoreRequest;
	
	pCoreRequest->phoneSettingsGet("UserSettingTest1");
	
	pCoreRequest->phoneSettingsGet("UserSettingTest2");
	
	pCoreRequest->phoneSettingsGet("UserSettingTest3");

	hResult = m_pVE->CoreRequestSend(pCoreRequest, &unCoreRequestId);
	CPPUNIT_ASSERT_MESSAGE ("CoreRequestSend failed", !stiIS_ERROR (hResult));

	bTestTimedOut = WaitForCoreResponse (unCoreRequestId, &pResponse);
	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Core request failed.", pResponse->ResponseResultGet () == estiOK);

	m_pVE->MessageCleanup(estiMSG_CORE_RESPONSE, (size_t)pResponse);
}


void IstiVideophoneEngineTest::RingsBeforeGreeting()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32RingsBeforeGreeting;
	uint32_t un32NewRingsBeforeGreeting1;
	uint32_t un32NewRingsBeforeGreeting2;
	uint32_t un32MaxRingsBeforeGreeting;
	
	CoreLogin ();
	
	hResult = m_pVE->RingsBeforeGreetingGet(&un32RingsBeforeGreeting, &un32MaxRingsBeforeGreeting);
	CPPUNIT_ASSERT_MESSAGE ("RingsBeforeGreetingGet failed.", !stiIS_ERROR (hResult));
		
	un32NewRingsBeforeGreeting1 = un32RingsBeforeGreeting + 1;
	
	if (un32NewRingsBeforeGreeting1 > un32MaxRingsBeforeGreeting)
	{
		un32NewRingsBeforeGreeting1 = un32RingsBeforeGreeting - 1;
	}
	
	hResult = m_pVE->RingsBeforeGreetingSet(un32NewRingsBeforeGreeting1);
	CPPUNIT_ASSERT_MESSAGE ("RingsBeforeGreetingSet failed.", !stiIS_ERROR (hResult));
	
	hResult = m_pVE->RingsBeforeGreetingGet(&un32NewRingsBeforeGreeting2, &un32MaxRingsBeforeGreeting);
	CPPUNIT_ASSERT_MESSAGE ("RingsBeforeGreetingGet failed.", !stiIS_ERROR (hResult));
	CPPUNIT_ASSERT_MESSAGE ("Rings before greeting incorrect.", un32NewRingsBeforeGreeting1 == un32NewRingsBeforeGreeting2);
	
	hResult = m_pVE->RingsBeforeGreetingSet(un32RingsBeforeGreeting);
	CPPUNIT_ASSERT_MESSAGE ("RingsBeforeGreetingSet failed.", !stiIS_ERROR (hResult));
}


#ifdef stiMESSAGE_MANAGER
void IstiVideophoneEngineTest::CleanupMessageList(
	 std::list<SstiMessageManagerItem *> *plMessageManagerList)
{
	std::list<SstiMessageManagerItem *>::iterator it;
	it = plMessageManagerList->begin();
	while (it != plMessageManagerList->end())
	{
		if (!(*it)->lItemList.empty())
		{
			CleanupMessageList(&(*it)->lItemList);
		}
		it = plMessageManagerList->erase(it);
	}
}

void IstiVideophoneEngineTest::FindMessageItem(
	const CstiItemId &itemId,
	const std::list<SstiMessageManagerItem *> *plMessageManagerList,
	SstiMessageManagerItem **ppMessageManagerItem)
{
	*ppMessageManagerItem = nullptr;
	std::list<SstiMessageManagerItem *>::const_iterator it;
	for (it = plMessageManagerList->begin();
		 it != plMessageManagerList->end();
		 it++)
	{
		if (itemId == (*it)->itemId)
		{
			*ppMessageManagerItem = (*it);
			break;
		}
		else if (!(*it)->lItemList.empty())
		{
			FindMessageItem(itemId,
							&(*it)->lItemList,
							ppMessageManagerItem);

			if (*ppMessageManagerItem)
			{
				break;
			}
		}
	}
}

bool IstiVideophoneEngineTest::RemoveMessageItem(
	const CstiItemId &itemId,
	std::list<SstiMessageManagerItem *> *plMessageManagerList)
{
	bool bReturn = false;

	std::list<SstiMessageManagerItem *>::iterator it;
	for (it = plMessageManagerList->begin();
		 it != plMessageManagerList->end();
		 it++)
	{
		if (itemId == (*it)->itemId)
		{
			it = plMessageManagerList->erase(it);
			bReturn = true;
			break;
		}
		else if (!(*it)->lItemList.empty())
		{
			bReturn = RemoveMessageItem(itemId,
										&(*it)->lItemList);

			if (bReturn)
			{
				break;
			}
		}
	}

	return bReturn;
}

void IstiVideophoneEngineTest::UpdateMessageList (
	const CstiItemId &itemId,
	std::list<SstiMessageManagerItem *> *plMessageManagerList)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nCategoryType = 0;
	unsigned int unIndexCount = 0;
	char szCatShortName[MESSAGE_NAME_BUFFER_SIZE];
	char szCatLongName[MESSAGE_NAME_BUFFER_SIZE];

	// Get the MessageManager and lock it.
	IstiMessageManager *pMessageManager = m_pVE->MessageManagerGet ();
	pMessageManager->Lock();

	hResult = pMessageManager->CategoryByIdGet(itemId,
											   &unIndexCount,
											   &nCategoryType,
											   szCatShortName, (unsigned int)MESSAGE_NAME_BUFFER_SIZE,
											   szCatLongName, (unsigned int)MESSAGE_NAME_BUFFER_SIZE);
	CPPUNIT_ASSERT_MESSAGE ("Failed to find Category", !stiIS_ERROR (hResult));

	SstiMessageManagerItem *pMessageManagerItem = nullptr;
	FindMessageItem(itemId, 
					plMessageManagerList, 
					&pMessageManagerItem);

	if (!pMessageManagerItem)
	{
		pMessageManagerItem = new SstiMessageManagerItem;
		pMessageManagerItem->itemId = itemId;
		pMessageManagerItem->nCategoryType = nCategoryType;
		pMessageManagerItem->nMessagType = MSGTYPE_CATEGORY;
		strcpy(pMessageManagerItem->szMessageName, szCatShortName);

		// Are we a cateogry or subcategory?
		CstiItemId *pParentId = nullptr;
		pMessageManager->CategoryParentGet(itemId,
										   &pParentId);

		// If we have a parentId we need to get the parent and add the item to the parent's list.
		if (pParentId)
		{
			pMessageManagerItem->nMessagType = MSGTYPE_SUBCATEGORY;

			SstiMessageManagerItem *pParentMessageItem = nullptr;
			FindMessageItem(*pParentId, 
							plMessageManagerList, 
							&pParentMessageItem);

			// If we didn't find the parent then create it.
			if (!pParentMessageItem)
			{
				hResult = pMessageManager->CategoryByIdGet(*pParentId,
														   &unIndexCount,
														   &nCategoryType,
														   szCatShortName, (unsigned int)MESSAGE_NAME_BUFFER_SIZE,
														   szCatLongName, (unsigned int)MESSAGE_NAME_BUFFER_SIZE);
				CPPUNIT_ASSERT_MESSAGE ("Failed to find Parent Category", !stiIS_ERROR (hResult));

				pParentMessageItem = new SstiMessageManagerItem;
				pParentMessageItem->itemId = *pParentId;
				strcpy(pParentMessageItem->szMessageName, szCatShortName);
				pParentMessageItem->nMessagType = MSGTYPE_CATEGORY;
				pParentMessageItem->nCategoryType = nCategoryType;
				plMessageManagerList->push_front(pParentMessageItem);
			}

			// Add the item to the parent's list.
			pParentMessageItem->lItemList.push_front(pMessageManagerItem);

			// Clean up the parentId.
			delete pParentId;
		}
		else
		{
			// We are a category so add it to the root list.
			plMessageManagerList->push_front(pMessageManagerItem);
		}
	}

	// Check for SubCategories
	unsigned int unSubCatCount = 0;
	hResult = pMessageManager->SubCategoryCountGet(itemId, &unSubCatCount);
	CPPUNIT_ASSERT_MESSAGE ("SubCategoryCountGet Failed", !stiIS_ERROR (hResult));

	if (unSubCatCount > 0)
	{
		// Look for the sub categories.
		for (unsigned int unSubCatIndex = 0;
			 unSubCatIndex < unSubCatCount;
			 unSubCatIndex++)
		{
			CstiItemId *pSubCatId = nullptr;
			hResult = pMessageManager->SubCategoryByIndexGet(itemId, unSubCatIndex, &pSubCatId);
			CPPUNIT_ASSERT_MESSAGE ("Failed to find Sub Category", !stiIS_ERROR (hResult));

			SstiMessageManagerItem *pSubCategoryItem = nullptr;
			FindMessageItem(*pSubCatId, 
							&pMessageManagerItem->lItemList, 
							&pSubCategoryItem);

			// If we didn't find the category then create it.
			if (!pSubCategoryItem)
			{
				hResult = pMessageManager->CategoryByIdGet(*pSubCatId,
														   &unIndexCount,
														   &nCategoryType,
														   szCatShortName, (unsigned int)MESSAGE_NAME_BUFFER_SIZE,
														   szCatLongName, (unsigned int)MESSAGE_NAME_BUFFER_SIZE);
				CPPUNIT_ASSERT_MESSAGE ("Failed to find Sub Category", !stiIS_ERROR (hResult));

				pSubCategoryItem = new SstiMessageManagerItem;
				pSubCategoryItem->itemId = *pSubCatId;
				strcpy(pSubCategoryItem->szMessageName, szCatShortName);
				pSubCategoryItem->nMessagType = MSGTYPE_SUBCATEGORY;
				pSubCategoryItem->nCategoryType = nCategoryType;
				pMessageManagerItem->lItemList.push_front(pSubCategoryItem);
			}

			// Clear the list.
			while (!pSubCategoryItem->lItemList.empty())
			{
				pSubCategoryItem->lItemList.pop_front();
			}
			 
			//Load the messages.
			unsigned int unMessagesInCategory = 0;
			pMessageManager->MessageCountGet(*pSubCatId, 
											 &unMessagesInCategory);

			if (unMessagesInCategory > 0)
			{
				for (unsigned int unMessageIndex = 0;
					 unMessageIndex < unMessagesInCategory;
					 unMessageIndex++)
				{
					CstiMessageInfo messageInfo;
					hResult = pMessageManager->MessageByIndexGet(*pSubCatId,
																 unMessageIndex,
																 &messageInfo);
					CPPUNIT_ASSERT_MESSAGE ("Failed to find Message", !stiIS_ERROR (hResult));

					auto pMessageItem = new SstiMessageManagerItem;
					pMessageItem->itemId = messageInfo.ItemIdGet();
					strcpy(pMessageItem->szMessageName, messageInfo.NameGet());
					pMessageItem->nMessagType = MSGTYPE_MESSAGE;
					pSubCategoryItem->lItemList.push_front(pMessageItem);
				}
			}
			delete pSubCatId;
		}
	}
	else
	{
		// Clear the list.
		while (!pMessageManagerItem->lItemList.empty())
		{
			pMessageManagerItem->lItemList.pop_front();
		}

		//Load the messages.
		unsigned int unMessagesInCategory = 0;
		pMessageManager->MessageCountGet(itemId, 
										 &unMessagesInCategory);

		if (unMessagesInCategory > 0)
		{
			for (unsigned int unMessageIndex = 0;
				 unMessageIndex < unMessagesInCategory;
				 unMessageIndex++)
			{
				CstiMessageInfo messageInfo;
				hResult = pMessageManager->MessageByIndexGet(itemId,
															 unMessageIndex,
															 &messageInfo);
				CPPUNIT_ASSERT_MESSAGE ("Failed to find Message", !stiIS_ERROR (hResult));

				auto pMessageItem = new SstiMessageManagerItem;
				pMessageItem->itemId = messageInfo.ItemIdGet();
				strcpy(pMessageItem->szMessageName, messageInfo.NameGet());
				pMessageItem->nMessagType = MSGTYPE_MESSAGE;
				pMessageManagerItem->lItemList.push_front(pMessageItem);
			}
		}
	}
	pMessageManager->Unlock();
}

void IstiVideophoneEngineTest::ProcessMsgMgrEvents(
	std::list<SstiMessageManagerItem *> *plMessageManagerList)
{
	size_t MessageParam;
	bool bTestTimedOut = false;

	while (!bTestTimedOut)
	{
		// Wait for notificatin from the message manager that a category has changed.
		WaitForMessage (estiMSG_MESSAGE_CATEGORY_CHANGED, &MessageParam, &bTestTimedOut);
		if (bTestTimedOut)
		{
			break;
		}

		auto pItemId = (CstiItemId *)MessageParam;
		CPPUNIT_ASSERT_MESSAGE ("Missing CategoryID", pItemId);

		UpdateMessageList(*pItemId,
						  plMessageManagerList);

		m_pVE->MessageCleanup(estiMSG_MESSAGE_CATEGORY_CHANGED, (size_t)MessageParam);
	}
}

void IstiVideophoneEngineTest::LoadCategoriesNoMessages ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bTestTimedOut = false;
	bool bHaveSignMail = false;
	bool bHaveSorenson = false;
	size_t MessageParam;
	unsigned int unCategoryCount = 0;
	int nCategoryType = 0;
	unsigned int unIndexCount = 0;
	char szCatShortName[MESSAGE_NAME_BUFFER_SIZE];
	char szCatLongName[MESSAGE_NAME_BUFFER_SIZE];

	// Copy the interceptor rules for empty message list to the interceptor directory.
	//****************************************
	FILE_COPY (INTERCEPTOR_EMPTY_MESSAGE_LIST, INTERCEPTOR_RULES_DESTINATION);
	sleep(INTERCEPTOR_COPY_DELAY);

	CoreLogin ();

	// Send a heartbeat so Interceptor will return the new list.
	m_pVE->StateNotifyHeartbeatRequest();

	// Get the MessageManager.
	IstiMessageManager *pMessageManager = m_pVE->MessageManagerGet ();

	// We should receive 2 estiMSG_MESSAGE_ITEM_CHANGED messages one for SignMail and one for Sorenson.
	// Wait for them and process them.
	while (!bHaveSignMail || !bHaveSorenson)
	{
		// Wait for notificatin from the message manager that a category has changed.
		WaitForMessage (estiMSG_MESSAGE_CATEGORY_CHANGED, &MessageParam, &bTestTimedOut);

		CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);

		auto pItemId = (CstiItemId *)MessageParam;
		CPPUNIT_ASSERT_MESSAGE ("Missing CategoryID", pItemId);

		pMessageManager->Lock();

		hResult = pMessageManager->CategoryByIdGet(*pItemId,
												   &unIndexCount,
												   &nCategoryType,
												   szCatShortName, (unsigned int)MESSAGE_NAME_BUFFER_SIZE,
												   szCatLongName, (unsigned int)MESSAGE_NAME_BUFFER_SIZE);
		CPPUNIT_ASSERT_MESSAGE ("Failed to find Category", !stiIS_ERROR (hResult));

		CPPUNIT_ASSERT_MESSAGE ("Received an unexpected Category", 
								nCategoryType == estiMT_SIGN_MAIL || nCategoryType == estiMT_MESSAGE_FROM_SORENSON); 

		if (nCategoryType == estiMT_SIGN_MAIL)
		{
		   CPPUNIT_ASSERT_MESSAGE ("Received SignMail Category twice", !bHaveSignMail);
		   bHaveSignMail = true;
		}
		else if (nCategoryType == estiMT_MESSAGE_FROM_SORENSON)
		{
			CPPUNIT_ASSERT_MESSAGE ("Received Sorenson Category twice", !bHaveSorenson);
			bHaveSorenson = true;
		}

		pMessageManager->Unlock();

		m_pVE->MessageCleanup(estiMSG_MESSAGE_CATEGORY_CHANGED, (size_t)MessageParam);
	}

	// Check to make sure that we only have 2 categories.  If we have more then we didn't
	// get an empty category list.
	pMessageManager->Lock();
	hResult = pMessageManager->CategoryCountGet(&unCategoryCount);
	CPPUNIT_ASSERT_MESSAGE ("CategoryCountGet() failed", !stiIS_ERROR (hResult));
	CPPUNIT_ASSERT_MESSAGE ("CategoryCountGet() returned bad category count", unCategoryCount == 2);
	pMessageManager->Unlock();

}

void IstiVideophoneEngineTest::LoadCategoriesMessages ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	unsigned int unMessageCount = 0;
	unsigned int unSubCategoryCount = 0;
	unsigned int unCategoryCount = 0;
	unsigned int unMsgMgrCategoryCount = 0;
	std::list<SstiMessageManagerItem *> lMessageManagerList;

	CoreLogin ();

	// Copy the interceptor rules for message list to the interceptor directory.
	//****************************************
	FILE_COPY (INTERCEPTOR_LOAD_MESSAGE_LIST_CNT11, INTERCEPTOR_RULES_DESTINATION);
	sleep(INTERCEPTOR_COPY_DELAY);

	// Send a heartbeat so Interceptor will return the new list.
	m_pVE->StateNotifyHeartbeatRequest();

	// Process the Message Manager's events.
	ProcessMsgMgrEvents(&lMessageManagerList);

	// Get the MessageManager and lock it.
	IstiMessageManager *pMessageManager = m_pVE->MessageManagerGet ();
	pMessageManager->Lock();

	unsigned int unSignMailCount = 0;
	hResult = pMessageManager->SignMailCountGet(&unSignMailCount);
	CPPUNIT_ASSERT_MESSAGE ("SignMailCountGet() failed", !stiIS_ERROR (hResult));
	CPPUNIT_ASSERT_MESSAGE ("Wrong number of SignMails", unSignMailCount == 3);

	unsigned int unSignMailMaxCount = 0;
	hResult = pMessageManager->SignMailMaxCountGet(&unSignMailMaxCount);
	CPPUNIT_ASSERT_MESSAGE ("SignMailMaxCountGet() failed", !stiIS_ERROR (hResult));
	CPPUNIT_ASSERT_MESSAGE ("Wrong number of SignMails", unSignMailMaxCount == 100);

	// Should have 4 root categories SignMail, DKN, SN, Sorenson
	hResult = pMessageManager->CategoryCountGet(&unMsgMgrCategoryCount);
	CPPUNIT_ASSERT_MESSAGE ("CategoryCountGet() failed", !stiIS_ERROR (hResult));

	pMessageManager->Unlock();

	// Count the items in the lists.
	std::list<SstiMessageManagerItem *>::iterator itCategory;
	for (itCategory = lMessageManagerList.begin();
		 itCategory != lMessageManagerList.end();
		 itCategory++)
	{
		unCategoryCount++;
		if (!(*itCategory)->lItemList.empty())
		{
			std::list<SstiMessageManagerItem *>::iterator itSubItem;
			for (itSubItem = (*itCategory)->lItemList.begin();
				 itSubItem != (*itCategory)->lItemList.end();
				 itSubItem++)
			{
				if ((*itSubItem)->nMessagType == MSGTYPE_SUBCATEGORY)
				{
					// Update the subcateogry count.
					unSubCategoryCount++;
					std::list<SstiMessageManagerItem *>::iterator itMsgItem;
					for (itMsgItem = (*itSubItem)->lItemList.begin();
						 itMsgItem != (*itSubItem)->lItemList.end();
						 itMsgItem++)
					{
						// We have a message.
						unMessageCount++;
					}
				}
				else
				{
					// We have a message.
					unMessageCount++;
				}
			}
		}
	}

	CPPUNIT_ASSERT_MESSAGE ("Wrong number of categories", unMsgMgrCategoryCount == unCategoryCount);
	CPPUNIT_ASSERT_MESSAGE ("Message Count is wrong", unMessageCount == 11);
	CPPUNIT_ASSERT_MESSAGE ("Sub Category Count is wrong", unSubCategoryCount == 3);

	// Update Message list with new messages and check new message count.
	// Set the last update time(s) on the message manager. 
	// This time is the time plus 1 hour set on the messages in the interceptor file.
	struct tm tmTime;
	memset(&tmTime, 0, sizeof(tm));
	tmTime.tm_mday = 27;
	tmTime.tm_mon = 0;
	tmTime.tm_year = 2012 - 1900;
	tmTime.tm_hour = 22;
	tmTime.tm_min = 0;
	tmTime.tm_sec = 36;
	
	// Lock the MessageManager.
	pMessageManager->Lock();
	pMessageManager->lastCategoryUpdateSet();
	pMessageManager->lastSignMailUpdateSet();
	pMessageManager->Unlock();

	// Update the message list that Interceptor will return.
	FILE_COPY (INTERCEPTOR_LOAD_MESSAGE_LIST_NEW_MESSAGES, INTERCEPTOR_RULES_DESTINATION);
	sleep(INTERCEPTOR_COPY_DELAY);

	// Send a heartbeat so Interceptor will return the new list.
	m_pVE->StateNotifyHeartbeatRequest();

	// Load the message list.
	ProcessMsgMgrEvents(&lMessageManagerList);

	// Get new message count.
	pMessageManager->Lock();

	unsigned int unNewMsgCount = 0;
	unsigned int unNewSignMailCount = 0;

	// Count the new items in the lists.
	for (itCategory = lMessageManagerList.begin();
		 itCategory != lMessageManagerList.end();
		 itCategory++)
	{
		unsigned int unTempCount = 0;
		pMessageManager->NewMessageCountGet((*itCategory)->itemId, &unTempCount);

		if ((*itCategory)->nCategoryType == estiMT_SIGN_MAIL)
		{
			unNewSignMailCount = unTempCount;
		}
		else
		{
			unNewMsgCount += unTempCount;
		}
	}
	pMessageManager->Unlock();

	CPPUNIT_ASSERT_MESSAGE ("New SignMail count is wrong", unNewSignMailCount == 1);
	CPPUNIT_ASSERT_MESSAGE ("New Message count is wrong", unNewMsgCount == 2);

	CleanupMessageList(&lMessageManagerList);
}

void IstiVideophoneEngineTest::UpdateMessageItems ()
{
	size_t MessageParam;
	bool bTestTimedOut = false;
	std::list<SstiMessageManagerItem *> lMessageManagerList;

	CoreLogin ();

	// Copy the interceptor rules for empty message list to the interceptor directory.
	//****************************************
	FILE_COPY (INTERCEPTOR_LOAD_MESSAGE_LIST_CNT11, INTERCEPTOR_RULES_DESTINATION);
	sleep(INTERCEPTOR_COPY_DELAY);

	// Send a heartbeat so Interceptor will return the new list.
	m_pVE->StateNotifyHeartbeatRequest();

	// Load the message list.
	ProcessMsgMgrEvents(&lMessageManagerList);

	unsigned int unUnviewedCount = 0;

	IstiMessageManager *pMessageManager = m_pVE->MessageManagerGet ();
	pMessageManager->Lock();

	// Count the new items in the lists.
	std::list<SstiMessageManagerItem *>::iterator itCategory;
	for (itCategory = lMessageManagerList.begin();
		 itCategory != lMessageManagerList.end();
		 itCategory++)
	{
		unsigned int unTempUnviewed = 0;
		pMessageManager->UnviewedMessageCountGet((*itCategory)->itemId, &unTempUnviewed);
		unUnviewedCount += unTempUnviewed;
	}
	pMessageManager->Unlock();
	CPPUNIT_ASSERT_MESSAGE ("Unviewed count is wrong", unUnviewedCount == 11);

	// Mark a message as viewed and set it's pause point.
	// Find a message to maniuplate.
	CstiItemId messageId;
	CstiItemId *pSubCategoryId = nullptr;  stiUNUSED_ARG (pSubCategoryId);
	for (itCategory = lMessageManagerList.begin();
		 itCategory != lMessageManagerList.end();
		 itCategory++)
	{
		if ((*itCategory)->nCategoryType != estiMT_SIGN_MAIL &&
			!(*itCategory)->lItemList.empty())
		{
			std::list<SstiMessageManagerItem *>::iterator itSubCategory;
			std::list<SstiMessageManagerItem *>::iterator itMessage;
			itSubCategory = (*itCategory)->lItemList.begin();
			pSubCategoryId = &(*itSubCategory)->itemId;
			itMessage = (*itSubCategory)->lItemList.begin();
			messageId = (*itMessage)->itemId;
			break;
		}
	}
	CPPUNIT_ASSERT_MESSAGE ("Didn't find a message to manipulate", messageId.IsValid ());

	// Update viewed and pause point messages that Interceptor will return.
	FILE_COPY (INTERCEPTOR_MODIFY_MESSAGE_VIEWED_PAUSE_POINT, INTERCEPTOR_RULES_DESTINATION);
	sleep(INTERCEPTOR_COPY_DELAY);

	// Get message info and update the viewed status.
	CstiMessageInfo messageInfo;

	pMessageManager->Lock();

	// Get the number of unviewed messages in the Parent category.
	pMessageManager->UnviewedMessageCountGet((*itCategory)->itemId, &unUnviewedCount);

	messageInfo.ItemIdSet(messageId);
	pMessageManager->MessageInfoGet(&messageInfo);

	messageInfo.ViewedSet(estiTRUE);
	pMessageManager->MessageInfoSet(&messageInfo);
	pMessageManager->Unlock();

	// Wait for notificatin from the message manager that a message has changed.
	WaitForMessage (estiMSG_MESSAGE_ITEM_CHANGED, &MessageParam, &bTestTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);

	auto pItemId = (CstiItemId *)MessageParam;
	CPPUNIT_ASSERT_MESSAGE ("Missing MessageID with estiMSG_MESSAGE_ITEM_CHAGNED", pItemId);
	CPPUNIT_ASSERT_MESSAGE ("Wrong MessageID retruned with estiMSG_MESSAGE_ITEM_CHAGNED", *pItemId == messageId);

	m_pVE->MessageCleanup(estiMSG_MESSAGE_ITEM_CHANGED, (size_t)MessageParam);

	pMessageManager->Lock();

	// Check the viewed state to make sure it is false.
	pMessageManager->MessageInfoGet(&messageInfo);
	CPPUNIT_ASSERT_MESSAGE ("Viewed status is not true", messageInfo.ViewedGet() == estiTRUE);

	// Validate that we now have the correct number of viewed items.
	unsigned int unNewUnviewedCount = 0;
	pMessageManager->UnviewedMessageCountGet((*itCategory)->itemId, &unNewUnviewedCount);
	CPPUNIT_ASSERT_MESSAGE ("Unviewed message count is not correct!!!", unNewUnviewedCount == unUnviewedCount - 1);
	 
	// Now update the pause point to 10 seconds.
	messageInfo.PausePointSet(10);
	pMessageManager->MessageInfoSet(&messageInfo);
	pMessageManager->Unlock();

	// Wait for notificatin from the message manager that a category has changed.
	WaitForMessage (estiMSG_MESSAGE_ITEM_CHANGED, &MessageParam, &bTestTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);

	pItemId = (CstiItemId *)MessageParam;
	CPPUNIT_ASSERT_MESSAGE ("Missing MessageID with estiMSG_MESSAGE_ITEM_CHAGNED", pItemId);
	CPPUNIT_ASSERT_MESSAGE ("Wrong MessageID retruned with estiMSG_MESSAGE_ITEM_CHAGNED", *pItemId == messageId);

	m_pVE->MessageCleanup(estiMSG_MESSAGE_ITEM_CHANGED, (size_t)MessageParam);

	pMessageManager->Lock();

	// Check the viewed state to make sure it is false.
	pMessageManager->MessageInfoGet(&messageInfo);
	pMessageManager->Unlock();
	CPPUNIT_ASSERT_MESSAGE ("Pause point is not correct", messageInfo.PausePointGet() == 10);

	CleanupMessageList(&lMessageManagerList);
}

void IstiVideophoneEngineTest::DeleteMessageItems ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	size_t MessageParam;
	bool bTestTimedOut = false;
	std::list<SstiMessageManagerItem *> lMessageManagerList;

	CoreLogin ();

	// Copy the interceptor rules for empty message list to the interceptor directory.
	//****************************************
	FILE_COPY (INTERCEPTOR_LOAD_MESSAGE_LIST_NEW_MESSAGES, INTERCEPTOR_RULES_DESTINATION);
	sleep(INTERCEPTOR_COPY_DELAY);

	// Send a heartbeat so Interceptor will return the new list.
	m_pVE->StateNotifyHeartbeatRequest();

	// Load the message list.
	ProcessMsgMgrEvents(&lMessageManagerList);

	// Copy the delete message response for interceptor.
	FILE_COPY (INTERCEPTOR_DELETE_MESSAGE, INTERCEPTOR_RULES_DESTINATION);
	sleep(INTERCEPTOR_COPY_DELAY);

	// Find a message to Delete.
	CstiItemId messageId;
	CstiItemId subCategoryId;
	bool bHaveMessage = FALSE;
	std::list<SstiMessageManagerItem *>::iterator itCategory;
	for (itCategory = lMessageManagerList.begin();
		 itCategory != lMessageManagerList.end() && !bHaveMessage;
		 itCategory++)
	{
		if (!strcmp((*itCategory)->szMessageName, "DKN"))
		{
			std::list<SstiMessageManagerItem *>::iterator itSubCategory;
			for (itSubCategory = (*itCategory)->lItemList.begin();
				 itSubCategory != (*itCategory)->lItemList.end();
				 itSubCategory++)
			{
				if (!strcmp((*itSubCategory)->szMessageName, "Story Time"))
				{
					subCategoryId = (*itSubCategory)->itemId;
					std::list<SstiMessageManagerItem *>::iterator itMessage;
					itMessage = (*itSubCategory)->lItemList.begin();
					SstiMessageManagerItem *pMessageItem = (*itMessage);
					messageId = pMessageItem->itemId;
					bHaveMessage = TRUE;
					break;
				}
			}
		}
	}
	CPPUNIT_ASSERT_MESSAGE ("Didn't find a message to manipulate", messageId.IsValid());

	IstiMessageManager *pMessageManager = m_pVE->MessageManagerGet ();

	// Get the number of messages in the sub category.
	pMessageManager->Lock();

	// Verify that our sub category is the same as the message managers.
	CstiItemId *pSubCategoryIdCheck = nullptr;
	pMessageManager->MessageCategoryGet(messageId, &pSubCategoryIdCheck);
	CPPUNIT_ASSERT_MESSAGE ("Sub categories don't match", *pSubCategoryIdCheck == subCategoryId);

	unsigned int unMsgCount = 0;
	pMessageManager->MessageCountGet(subCategoryId, &unMsgCount);

	// Delete message.
	pMessageManager->MessageItemDelete(messageId);
	pMessageManager->Unlock();

	// Wait for notificatin from the message manager that a category has changed.
	WaitForMessage (estiMSG_MESSAGE_CATEGORY_CHANGED, &MessageParam, &bTestTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);

	auto pItemId = (CstiItemId *)MessageParam;
	CPPUNIT_ASSERT_MESSAGE ("Missing MessageID with estiMSG_MESSAGE_CATEGORY_CHAGNED", pItemId);
	CPPUNIT_ASSERT_MESSAGE ("ItemId sent with estiMSG_MESSAGE_CATEGORY_CHAGNED does not back sub categoryId", *pItemId == subCategoryId);

	// Check to see that it was deleted from the message manager.
	pMessageManager->Lock();
	hResult = pMessageManager->MessageCategoryGet(messageId, &pSubCategoryIdCheck);
	CPPUNIT_ASSERT_MESSAGE ("Message was not deleted from Message Manager", hResult != stiRESULT_SUCCESS);

	// Check the message count in that sub category.
	unsigned int unNewMsgCount = 0;
	pMessageManager->MessageCountGet(subCategoryId, &unNewMsgCount);
	CPPUNIT_ASSERT_MESSAGE ("Remaining Message count is wrong!", unNewMsgCount == unMsgCount - 1);

	// Remove the message from local list.
	RemoveMessageItem(messageId,
					  &lMessageManagerList);
	m_pVE->MessageCleanup(estiMSG_MESSAGE_CATEGORY_CHANGED, (size_t)MessageParam);

	// Delete all messages in a list.
	pMessageManager->MessagesInCategoryDelete(subCategoryId);
	pMessageManager->Unlock();

	// Wait for notificatin from the message manager that a category has changed.
	WaitForMessage (estiMSG_MESSAGE_CATEGORY_CHANGED, &MessageParam, &bTestTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);

	pItemId = (CstiItemId *)MessageParam;
	CPPUNIT_ASSERT_MESSAGE ("Missing MessageID with estiMSG_MESSAGE_CATEGORY_CHAGNED", pItemId);

	// Check to see that it was deleted from the message manager.
	pMessageManager->Lock();
	hResult = pMessageManager->CategoryByIdGet(*pItemId,
											   nullptr,
											   nullptr,
											   nullptr, 0,
											   nullptr, 0);
	pMessageManager->Unlock();
	CPPUNIT_ASSERT_MESSAGE ("Category was not deleted from Message Manager", hResult != stiRESULT_SUCCESS);

	// Remove the category from local list.
	RemoveMessageItem(*pItemId,
					  &lMessageManagerList);

	m_pVE->MessageCleanup(estiMSG_MESSAGE_CATEGORY_CHANGED, (size_t)MessageParam);

	// Copy the interceptor rule for Message List Changed.
	FILE_COPY (INTERCEPTOR_MESSAGE_LIST_CHANGED, INTERCEPTOR_RULES_DESTINATION);
	sleep(INTERCEPTOR_COPY_DELAY);

	// Send a heartbeat so Interceptor will return the new list.
	m_pVE->StateNotifyHeartbeatRequest();

	bool bSNFound = FALSE;
	while (!bTestTimedOut && !bSNFound)
	{
		// Wait for notificatin from the message manager that a category has changed.
		WaitForMessage (estiMSG_MESSAGE_CATEGORY_CHANGED, &MessageParam, &bTestTimedOut);
		if (bTestTimedOut)
		{
			CPPUNIT_ASSERT_MESSAGE ("Did not get an ID for the SN Category", !bTestTimedOut);
			break;
		}

		pItemId = (CstiItemId *)MessageParam;
		CPPUNIT_ASSERT_MESSAGE ("Missing MessageID with estiMSG_MESSAGE_CATEGORY_CHAGNED", pItemId);

		// This item Id should be for the SN category make sure it has been removed from the Message Manager
		// and it was the SN category.
		pMessageManager->Lock();
		hResult = pMessageManager->CategoryByIdGet(*pItemId,
												   nullptr,
												   nullptr,
												   nullptr, 0,
												   nullptr, 0);
		pMessageManager->Unlock();
		CPPUNIT_ASSERT_MESSAGE ("Category was not deleted from Message Manager", hResult != stiRESULT_SUCCESS);

		SstiMessageManagerItem *pSNCategory = nullptr;
		FindMessageItem(*pItemId, 
						&lMessageManagerList,
						&pSNCategory);
		CPPUNIT_ASSERT_MESSAGE ("Counldn't find a matching category in local list.", pSNCategory);
		if (strcmp("SN", pSNCategory->szMessageName))
		{
			bSNFound = TRUE;
		}

		m_pVE->MessageCleanup(estiMSG_MESSAGE_CATEGORY_CHANGED, (size_t)MessageParam);
	}

	// Wait for notificatin from the message manager that a category has changed.
	WaitForMessage (estiMSG_MESSAGE_CATEGORY_CHANGED, &MessageParam, &bTestTimedOut);
	CPPUNIT_ASSERT_MESSAGE ("Test timed out", !bTestTimedOut);

	// Remove all messages in a category.
	CleanupMessageList(&lMessageManagerList);
}

void IstiVideophoneEngineTest::MessageInfoCoverage ()
{
	CstiItemId itemId1;
	CstiItemId itemId2;

	// log in to core
	CoreLogin ();

	itemId1.ItemIdSet(10);
	itemId2.ItemIdSet(11);
	CPPUNIT_ASSERT_MESSAGE ("ItemId operator '!=' check failed!", itemId1 != itemId2);

	auto pMessageInfo = new CstiMessageInfo();
	pMessageInfo->ItemIdSet(itemId1);
	CstiItemId itemId = pMessageInfo->ItemIdGet();
	CPPUNIT_ASSERT_MESSAGE ("ItemId was not set!", itemId.IsValid ());

	pMessageInfo->NameSet("MessageInfoName");
	const char *pszMsgName = pMessageInfo->NameGet();
	CPPUNIT_ASSERT_MESSAGE ("MesageInfo NameSet() or NameGet() failed!", !strcmp(pszMsgName, "MessageInfoName"));

	pMessageInfo->PausePointSet(10);
	uint32_t un32PausePoint = pMessageInfo->PausePointGet();
	CPPUNIT_ASSERT_MESSAGE ("MesageInfo PausePointSet() or PausePonitGet() failed!", un32PausePoint == 10);

	pMessageInfo->DialStringSet("8013334444");
	const char *pszDialString = pMessageInfo->DialStringGet();
	CPPUNIT_ASSERT_MESSAGE ("MesageInfo DialStringSet() or DialStringGet() failed!", !strcmp(pszDialString, "8013334444"));

	pMessageInfo->ViewedSet(estiTRUE);
	EstiBool bViewed = pMessageInfo->ViewedGet();
	CPPUNIT_ASSERT_MESSAGE ("MesageInfo ViewedSet() or ViewedGet() failed!", bViewed == estiTRUE);

	delete pMessageInfo;
}


#endif
