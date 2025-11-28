////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Module Name: stiTaskInfo
//
//  File Name:  stiTaskInfo.h
//
//  Abstract:
//		This file will declare values used in creating the Tasks and Message
//		queues.  Add appropriate values to the specific sections so that all
//		similar values will be located together.
//
//  NOTE: this file is included by C files therefore it must always be
//        compatible with C
//
////////////////////////////////////////////////////////////////////////////////
#ifndef STITASKINFO_H
#define STITASKINFO_H

//
// Includes
//

//
// Constants
//

// Task Names
#define stiCM_TASK_NAME		                "stiCMgr"
#define stiSIP_MGR_TASK_NAME		        "stiSIPMgr"
//#define stiBUFFMAN_TASK_NAME                "stiBMgr"
#define stiSRVR_DATA_HNDLR_TASK_NAME		"stiSrvrDataHndlr"
#define stiDATA_TRANSFER_HNDLR_TASK_NAME	"stiDataTransferHndlr"
#define stiFP_TASK_NAME						"stiFP"
#define stiVP_TASK_NAME		                "stiVP"
#define stiVP_READ_TASK_NAME		        "stiVPRead"
#define stiVR_TASK_NAME		                "stiVR"
//#define stiVR_TIMER_TASK_NAME		        "stiVRTimer"
#define stiAP_TASK_NAME		                "stiAP"
#define stiAP_READ_TASK_NAME		        "stiAPRead"
#define stiAR_TASK_NAME		                "stiAR"
//#define stiAR_TIMER_TASK_NAME		        "stiARTimer"
#define stiTP_TASK_NAME		                "stiTP"
#define stiTP_READ_TASK_NAME		        "stiTPRead"
#define stiTR_TASK_NAME		                "stiTR"
//#define stiTR_TIMER_TASK_NAME		        "stiTRTimer"
#define stiCDT_TASK_NAME					"stiCtrlDataTask"
#define stiCCI_TASK_NAME					"stiCCI"
//#define stiUSB_IRQ_TASK_NAME                "tUSBIrq"
//#define stiTCP0SUP_READ_TASK_NAME           "tTCP0SUPRead"
//#define stiUDP0SUP_READ_TASK_NAME           "tUDP0SUPRead"
//#define stiUDP1SUP_READ_TASK_NAME           "tUDP1SUPRead"
//#define stiCOM_TASK_NAME                    "stiCom"
//#define stiDS_TASK_NAME                  	"stiDirSer"
//#define stiSM_TASK_NAME						"stiSysM"
//#define stiADIP_TASK_NAME					"stiADIP"
#define stiVRCL_TASK_NAME					"stiVRCL"
#define stiHTTP_TASK_NAME                   "stiHTTP"
#define stiURL_TASK_NAME                    "stiURL"
#define stiCS_TASK_NAME						"stiCS"
#define stiNS_TASK_NAME				        "stiNS"
#define stiMS_TASK_NAME						"stiMS"
#define stiCNF_TASK_NAME					"stiCNF"
#define stiRLS_TASK_NAME					"stiRLS"
#define stiUPDATE_TASK_NAME                 "stiUpdate"
#define stiUPDATE_WRITER_TASK_NAME          "stiUpdateWriter"
//#define stiOHV_TASK_NAME					"stiOHV"
//#define stiRTR_TASK_NAME					"stiRTR"
#define stiSTUN_TASK_NAME					"stiStun"
#define stiNETWORK_TASK_NAME				"stiNET"
#define stiDHCP_TASK_NAME					"stiDHCP"
#define stiPS_TASK_NAME						"stiPS"
#define stiTUN_TASK_NAME					"stiTunnelMgr"
#define stiAPPLOADER_TASK_NAME				"stiAppLoader"
#define stiFILEDOWNLOAD_TASK_NAME           "stiFileDownload"
#define stiHold_Server						"stiHS"

// Task priorities
#define stiGENERIC_TASK_PRIORITY			251

// Needs to be a little lower priority than the Data tasks
#define stiCM_TASK_PRIORITY                 200
#define stiSIP_MGR_TASK_PRIORITY            202
#define stiSRVR_DATA_HNDLR_TASK_PRIORITY	151
#define stiFP_TASK_PRIORITY					150
#define stiVP_TASK_PRIORITY                 150
#define stiVP_READ_TASK_PRIORITY            151
#define stiVR_TASK_PRIORITY                 149
#define stiVR_TIMER_TASK_PRIORITY           148
#define stiOHV_TASK_PRIORITY				147
#define stiAP_TASK_PRIORITY                 71
#define stiAP_READ_TASK_PRIORITY            72
#define stiAR_TASK_PRIORITY                 70
#define stiAR_TIMER_TASK_PRIORITY           69
#define stiCDT_TASK_PRIORITY				80
#define stiVRCL_TASK_PRIORITY				90
#define stiCCI_TASK_PRIORITY                150	// Must be higher priority than
												// Conference Manager and the
												// Audio and Video Record and
												// Playback tasks.  This is so
												// The application is notified
												// of changes without delay.
