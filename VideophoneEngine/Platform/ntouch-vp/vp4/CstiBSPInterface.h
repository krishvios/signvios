// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved

#pragma once

#include "CstiBSPInterfaceBase.h"
#include "SocketListenerBase.h"
#include "CFilePlayGS.h"
#include "AudioTest.h"
//#include "CstiMatter.h"
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
	// EstiRestartReason hardwareReasonGet () override;

	stiHResult cpuSpeedSet (
		EstiCpuSpeed eCpuSpeed) override;
		
	void rebootSystem () override;

	static CFilePlayGS *m_filePlay;
	//static CstiMatter *m_matter;
	static CstiMatterStubs *m_matter;

	// BIOS interface functions
	std::string serialNumberGet () const override;
	
	stiHResult serialNumberSet (
	 	const std::string &serialNumber) override;
		
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
	
	std::unique_ptr<SocketListenerBase> m_socketListener {};

	mutable std::mutex m_mutex;
	std::string m_serialNumber {};
};
