/*!
 * \file stiDefs.h
 * \brief STI standard definitions header
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef STIDEFS_H
#define STIDEFS_H

/*
 * Includes
 */
#include <cinttypes>

/*
 * Constants
 */
#ifdef HOLDSERVER_DLL
#define HOLDSERVER_EXPORT __declspec(dllexport)
#else
#define HOLDSERVER_EXPORT __declspec(dllimport)
#endif


/*
 * Typedefs
 */
enum ETriState
{
	TRI_FALSE,
	TRI_TRUE,
	TRI_UNKNOWN
};

#ifdef stiHOLDSERVER
enum EstiMediaServiceSupportStates
{
	CALL_IS_SERVICEABLE,
	USER_UNREGISTERED,
	N11_CALL_NOT_SERVICEABLE,
	DUAL_RELAY_REJECT
};
#endif // stiHOLDSERVER

enum stiHResult
{
	stiRESULT_SUCCESS							= 0,
	stiRESULT_PM_FAILURE						= 1,
	stiRESULT_UNABLE_TO_DIAL_SELF				= 4,
	stiRESULT_INVALID_CALL_OBJECT_STATE			= 9,
	stiRESULT_MEMORY_ALLOC_ERROR				= 10,
	stiRESULT_DRIVER_ERROR						= 11,
	stiRESULT_MP4_LIB_ERROR						= 12,
	stiRESULT_INVALID_CODEC						= 13,
	stiRESULT_QUEUE_CREATE_FAILED				= 14,
	stiRESULT_DEVICE_OPEN_FAILED				= 15,
	stiRESULT_NO_VIDEO_DRIVER					= 16,
	stiRESULT_TASK_INIT_FAILED					= 17,
	stiRESULT_MEDIA_SAMPLE_GET_FAILED			= 18,
	stiRESULT_PACKET_QUEUE_ERROR				= 19,
	stiRESULT_INVALID_MEDIA						= 20,
	stiRESULT_DRIVER_STOP_ERROR					= 21,
	stiRESULT_UNSUPPORTED_DIAL_TYPE				= 22,
	stiRESULT_UNABLE_TO_GET_HOST_IP				= 23,
	stiRESULT_WINDOW_CREATE_ERROR				= 24,
	stiRESULT_TASK_ALREADY_STARTED				= 25,
	stiRESULT_TASK_STARTUP_FAILED				= 26,
	stiRESULT_TASK_SHUTDOWN_HANDLE_FAILED		= 27,
	stiRESULT_TASK_SPAWN_FAILED					= 28,
	stiRESULT_TASK_ERROR_LOG_INIT_FAILED		= 46,
	stiRESULT_TASK_HTTP_NO_RESOLVE_TASK			= 47,
	stiRESULT_TASK_HTTP_NO_TIMER				= 48,
	stiRESULT_TASK_HTTP_WATCH_DOG_START_FAILED	= 49,
	stiRESULT_CONFERENCE_ENGINE_ALREADY_STARTED	= 50,
	stiRESULT_CONFERENCE_MANAGER_INIT_FAILED	= 51,
	stiRESULT_CONFERENCE_SHUTDOWN_FAILED		= 52,
	stiRESULT_HTTP_INIT_FAILED					= 53,
	stiRESULT_CORE_SERVICE_INIT_FAILED			= 54,
	stiRESULT_STATE_NOTIFY_INIT_FAILED			= 55,
	stiRESULT_MESSAGE_SERVICE_INIT_FAILED		= 56,
	stiRESULT_CORE_SERVICE_SHUTDOWN_FAILED		= 57,
	stiRESULT_STATE_NOTIFY_SHUTDOWN_FAILED		= 58,
	stiRESULT_MESSAGE_SERVICE_SHUTDOWN_FAILED	= 59,
	stiRESULT_HTTP_SHUTDOWN_FAILED				= 60,
	stiRESULT_UPDATE_INIT_FAILED				= 61,
	stiRESULT_UPDATE_SHUTDOWN_FAILED			= 62,
	stiRESULT_TASK_REGISTER_FAILED				= 63,
	stiRESULT_BUFFER_TOO_SMALL					= 64,
	stiRESULT_DNS_COULD_NOT_RESOLVE				= 65,
	stiRESULT_UNABLE_TO_GET_LOCAL_IP			= 66,
	stiRESULT_UNABLE_TO_OPEN_FILE				= 67,
	stiRESULT_VM_CIRCULAR_BUFFER_ERROR			= 68,
	stiRESULT_SRVR_DATA_HNDLR_DOWNLOAD_ERROR	= 69,
	stiRESULT_SRVR_DATA_HNDLR_CRITICAL_ERROR	= 70,
	stiRESULT_SRVR_DATA_HNDLR_MSG_NOT_AVAIL 	= 71,
	stiRESULT_SRVR_DATA_HNDLR_BAD_GUID			= 72,
	stiRESULT_CORE_SERVICE_NOT_CONNECTED		= 73,
	stiRESULT_CORE_SERVICE_ERROR_NOT_HANDLE		= 74,
	stiRESULT_INVALID_PARAMETER					= 75,
	stiRESULT_PRIORITY_TOO_LOW					= 76,
	stiRESULT_REQUEST_SEND_FAILED				= 77,
	stiRESULT_CORE_REQUEST_ERROR				= 78,
	stiRESULT_ERROR								= 79,
	stiRESULT_SERVER_BUSY						= 80,
	stiRESULT_PM_PROPERTY_NOT_FOUND				= 81,
	stiRESULT_WIRELESS_AVAILABLE				= 82,
	stiRESULT_WIRELESS_NOT_AVAILABLE			= 83,
	stiRESULT_WIRELESS_CONNECTION_SUCCESS		= 84,
	stiRESULT_WIRELESS_CONNECTION_FAILURE		= 85,
	stiRESULT_SRVR_DATA_HNDLR_UPLOAD_ERROR		= 86,
	stiRESULT_WIRED_CONNECTION_FAILURE			= 87,
	stiRESULT_SRVR_DATA_HNDLR_READ_ERROR		= 88,
	stiRESULT_SRVR_DATA_HNDLR_EXPIRED_GUID		= 89,
	stiRESULT_ALREADY_SET						= 90,
	stiRESULT_NOT_A_PHONE_NUMBER				= 91,
	stiRESULT_TIME_EXPIRED						= 92,
	stiRESULT_BLOCKED_NUMBER_EXISTS				= 93,
	stiRESULT_BLOCK_LIST_FULL					= 94,
	stiRESULT_VALUE_NOT_FOUND					= 95,
	stiRESULT_SVRS_CALLS_NOT_ALLOWED			= 96,
	stiRESULT_UNABLE_TO_TRANSFER_TO_911 		= 97,
	stiRESULT_INVALID_RANGE_REQUEST				= 98,
	stiRESULT_DUPLICATE							= 99,
	stiRESULT_PORT_IN_USE						= 100,
	stiRESULT_PACKET_SEND_FAIL_CONTINUE			= 101,
	stiRESULT_OFFLINE_ACTION_NOT_ALLOWED		= 102,
	stiRESULT_DFU_IN_PROGRESS					= 103,
	stiRESULT_CONNECTION_IN_PROGRESS			= 104,
	stiRESULT_SECURITY_INADEQUATE				= 105,
	stiRESULT_LAST = 0xFFFFFFFF
};