//#define stiPD_TASK_PRIORITY					149	// This priority value is
												// defined in the file
												// CstiPersistentData.cpp. If it
												// needs to be changed, change
												// it there and update the value
												// here.
#define stiUSB_IRQ_TASK_PRIORITY            5
#define stiSUP_READ_TASK_PRIORITY           20
#define stiCOM_TASK_PRIORITY                250
#define stiDS_TASK_PRIORITY					252	// Lower priority than auto
												// detect IP to give every
												// chance to detect the IP
												// before attempting to sign-on
												// to the directory server.
#define stiSM_TASK_PRIORITY					253
#define stiADIP_TASK_PRIORITY				251
#define stiHTTP_TASK_PRIORITY				251
#define stiURL_TASK_PRIORITY				251
#define stiUPDATE_TASK_PRIORITY             251
#define stiUPDATE_WRITER_TASK_PRIORITY      251
#define stiRTR_TASK_PRIORITY				251
#define stiSTUN_TASK_PRIORITY				251
#define stiNETWORK_TASK_PRIORITY			251
#define stiDHCP_TASK_PRIORITY				251
#define stiPS_TASK_PRIORITY					251
#define stiTUN_TASK_PRIORITY				251
#define stiDNS_TASK_PRIORITY				150
#define stiFILEDOWNLOAD_TASK_PRIORITY       251


// Task stack sizes
#define stiGENERIC_STACK_SIZE				8192
#define stiDNS_STACK_SIZE					2048
#define stiCM_STACK_SIZE		            4096
#define stiSIP_MGR_STACK_SIZE		        16384
#define stiBUFFMAN_STACK_SIZE               2048
#define stiSRVR_DATA_HNDLR_STACK_SIZE		8192
#define stiFP_STACK_SIZE					8192
#define stiVP_STACK_SIZE                    8192
#define stiVP_READ_STACK_SIZE               8192
#define stiVR_STACK_SIZE                    8192
#define stiVR_TIMER_STACK_SIZE              8192
#define stiOHV_STACK_SIZE					8192
#define stiAP_STACK_SIZE                    8192
#define stiAP_READ_STACK_SIZE               8192
#define stiAR_STACK_SIZE                    8192
#define stiAR_TIMER_STACK_SIZE              8192
#define stiCDT_STACK_SIZE					8192
#define stiCCI_STACK_SIZE                   4096
#define stiUSB_IRQ_STACK_SIZE               5000
#define stiSUP_READ_STACK_SIZE              5000
#define stiCOM_STACK_SIZE                   2048
#define stiDS_STACK_SIZE                	16384
#define stiSM_STACK_SIZE					2048
#define stiADIP_STACK_SIZE					8192
#define stiVRCL_STACK_SIZE					16384
#define stiHTTP_STACK_SIZE                  8192
#define stiURL_STACK_SIZE					8192
#define stiUPDATE_STACK_SIZE                8192
#define stiUPDATE_WRITER_STACK_SIZE         8192
#define stiRTR_STACK_SIZE					2048
#define stiSTUN_STACK_SIZE					2048
#define stiNETWORK_STACK_SIZE				2048
#define stiDHCP_STACK_SIZE					2048
#define stiPS_STACK_SIZE					2048
#define stiTUN_STACK_SIZE					2048
#define stiFILEDOWNLOAD_STACK_SIZE          8192

// Maximum messages in message queues
#define stiGENERIC_MAX_MESSAGES_IN_QUEUE	32

// Hold Server needs Message Queue size * 200
#if defined(stiHOLDSERVER) || defined(stiMEDIASERVER)
	#define stiCM_MAX_MESSAGES_IN_QUEUE		    1600
	#define stiSIP_MGR_MAX_MESSAGES_IN_QUEUE	3200
#else
	#define stiCM_MAX_MESSAGES_IN_QUEUE		    8
	#define stiSIP_MGR_MAX_MESSAGES_IN_QUEUE	16
#endif

