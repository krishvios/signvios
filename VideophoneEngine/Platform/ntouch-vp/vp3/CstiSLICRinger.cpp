/*!
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
 */
#include "CstiSLICRinger.h"
#include "stiTrace.h"
#include "stiRemoteLogEvent.h"
#include <fstream>

#define LAST_SLIC_DETECT_FILE "lastSlicDetectTime"
#define HOURS_BETWEEN_DETECTS 336	// 14 days

#define MREN_CONNECTED_THRESHOLD 40
#define DEVICE_ID 0x0400
#define EVENT_POLL_MS 20

/************** Device Profile **************/
/* Device Configuration Data - Tracker 135V Inverting Boost */
const VpProfileDataType DEV_PROFILE_Le9653_135V_IB[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0xFF, 0x00,       /* Device Profile for Le9653 device */
   0x33,                   /* Profile length = 51 + 4 = 55 bytes */
   0x08,                   /* Profile format version = 8 */
   0x14,                   /* MPI section length = 20 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0x46, 0x83,             /* PCLK = 2.048 MHz; INTM = Open Drain */
   0x44, 0x00,             /* PCM Clock Slot = 0 TX / 0 RX; XE = Neg. */
   0x5E, 0x3C, 0x04,       /* Device Mode Register */
   0xF6, 0x96, 0x00, 0xB0, /* Switching Regulator Timing Params */
         0x52, 0xFC, 0x52,
   0xE4, 0x04, 0x92, 0x0A, /* Switching Regulator Params */
   0xE6, 0x00,             /* Switching Regulator Control */
/* Formatted Parameters Section -------------------------------------------- */
   0x02,                   /* IO21 Mode: Analog Voltage Sense */
                           /* IO22 Mode: Digital */
   0x04,                   /* Dial pulse correction: 0 ms */
                           /* Switcher Configuration =
                              Inverting Boost (12 V in, 135 V out) */
   0x00, 0x00,             /* Over-current event threshold = Off */
   0x01,                   /* Leading edge blanking window = 81 ns */
   0x65, 0x00, 0x64, 0x52, /* FreeRun Timing Parameters */
         0x64, 0x52,
   0xFF,
   0x97, 0xFF,             /* Low-Power Timing Parameters */
   0x87, 0x87,             /* Switcher Limit Voltages 135 V, 135 V */
   0x04,                   /* Charge pump disabled */
                           /* Charge Pump Overload Protection: Disabled */
   0x3C,                   /* Switcher Input 12 V */
   0x00, 0x00, 0x00, 0x00, /* Battery Backup Timing Parameters */
         0x00, 0x00,
   0x00, 0x00, 0x00,       /* Soft Startup Timing (Disabled) */
   0x00, 0x00              /* SwCtrl for Free Run and Battery Backup */
};

/************** DC Profile **************/
/* DC FXS Default - Use for all countries unless country file exists - 25mA Current Feed */
const VpProfileDataType DC_FXS_miSLIC_IB_DEF[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x01, 0x00,       /* DC Profile for Le9653 device */
   0x0D,                   /* Profile length = 13 + 4 = 17 bytes */
   0x03,                   /* Profile format version = 3 */
   0x03,                   /* MPI section length = 3 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC6, 0x92, 0x27,       /* DC Feed Parameters: ir_overhead = 100 ohm; */
                           /* VOC = 48 V; LI = 50 ohm; VAS = 8.78 V; ILA = 25 mA */
                           /* Maximum Voice Amplitude = 2.93 V */
/* Formatted Parameters Section -------------------------------------------- */
   0x1C,                   /* Switch Hook Threshold = 11 mA */
                           /* Ground-Key Threshold = 18 mA */
   0x84,                   /* Switch Hook Debounce = 8 ms */
                           /* Ground-Key Debounce = 16 ms */
   0x58,                   /* Low-Power Switch Hook Hysteresis = 2 V */
                           /* Ground-Key Hysteresis = 6 mA */
                           /* Switch Hook Hysteresis = 2 mA */
   0x80,                   /* Low-Power Switch Hook Threshold = 22 V */
   0x03,                   /* Floor Voltage = -20 V */
   0x00,                   /* R_OSP = 0 ohms */
   0x07,                   /* R_ISP = 7 ohms */
   0x30                    /* VOC = 48 V */
};

