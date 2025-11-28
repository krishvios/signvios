// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "stiError.h"
#include "CstiBSPInterfaceBase.h"
#include "CFilePlayGS.h"
#include "SocketListenerBase.h"
#include "AudioTest.h"
#include "CstiMatterStubs.h"

class CFilePlayGS;

class CstiBSPInterface : public CstiBSPInterfaceBase
{
public:

	CstiBSPInterface () = default;
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

	stiHResult userInputSetup() override;

	stiHResult cpuSpeedSet (
		EstiCpuSpeed eCpuSpeed) override;
		
	void rebootSystem () override;
	
	static std::shared_ptr<AudioTest> m_audioTest;

	static CFilePlayGS *m_filePlay;
	static CstiMatterStubs *m_matter;

private:
	
	std::unique_ptr<SocketListenerBase> m_socketListener {};
};
