/*!
* \file CstiDHCP.h
* \brief Performs DHCP specific functionality.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*/
#ifndef CSTIDHCP_H
#define CSTIDHCP_H

#include "stiDefs.h"
#include "CstiOsTaskMQ.h"
#include "stiError.h"
#include <condition_variable>

class CstiDHCP : public CstiOsTaskMQ
{

public:
	CstiDHCP (
		PstiObjectCallback pfnAppCallback,
		size_t CallbackParam);

	CstiDHCP (const CstiDHCP &other) = delete;
	CstiDHCP (CstiDHCP &&other) = delete;
	CstiDHCP &operator= (const CstiDHCP &other) = delete;
	CstiDHCP &operator= (CstiDHCP &&other) = delete;

	~CstiDHCP () override;
	

	stiHResult Initialize (const char *);
	void Acquire (const char *);
	int Release ();

	enum
	{
		estiMSG_CB_NONE = 0, ///\Note: zero is reserved to indicate no message and will not be sent to the application.
		estiMSG_CB_DHCP_DECONFIG_STARTED,
		estiMSG_CB_DHCP_DECONFIG_COMPLETED,
		estiMSG_CB_DHCP_IPCONFIG_STARTED,
		estiMSG_CB_DHCP_IPCONFIG_COMPLETED
	};
	
private:
	
	int Task () override;
	
	bool IsRunning ();
	stiHResult Start (const char *);

	int m_nPipe;
	
	int m_nPID;

	char * m_pszCurrentNetDevice;

	std::mutex m_conditionMutex;
	std::condition_variable m_releaseCond;

	void ClientPipeRead (
		int nPipe);
};
#endif // #ifndef CSTIDHCP_H