#define stiERROR_FLAG				0x80000000
#define stiERROR_LOGGED_FLAG		0x40000000
#define stiRESULT_CODE_MASK			0x3FFFFFFF

#define stiRESULT_CODE(E) ((stiHResult)((E) & stiRESULT_CODE_MASK))
#define stiIS_ERROR(E)	(((E) & stiERROR_FLAG) != 0)

#define stiMAKE_ERROR(E) ((stiHResult)((E) | stiERROR_FLAG | stiERROR_LOGGED_FLAG))

#define stiTHROW(E) { hResult = stiMAKE_ERROR(E); stiLogError(E, __FILE__, __LINE__, estiLOG_HRESULT); goto STI_BAIL; }
#define stiTHROWMSG(E,format,...) { hResult = stiMAKE_ERROR(E); stiLogErrorMsg(E, __FILE__, __LINE__, estiLOG_HRESULT, format, ##__VA_ARGS__); goto STI_BAIL; }

#define stiTHROW_NOLOG(E) { hResult = stiMAKE_ERROR(E); goto STI_BAIL; }

#define stiTESTCOND(cond, E) if (!(cond)) { hResult = stiMAKE_ERROR(E); stiLogError(E, __FILE__, __LINE__, estiLOG_HRESULT); goto STI_BAIL; }
#define stiTESTCONDMSG(cond,E,format,...) if (!(cond)) { hResult = stiMAKE_ERROR(E); stiLogErrorMsg(E, __FILE__, __LINE__, estiLOG_HRESULT, format, ##__VA_ARGS__); goto STI_BAIL; }

#define stiTESTCOND_NOLOG(cond, E) if (!(cond)) { hResult = stiMAKE_ERROR(E); goto STI_BAIL; }

#define stiTESTRESULT() if (stiIS_ERROR(hResult)) { if ((hResult & stiERROR_LOGGED_FLAG) == 0) { stiLogError(hResult, __FILE__, __LINE__, estiLOG_HRESULT); hResult = (stiHResult)(hResult | stiERROR_LOGGED_FLAG); } goto STI_BAIL; }
#define stiTESTRESULTMSG(format, ...) if (stiIS_ERROR(hResult)) { if ((hResult & stiERROR_LOGGED_FLAG) == 0) { stiLogErrorMsg(hResult, __FILE__, __LINE__, estiLOG_HRESULT, format, ##__VA_ARGS__); hResult = (stiHResult)(hResult | stiERROR_LOGGED_FLAG); } goto STI_BAIL; }
#define stiTESTRESULT_NOLOG() if (stiIS_ERROR(hResult)) { hResult = (stiHResult)(hResult | stiERROR_LOGGED_FLAG); goto STI_BAIL; }
#define stiTESTRESULT_FORCELOG() if (stiIS_ERROR(hResult)) { stiLogError(hResult, __FILE__, __LINE__, estiLOG_HRESULT); hResult = (stiHResult)(hResult | stiERROR_LOGGED_FLAG); goto STI_BAIL; }
#define stiTESTRESULTMSG_FORCELOG(format, ...) if (stiIS_ERROR(hResult)) { stiLogErrorMsg(hResult, __FILE__, __LINE__, estiLOG_HRESULT, format, ##__VA_ARGS__); hResult = (stiHResult)(hResult | stiERROR_LOGGED_FLAG); goto STI_BAIL; }

