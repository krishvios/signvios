// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2019 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "stiDefs.h"
#include "stiSVX.h"
#include "ISignal.h"

#include <list>

class BluetoothDevice
{
public:

	// Enums based on: https://www.bluetooth.com/specifications/assigned-numbers/baseband

	enum EBluetoothMajorClass
	{
		eMiscellaneous = 0,
		eComputer,
		ePhone,
		eLanNAP,
		eAudioVideo,
		ePeripheral,
		eImaging,
		eWearable,
		eToy,
		eHealth,
		eUncategorizedMajor = 0x1F
	};

	enum EPeripheralMinorClass
	{
		eNotKeyboard = 0,
		eJoystick = 1,
		eGamepad = 2,
		eRemoteControl = 3,
		eSensingDevice = 4,
		eDigitizer = 5,
		eCardReader = 6,
		eKeyboard = 16,
		ePointingDevice = 32,
		eComboDevice = 48
	};

	enum EAudioVideoMinorClass
	{
		eUncategorizedAV = 0,
		eWearableHeadset,
		eHandsFreeDevice,
		eReserved_1,
		eMicrophone,
		eLoudspeaker,
		eHeadphones,
		ePortableAudio,
		eCarAudio,
		eSetTopBox,
		eHiFiAudioDevice,
		eVCR,
		eVideoCamera,
		eCamcorder,
		eVideoMonitor,
		eVideoDisplayAndLoudspeaker,
		eVideoConverencing,
		eReserved_2,
		eGamingToy
	};

	inline bool operator== (const BluetoothDevice &rhs)
	{
		return(objectPath == rhs.objectPath);
	}
	inline bool operator!= (const BluetoothDevice &rhs)
	{
		return(objectPath != rhs.objectPath);
	}

	/*!
	* \brief Returns true if the device supports audio
	*/
	bool isAudio () const
	{
		return isHeadset() || isSpeaker();
	}

	/*!
	* \brief Returns true if the device has headset capability
	*/
	bool isHeadset () const
	{
		return (majorDeviceClass == eAudioVideo && (
				minorDeviceClass == eWearableHeadset ||
				minorDeviceClass == eHandsFreeDevice));
	}

	/*!
	* \brief Returns true if the device has speaker (A2DP) capability
	*/
	bool isSpeaker () const
	{
		return (majorDeviceClass == eAudioVideo && (
				minorDeviceClass == eLoudspeaker));
	}

	static const char * majorString(EBluetoothMajorClass majorClass)
	{
		const char * ret = "UNKNOWN";
		switch(majorClass)
		{
		case eMiscellaneous:
			ret = "Miscellaneous";
			break;
		case eComputer:
			ret = "Computer";
			break;
		case ePhone:
			ret = "Phone";
			break;
		case eLanNAP:
			ret = "LanNAP";
			break;
		case eAudioVideo:
			ret = "AudioVideo";
			break;
		case ePeripheral:
			ret = "Peripheral";
			break;
		case eImaging:
			ret = "Imaging";
			break;
		case eWearable:
			ret = "Wearable";
			break;
		case eToy:
			ret = "Toy";
			break;
		case eHealth:
			ret = "Health";
			break;
		case eUncategorizedMajor:
			break;
		}

		return ret;
	}

	static const char * minorPeripheralString(EPeripheralMinorClass minorAV)
	{
		const char * ret = "UNKNOWN";
		switch(minorAV)
		{
		case eNotKeyboard:
			ret = "NotKeyboard";
			break;
		case eJoystick:
			ret = "Joystick";
			break;
		case eGamepad:
			ret = "Gamepad";
			break;
		case eRemoteControl:
			ret = "RemoteControl";
			break;
		case eSensingDevice:
			ret = "SensingDevice";
			break;
		case eDigitizer:
			ret = "Digitizer";
			break;
		case eCardReader:
			ret = "CardReader";
			break;
		case eKeyboard:
			ret = "Keyboard";
			break;
		case ePointingDevice:
			ret = "PointingDevice";
			break;
		case eComboDevice:
			ret = "ComboDevice";
			break;
		}

		return ret;
	}

	static const char * minorAVString(EAudioVideoMinorClass minorAV)
	{
		const char * ret = "UNKNOWN";
		switch(minorAV)
		{
		case eUncategorizedAV:
			ret = "UncategorizedAV";
			break;
		case eWearableHeadset:
			ret = "WearableHeadset";
			break;
		case eHandsFreeDevice:
			ret = "HandsFreeDevice";
			break;
		case eReserved_1:
			ret = "Reserved_1";
			break;
		case eMicrophone:
			ret = "Microphone";
			break;
		case eLoudspeaker:
			ret = "Loudspeaker";
			break;
		case eHeadphones:
			ret = "Headphones";
			break;
		case ePortableAudio:
			ret = "PortableAudio";
			break;
		case eCarAudio:
			ret = "CarAudio";
			break;
		case eSetTopBox:
			ret = "SetTopBox";
			break;
		case eHiFiAudioDevice:
			ret = "HiFiAudioDevice";
			break;
		case eVCR:
			ret = "VCR";
			break;
		case eVideoCamera:
			ret = "VideoCamera";
			break;
		case eCamcorder:
			ret = "Camcorder";
			break;
		case eVideoMonitor:
			ret = "VideoMonitor";
			break;
		case eVideoDisplayAndLoudspeaker:
			ret = "VideoDisplayAndLoudspeaker";
			break;
		case eVideoConverencing:
			ret = "VideoConverencing";
			break;
		case eReserved_2:
			ret = "Reserved_2";
			break;
		case eGamingToy:
			ret = "GamingToy";
			break;
		}

		return ret;
	}

