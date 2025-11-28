////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: CstiSipRegistration
//
//  File Name:  CstiSipRegistration.h
//
//  Abstract:
//	See CstiSipRegistration.cpp for abstract.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTISIPREGISTRATION_H
#define CSTISIPREGISTRATION_H


//
// Forward Declarations
//
class CstiSipRegistration;

//
// Includes
//
#include <vector>
#include <string>
#include <mutex>
#include "stiSVX.h"
#include "stiConfDefs.h"


// Sip
#include "RvSipMid.h"
#include "RvSipStackTypes.h"
#include "RvSipStack.h"


//
// Constants
//


//
// Typedefs
//
enum EstiRegistrationEvent
{
	REG_EVENT_REGISTERING,
	REG_EVENT_REGISTERED,
	REG_EVENT_FAILED,
	REG_EVENT_CONNECTION_LOST,
	REG_EVENT_REREGISTER_NEEDED,
	REG_EVENT_BAD_CREDENTIALS,
	REG_EVENT_TIME_SET,
	REG_EVENT_OUT_OF_RESOURCES
};

enum EstiRegistrationOperation
{
	eOperationNone = 0,
	eOperationQuery = 1,
	eOperationRegister = 2,
	eOperationUnregisterInvalid = 3,
	eOperationUnregister = 4,
};


typedef stiHResult (*PstiSipRegistrationEventCallback) (void *pUserData, CstiSipRegistration *pRegistration, EstiRegistrationEvent eEvent, size_t EventParam);

//
// Forward Declarations
//

//
// Globals
//

//
// Class Declaration
//
class CstiSipRegistration
{
public:
	
	struct SstiRegistrarSettings
	{
		//
		// Constructor
		//
		SstiRegistrarSettings () = default;
		
		SstiRegistrarSettings (const SstiRegistrarSettings &Other) = default;
		SstiRegistrarSettings &operator = (const SstiRegistrarSettings &Other) = default;
		SstiRegistrarSettings (SstiRegistrarSettings &&other) = delete;
		SstiRegistrarSettings &operator= (SstiRegistrarSettings &&other) = delete;

		~SstiRegistrarSettings () = default;

		EstiProtocol eProtocol{estiSIP};
		std::string User;
		std::string Password;
//		bool bAllowMultipleDevices;
		int nMaxContacts{0};
		EstiTransport eTransport{estiTransportUDP};
		uint16_t un16LocalSipPort{0};
		uint16_t un16ProxyPort{0};
		std::string ProxyAddress;
		std::string ProxyIpAddress;
	};
	
	CstiSipRegistration (
		const char *pszPhoneNumber,
		const std::string &SipInstanceGUID,
		int nRegistrationID,
		HRPOOL hAppPool,
		RvSipRegClientMgrHandle hRegClientMgr,
		RvSipStackHandle hStackMgr,
		std::recursive_mutex &execMutex,
		PstiSipRegistrationEventCallback pEventCallback,
		void *pCallbackParam);

	virtual ~CstiSipRegistration ();

	CstiSipRegistration (const CstiSipRegistration &other) = delete;
	CstiSipRegistration (CstiSipRegistration &&other) = delete;
	CstiSipRegistration &operator= (const CstiSipRegistration &other) = delete;
	CstiSipRegistration &operator= (CstiSipRegistration &&other) = delete;

	const char *PhoneNumberGet ()
	{
		return m_PhoneNumber.c_str ();
	}
	
	void RegistrarSettingsSet (const SstiRegistrarSettings *pstiRegistrarSettings);
	
	const SstiRegistrarSettings *RegistrarSettingsGet () const
	{
		return &m_stiRegistrarSettings;
	}
	
	stiHResult Reset ();
	
	static void RVCALLCONV RegistrarStateChangedCB (
		RvSipRegClientHandle hRegClient,
		RvSipAppRegClientHandle hAppRegClient,
		RvSipRegClientState eState,
		RvSipRegClientStateChangeReason eReason);
	
	static RvStatus RVCALLCONV FinalDestResolvedCB (
		RvSipRegClientHandle hRegClient,
		RvSipAppRegClientHandle hAppRegClient,
		RvSipTranscHandle hTransc,
		RvSipMsgHandle hMsgToSend);
	
