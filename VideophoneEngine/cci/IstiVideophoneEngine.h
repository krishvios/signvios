/*!
 * \file IstiVideophoneEngine.h
 * \brief Videophone engine interface class
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 * 
 */
#ifndef ISTIVIDEOPHONEENGINE_H
#define ISTIVIDEOPHONEENGINE_H

#include "stiSVX.h"
#include "IstiCall.h"
#include "IVRCLCall.h"
#include "ServiceOutageMessage.h"
#include "ISignal.h"
#include "OptString.h"
#include "ServiceResponse.h"
#include "ExternalConferenceData.h"

#include <vector>
#include <list>
#include <memory>
#include <functional>


// The static port start and end are for the static media and control start and end ranges.
#define DYNAMIC_PORT_START	49153
#define DYNAMIC_PORT_END	65535
#define STATIC_PORT_START	15328
#define STATIC_PORT_END	65535
#if APPLICATION == APP_NTOUCH_PC || defined (stiMVRS_APP)
#define DEFAULT_CONTROL_RANGE	256
#define DEFAULT_MEDIA_RANGE		256
#else
#define DEFAULT_CONTROL_RANGE	12
#define DEFAULT_MEDIA_RANGE		12
#endif

//
// Forward Declarations
//
class IstiVideophoneEngine;
class IstiMessageViewer;
class CstiCoreRequest;
class CstiMessageRequest;
class CstiCoreResponse;
class CstiItemId;
class IstiImageManager;
class IstiBlockListManager;
class IstiRingGroupManager;
class IstiContactManager;
#ifdef stiLDAP_CONTACT_LIST
class IstiLDAPDirectoryContactManager;
#endif
#ifdef stiMESSAGE_MANAGER
class IstiMessageManager;
#endif
#ifdef stiCALL_HISTORY_MANAGER
class IstiCallHistoryManager;
#endif
#ifdef stiTUNNEL_MANAGER
class IstiTunnelManager;
#include "IstiTunnelManager.h"
#endif
class IUserAccountManager;

struct ClientAuthenticateResult;
struct VPServiceRequestContext;
class CstiUserAccountInfo;

namespace vpe
{
	struct Address;
}

using VPServiceRequestContextSharedPtr = std::shared_ptr<VPServiceRequestContext>;

/*! 
 * \struct  SstiCallResolution 
 *  
 * \brief Contains information about the directory resolution.
 * 
 * This is sent to the application when a directory resolve has completed to allow the application
 * to present information to the user before proceeding.  The common case is when the remote
 * user has changed their number.
 *  
 */
struct SstiCallResolution
{
	IstiCall *pCall = nullptr;

	/*! 
	* \enum EReason 
	* 
	* \brief The reasons the directory resolution should be addressed. 
	*  The values in this enumeration are used in a bit mask
	*  so the values must have unique bits set.
	* 
	*/ 
	enum EReason
	{
		eUNKNOWN = 0x00,
		eREDIRECTED_NUMBER = 0x01,
		eDIAL_METHOD_DETERMINED = 0x02,
		eDIRECTORY_RESOLUTION_FAILED = 0x04,
		eVRS_CALL_NOT_ALLOWED = 0x08,
	};

	int nReason{0};	//! This holds a combination of the enumerated values in EReason.
};


/*!
 * \ingroup VideophoneEngineLayer
 * \brief This function is used to create the videophone engine.
 *
 * This function initializes the platform layer and all related modules.  It should be called
 * before any portion of the videophone engine is used.
 *
 * \param pszProductName The name of the product (Sorenson Videophone, Sorenson Videophone V2)
 * \param pszPhoneType The phone type (i.e. VP-100, VP-200, ntouch)
 * \param pszVersion The version number
 * \param bVerifyAddress Does the address need to be verified (e.g. for 911 calls)?
 * \param bReportDialMethodDetermination Inform the application when a dial method is determined.
 * \param fpCallBack Pointer to the function which handles Videophone Engine notifications
 * \param CallbackParam Parameters for the callback function.
 *
 * \return A pointer to the instantiated videophone engine.
*/
IstiVideophoneEngine *videophoneEngineCreate (
	ProductType productType, 				///< The product type (i.e. VP-100, VP-200, ntouch)
	const std::string &version,				 	///< The version number
	bool verifyAddress,					///< Indicates that E911 addresses must be validated
	bool reportDialMethodDetermination,	///< Inform the application when a dial method is determined
	const std::string &dynamicDataFolder,		///< The folder, under which, dynamic, persistent data will be written (e.g. Property Manager, etc)
	const std::string &staticDataFolder,		///< The folder, under which, static, persistent data will be read (e.g. certificates).
	PstiAppGenericCallback callback,		///< Function called when the engine sends messages to the application
	size_t callbackParam);					///< The callback parameter used when the callback is called.