	static const char * minorString(EBluetoothMajorClass major, int minor)
	{
		const char * ret = "UNKNOWN";

		if(eAudioVideo == major)
		{
			ret = minorAVString(static_cast<EAudioVideoMinorClass>(minor));
		}
		else if(ePeripheral == major)
		{
			ret = minorPeripheralString(static_cast<EPeripheralMinorClass>(minor));
		}

		return ret;
	}

	std::string typeString () const
	{
		return std::string(majorString(majorDeviceClass)) + "/" + minorString(majorDeviceClass, minorDeviceClass);
	}

	std::string name;			// friendly name of device
	std::string address;		// bluetooth hardware address
	std::string icon;			// name for display icon
	bool paired = false;		// is the device already paired
	std::string objectPath;		// the object path to the device
	EBluetoothMajorClass majorDeviceClass = eMiscellaneous;	// the Major type of device
	// Multiple enums represented here.  TODO: how to represent that better?
	int minorDeviceClass = 0;	// the Minor type of device
	bool connected = false;		// is the device currently connected
	bool trusted = false;		// is the a trusted device
	bool blocked = false;		// is this device blocked
	bool legacyPairing = false;	// should we use legacy pairing instead of simple
	bool autoReconnect {false};		// Device is a will auto reconnect when disconnected
	std::list<std::string> uuids;			// uuid list
};


class IstiBluetooth
{
public:

	IstiBluetooth (const IstiBluetooth &other) = delete;
	IstiBluetooth (IstiBluetooth &&other) = delete;
	IstiBluetooth &operator= (const IstiBluetooth &other) = delete;
	IstiBluetooth &operator= (IstiBluetooth &&other) = delete;

	/*!
	 * \brief Obtain the handle to the Bluetooth object.  Through this handle, all other calls will be made.
	 *
	 * \return The handle to the Bluetooth object.
	 */
	static IstiBluetooth *InstanceGet ();

	/*!
	 * \brief enable or disable the bluetooth device
	 */
	virtual stiHResult Enable(bool bEnable) = 0;

	/*!
	 * \brief scan for in range bluetooth devices
	 */
	virtual stiHResult Scan() = 0;

	/*!
	 * \brief cancel an in progres bluetooth scan
	 */
	virtual stiHResult ScanCancel() = 0;

	/*!
	 * \brief pair with an unpaired device
	 */
	virtual stiHResult Pair(const BluetoothDevice&) = 0;

	/*!
	 * \brief cancel an in-progress pairing
	 */
	virtual stiHResult PairCancel() = 0;

	/*!
	 * \brief connect to a newly paired device
	 */
	virtual stiHResult Connect(BluetoothDevice) = 0;

	/*!
	 * \brief remove a paired device
	 */
	virtual stiHResult PairedDeviceRemove(BluetoothDevice) = 0;

	/*!
	 * \brief return a list of devices we are currently paired with
	 */
	virtual stiHResult PairedListGet(std::list<BluetoothDevice> *) = 0;

	/*!
	 * \brief return a list of devices returned from the scan
	 */
	virtual stiHResult ScannedListGet(std::list<BluetoothDevice> *) = 0;

	/*!
	 * \brief set a pin code for pairing
	 */
	virtual stiHResult PincodeSet(const char *pszPinCode) = 0;

	/*!
	 * \brief is the bluetooth device powered?
	 */
	virtual bool Powered() = 0;

	/*!
	 * \brief Remote log list of paired devices on startup
	 *
	 * \return Success or failure result
	 */
	virtual stiHResult PairedDevicesLog () = 0;

	/*!
	 * \brief change the friendly name of a bluetooth device
	 *
	 */
	virtual stiHResult DeviceRename(const BluetoothDevice&, const std::string) = 0;

	/*
	 * \brief disconnect a currently connected device
	 *
	 * \return 
	 */
	virtual stiHResult Disconnect(BluetoothDevice) = 0;

public:
	// NOTE: will probably add arguments to these as needed...
	virtual ISignal<BluetoothDevice> &deviceAddedSignalGet () = 0;
	virtual ISignal<BluetoothDevice> &deviceConnectedSignalGet () = 0;
	virtual ISignal<> &deviceConnectFailedSignalGet () = 0;
	virtual ISignal<BluetoothDevice> &deviceDisconnectedSignalGet () = 0;
	virtual ISignal<> &deviceDisconnectFailedSignalGet () = 0;
	virtual ISignal<> &deviceRemovedSignalGet () = 0;
	virtual ISignal<> &scanCompleteSignalGet () = 0;
	virtual ISignal<> &scanFailedSignalGet () = 0;
	virtual ISignal<> &pairingCompleteSignalGet () = 0;
	virtual ISignal<> &pairingFailedSignalGet () = 0;
	virtual ISignal<> &pairingCanceledSignalGet () = 0;
	virtual ISignal<> &pairingInputTimeoutSignalGet () = 0;
	virtual ISignal<> &pairedMaxCountExceededSignalGet () = 0;
	virtual ISignal<> &unpairCompleteSignalGet () = 0;
	virtual ISignal<> &unpairFailedSignalGet () = 0;
	virtual ISignal<std::string> &displayPincodeSignalGet () = 0;
	virtual ISignal<> &requestPincodeSignalGet () = 0;
	virtual ISignal<> &deviceAliasChangedSignalGet () = 0;

protected:

	IstiBluetooth () = default;
	virtual ~IstiBluetooth () = default;

};
