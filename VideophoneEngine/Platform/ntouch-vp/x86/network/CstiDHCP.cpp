/*!
* \file CstiDHCP.cpp
* \brief Performs DHCP specific functionality.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*/
#include "CstiDHCP.h"

#include <fcntl.h>
#include <csignal>
#include <sys/stat.h>

#include "stiTools.h"
#include "stiTrace.h"
#include "stiTaskInfo.h"
#include <algorithm>
#include "CstiEvent.h"

#ifdef stiMVRS_APP
#include <sys/wait.h>
#endif

//
// Constants
//
static const int nMAX_COMMAND_LENGTH = 256;
static const int nMAX_TASK_SHUTDOWN_WAIT = 30;
static const int nMAX_PIPE_MESSAGE_SIZE = 32;

#define PID_UDHCPC_FILE	"/tmp/pidudhcpc"
#define UDHCPC_PIPE	"/tmp/udhcpcpipe"

static const char g_szDHCP_DECONFIG_STARTED[]= "DHCP_DECONFIG_STARTED";
static const char g_szDHCP_DECONFIG_COMPLETED[] = "DHCP_DECONFIG_COMPLETED";
static const char g_szDHCP_IPCONFIG_STARTED[] = "DHCP_IPCONFIG_STARTED";
static const char g_szDHCP_IPCONFIG_COMPLETED[] = "DHCP_IPCONFIG_COMPLETED";


/*!
* \brief Constructor.
*/
CstiDHCP::CstiDHCP (
		PstiObjectCallback pfnAppCallback,
		size_t CallbackParam)
	:
	CstiOsTaskMQ (
		pfnAppCallback,
		CallbackParam,
		reinterpret_cast<size_t>(this), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
		stiDHCP_MAX_MESSAGES_IN_QUEUE,
		stiDHCP_MAX_MSG_SIZE,
		stiDHCP_TASK_NAME,
		stiDHCP_TASK_PRIORITY,
		stiDHCP_STACK_SIZE),
	m_nPipe (0),
	m_nPID (0),
	m_pszCurrentNetDevice(nullptr)
{
	//
	// Remove the file in case it is there.
	//
	unlink (UDHCPC_PIPE);
	
	//
	// Create the fifo.
	//
	mkfifo (UDHCPC_PIPE, 0600);
}


CstiDHCP::~CstiDHCP ()
{
	if (m_nPipe)
	{
		close (m_nPipe);
		m_nPipe = 0;
	}
	
	unlink (UDHCPC_PIPE);
	
	delete [] m_pszCurrentNetDevice;
}


//
// Initialize sets the network device currently running.
//
stiHResult CstiDHCP::Initialize(const char * pszCurrDevice)
{ 
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_pszCurrentNetDevice && strcmp(m_pszCurrentNetDevice,pszCurrDevice) != 0)
	{
		delete [] m_pszCurrentNetDevice;
		m_pszCurrentNetDevice = nullptr;

		m_pszCurrentNetDevice = new char[strlen(pszCurrDevice) + 1];
		stiTESTCOND (m_pszCurrentNetDevice, stiRESULT_ERROR);

		strcpy(m_pszCurrentNetDevice,pszCurrDevice);
	}
	else if (!m_pszCurrentNetDevice)
	{
		m_pszCurrentNetDevice = new char[strlen(pszCurrDevice) + 1];
		stiTESTCOND (m_pszCurrentNetDevice, stiRESULT_ERROR);

		strcpy(m_pszCurrentNetDevice,pszCurrDevice);
	}

STI_BAIL:

	return hResult;
}


/*!
* \brief Returns the process identifier for the DHCP client
* \retval The process identifier for the DHCP client
*/
static int PIDGet()
{
	int nPID = 0;
	char buffer[16];

	int fd = open(PID_UDHCPC_FILE, O_RDONLY);

	if (-1 != fd)
	{
		int len = stiOSRead(fd, buffer, sizeof(buffer) - 1);
		close(fd);
		buffer[len] = '\0';

		if (len > 0)
		{
			nPID = strtol(buffer, nullptr, 10);
			stiDEBUG_TOOL (g_stiDHCPDebug,
				stiTrace("<CstiDHCP::PIDGet> DHCP client is running, pid is %d\n", nPID);
			);
		}
		else
		{
			stiDEBUG_TOOL (g_stiDHCPDebug,
				stiTrace("<CstiDHCP::PIDGet> DHCP client is running, but ERROR for getting pid !!!\n");
			);
		}
	}
	else
	{
		stiDEBUG_TOOL (g_stiDHCPDebug,
			stiTrace("<CstiDHCP::PIDGet> DHCP client is not running yet\n");
		);
	}

	return nPID;
}