#define stiBUFFMAN_MAX_MESSAGES_IN_QUEUE    6
#define stiSRVR_DATA_HNDLR_MAX_MESSAGES_IN_QUEUE	12
#define stiFP_MAX_MESSAGES_IN_QUEUE			20
#define stiVP_MAX_MESSAGES_IN_QUEUE         24
#define stiVP_READ_MAX_MESSAGES_IN_QUEUE	6
#define stiVR_MAX_MESSAGES_IN_QUEUE         24
#define stiVR_TIMER_MAX_MESSAGES_IN_QUEUE   6
#define stiOHV_MAX_MESSAGES_IN_QUEUE		6
#define stiAP_MAX_MESSAGES_IN_QUEUE         12
#define stiAP_READ_MAX_MESSAGES_IN_QUEUE    6
#define stiAR_MAX_MESSAGES_IN_QUEUE         6
#define stiAR_TIMER_MAX_MESSAGES_IN_QUEUE   6
#define stiCDT_MAX_MESSAGES_IN_QUEUE		32
#define stiCCI_MAX_MESSAGES_IN_QUEUE        32
#define stiCOM_MAX_MESSAGES_IN_QUEUE        1
#define stiDS_MAX_MESSAGES_IN_QUEUE			25
#define stiSM_MAX_MESSAGES_IN_QUEUE			25
#define stiADIP_MAX_MESSAGES_IN_QUEUE		6
#define stiVRCL_MAX_MESSAGES_IN_QUEUE		6
#define stiHTTP_MAX_MESSAGES_IN_QUEUE       6
#define stiURL_MAX_MESSAGES_IN_QUEUE       6
#define stiUPDATE_MAX_MESSAGES_IN_QUEUE     6
#define stiUPDATE_WRITER_MAX_MESSAGES_IN_QUEUE 6
#define stiRTR_MAX_MESSAGES_IN_QUEUE		20
#define stiSTUN_MAX_MESSAGES_IN_QUEUE		20
#define stiASYNC_MAX_MESSAGES_IN_QUEUE 		4
#define stiNETWORK_MAX_MESSAGES_IN_QUEUE	20
#define stiDHCP_MAX_MESSAGES_IN_QUEUE		20
#define stiPS_MAX_MESSAGES_IN_QUEUE			20
#define stiTUN_MAX_MESSAGES_IN_QUEUE		20
#define stiFILEDOWNLOAD_MAX_MESSAGES_IN_QUEUE 6

// Maximum message size to be placed in queues
// Need to declare Max Message Size to be the size of the largest event
//		class made (or message being sent).
#define stiGENERIC_MAX_MSG_SIZE				512
#define stiCM_MAX_MESSAGE_SIZE	            512
#define stiSIP_MGR_MAX_MESSAGE_SIZE	        512
#define stiBUFFMAN_MAX_MSG_SIZE             512
#define stiFP_MAX_MSG_SIZE                  512
#define stiSRVR_DATA_HNDLR_MAX_MSG_SIZE		512
#define stiVP_MAX_MSG_SIZE                  512
#define stiVP_READ_MAX_MSG_SIZE             512
#define stiVR_MAX_MSG_SIZE                  512
#define stiVR_TIMER_MAX_MSG_SIZE            512
#define stiOHV_MAX_MSG_SIZE					512
#define stiAP_MAX_MSG_SIZE                  512
#define stiAP_READ_MAX_MSG_SIZE             512
#define stiAR_MAX_MSG_SIZE                  512
#define stiAR_TIMER_MAX_MSG_SIZE            512
#define stiCDT_MAX_MSG_SIZE					512
#define stiCCI_MAX_MSG_SIZE                 512	// Must equal or exceed value of
												// n32stiMAX_CCI_MSG_LENGTH -
												// defined in stiCCIMsgs.h
#define stiCOM_MAX_MSG_SIZE                 16
#define stiDS_MAX_MSG_SIZE 	                512
#define stiSM_MAX_MSG_SIZE					16
#define stiADIP_MAX_MSG_SIZE				16
#define stiVRCL_MAX_MSG_SIZE				512
#define stiHTTP_MAX_MSG_SIZE                512
#define stiURL_MAX_MSG_SIZE					512
#define stiCS_MAX_MSG_SIZE					512
#define stiNS_MAX_MSG_SIZE					512
#define stiMS_MAX_MSG_SIZE					512
#define stiRLS_MAX_MSG_SIZE					512
#define stiUPDATE_MAX_MSG_SIZE              512
#define stiUPDATE_WRITER_MAX_MSG_SIZE       512
#define stiRTR_MAX_MSG_SIZE					512
#define stiSTUN_MAX_MSG_SIZE				512
#define stiASYNC_MAX_MSG_SIZE 				16
#define stiNETWORK_MAX_MSG_SIZE				512
#define stiDHCP_MAX_MSG_SIZE				512
#define stiPS_MAX_MESSAGE_SIZE				512
#define stiTUN_MAX_MESSAGE_SIZE				512
#define stiFILEDOWNLOAD_MAX_MSG_SIZE       512

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//

//
// Function Declarations
//

#endif // STITASKINFO_H
// end file stiTaskInfo.h