/*!
 * \ingroup VideophoneEngineLayer
 * \brief This function is used to create the videophone engine.
 * \deprecated Use the version that accepts the callback function and callback parameter
 *
 * This function initializes the platform layer and all related modules.  It should be called
 * before any portion of the videophone engine is used.
 *
 * \param pszProductName The name of the product (Sorenson Videophone, Sorenson Videophone V2)
 * \param pszPhoneType The phone type (i.e. VP-100, VP-200, ntouch)
 * \param pszVersion The version number
 * \param bVerifyAddress Does the address need to be verified (e.g. for 911 calls)?
 * \param bReportDialMethodDetermination Inform the application when a dial method is determined.
 *
 * \return A pointer to the instantiated videophone engine.
*/
IstiVideophoneEngine *videophoneEngineCreate (
	ProductType productType, 				///< The phone type (i.e. VP-100, VP-200, ntouch)
	const std::string &version,				 	///< The version number
	bool verifyAddress,					///< Indicates that E911 addresses must be validated
	bool reportDialMethodDetermination,	///< Inform the application when a dial method is determined
	const std::string &dynamicDataFolder,		///< The folder, under which, dynamic, persistent data will be written (e.g. Property Manager, etc)
	const std::string &staticDataFolder);		///< The folder, under which, static, persistent data will be read (e.g. certificates).

/*!
 * \ingroup VideophoneEngineLayer
 * \brief This function is used to destroy the videophone engine.
 *
 * This function destroys the videophone engine and all related components.
 * It also uninitializes the Platform layer.
 *
 * \param pEngine A pointer to the videophone engine to destroy.
 */
void DestroyVideophoneEngine (
	IstiVideophoneEngine *pEngine);

/*!
 * \ingroup VideophoneEngineLayer
 * \brief This class represents the videophone engine.
 */
class IstiVideophoneEngine
{
public:

	//
	// Setup methods
	//
	/*!
	 * \brief Applicaton Call Back 
	 *  
	 * This sets a call back function to the application in order to 
	 * notify the application of events happening in the engine.  
	 * 
	 * \param fpCallBack Pointer to the function which handles 
	 * Videophone Engine notifications 
	 *  
	 * \param CallbackParam Parameters for the callback function. 
	 */
	virtual void AppNotifyCallBackSet (
		PstiAppGenericCallback fpCallBack,
		size_t CallbackParam) = 0;

	/*!
	* \brief Performs default handling of messages.
	*
	* \param n32Message The message to handle
	* \param Param Parameter data associated to the message
	*/
	virtual stiHResult MessageCleanup (
		int32_t n32Message,
		size_t Param) = 0;
	
	/*!
	 * \brief Startup VP Engine
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult Startup () = 0;

	/*!
	 * \brief Shutdown Videoengine
	 * 
	 * \return Success or failure result
	 */
	virtual stiHResult Shutdown () = 0;

	/*!
	 * \brief Network startup. 
	 *  
	 * Causes network to call network startup, 
	 * setting the network state, 
	 * and starting VRCL.
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult NetworkStartup () = 0;

	/*!
	 * \brief Shut down the network 
	 * Calls shutdown on the network object.  
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult NetworkShutdown () = 0;

	/*!
	 * \brief Services startup 
	 *  
	 * Calls HTTP task startup and core serivces startup, 
	 * state notify service, message service, update service 
	 * file player service, image manager, and stun server. 
	 * 
	 * \return Success or failure result 
	 * stiRESULT_TASK_ALREADY_STARTED if started 
	 */
	virtual stiHResult ServicesStartup () = 0;

	/*!
	 * \brief Shutdown services. 
	 * HTTP, state, core, message, update, file player, 
	 * image manager, and stun serivices, 
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult ServicesShutdown () = 0;

	//
	// VRCL methods
	//
	/*!
	 * \brief Set VRCL Port
	 * 
	 * \param un16Port 
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult VRCLPortSet (
		uint16_t un16Port) = 0;


	/*!
	 * \brief Remove a call object from the list
	 * 
	 * \param poCall Call object to remove.
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult CallObjectRemove (
		IstiCall *call) = 0;

	/*!
	 * \brief Removes any call objects that are left in callStorage
	 * 
	 */
	virtual void allCallObjectsRemove () = 0;

	/*!
	 * \brief Dial a call
	 * 
	 * \param eDialMethod 
	 *  estiDM_BY_DIAL_STRING  User supplied dial string. 
	 *  estiDM_BY_DS_PHONE_NUMBER  Phone number supplied, used for look up in a Directory Service like LDAP.
	 *  estiDM_BY_VRS_PHONE_NUMBER Phone number supplied, used for dialing via a Video Relay Service.	
	 *  estiDM_BY_VRS_WITH_VCO Phone number supplied, used for dialing via a Video Relay Service using Voice Carry Over.
	 *  estiDM_UNKNOWN Unknown dial method.  The number may be a hearing number or a deaf number. 
	 *  estiDM_BY_OTHER_VRS_PROVIDER Call is placed through another VRS provider. 
	 *  estiDM_UNKNOWN_WITH_VCO Unknown dial method. The number may be a hearing number or a deaf number.
	 *  If the number is determined to be a hearing number then use VCO.
	 *  
	 * \param pszDialString String to be dialed ie 8015551212
	 * \param pszCallListName Phone number calling from if different (can be set to NULL) 
	 * \param pszRelayLanguage English or Spanish thus far 
	 * \param pCallID pointer to a call object to manage the call
	 * \return Success or failure result 
	 *  
	 */
	virtual stiHResult CallDial (
		EstiDialMethod eDialMethod,
		const std::string &dialString,
		const OptString &callListName,
		const OptString &relayLanguage,
		EstiDialSource eDialSource,
		IstiCall **pCallID) = 0;