/************** AC Profile **************/
/* AC FXS RF14 600R Normal Coefficients (Default)  */
const VpProfileDataType AC_FXS_RF14_600R_DEF[] =
{
  /* AC Profile */
 0xA4, 0x00, 0xF4, 0x4C, 0x01, 0x49, 0xCA, 0xF5, 0x98, 0xAA, 0x7B, 0xAB,
 0x2C, 0xA3, 0x25, 0xA5, 0x24, 0xB2, 0x3D, 0x9A, 0x2A, 0xAA, 0xA6, 0x9F,
 0x01, 0x8A, 0x1D, 0x01, 0xA3, 0xA0, 0x2E, 0xB2, 0xB2, 0xBA, 0xAC, 0xA2,
 0xA6, 0xCB, 0x3B, 0x45, 0x88, 0x2A, 0x20, 0x3C, 0xBC, 0x4E, 0xA6, 0x2B,
 0xA5, 0x2B, 0x3E, 0xBA, 0x8F, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0x2A, 0x42, 0xA1, 0xCB, 0x1B, 0xA3, 0xA8, 0xFB, 0x87, 0xAA,
 0xFB, 0x9F, 0xA9, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/************** Ringing Profile **************/
/* USA AT&T Ringing 20Hz 62Vrms + 20VDC Tracking, ACTRIP */
const VpProfileDataType RING_MISLIC_IB135V_US[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for Le9653 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x10, 0x10, 0x95, /* Ringing Generator Parameters: */
         0x00, 0x37, 0x48, /* 20.1 Hz Sine; 1.41 CF; 87.70 Vpk; 20.00 V bias */
         0xB4, 0x00, 0x00, /* Ring trip cycles = 2; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0xC2,                   /* Ring Trip Method = AC; Threshold = 33.0 mA */
   0x0D,                   /* Ringing Current Limit = 76 mA */
   0x15,                   /* Tracked; Max Ringing Supply = 110 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/************** Cadence Profile **************/
/* 2 Seconds On, 4 Seconds Off */
const VpProfileDataType HAWKEYE_SLIC_CADENCE[] =
{
    /* Cadence Profile */
   0x00, 0x08, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x8F, 0x01, 0x07,
   0x21, 0x90, 0x01, 0x00, 0x23, 0x20, 0x41, 0x00
};

char *MapStatus (VpStatusType status)
{
    int idx = 0;

    static char buff[50];

    static const char *strTable[VP_STATUS_NUM_TYPES] = {
        "VP_STATUS_SUCCESS",
        "VP_STATUS_FAILURE",
        "VP_STATUS_FUNC_NOT_SUPPORTED",
        "VP_STATUS_INVALID_ARG",
        "VP_STATUS_MAILBOX_BUSY",
        "VP_STATUS_ERR_VTD_CODE",
        "VP_STATUS_OPTION_NOT_SUPPORTED",
        "VP_STATUS_ERR_VERIFY",
        "VP_STATUS_DEVICE_BUSY",
        "VP_STATUS_MAILBOX_EMPTY",
        "VP_STATUS_ERR_MAILBOX_DATA",
        "VP_STATUS_ERR_HBI",
        "VP_STATUS_ERR_IMAGE",
        "VP_STATUS_IN_CRTCL_SECTN",
        "VP_STATUS_DEV_NOT_INITIALIZED",
        "VP_STATUS_ERR_PROFILE",
        "VP_STATUS_INVALID_VOICE_STREAM",
        "VP_STATUS_CUSTOM_TERM_NOT_CFG",
        "VP_STATUS_DEDICATED_PINS",
        "VP_STATUS_INVALID_LINE",
        "VP_STATUS_LINE_NOT_CONFIG",
        "VP_STATUS_ERR_SPI"
    };

    if (status < VP_STATUS_NUM_TYPES)
    {
        idx = sprintf(&buff[idx], "(%s)", (char *)strTable[status]);
    }
    idx = sprintf(&buff[idx], " = %d", (uint16)status);

    return buff;
}


CstiSLICRinger::CstiSLICRinger ()
	:
    m_eventPollTimer (EVENT_POLL_MS, this)
{
	lastDetectTimeGet ();
	
	m_signalConnections.push_back (m_eventPollTimer.timeoutSignal.Connect (
			[this]{
				slicEventGet ();
				if (!m_deviceInitialized || m_bFlashing || m_testInProgress)
				{
                	m_eventPollTimer.restart ();
				}
			}));
}

CstiSLICRinger::~CstiSLICRinger ()
{
    std::lock_guard<std::recursive_mutex> lock (m_execMutex);
    eventRingerStop ();
}

void CstiSLICRinger::eventInitialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiUNUSED_ARG (hResult);
    
	VpSysInit ();
    
    auto status = VpMakeDeviceObject (VP_DEV_887_SERIES, DEVICE_ID, &m_deviceContext, &m_deviceObject);
	stiTESTCONDMSG (status == VP_STATUS_SUCCESS, stiRESULT_ERROR, "Failed to make slic device object");


    stiDEBUG_TOOL(g_stiSlicRingerDebug,
	{
	    stiTrace ("Successfully created slic device object\n");
    });

    status = VpMakeLineObject(VP_TERM_FXS_LOW_PWR, 0, &m_lineContext, &m_lineObject, &m_deviceContext);
	stiTESTCONDMSG (status == VP_STATUS_SUCCESS, stiRESULT_ERROR, "Failed to make slic line object");

    stiDEBUG_TOOL(g_stiSlicRingerDebug,
    {
        stiTrace ("Successfully created slic line object\n");
    });

    status = VpInitDevice(&m_deviceContext, DEV_PROFILE_Le9653_135V_IB, AC_FXS_RF14_600R_DEF, DC_FXS_miSLIC_IB_DEF, RING_MISLIC_IB135V_US, VP_PTABLE_NULL, VP_PTABLE_NULL);
	stiTESTCONDMSG (status == VP_STATUS_SUCCESS, stiRESULT_ERROR, "Call to VpInitDevice failed: %s", MapStatus(status));

	m_eventPollTimer.restart ();
	stiDEBUG_TOOL(g_stiSlicRingerDebug,
	{
		stiTrace ("Device initialization started\n");
	});

STI_BAIL:

	return;
}


stiHResult CstiSLICRinger::startup ()
{
    CstiSLICRingerBase::startup ();

    PostEvent ([this]{eventInitialize ();});

	return stiRESULT_SUCCESS;
}


void CstiSLICRinger::eventRingerStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiUNUSED_ARG (hResult);

	if (!m_deviceInitialized || m_bFlashing || m_testInProgress)
    {
		stiDEBUG_TOOL(g_stiSlicRingerDebug,
		{
			vpe::trace ("Cannot start ringing of SLIC device:\n");
			vpe::trace ("\tinitialized: ", m_deviceInitialized, "\n");
			vpe::trace ("\tflashing: ", m_bFlashing, "\n");
			vpe::trace ("\ttestInProgress: ", m_testInProgress, "\n");
		});
        return;
    }

	auto status = VpInitRing (&m_lineContext, HAWKEYE_SLIC_CADENCE, VP_NULL);
	stiTESTCONDMSG (status == VP_STATUS_SUCCESS, stiRESULT_ERROR, "Error setting SLIC cadence profile: %s", MapStatus(status));

	status = VpSetLineState(&m_lineContext, VP_LINE_RINGING);
	stiTESTCONDMSG (status == VP_STATUS_SUCCESS, stiRESULT_ERROR, "Error setting SLIC line state to ringing: %s", MapStatus(status));

	m_bFlashing = true;

	m_eventPollTimer.restart ();

STI_BAIL:

	return;
}


