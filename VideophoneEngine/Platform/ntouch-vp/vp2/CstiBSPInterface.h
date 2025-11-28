// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef CSTIBSPINTERFACE_H
#define CSTIBSPINTERFACE_H

#include "CstiBSPInterfaceBase.h"
#include "stiError.h"
#include "IstiPlatform.h"
#include "CstiSignal.h"
#include "CFilePlay.h"
#include "CstiMatterStubs.h"
#include <gio/gio.h>


class CstiISPMonitor;


class CstiBSPInterface : public CstiBSPInterfaceBase
{
public:

	CstiBSPInterface ();

	CstiBSPInterface (const CstiBSPInterface &other) = delete;
	CstiBSPInterface (CstiBSPInterface &&other) = delete;
	CstiBSPInterface &operator= (const CstiBSPInterface &other) = delete;
	CstiBSPInterface &operator= (CstiBSPInterface &&other) = delete;
	
	~CstiBSPInterface() = default;

	stiHResult Initialize () override;
	
	stiHResult Uninitialize () override;
	
	stiHResult HdmiInStatusGet (
		int *pnHdmiInStatus) override;

	stiHResult HdmiInResolutionGet (
		int *pnHdmiInResolution) override;

	stiHResult HdmiPassthroughSet (
		bool bHdmiPassthrough) override;

	stiHResult statusLEDSetup () override;

	stiHResult audioInputSetup () override;

	stiHResult lightRingSetup () override;

	stiHResult userInputSetup () override;

	stiHResult cpuSpeedSet (
		EstiCpuSpeed eCpuSpeed) override;

	static CFilePlay *m_filePlay;
	static CstiMatterStubs *m_matter;

	EstiRestartReason hardwareReasonGet () override;
	
	void currentTimeSet (
		const time_t currentTime) override;

private:

	stiHResult serialNumberGet (
		std::stringstream &callStatus,
		const SstiAdvancedStatus &advancedStatus) const override;

	static CstiISPMonitor *m_pISPMonitor;
};


#endif // CSTIBSPINTERFACE_H
