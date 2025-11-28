// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019-2025 Sorenson Communications, Inc. -- All rights reserved

#pragma once

#include "IPlatformVP.h"
#include "stiError.h"
#include "CstiSignal.h"
#include "CstiAudioInput.h"
#include "CstiAudioOutput.h"
#include "CstiBluetooth.h"
#include "CstiStatusLED.h"
#include "CstiLightRing.h"
#include "CstiLogging.h"
#include "CstiUserInput.h"
#include "FanControlBase.h"
#include <gio/gio.h>
#include <gst/gst.h>


class CstiLightRing;
class CstiUserInput;
class CstiAudibleRinger;
class CstiNetwork;
class CstiBluetooth;
class CstiHelios;
class CstiVideoOutput;
class CstiVideoInput;
class CstiAudioHandler;
class CstiAdvancedStatus;
class CstiMonitorTask;
class CstiSLICRinger;
class CstiCECControl;
class CstiUpdate;

class CstiBSPInterfaceBase : public IPlatformVP
{
public:

	CstiBSPInterfaceBase () = default;

	CstiBSPInterfaceBase (const CstiBSPInterfaceBase &other) = delete;
	CstiBSPInterfaceBase (CstiBSPInterfaceBase &&other) = delete;
	CstiBSPInterfaceBase &operator= (const CstiBSPInterfaceBase &other) = delete;
	CstiBSPInterfaceBase &operator= (CstiBSPInterfaceBase &&other) = delete;
	
	~CstiBSPInterfaceBase() override = default;
	
	/*!
	* \brief Allowed CPU speeds
	*/
	enum EstiCpuSpeed
	{
		estiCPU_SPEED_LOW,
		estiCPU_SPEED_MED,
		estiCPU_SPEED_HIGH
	};

	stiHResult Initialize () override;
	
	stiHResult Uninitialize () override;
	
	stiHResult RestartSystem (
		EstiRestartReason eRestartReason) override;
	
	stiHResult RestartReasonGet (
		EstiRestartReason *peRestartReason) override;
	
	std::string mpuSerialNumberGet () override;

	std::string rcuSerialNumberGet () override;
	
	stiHResult HdmiInStatusGet (
		int *pnHdmiInStatus) override;

	stiHResult HdmiInResolutionGet (
		int *pnHdmiInResolution) override;

	stiHResult HdmiPassthroughSet (
		bool bHdmiPassthrough) override;

	static stiHResult MacAddressWrite (
		const char *pszMac);

	stiHResult BluetoothPairedDevicesLog () override;

	void callStatusAdd (
		std::stringstream &callStatus) override;

	virtual stiHResult statusLEDSetup () = 0;
	
	virtual stiHResult audioInputSetup () = 0;

	virtual stiHResult lightRingSetup () = 0;

	virtual stiHResult userInputSetup () = 0;

	virtual stiHResult audioHandlerSetup ();

	virtual stiHResult cpuSpeedSet (
		EstiCpuSpeed eCpuSpeed) = 0;

	void ringStart () override;
  
	void ringStop () override;
	
	bool webServiceIsRunning () override;
	
	void webServiceStart () override;

	void systemSleep () override
	{
		vpe::trace ("sleep/wake is not implemented\n");
	};

	void systemWake () override
	{
		vpe::trace ("sleep/wake is not implemented\n");
	};
	
	// TODO: Intel bug. This is needed because gstreamer/icamerasrc/libcamhal
	// have a problem when we set system time. (libcamhal timeout)
	void currentTimeSet (
		const time_t currentTime) override
	{
		vpe::trace ("currentTimeSet is not implemented\n");
	};

	void tofDistanceGet (
		const std::function<void(int)> &onSuccess) override
	{
		vpe::trace ("tofDistanceGet is not implemented\n");
	};

	// BIOS interface functions
	std::string serialNumberGet () const override {
		vpe::trace ("serialNumberGet is not implemented\n"); return "unavailable";
	};
	
	stiHResult serialNumberSet (
		const std::string &serialNumber) override
	{
		vpe::trace ("serialNumberSet is not implemented\n");
		return stiRESULT_ERROR;
	};
	
	std::tuple<int,boost::optional<int>> hdccGet () const override
	{
		vpe::trace ("hdccGet is not implemented\n");
		return std::make_tuple (0,boost::optional<int>{});
	};
	
	std::tuple<stiHResult, stiHResult> hdccSet (
		boost::optional<int> hdcc, boost::optional<int> hdccOverride) override
	{
		vpe::trace ("hdccSet is not implemented\n");
		return std::make_tuple (stiRESULT_ERROR, stiRESULT_ERROR);
	};
	
	void hdccOverrideDelete () override {
		vpe::trace ("hdccOverrideDelete is not implemented\n");
	};

	static CstiStatusLED *m_pStatusLED;
	static CstiLightRing *m_pLightRing;
	static CstiUserInput *m_pUserInput;
	static CstiAudibleRinger *m_pAudibleRinger;
	static CstiNetwork *m_pNetwork;
	static CstiBluetooth *m_pBluetooth;
	static std::shared_ptr<CstiHelios> m_helios;
	static CstiVideoOutput *m_pVideoOutput;
	static CstiAudioHandler *m_pAudioHandler;
	static CstiAudioInputSharedPtr m_pAudioInput;
	static CstiAudioOutputSharedPtr m_pAudioOutput;
	static CstiAdvancedStatus *m_pAdvancedStatus;
	static CstiMonitorTask *m_pMonitorTask;
	static CstiVideoInput *m_pVideoInput;
	static CstiSLICRinger *m_slicRinger;
	static std::shared_ptr<CstiCECControl> m_cecControl;
	static std::shared_ptr<FanControlBase> m_fanControl;
	static std::shared_ptr<CstiUpdate> m_update;
	static CstiLogging *m_logging;

protected:
	std::string m_cecPort;			// cec port name H2C/GPIO
	std::string m_osdName;			// on screen display name
	
	virtual EstiRestartReason hardwareReasonGet ()
	{
		vpe::trace ("hardwareReasonGet is not implemented\n");
		return estiRESTART_REASON_UNKNOWN;
	};

	
private:
	
	virtual stiHResult serialNumberGet (
		std::stringstream &callStatus,
		const SstiAdvancedStatus &advancedStatus) const
	{
		stiHResult hResult = stiRESULT_SUCCESS;
		vpe::trace ("serialNumberGet (for call status) is not implemented\n");
		return hResult;
	};
	
	stiHResult DisplayTaskStartup ();

	/*!
	 * \brief callback function when the advanced status is ready
	 */
	static stiHResult AdvancedStatusReady (
		int32_t message,
		size_t message_param,
		size_t callback_param,
		size_t callback_from_id);

	stiHResult RestartReasonDetermine ();

	virtual void rebootSystem ();

	stiHResult MainLoopStart ();

	static void * MainLoopTask (
		void * param);

	GMainLoop *m_pMainLoop {nullptr};

	EstiRestartReason m_eRestartReason {estiRESTART_REASON_UNKNOWN};

protected:

	CstiSignalConnection::Collection m_signalConnections;
};