void CstiSLICRinger::eventRingerStop ()
{
    stiHResult hResult = stiRESULT_SUCCESS;
	stiUNUSED_ARG (hResult);
	
	if (m_bFlashing)
	{
		auto status = VpSetLineState(&m_lineContext, VP_LINE_DISCONNECT);
		stiTESTCONDMSG (status == VP_STATUS_SUCCESS, stiRESULT_ERROR, "Error setting SLIC line state to disconnect: %s", MapStatus(status));

		m_bFlashing = false;

		auto now = std::chrono::system_clock::now ();
		auto hours = std::chrono::duration_cast<std::chrono::hours>(now - m_lastDetectTime).count ();
		if (hours > HOURS_BETWEEN_DETECTS)
		{
			eventDeviceDetect ();
		}		
	}

STI_BAIL:

	return;
}


void CstiSLICRinger::eventDeviceDetect ()
{
    stiHResult hResult = stiRESULT_SUCCESS;
	stiUNUSED_ARG (hResult);
	
	if (!m_deviceInitialized || m_bFlashing || m_testInProgress)
    {
		stiDEBUG_TOOL(g_stiSlicRingerDebug,
		{
			vpe::trace ("Cannot run SLIC device detect:\n");
			vpe::trace ("\tinitialized: ", m_deviceInitialized, "\n");
			vpe::trace ("\tflashing: ", m_bFlashing, "\n");
			vpe::trace ("\ttestInProgress: ", m_testInProgress, "\n");
		});
        return;
    }

    m_testInput.ringerTestType = LT_RINGER_ELECTRONIC_PHNE_TEST;
    m_testInput.vRingerTest = 57000;
    m_testInput.freq = 20000;
    m_testInput.renFactor = 1210000;

    m_testCriteria.renHigh = 5000;
    m_testCriteria.renLow = 0;

    VpMemSet (&m_testAttributes, 0, sizeof (LtTestAttributesType));
    m_testAttributes.ltDbgFlag = 0; // Set to 0xFFFFFFFF to turn on line test debug statements
    m_testAttributes.inputs.pRingersInp = &m_testInput;
    m_testAttributes.criteria.pRingersCrt = &m_testCriteria;

    auto status = LtStartTest (&m_lineContext, LT_TID_RINGERS, 0xBEEF, &m_testAttributes, &m_tempTestData, &m_testResults, &m_testContext);
	stiTESTCONDMSG (status == LT_STATUS_RUNNING, stiRESULT_ERROR, "Unable to start SLIC device detect test, status: %u", status);

    m_testInProgress = true;

	lastDetectTimeSet ();

	m_eventPollTimer.restart ();

STI_BAIL:

	return;
}