	stiHResult UnregisterStart ();
	stiHResult RegisterStart ();
	stiHResult RegisterStop ();
	void RegisterFailureRetry ();
	void ReflexiveAddressGet (std::string *pAddress, uint16_t *pun16Port) const
	{
		pAddress->assign (m_ReflexiveAddress);
		*pun16Port = m_un16RPort;
	}
	
	EstiTransport TransportGet () const
	{
		return m_stiRegistrarSettings.eTransport;
	}
	
	stiHResult Lock ();

	void Unlock ();
	
	uint32_t RegistrationExpireTimeGet ()
	{
		return m_un32RegistrationExpireTime;
	}
	
	EstiRegistrationOperation CurrentOperationGet () const
	{
		return m_eCurrentOperation;
	}

	RvSipTransportConnectionHandle ConnectionHandleGet () const;

private:
	
	stiHResult ContactSet ();
	stiHResult ContactClear ();
	
	bool m_bUnregistering;
	bool m_bTerminating;
	bool m_bQueryCompleted;
	HRPOOL m_hAppPool;
	RvSipRegClientHandle m_hRegClient;
	bool m_bRegistrarLoginAttempted;
	unsigned int m_nRegistrationTimeout; // In ms.
	SstiRegistrarSettings m_stiRegistrarSettings;
	stiWDOG_ID m_wdidReRegisterTimer;
	std::vector<RvSipContactHeaderHandle> m_vInvalidContacts;
//	HPAGE m_hInvalidContactsPage;
	std::vector<RvSipContactHeaderHandle> m_vValidContacts;
	HPAGE m_hValidContactsPage;
	void *m_pUserData;
	PstiSipRegistrationEventCallback m_pEventCallback;
	std::string m_PhoneNumber;
	std::string m_SipInstanceGUID;
	int m_nRegistrationID;
	RvSipRegClientMgrHandle m_hRegClientMgr;
	RvSipStackHandle m_hStackMgr;
	std::string m_ReflexiveAddress;
	uint16_t m_un16RPort;
	std::string m_Protocol;
	std::list<EstiRegistrationOperation> m_Queue;
	EstiRegistrationOperation m_eCurrentOperation;
	
	std::recursive_mutex &m_execMutex;

	stiHResult EventCallback (EstiRegistrationEvent eEvent, size_t EventParam = 0);
//	stiHResult BogusContactsDetermine (RvSipMsgHandle hRegisterResponseMsg);
	stiHResult InvalidContactsAttach (RvSipMsgHandle hMsg);
	void ReRegisterTimerRun ();
	void StartReRegisterFor (RvSipMsgHandle hMsg);
	static int ReRegisterTimerCB (
		size_t param);
	void SetReRegistrationTimeout (unsigned int nMilliseconds);
	stiHResult RegistrationClientCreate ();
	stiHResult RegistrationExecute (
		EstiRegistrationOperation eOperation,
		bool *pbOperationStarted);
	
	stiHResult RegistrationNext ();
	
	stiHResult RegistrationRequestCreate (
		EstiRegistrationOperation *peOperation,
		RvSipMsgHandle hMsg);
	
	stiHResult WithAuthenticationReRegister (
		RvSipRegClientHandle hRegClient);

	
	stiHResult ValidContactCreate (
		const char *pszPhoneNumber,
		const char *pszProtocol, /// sip
		EstiTransport eTransport, /// tcp or udp
		const char *pszIpAddress, /// the ip address
		uint16_t un16Port, /// the listening port (if 0, will not create contact)
		float fQVal, /// The order of preference for ordered lists. (if 0.0, it will not be added)
		RvSipContactHeaderHandle *phContactHeader);
	
	stiHResult ValidContactsAttach (
		RvSipMsgHandle hMsg,
		int nMaxContacts=999);
	
	stiHResult ReflexiveAddressDetermine (
		RvSipMsgHandle hMsg,
		bool *pbReflexiveAddressChanged);
	
	uint32_t m_un32RegistrationExpireTime;
	
};  // End class CstiSipRegistration declaration


#endif // CSTISIPREGISTRATION_H
// end file CstiSipRegistration.h
