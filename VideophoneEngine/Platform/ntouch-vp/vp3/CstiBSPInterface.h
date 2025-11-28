// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved

#pragma once

#include "CstiBSPInterfaceBase.h"
#include "SocketListenerBase.h"
#include "CFilePlayGS.h"
#include "ToFSensor.h"
#include "AudioTest.h"
#include "CstiMatterStubs.h"
#include <memory>
#include <boost/optional.hpp>

class CstiBSPInterface : public CstiBSPInterfaceBase
{
public:

	CstiBSPInterface ();
	~CstiBSPInterface() override = default;

	CstiBSPInterface (const CstiBSPInterface &other) = delete;
	CstiBSPInterface (CstiBSPInterface &&other) = delete;
	CstiBSPInterface &operator= (const CstiBSPInterface &other) = delete;
	CstiBSPInterface &operator= (CstiBSPInterface &&other) = delete;

	stiHResult Initialize () override;
	stiHResult Uninitialize () override;
	
	stiHResult statusLEDSetup () override;
	stiHResult audioInputSetup () override;
	stiHResult lightRingSetup () override;
	stiHResult userInputSetup () override;
	stiHResult audioHandlerSetup() override;
	EstiRestartReason hardwareReasonGet () override;

	void tofDistanceGet (const std::function<void(int)> &onSuccess) override;

	stiHResult cpuSpeedSet (
		EstiCpuSpeed eCpuSpeed) override;
		
	void rebootSystem () override;

	static CFilePlayGS *m_filePlay;
	static CstiMatterStubs *m_matter;

	// BIOS interface functions
	std::string serialNumberGet () const override;
	
	stiHResult serialNumberSet (
		const std::string &serialNumber) override;
		
	std::tuple<int, boost::optional<int>> hdccGet () const override;
	
	std::tuple<stiHResult, stiHResult> hdccSet (
		boost::optional<int> hdcc,
		boost::optional<int> hdccOverride) override;
		
	stiHResult hdccSet (
		int hdcc);
	
	stiHResult hdccOverrideSet (
		int hdccOverride);
	
	void hdccOverrideDelete () override;

	void systemSleep () override;

	void systemWake () override;
	
	void currentTimeSet (
		const time_t currentTime) override;

	static std::shared_ptr<AudioTest> m_audioTest;
	
private:
	
	stiHResult serialNumberGet (
		std::stringstream &callStatus,
		const SstiAdvancedStatus &advancedStatus) const override;
		
	void serialNumberRead ();
	
	void hdccRead ();
	
	std::unique_ptr<SocketListenerBase> m_socketListener {};
	std::unique_ptr<ToFSensor> m_tofSensor {};

	mutable std::mutex m_mutex;
	std::string m_serialNumber {};
	int m_hdcc {0};
};