void CstiSLICRinger::lastDetectTimeGet ()
{
	std::string lastDetectFilePath;
	stiOSDynamicDataFolderGet (&lastDetectFilePath);
	lastDetectFilePath += LAST_SLIC_DETECT_FILE;
	
	std::ifstream ifs (lastDetectFilePath, std::ios::binary);
	if (ifs.is_open ())
	{
		std::chrono::system_clock::rep fileTimeRep;
		if (ifs.read (reinterpret_cast<char*>(&fileTimeRep), sizeof (fileTimeRep)))
		{
			m_lastDetectTime = std::chrono::system_clock::time_point {std::chrono::system_clock::duration {fileTimeRep}};
		}
	}
}


void CstiSLICRinger::lastDetectTimeSet ()
{
	m_lastDetectTime = std::chrono::system_clock::now ();
	
	std::string lastDetectFilePath;
	stiOSDynamicDataFolderGet (&lastDetectFilePath);
	lastDetectFilePath += LAST_SLIC_DETECT_FILE;

	std::ofstream ofs (lastDetectFilePath, std::ios::binary);
	if (ofs.is_open ())
	{
		auto const timeSinceEpoch = m_lastDetectTime.time_since_epoch().count ();
		ofs.write (reinterpret_cast<char const*>(&timeSinceEpoch), sizeof (timeSinceEpoch));
	}
}


void CstiSLICRinger::slicEventGet ()
{
    VpEventType event;
    while (VpGetEvent (&m_deviceContext, &event))
    {
        if (m_testInProgress && &m_lineContext == event.pLineCtx)
        {
            lineTestEventProcess (&event);
            continue;
        }
        
        /* Parse the RESPONSE category */
        if (event.eventCategory == VP_EVCAT_RESPONSE)
        {
            if (event.eventId == VP_DEV_EVID_DEV_INIT_CMP)
            {
                if (event.eventData != 0)
                {
                    stiASSERTMSG (false, "SLIC device initialization failed, error: %u", event.eventData);
                    continue;
                }

                initSetMasks ();

                stiDEBUG_TOOL(g_stiSlicRingerDebug,
		        {
                    stiTrace ("Received SLIC device initialization complete event\n");
                });

				auto status = VpSetLineState(&m_lineContext, VP_LINE_DISCONNECT);
                if (status != VP_STATUS_SUCCESS)
                {
                    stiASSERTMSG (false, "Error setting SLIC line state to disconnect: %s", MapStatus(status));
                    continue;
                }

                m_deviceInitialized = true;
            }
        }
    }
}