	/*!
	 * \brief Dial a call
	 *
	 * \param eDialMethod
	 *  estiDM_BY_DIAL_STRING  User supplied dial string.
	 *  estiDM_BY_DS_PHONE_NUMBER  Phone number supplied, used for look up in a Directory Service like LDAP.
	 *  estiDM_BY_VRS_PHONE_NUMBER Phone number supplied, used for dialing via a Video Relay Service.
	 *  estiDM_BY_VRS_WITH_VCO Phone number supplied, used for dialing via a Video Relay Service using Voice Carry Over.
	 *  estiDM_UNKNOWN Unknown dial method.  The number may be a hearing number or a deaf number.
	 *  estiDM_BY_OTHER_VRS_PROVIDER Call is placed through another VRS provider.
	 *  estiDM_UNKNOWN_WITH_VCO Unknown dial method. The number may be a hearing number or a deaf number.
	 *  If the number is determined to be a hearing number then use VCO.
	 *
	 * \param pszDialString String to be dialed ie 8015551212
	 * \param pszCallListName Phone number calling from if different (can be set to NULL)
	 * \param pszRelayLanguage English or Spanish thus far
	 * \param eDialSource where this call originated from in the UI
	 * \param enableEncryption Enable encryption only for this call
	 * \param pCallID pointer to a call object to manage the call
	 * \return Success or failure result
	 *
	 */
	virtual stiHResult CallDial (
		EstiDialMethod eDialMethod,
		const std::string& dialString,
		const OptString& callListName,
		const OptString& relayLanguage,
		EstiDialSource eDialSource,
		bool enableEncryption,
		IstiCall** pCallID) = 0;

	/*!
	 * \brief Dial a call (Specifically used for dialing a phone number that's
	 *        in our contact list)
	 *
	 * \param ContactId The ID of the contact that we're calling
	 * \param PhoneNumberId The ID of the phone number within the contact
	 * \param pCallID pointer to a call object to manage the call
	 * \return Success or failure result
	 *
	 */
	virtual stiHResult CallDial (
		const CstiItemId &ContactId,
		const CstiItemId &PhoneNumberId,
		EstiDialSource eDialSource,
		bool enableEncryption,
		IstiCall **pCallID) = 0;

	virtual stiHResult SignMailSend (
		std::string dialString,
		EstiDialSource eDialSource,
		IstiCall **ppCall) = 0;

	virtual stiHResult SignMailSend (
		const CstiItemId &contactId,
		const CstiItemId &phoneNumberPublicId,
		EstiDialSource dialSource,
		IstiCall **ppCall) = 0;

	/*!
	 * \brief Reformat the phone number
	 * 
	 * \param pszOldPhoneNumber  Old Number
	 * \param pszNewPhoneNumber  Reformated number
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult PhoneNumberReformat(
		const std::string &oldPhoneNumber,
		std::string *newPhoneNumber) = 0;

	/*!
	 * \brief Test dial string for validity
	 *
	 * \param pszPhoneNumber Phone number to test
	 *
	 * \return bool
	 */
	virtual bool DialStringIsValid(
		const std::string &phoneNumber) = 0;

	/*!
	 * \brief Test phone number for validity
	 *
	 * \param pszPhoneNumber Phone number to test
	 *
	 * \return bool
	 */
	virtual bool PhoneNumberIsValid(
		const std::string &phoneNumber) = 0;

	//
	// Core Services methods
	//
	/*!
	 * \brief  Update the Password in CoreService. 
	 * 
	 * \param pszPin New pin
	 */
	virtual void CoreAuthenticationPinUpdate (
		const std::string &pin) = 0;

	/*!
	 * \brief Core Login, should be called once core services has been connected
	 * 
	 * \param pszPhoneNumber The user's phone number.
	 * \param pszPin         The user's pin.         
	 * \param pContext                               
	 * \param punRequestId   The ID of the request.
	 * \param pszLoginAs
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult CoreLogin (
		const std::string &phoneNumber,
		const std::string &pin,
		const VPServiceRequestContextSharedPtr &context,
		unsigned int *punRequestId,
		const OptString &loginAs = nullptr) = 0;

	/*!
	 * \brief Authenticates a user using cached information
	 *
	 * \param phoneNumber The user's phone number.
	 * \param pin         The user's pin.
	 *
	 * \return Success or failure result.
	 */
	virtual stiHResult logInUsingCache (
		const std::string &phoneNumber,
		const std::string &pin
	) = 0;