#define stiASSERTRESULT(hr) stiASSERT(!stiIS_ERROR(hr))
#define stiASSERTRESULTMSG(hr, format, ...) stiASSERTMSG(!stiIS_ERROR(hr), format, ##__VA_ARGS__)

#define stiRESULT_CONVERT(e) ((e) == estiOK ? stiRESULT_SUCCESS : (stiHResult)(stiRESULT_ERROR | stiERROR_FLAG))

#define stiTESTRVSTATUS()\
if (RV_OK != rvStatus)\
{\
	stiTHROWMSG (stiRESULT_ERROR, "rvStatus = %d", rvStatus);\
}

#define stiTESTRVSTATUS_NOLOG()\
if (RV_OK != rvStatus)\
{\
	stiTrace ("rvStatus = %d\n", rvStatus);\
	stiTHROW_NOLOG (stiRESULT_ERROR);\
}

/*!\brief Simple macro to print the line number within a file for use with "printf debugging"
 *
 */
#define stiHERE() {stiTrace ("At %s:%d\n", __FILE__, __LINE__);}

/*! \brief This enumeration is used to determine the success or failure of many 
 * of the functions within the Sorenson development environment.
 * 
 * \deprecated This return type has been deprecated.  Use stiHResult instead.
 */
typedef enum EstiResult
{
	estiOK	= stiRESULT_SUCCESS,                /*!< Success */
	estiERROR = (stiERROR_FLAG | stiRESULT_ERROR),     /*!< Failure */
	estiTIME_OUT_EXPIRED = (stiERROR_FLAG | stiRESULT_TIME_EXPIRED)   /*!< A time out has occurred. */
} EstiResult;


/*! \brief This enumeration defines the boolean variable used by the Sorenson
 * conferencing engine and many of the other tasks.
 */
typedef enum EstiBool
{
	estiTRUE  = 1,  /*!< Boolean value "True". */
	estiFALSE = 0   /*!< Boolean value "False". */
} EstiBool;

/*! \brief This enumeration is used to identify with variables that are "On" or
 * "Off".  For example a feature such as "Auto Answer" is stored as either "On"
 * or "Off".
 */
typedef enum EstiSwitch
{
	/*! On */
	estiON  = 1, 

	/*! Off */
	estiOFF = 0  
} EstiSwitch;

#define stiMAX_INT32 (int32_t)(((uint32_t)~0) >> 1)
#define stiMAX_UINT32 ~0

#define IN
#define OUT

typedef int STATUS;

#if !defined(stiERROR)
#define stiERROR (-1)
#endif // !defined(stiERROR)

#define BOOT_ADDR_LEN   (64)
#define BOOT_HOST_LEN   (64)

#if defined(stiLINUX)
#include <unistd.h>
#include <cstring>
//#include <c++/4.3/cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#ifndef __APPLE__
#include <linux/sockios.h>
#endif
#define errnoGet() errno

#endif // defined(stiLINUX)

#define stiUNUSED_ARG(arg) (void)(arg)

#if defined(WIN32)
/*
 * NOTE: These were pulled from the old pthreads-win32
 * pthread.h (v1.x.x) since they were removed in v2.x.x
 * 
 * WIN32 C runtime library had been made thread-safe
 * without affecting the user interface. Provide
 * mappings from the UNIX thread-safe versions to
 * the standard C runtime library calls.
 * Only provide function mappings for functions that
 * actually exist on WIN32.
 */
#define strtok_r( _s, _sep, _lasts ) \
        ( *(_lasts) = strtok( (_s), (_sep) ) )

#define asctime_r( _tm, _buf ) \
        ( strcpy( (_buf), asctime( (_tm) ) ), \
          (_buf) )

#define ctime_r( _clock, _buf ) \
        ( strcpy( (_buf), ctime( (_clock) ) ),  \
          (_buf) )

#define gmtime_r( _clock, _result ) \
        ( *(_result) = *gmtime( (_clock) ), \
          (_result) )

#define localtime_r( _clock, _result ) \
        ( *(_result) = *localtime( (_clock) ), \
          (_result) )

#define rand_r( _seed ) \
        ( _seed == _seed? rand() : rand() )
#endif

/*
 * Forward Declarations
 */

/*
 * Globals
 */

/*
 * Function Declarations
 */

#endif /* STIDEFS_H */
/* end file stiDefs.h  */