void CstiSLICRinger::initSetMasks ()
{
    VpOptionEventMaskType eventMask;

    /* mask everything */
    VpMemSet(&eventMask, 0xFF, sizeof(VpOptionEventMaskType));

    /* unmask only the events the application is interested in */
    // eventMask.faults = VP_EVCAT_FAULT_UNMASK_ALL;
    // eventMask.signaling = VP_EVCAT_SIGNALING_UNMASK_ALL | VP_DEV_EVID_TS_ROLLOVER;
    eventMask.response = VP_EVCAT_RESPONSE_UNMASK_ALL;
    eventMask.test = VP_EVCAT_TEST_UNMASK_ALL;
    // eventMask.process = VP_EVCAT_PROCESS_UNMASK_ALL;

    /* inform the API of the mask */
    VpSetOption(VP_NULL, &m_deviceContext, VP_OPTION_ID_EVENT_MASK, &eventMask);
}

void CstiSLICRinger::lineTestEventProcess (VpEventType *event)
{
    stiHResult hResult = stiRESULT_SUCCESS;
	stiUNUSED_ARG (hResult);

	VpStatusType vpStatus;
	LtTestStatusType ltStatus;
	VpResultsType results;
    LtEventType ltEvent;

    ltEvent.pVpEvent = event;

    if (event->hasResults)
    {
        vpStatus = VpGetResults (event, &results);
		stiTESTCONDMSG (vpStatus == VP_STATUS_SUCCESS, stiRESULT_ERROR, "VpGetResult() failed: %s", MapStatus (vpStatus));

        ltEvent.pResult = &results;
    }
    else
    {
        stiDEBUG_TOOL(g_stiSlicRingerDebug,
		{
			stiTrace ("slic line test event does not have any results\n");
		});

        ltEvent.pResult = nullptr;
    }
    
    ltStatus = LtEventHandler (&m_testContext, &ltEvent);

    switch (ltStatus)
    {
        case LT_STATUS_DONE:
        {
            m_testInProgress = false;
			auto connected {0};
			
			stiTESTCONDMSG (m_testResults.result.ringers.fltMask == LT_TEST_PASSED, stiRESULT_ERROR,
				"slic line test failed, fltMask: 0x%x, measStatus: %u", m_testResults.result.ringers.fltMask, m_testResults.result.ringers.measStatus);

			if (m_testResults.result.ringers.ren >= MREN_CONNECTED_THRESHOLD)
			{
				connected = 1;
				deviceConnectedSignal.Emit (true, m_testResults.result.ringers.ren);

				stiDEBUG_TOOL(g_stiSlicRingerDebug,
				{
					stiTrace ("slic device is connected, mREN: %d\n", m_testResults.result.ringers.ren);
				});
			}
			else
			{
				deviceConnectedSignal.Emit (false, m_testResults.result.ringers.ren);

				stiDEBUG_TOOL(g_stiSlicRingerDebug,
				{
					stiTrace ("slic device is NOT connected, mREN: %d\n", m_testResults.result.ringers.ren);
				});
			} 

			stiRemoteLogEventSend ("EventType=SLICDetect Connected=%d mRen=%d", connected, m_testResults.result.ringers.ren);

            break;
        }
        case LT_STATUS_RUNNING:
        case LT_STATUS_ABORTED:
        {
            stiDEBUG_TOOL(g_stiSlicRingerDebug,
            {
                stiTrace ("line test running or aborted\n");
            });
            break;
        }
        default:
        {
            stiTHROWMSG (stiRESULT_ERROR, "slic line test error: %u, error code: %u, measStatus: %u",
				ltStatus, m_testResults.result.ringers.fltMask, m_testResults.result.ringers.measStatus);
            break;
        }
    }

STI_BAIL:

	return;
}