	/*!
	 * \brief Logout of core 
	 * This function should be called when Core Services has been logged in.    
	 *                                                                                                 
	 * \param pContext        
	 * \param punRequestId    
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult CoreLogout (
		const VPServiceRequestContextSharedPtr &context,
		unsigned int *punRequestId) = 0;

	/*!
	 * \brief Send a core request
	 *  This function should be called when Core Services has been logged in.  If  
	 *  not logged in, the CoreService.RequestSend will initiate a re-login        
	 *  sequence.                                                                  
	 *                                                                             
	 *  \note The poCoreRequest parameter MUST have been created with the 'new'    
	 *  operator. This object WILL NO LONGER belong to the calling code, and will  
	 *  be freed by this function even upon failure. 
	 *  
	 * \param poCoreRequest 
	 * \param punRequestId 
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult CoreRequestSend (
		CstiCoreRequest *poCoreRequest,
		unsigned int *punRequestId = nullptr) = 0;

	/*!
	 * \brief Send a core request
	 *  This function should be called when Core Services has been logged in.  If
	 *  not logged in, the CoreService.RequestSend will initiate a re-login
	 *  sequence.
	 *
	 *  \note The poCoreRequest parameter MUST have been created with the 'new'
	 *  operator.
	 *
	 * \param poCoreRequest
	 * \param punRequestId
	 *
	 * \return Success or failure result
	 */
	virtual stiHResult CoreRequestSendEx (
		CstiCoreRequest *poCoreRequest,
		unsigned int *punRequestId) = 0;

	//
	// Message Services methods
	//
	/*!
	 * \brief Send a Message Request 
	 * Sends a message request.  Will automatically reauthenticate if somehow     
	 * core service was logged out.                                               
	 *                                                                            
	 * \note The poMessageRequest parameter MUST have been created with the 'new' 
	 * operator. This object WILL NO LONGER belong to the calling code, and will  
	 * be freed by this function even upon failure. 
	 *  
	 * \param poMessageRequest 
	 * \param punRequestId 
	 * 
	 * \return Success or failure result
	 *
	 * \deprecated Use MessageRequestSendEx instead.  The caller is responsilbe for deleting poMessageRequest when useing MessageRequestSenedEx
	 */
	virtual stiHResult MessageRequestSend (
		CstiMessageRequest *poMessageRequest,
		unsigned int *punRequestId = nullptr) = 0;

	/*!
	 * \brief Send a Message Request
	 * Sends a message request.  Will automatically reauthenticate if somehow
	 * core service was logged out.
	 *
	 * \note The poMessageRequest parameter MUST have been created with the 'new'
	 * operator. If an error occurs then the caller is responsible for deleting
	 * the poMessageRequest object.
	 *
	 * \param poMessageRequest
	 * \param punRequestId
	 *
	 * \return Success or failure result
	 */
	virtual stiHResult MessageRequestSendEx (
		CstiMessageRequest *poMessageRequest,
		unsigned int *punRequestId = nullptr) = 0;

	/*!
	 * \brief Cancel a message request
	 * Cancels a previously submitted message request.  
	 *  
	 * \param unRequestId 
	 * 
	 * \return Success or failure result
	 */
	virtual void MessageRequestCancel (
		unsigned int unRequestId) = 0;

	//
	// State Notify Services methods
	//
	/*!
	 * \brief Requests a heartbeat to be sent to State Notify
	 * 
	 * \return Success or failure result
	 */
	virtual void StateNotifyHeartbeatRequest () = 0;

	//
	// General settings methods
	//
	/*!
	 * \brief Get Local Interface Mode
	 * 
	 * \return The current interface as defined in ::EstiInterfaceMode
	 */
	virtual EstiInterfaceMode LocalInterfaceModeGet () const = 0;

	/*!
	 * \brief Set Local Interface Mode
	 * 
	 * \param eMode 
	 * 
	 * \return Success or failure result
	 */
	virtual stiHResult LocalInterfaceModeSet (
		EstiInterfaceMode eMode) = 0;

	/*!
	 * \brief Default Providor Agreement
	 * 
	 * \param bAgreed 
	 */
	virtual void DefaultProviderAgreementSet (bool bAgreed) = 0;

	//
	// Public IP methods
	//
	/*!
	 * \brief Gets the current public IP address assignment method. 
	 * It is recommended that this function not be called until the conference       
	 * engine has finished initializing.  This function may be called at any time,   
	 * but there is no gaurantee that the setting is initialized or valid until the  
	 * conference engine has reached the WAITING state.                              
	 * 
	 * \return The current public IP assignment method as defined by ::EstiPublicIPAssignMethod
	 */
	virtual EstiPublicIPAssignMethod PublicIPAssignMethodGet () const = 0;

