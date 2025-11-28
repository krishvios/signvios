/*!
* \file IstiPlatform.h
* \brief Platform layer interface
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
*/
#ifndef ISTIPLATFORM_H
#define ISTIPLATFORM_H

#include "stiError.h"
#include "stiSVX.h"
#include "ISignal.h"

/*!
 * \brief this closely models the SstiStateChange struct, but allows
 * not pulling unnecessary stuff to the platform layer
 * (ie: protocol manager and CstiCall)
 *
 * Also prevents exposing call to the platform layer
 */
struct PlatformCallStateChange // TODO: better name?
{
	// Really just need a unique identifier to differentiate calls
	int callIndex = -1;
	EsmiCallState prevState = esmiCS_UNKNOWN;
	uint32_t prevSubStates = 0;
	EsmiCallState newState = esmiCS_UNKNOWN;
	uint32_t newSubStates = 0;
};

#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
#include "IstiVideoInput.h"

/*!
 * \brief supported wifi modes
 */
enum eWifiMode {
	eWifiMode_Unknown = -1,
	eWifiMode_None = 0,
	eWifiMode_B,
	eWifiMode_G,
	eWifiMode_N
};

/*!
 * \brief Name of paired bluetooth device, and if it is currently connected
 */
struct SstiBluetoothStatus
{
	std::string Name;
	bool bConnected{false};
};

/*!
 * \brief structure for returning the advanced status information
 */
struct SstiAdvancedStatus
{
	// Rcu static values
	bool rcuConnected {false};
	bool cameraConnected {false};
	std::string rcuSerialNumber;
	std::string rcuLensType;
	std::string rcuFirmwareVersion;
	std::string rcuDriverVersion;

	//Rcu dynamic values
	int rcuTemp {0};

	std::string mpuSerialNumber;

	int cpuTemp {0};
	int upTime {0};
	std::string loadAvg;

	int focusPosition {0};
	int brightness {0};
	int saturation {0};

	bool audioJack {false};
	int inputAudioLevel {0};

	bool wiredConnected {false};
	int ethernetSpeed {0};

	bool wirelessConnected {false};
	float wirelessFrequency {0};
	int wirelessDataRate {0};
	int wirelessBandwidth {0};
	std::string wirelessProtocol {"None"};
	int wirelessSignalStrength {0};
	int wirelessTransmitPower {0};
	int wirelessChannel {0};

	std::list<SstiBluetoothStatus>bluetoothList;
	std::list<std::string> usbList;

	std::string cecTvVendor;
	
};
#endif

/*! 
 * \defgroup PlatformLayer Platform Layer
 * The platform layer is a collection of interfaces classes that abstract devices that
 * are often found on the different platforms.  Each of the classes need to have a 
 * corresponding concrete class implemented.  The videophone engine will expect to
 * find the interfaces that are defined by these classes.
 */

/*!
 * \ingroup PlatformLayer
 * \brief This class represents the platform.
 */
class IstiPlatform
{
public:

	IstiPlatform (const IstiPlatform &other) = delete;
	IstiPlatform (IstiPlatform &&other) = delete;
	IstiPlatform &operator= (const IstiPlatform &other) = delete;
	IstiPlatform &operator= (IstiPlatform &&other) = delete;

	/*!
	 * \brief Instance Get 
	 * returns a pointer to the interface 
	 * Use this pointer to access functions inside 
	 * the Platform class 
	 * 
	 * \return An instance to the platform object.
	 */
	static IstiPlatform *InstanceGet ();
	
#if defined(stiHOLDSERVER)
	/*!
	 * \brief Initialize 
	 * Call this once to setup the Platform interface for use 
	 * 
	 * \param nMaxCalls 
	 * 
	 * \return Success or failure result  
	 */
	virtual stiHResult Initialize (
		int nMaxCalls) = 0;
#else
	/*!
	 * \brief Initalize function used to setup the Platform interface
	 * 
	 * \return Success or failure result  
	 */
	virtual stiHResult Initialize () = 0;

	/*!
	 * \brief Uninitialize used to tear down the Platform interface
	 * 
	 * \return Success or failure result 
	 */
	virtual stiHResult Uninitialize () = 0;
#endif