/*!
* \brief Returns whether or not the DHCP client is running.
* \retval true - Running
* \retval false - Not running
*/
bool CstiDHCP::IsRunning()
{
	bool bRunning = false;

	if (0 == m_nPID)
	{
		// Call PIDGet to see if it can find the dhcp process.
		m_nPID = PIDGet ();
	}

	if (0 < m_nPID && 0 == kill (m_nPID, 0))
	{
		bRunning = true;
	}

	stiDEBUG_TOOL (g_stiDHCPDebug,
		if (bRunning)
		{
			stiTrace("<CstiDHCP::IsRunning> - DHCP client %d is running\n", m_nPID);
		}
		else
		{
			stiTrace("<CstiDHCP::IsRunning> - DHCP client %d is NOT running\n", m_nPID);
		}
	);

	return bRunning;
}


/*!
* \brief Waits for the DHCP client to release the IP address
* \param nPidDhcp The process identifier for the DHCP client
* \retval 0 for success otherwise for failure
*/
int CstiDHCP::Release()
{
	int nResult = 0;

	// force a release of a current lease,
	// and cause dhcp client to go into the release state
	stiDEBUG_TOOL (g_stiDHCPDebug,
		stiTrace ("<CstiDHCP::Release> Releasing the DHCP client, issuing SIGUSR2\n");
	);

	std::unique_lock<std::mutex> lock (m_conditionMutex);
	
	//
	// This line simulates the DHCP client releasing an IP address.
	//
	int nSysStatus = system ("(sleep 3; echo -n DHCP_DECONFIG_COMPLETED > " UDHCPC_PIPE ") &");  stiUNUSED_ARG (nSysStatus);
	
	m_releaseCond.wait_for(lock, std::chrono::seconds(nMAX_TASK_SHUTDOWN_WAIT));

	
	return nResult;
}


/*!
* \brief Starts the DHCP client
* \retval The process identifier for the DHCP client
*/
stiHResult CstiDHCP::Start (const char * pszDevice)
{
	char command[nMAX_COMMAND_LENGTH];
	stiHResult hResult = stiRESULT_SUCCESS;
	int nSysStatus; stiUNUSED_ARG (nSysStatus);

	stiTESTCOND (m_pszCurrentNetDevice, stiRESULT_ERROR);
	stiTESTCOND (pszDevice, stiRESULT_ERROR);

	if (strcmp(m_pszCurrentNetDevice,pszDevice) != 0)
	{
		if (m_pszCurrentNetDevice)
		{
			delete [] m_pszCurrentNetDevice;
			m_pszCurrentNetDevice = nullptr;
		}

		m_pszCurrentNetDevice = new char[strlen(pszDevice) + 1];
		stiTESTCOND (m_pszCurrentNetDevice, stiRESULT_ERROR);

		strcpy(m_pszCurrentNetDevice,pszDevice);
	}

	sprintf(command, "udhcpc -p " PID_UDHCPC_FILE " -f -i %s &", m_pszCurrentNetDevice);

	// run the udhcp client now
	stiDEBUG_TOOL (g_stiDHCPDebug,
		stiTrace("<CstiDHCP::Start> %s\n", command);
	);

#if DEVICE != DEV_X86
	nSysStatus = system(command);
	nSysStatus = WEXITSTATUS(nSysStatus);
#else
	nSysStatus = system ("(sleep 3; echo -n DHCP_IPCONFIG_COMPLETED > " UDHCPC_PIPE ") &");
#endif

STI_BAIL:

	return hResult;


}


/*!
* \brief Kills the DHCP service directly
* \retval none
*/
void CstiDHCP::Acquire (const char * pszDevice)
{
	if (IsRunning ())
	{
		if (strcmp(pszDevice, m_pszCurrentNetDevice) != 0)
		{
			//
			// DHCP is already running but running for the wrong device
			//
			kill (m_nPID, SIGTERM);
			Start (pszDevice);
		}
		else
		{
			//
			// Since the client is running then tell it to acquire a lease.
			//
			kill (m_nPID, SIGUSR2);
			kill (m_nPID, SIGUSR1);
		}
	}
	else
	{
		//
		// Since it is not running then start.  This will cause it to get a lease.
		//
		Start (pszDevice);
	}
}