	/*!
	 * \brief Get Public IP Address
	 * 
	 * \param eIPAssignMethod The assignment method for which the IP address is desired.                
	 * \param pszAddress 
	 *  On success a constant string containing the system's public IP address in the
	 *  format "xxx.xxx.xxx.xxx" where "xxx" is a number between 0 and 255.
	 * \param unBufferLength Length of the pszAddress buffer.
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult PublicIPAddressGet (
			EstiPublicIPAssignMethod eAssignMethod,
			std::string *pAddress,
			EstiIpAddressType eIpAddressType) const = 0;

	/*!
	 * \brief Set the Public IP Address assignment type
	 * 
	 * \param eIPAssignMethod The public IP assignment method as defined by ::EstiPublicIPAssignMethod
	 *  
	 * \param pszIPAddress The public IP address if one is using the
	 * estiIP_ASSIGN_MANUAL method, otherwise this should be set to NULL. Use the
	 * "xxx.xxx.xxx.xxx" format where "xxx" is a number between 0 and 255.
	 *  
	 * \return Success or failure result 
	 */
	virtual stiHResult PublicIPAddressInfoSet (
		EstiPublicIPAssignMethod eIPAssignMethod,
		const std::string &ipAddress) = 0;

	//
	// Conference setting methods
	//
	/*!
	 * \brief Gets the maximum conferencing send and receive speeds
	 * 
	 * \param pun32RecvSpeed
	 * \param pun32SendSpeed 
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult MaxSpeedsGet (
		uint32_t *pun32RecvSpeed,
		uint32_t *pun32SendSpeed) const = 0;

	/*!
	 * \brief Sets the maximum conferencing send and receive speeds
	 * 
	 * \param un32RecvSpeed 
	 * \param un32SendSpeed 
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult MaxSpeedsSet (
		uint32_t un32RecvSpeed,
		uint32_t un32SendSpeed) = 0;
	
	/*!
	 * \brief Gets the current AutoSpeedSetting based on AutoSpeedEnabled
	 *  and AutoSpeedMode
	 *
	 * \return Auto Speed Mode by enum
	 */
	virtual EstiAutoSpeedMode AutoSpeedSettingGet () const = 0;
	
	/*!
	 * \brief Sets the maximum send and receive speeds to be used if Auto
	 *	Speed mode is auto
	 *
	 *	/param un32MaxAutoRecvSpeed
	 *	/param un32MaxAutoSendSpeed
	 *
	 * \return Success or failure result
	 */
	virtual stiHResult MaxAutoSpeedsSet (
		uint32_t un32MaxAutoRecvSpeed,
		uint32_t un32MaxAutoSendSpeed) = 0;

	/*!
	 * \brief Gets the current number of rings before the signmail greeting begins to play 
	 *  	  and returns the maximum number of rings allowed.
	 *  
	 * \param pun32CurrentMaxRings 
	 * \param pun32MaxRings 
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult RingsBeforeGreetingGet (
		uint32_t *pun32CurrentMaxRings,
		uint32_t *pun32MaxRings) const = 0;

	/*!
	 * \brief Sets the number of rings before the signmail greeting begins to play. 
	 * 
	 * \param un32MaxRings 
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult RingsBeforeGreetingSet (
		uint32_t un32MaxRings) = 0;

	/*!
	 * \brief Gets the Personal SignMaiil Greeting preferences.
	 *  
	 * \param pGreetingPreferences
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult GreetingPreferencesGet (
		SstiPersonalGreetingPreferences *pGreetingPreferences) const = 0;

	/*!
	 * \brief Sets the Personal SignMail Greeting preferences that have changed. 
	 * 
	 * \param pGreetingPreferences 
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult GreetingPreferencesSet(
		const SstiPersonalGreetingPreferences *pGreetingPreferences) = 0;

	/*!
	 * \brief Gets the current auto-reject setting (on/off). 
	 *  
	 * \return Indicates whether or not auto reject is on.  Possible values are defined in ::EstiSwitch.
	 *  
	 */
	virtual EstiSwitch AutoRejectGet () const = 0;

	/*!
	 * \brief Sets the auto-reject switch (on/off) 
	 * 
	 * \param eReject Auto-reject switch - estiON or estiOFF. 
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult AutoRejectSet (
		EstiSwitch eReject) = 0;

	/*!
	 * \brief Gets the maximum number of calls
	 *
	 * \return The max number of calls
	 */
	virtual int MaxCallsGet () const = 0;

	/*!
	 * \brief Sets the maximum number of calls
	 * 
	 * The interface mode may override the actual number of max calls.
	 * For exmaple, interpreter mode only allows one call.
	 * 
	 * \param unNumMaxCalls The number of max calls
	 * 
	 * \return Success or failure
	 */
	virtual stiHResult MaxCallsSet (
		int numMaxCalls) = 0;
		
	/*!\brief Sets the user phone numbers
	 *
	 * \param psUserPhoneNumbers 
	 *
	 * \return Success or failure result 
	 */
	virtual stiHResult UserPhoneNumbersSet (
		const SstiUserPhoneNumbers *psUserPhoneNumbers) = 0;

	//
	// Conference ports methods
	//
	/*!
	 * \brief Sets the conference port settings 
	 * 
	 * \param pstConferencePorts Includes the start and end ports 
	 *   
	 * \return Success or failure result
	 */
	virtual stiHResult ConferencePortsSettingsSet (
		SstiConferencePortsSettings *pstConferencePorts) = 0;