	/*!\brief Reason codes to indicate why a reboot was requested
	 *
	 */
	enum EstiRestartReason
	{
		estiRESTART_REASON_UNKNOWN = 0,			//!< estiRESTART_REASON_UNKNOWN
		estiRESTART_REASON_MEDIA_LOSS = 1,		//!< estiRESTART_REASON_MEDIA_LOSS
		estiRESTART_REASON_VRCL_COMMAND = 2,		//!< estiRESTART_REASON_VRCL
		estiRESTART_REASON_UPDATE = 3,			//!< estiRESTART_REASON_UPDATE
		estiRESTART_REASON_CRASH = 4,			//!< estiRESTART_REASON_CRASH
		estiRESTART_REASON_ACCOUNT_TRANSFER = 5,	//!< estiRESTART_REASON_ACCOUNT_TRANSFER
		estiRESTART_REASON_DNS_CHANGE = 6,		//!< estiRESTART_REASON_DNS_CHANGE
		estiRESTART_REASON_STATE_NOTIFY_EVENT = 7,	//!< estiRESTART_REASON_STATE_NOTIFY_EVENT
		estiRESTART_REASON_TERMINATED = 8,		//!< estiRESTART_REASON_TERMINATED
		estiRESTART_REASON_UPDATE_FAILED = 9,	//!< estiRESTART_REASON_UPDATE_FAILED
		estiRESTART_REASON_OUT_OF_MEMORY = 10,       //!< estiRESTART_REASON_OUT_OF_MEMORY
		estiRESTART_REASON_UPDATE_TIMEOUT = 11,	//!< estiRESTART_REASON_UPDATE_FAILED
		estiRESTART_REASON_UNHANDLED_EXCEPTION = 12,
		estiRESTART_REASON_POWER_ON_RESET = 13,
		estiRESTART_REASON_WATCHDOG_RESET = 14,
		estiRESTART_REASON_OVER_TEMPERATURE_RESET = 15,
		estiRESTART_REASON_SOFTWARE_RESET = 16,
		estiRESTART_REASON_LOW_POWER_EXIT = 17,
		estiRESTART_REASON_UI_BUTTON = 18,
		estiRESTART_REASON_DAILY = 19
	};


	/*!
	 * \brief Restart System
	 * 
	 * \return Success or failure result  
	 */
	virtual stiHResult RestartSystem (
		EstiRestartReason eRestartReason) = 0;

	virtual stiHResult RestartReasonGet (
		EstiRestartReason *peRestartReason) = 0;

#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
	/*!
	 * return success if the call succeeds, else error.
	 */
	virtual std::string mpuSerialNumberGet () = 0;

	virtual std::string rcuSerialNumberGet () = 0;
		
	virtual stiHResult HdmiInStatusGet (
		int *pnHdmiInStatus) = 0;
	 
	virtual stiHResult HdmiInResolutionGet (
		int *pnHdmiInResolution) = 0;

	virtual stiHResult HdmiPassthroughSet (
		bool bHdmiPassthrough) = 0;

	// HACK/TODO: This should be moved to the bluetooth platform API to be generic across all endpoints
	virtual stiHResult BluetoothPairedDevicesLog () = 0;
	
	virtual void systemSleep () = 0;
	
	virtual void systemWake () = 0;
	
	virtual void currentTimeSet (
		time_t currentTime) = 0;
#endif

	// TODO: set up some signals/callbacks that would signal platform events
	// that may want to control calls (ie: bluetooth headset answer/hangup button)
	
#if APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_NTOUCH_ANDROID
	/*!
	 * \brief Updates the geolocation of those connected to the geolocationChangedSignal
	 *
	 * \param viable - Whether this geolocation is valid or not.
	 * \param latitude - in degrees
	 * \param longitude - in degrees
	 * \param uncertainty - Horizontal radius of geolocation uncertainty in meters. Set to -1 if unknown.
	 * \param altitudeViable - Whether the following altitude variable is viable or not.
	 * \param altitude - in meters
	 *
	 * \return none
	 */
	virtual void geolocationUpdate (
		bool viable,
		double latitude,
		double longitude,
		int uncertainty,
		bool altitudeViable,
		int altitude) = 0;

	/*!
	 * \brief Informs those connected to the geolocationClearSignal that geolocation services have been
	 *  shut down. It is recommended to clear any local copies of geolocation as it is likely no longer viable.
	 *
	 * \param none
	 *
	 * \return none
	 */
	virtual void geolocationClear () = 0;
#endif
	
	virtual void callStatusAdd (
		std::stringstream &callStatus) = 0;

	virtual void ringStart () = 0;
	
	virtual void ringStop () = 0;
	
public:
	// Event Signals
	virtual ISignal<const std::string&>& geolocationChangedSignalGet () = 0;
	virtual ISignal<>& geolocationClearSignalGet () = 0;
	virtual ISignal<int>& hdmiInStatusChangedSignalGet () = 0;
	virtual ISignal<const PlatformCallStateChange &>& callStateChangedSignalGet () = 0;
	
protected:

	IstiPlatform () = default;
	virtual ~IstiPlatform () = default;

};

#endif // ISTIPLATFORMINTERFACE_H