///\brief Reads for the pipe that the DHCP client may write to when taking certiain actions.
///
void CstiDHCP::ClientPipeRead (
	int nPipe)					///< The file descriptor to the pipe.
{
	int32_t n32Msg = estiMSG_CB_NONE;
	char szBuffer[nMAX_PIPE_MESSAGE_SIZE];
	unsigned int unOffset = 0;
	
	Lock ();
	
	//
	// Read the pipe until we reach the file end (or the buffer fills).
	//
	for (;;)
	{
		int nLength = stiOSRead (m_nPipe, &szBuffer[unOffset], sizeof (szBuffer) - 1 - unOffset);
		
		//
		// Did we reach the end of the file?
		//
		if (nLength <= 0)
		{
			break;
		}

		unOffset += nLength;
		
		if (unOffset > sizeof (szBuffer) - 1)
		{
			break;
		}
	}
	
	szBuffer[unOffset] = '\0';
	
	//
	// Determine which message we received.
	//
	stiDEBUG_TOOL (g_stiDHCPDebug,
		stiTrace ("Read %d from pipe: %s\n", unOffset, szBuffer);
	);
	
	if (0 == strcmp (szBuffer, g_szDHCP_DECONFIG_STARTED))
	{
		n32Msg = (int32_t)estiMSG_CB_DHCP_DECONFIG_STARTED;
	}
	else if (0 == strcmp (szBuffer, g_szDHCP_DECONFIG_COMPLETED))
	{
		std::unique_lock<std::mutex> lock (m_conditionMutex);

		m_releaseCond.notify_all();
		
		n32Msg = (int32_t)estiMSG_CB_DHCP_DECONFIG_COMPLETED;
	}				
	else if (0 == strcmp (szBuffer, g_szDHCP_IPCONFIG_STARTED))
	{
		n32Msg = (int32_t)estiMSG_CB_DHCP_IPCONFIG_STARTED;
	}				
	else if (0 == strcmp (szBuffer, g_szDHCP_IPCONFIG_COMPLETED))
	{
		n32Msg = (int32_t)estiMSG_CB_DHCP_IPCONFIG_COMPLETED;
	}					
	
	Unlock ();

	if (n32Msg != estiMSG_CB_NONE)
	{
		Callback (n32Msg, 0);
	}
}


int CstiDHCP::Task ()
{
	stiLOG_ENTRY_NAME (CstiNetwork::Task);
	
	//
	// Initialize loop variables
	//
	//EstiResult eResult = estiOK;
	char buffer[stiNETWORK_MAX_MSG_SIZE + 1];
	int nMsgFd = FileDescriptorGet ();
	fd_set SReadFds;
	
	//
	// Task loop
	// As long as we don't get a SHUTDOWN message
	//
	for (;;)
	{	
		//
		// If it is not open the open the pipe.
		//
		if (m_nPipe == 0)
		{
			m_nPipe = open (UDHCPC_PIPE, O_RDONLY | O_NONBLOCK);
		}

		stiFD_ZERO (&SReadFds);

		//
		// select on the message pipe
		//
		stiFD_SET (nMsgFd, &SReadFds);
		stiFD_SET (m_nPipe, &SReadFds);

		//
		// Wait for data to come in
		//
		int nMaxFD = std::max (nMsgFd, m_nPipe) + 1;
		int nSelectRet = stiOSSelect (nMaxFD, &SReadFds, nullptr, nullptr, nullptr);
		
		if (stiERROR != nSelectRet)
		{
			if (stiFD_ISSET (m_nPipe, &SReadFds))
			{
				ClientPipeRead (m_nPipe);

				//
				// Close the pipe.
				//
				close (m_nPipe);
				m_nPipe = 0;
			}
			else
			{
				//
				// Read a message from the message queue
				//
				stiHResult hResult = MsgRecv (buffer, stiFP_MAX_MSG_SIZE);

				if (!stiIS_ERROR (hResult))
				{
					//
					// A message was recieved. Process the message.
					//
					auto  poEvent = reinterpret_cast<CstiEvent*>(buffer); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
					
					//
					// Lookup and handle the event
					//
					
					if (estiEVENT_SHUTDOWN == poEvent->EventGet ())
					{
						ShutdownHandle ();
						
						break;
					}
					
				}// end if (CstiOsTask::eOK == eMsgResult)
			}
		}
	}// end while (bContinue)
	
	//
	// Close the pipe if it is open.
	//
	if (m_nPipe)
	{
		close (m_nPipe);
		m_nPipe = 0;
	}
	
	return (0);
}