	/*!
	 * \brief Gets the ports used in conferencing  
	 * 
	 * \param pstConferencePorts 
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult ConferencePortsSettingsGet (
		SstiConferencePortsSettings *pstConferencePorts) const = 0;

	/*!
	 * \brief Gets the status of the conferencing ports
	 * 
	 * \param pstConferencePorts 
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult ConferencePortsStatusGet (
		SstiConferencePortsStatus *pstConferencePorts) const = 0;
		
	/*!
	 * \brief Get the list of languages handled
	 * 
	 * \param pList 
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult RelayLanguageListGet (
		std::vector <std::string> *pList) const = 0;

	virtual void RelayLanguageSet (
			IstiCall *call,
			OptString langugage) = 0;
	
	virtual stiHResult SipProxyAddressSet (const std::string &sipProxyAddress) = 0;

	virtual bool GroupVideoChatAllowed (
		IstiCall *poCall1,
		IstiCall *poCall2) const = 0;

	virtual stiHResult GroupVideoChatJoin (
		IstiCall *poCall1,
		IstiCall *poCall2) = 0;

	virtual bool GroupVideoChatEnabled() const = 0;
	
	virtual stiHResult dhviCreate (
		IstiCall *call) = 0;

	/*!\brief Mutes/Unmutes the audible ringer.
	 *
	 *  This function can be called while in the waiting state.
	 * 
	 * \param bMute 
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult AudibleRingerMute (
		bool bMute) = 0;

	/*!\brief Sets the callback number for Voice Carry-Over
	 *
	 *  This function should be called while in the WAITING state and will fail if
	 *  the state not WAITING.
	 *
	 *  This function sets a preference for the use of VCO with VRS calls.
	 * 
	 * \param pszCallbackNumber 
	 *  
	 * \return Success or failure result
	 */
	virtual stiHResult VCOCallbackNumberSet (
		const std::string &callbackNumber) = 0;

	/*!\brief Use this method to determine if VCO is enabled or disabled.
	 *
	 *  This function sets a preference for the use of VCO with VRS calls.
	 * 
	 * 	\return true if VCO is enable.  Otherwise, false is returned.
	 */
	virtual bool VCOUseGet () const = 0;
	
	/*!\brief Sets whether to use VCO by default
	 *
	 *  This function sets a preference for the use of VCO with VRS calls.
	 * 
	 * 	\param bUseVCO Set to true to enable VCO.  Otherwise, set to false.
	 * 
	 * 	\return Success or failure result
	 */
	virtual stiHResult VCOUseSet (
		bool bUseVCO) = 0;

	/*!\brief Sets type of VCO to use by default
	 *
	 *  This function sets a preference for the type of VCO with VRS calls.
	 * 
	 * 	\param eVCOType Indicates if VCO is enabled and, if so, what type of VCO.
	 * 
	 * 	\return Success or failure result
	 */
	virtual stiHResult VCOTypeSet (
		EstiVcoType eVCOType) = 0;
	
	/*!
	 * \brief Sets audio enabled (on/off) 
	 * 
	 * \param eEnabled Audio Enabled - true or false. 
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult AudioEnabledSet (
		bool eEnabled) = 0;

#ifdef stiTUNNEL_MANAGER
	/*!
	 * \brief Gets the current tunneling setting (on/off).
	 *
	 * \return Indicates whether or not tunneling is enabled.
	 *
	 */
	virtual bool TunnelingEnabledGet () const = 0;

	/*!
	 * \brief Enables or disables tunneling
	 *
	 * \param eEnableTunneling Set to true to enable Tunneling.  Otherwise, set to false.
	 *
	 * \return Success or failure result
	 */
	virtual stiHResult TunnelingEnabledSet (
		bool eEnableTunneling) = 0;
#endif

	/*!
	 * \brief Get the local alternate name
	 *
	 *  It is used to get the Alternate Name.  This will often be the same
	 *  as the Local Name.  In some cases, such as when in Interpreter
	 *  mode, the display name will be something like
	 *  Sorenson VRS and the Alternate name will end up being set to the
	 *  Interpreter's ID.
	 *
	 * \param pAltName A pointer to a std::string to fill with the local name.
	 *
	 * \return Success or failure result
	 */
	virtual stiHResult LocalAlternateNameGet (
		std::string *pAltName) = 0; 	//! A pointer to a std::sting to fill with the local name.

	/*!
	 * \brief Set the local names into the engine 
	 * 
	 *  This function should be called by the application layer.  It is
	 *  used to set the Display Name (sent in the DisplayName field of the
	 *  Q.931 Setup message and the Alternate Name (sent in the SInfo).
	 *  In some cases, these will be the same but in other cases, such as
	 *  when in Interpreter mode, the display name will be something like
	 *  Sorenson VRS and the Alternate name will end up being set to the
	 *  Interpreter's ID.
	 * \param pszName A pointer to a character array to fill with the local name. 
	 * 
	 * \return Success or failure result
	 */
	virtual stiHResult LocalNameSet (
		const std::string &name) = 0;

	/*!
	 * \brief Get the local name
	 *  
	 *  It is used to set the Display Name (sent in the DisplayName field of the
	 *  Q.931 Setup message and the Alternate Name (sent in the SInfo).
	 *  In some cases, these will be the same but in other cases, such as
	 *  when in Interpreter mode, the display name will be something like
	 *  Sorenson VRS and the Alternate name will end up being set to the
	 *  Interpreter's ID. 	//! A pointer to a character array to fill with the local name.
	 *  
	 * \param pszName A pointer to a character array to fill with the local name.  
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult LocalNameGet (
		std::string *name) 	//! A pointer to a character array to fill with the local name.
		const = 0;

	/*!
	 * \brief Requests the Greeting's URL, whether it is enabled and record time 
	 * 
	 * \return Success or failure result
	 */
	virtual stiHResult GreetingInfoGet() = 0;

	/*!
	 * \brief Requests the Greeting's upload GUID and then passes it to the FilePlayer 
	 * 		  so that the Greeting can be uploaded. 
	 * 		  
	 * 
	 * \return Success or failure result
	 */
	virtual stiHResult GreetingUpload() = 0;

#ifdef stiIMAGE_MANAGER
	/*!
	 * \brief Returns a pointer to the Image Manager 
	 * 
	 * \return A pointer to a ::IstiImageManager object. 
	 */
	virtual IstiImageManager *ImageManagerGet() = 0;
#endif //stiIMAGE_MANAGER

	/*!
	 * \brief Returns a pointer to the block list manager. 
	 * 
	 * \return A pointer to a ::IstiBlockListManager object.
	 */
	virtual IstiBlockListManager *BlockListManagerGet () = 0;

	/*!
	 * \brief Returns a pointer to the ring group manager. 
	 * 
	 * \return A pointer to a ::IstiRingGroupManager object.
	 */
	virtual IstiRingGroupManager *RingGroupManagerGet () = 0;
	
	/*!
	 * \brief Returns a pointer to the Contact Manager object.
	 * 
	 * \return A pointer to a ContactManager object.
	 */
	virtual IstiContactManager *ContactManagerGet () = 0;
	
#ifdef stiLDAP_CONTACT_LIST
	/*!
	 * \brief Returns a pointer to the LDAPDirectory Contact Manager object.
	 *
	 * \return A pointer to a LDAPDirectoryContactManager object.
	 */
	virtual IstiLDAPDirectoryContactManager *LDAPDirectoryContactManagerGet () = 0;
#endif //stiLDAP_CONTACT_LIST

#ifdef stiMESSAGE_MANAGER
	/*!
	 * \brief Returns a pointer to the Message Manager. 
	 * 
	 * \return A pointer to a ::IstiMessageManager object.
	 */
	virtual IstiMessageManager *MessageManagerGet () = 0;
#endif //stiMESSAGE_MANAGER

#ifdef stiCALL_HISTORY_MANAGER
	/*!
	 * \brief Returns a pointer to the Call History manager.
	 *
	 * \return A pointer to a ::IstiCallHistoryManager object.
	 */
	virtual IstiCallHistoryManager *CallHistoryManagerGet () = 0;
#endif

#ifdef stiTUNNEL_MANAGER
	/*!
	 * \brief Returns a pointer to the Tunnel Manager.
	 *
	 * \return A pointer to a ::IstiTunnelManager object.
	 */
	virtual IstiTunnelManager *TunnelManagerGet () = 0;
#endif //stiTUNNEL_MANAGER

	virtual IUserAccountManager *userAccountManagerGet () = 0;

	virtual void NetworkChangeNotify() = 0;
	virtual void ClientReRegister() = 0;
	virtual void SipStackRestart() = 0;

	virtual void AllowIncomingCallsModeSet(bool bAllow) = 0;

	virtual std::string MessageStringGet (
		int32_t n32Message) = 0;
		
		
	virtual EstiSecureCallMode SecureCallModeGet () = 0;
	
	virtual void SecureCallModeSet (EstiSecureCallMode eSecureCallMode) = 0;

	virtual DeviceLocationType deviceLocationTypeGet () = 0;
	
	virtual void deviceLocationTypeSet (DeviceLocationType deviceLocationType) = 0;
	
	/*!
	 * \brief sends a text event to the remote logging service.
	 *
	 * \param pszBuff
	 *
	 * \return Success or failure result
	 */
	virtual stiHResult RemoteLogEventSend (
		const std::string &buff) = 0;

	/*!
	 * \brief Sets the HTTP Proxy server for the engine to use with all HTTP(s) communications
	 *
	 * \param ServerAddressAndPort The server and port of the HTTP Proxy.  Set to an empty string to not use a proxy.
	 */
	virtual stiHResult HTTPProxyServerSet (
		const std::string &HTTPProxyAddress,
		uint16_t un16Port) = 0;

	/*!
	 * \brief Gets the HTTP Proxy server that the engine is currently using
	 *
	 * \param ServerAddressAndPort The server and port of the HTTP Proxy.  This may be an empty string if a proxy is not being used.
	 */
	virtual stiHResult HTTPProxyServerGet (
		std::string *pHTTPProxyAddress,
		uint16_t *pun16HTTPProxyPort) const = 0;
	
	/*!
	 * \brief Gets the enabled status of the block caller id feature.
	 *
	 * \return Indicates whether or not block caller ID is enabled.  Possible values are defined in ::EstiSwitch.
	 *
	 */
	virtual EstiSwitch BlockCallerIDEnabledGet () const = 0;
	
	/*!
	 * \brief Gets the current block caller ID setting (on/off).
	 *
	 * \return Indicates whether or not block caller ID is on.  Possible values are defined in ::EstiSwitch.
	 *
	 */
	virtual EstiSwitch BlockCallerIDGet () const = 0;
	
	/*!
	 * \brief Sets the block caller ID switch (on/off)
	 *
	 * \param eBlock block caller ID switch - estiON or estiOFF.
	 *
	 * \return Success or failure result
	 */
	virtual stiHResult BlockCallerIDSet (
		EstiSwitch eBlock) = 0;

	/*!
	 * \brief Gets the current block anonymous callers setting (on/off).
	 *
	 * \return Indicates whether or not block anonymous callers is on.  Possible values are defined in ::EstiSwitch.
	 *
	 */
	virtual EstiSwitch BlockAnonymousCallersGet () const = 0;
	
	/*!
	 * \brief Sets the block anonymous callers switch (on/off)
	 *
	 * \param eBlock block anonymous callers switch - estiON or estiOFF.
	 *
	 * \return Success or failure result
	 */
	virtual stiHResult BlockAnonymousCallersSet (
		EstiSwitch eBlock) = 0;

	/*!
	 * \brief Gets whether IPv6 is enabled or not
	 *
	 * \return Success or failure result
	 */
	virtual bool IPv6EnabledGet () = 0;

	/*!
	 * \brief Enables/Disables IPv6
	 *
	 * \param bEnable true or false
	 *
	 * \return Success or failure result
	 */
	virtual stiHResult IPv6EnabledSet (
		bool bEnable) = 0;

	/*!
	 * \brief Gets the current hearing status
	 *
	 * \return Enum representation of status
	 */
	virtual EstiHearingCallStatus HearingStatusGet () = 0;



	/*!
	* \brief Registers a callback to notify the UI of service outage changes.
	*/
	virtual void ServiceOutageMessageConnect (std::function <void(std::shared_ptr<vpe::ServiceOutageMessage>)> callback) = 0;
	
	/*!
	 * \brief Force ServiceOutageClient to check for Outage
	 */
	virtual void ServiceOutageForceCheck() = 0;

	/*!
	* \brief Tells us if we can reach Core or not
	*
	* \return True if we cannot reach Core, False otherwise
	*/
	virtual bool isOffline() = 0;

	/*!
	* \brief Compares dial string against know customer service numbers
	*
	* \return True if number is customer service, False otherwise
	*/
	virtual bool isCustomerServiceNumber(std::string dialString) = 0;

	/*!
	* \brief A Signal to notify the UI of a Shared Contact being received.
	*/
	virtual ISignal<const SstiSharedContact&>& contactReceivedSignalGet() = 0;


	/*!
	* \brief A Signal to notify the UI of the result of a client authentication request
	*/
	virtual ISignal<const ServiceResponseSP<ClientAuthenticateResult>&>& clientAuthenticateSignalGet () = 0;

	virtual ISignal<const ServiceResponseSP<CstiUserAccountInfo>&>& userAccountInfoReceivedSignalGet () = 0;

	virtual ISignal<const ServiceResponseSP<vpe::Address>&>& emergencyAddressReceivedSignalGet () = 0;

	virtual ISignal<const ServiceResponseSP<EstiEmergencyAddressProvisionStatus>&>& emergencyAddressStatusReceivedSignalGet () = 0;

	virtual ISignal<EstiSecureCallMode>& secureCallModeChangedSignalGet () = 0;
	
	virtual ISignal<DeviceLocationType>& deviceLocationTypeChangedSignalGet () = 0;

	virtual ISignal<const ExternalConferenceData&>& externalConferenceSignalGet() = 0;

	virtual ISignal<bool>& externalCameraUseSignalGet() = 0;

	/*!\brief Calls CoreLogin() to login a ported user.
	 *
	 *  This function should be called when reconnecting a ported user.
	 *
	 * \param phoneNumber The user's phone number.
	 * \param password         The user's pin.
	 *
	 * \return Success or failure result
	 */
	virtual stiHResult portBackLogin(
		const std::string &phoneNumber,
		const std::string &password) = 0;
	
	/*!
	 * \brief Used to set if we want conference manager to use the proxy keep alive code.
	 *
	 *\param useProxyKeepAlive Sets wether to use keep alive code. If never called default is true.
	 */
	virtual void useProxyKeepAliveSet (bool useProxyKeepAlive) = 0;

	/*!
	 * \brief Used to setup an external call for VRCL to use
	 *
	 *\param the external VRCL call interface
	 */
	virtual void externalCallSet(VRCLCallSP& call) = 0;

	virtual stiHResult externalCallStatusSet(std::string& callStatus) = 0;

	virtual stiHResult externalCallParticipantCountSet(int participantCount) = 0;

};

#endif // ISTIVIDEOPHONEENGINE_H
