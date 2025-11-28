// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "CstiISPMonitor.h"
#include "stiTrace.h"
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "stiOVUtils.h"
#include "CstiMonitorTask.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static const char g_szISPMonitorPipe[] = "/tmp/ispmon";
static const char g_szConfigurationFile[] = "ISP.cfg";

/*! \brief Constructor
 *
 * This is the default constructor for the CstiISPMonitor class.
 *
 * \return None
 */
CstiISPMonitor::CstiISPMonitor ()
:
	CstiEventQueue ("CstiISPMonitor"),
	m_monitorTimer (0, this)
{
	stiLOG_ENTRY_NAME (CstiISPMonitor::CstiISPMonitor);

	m_signalConnections.push_back (m_monitorTimer.timeoutSignal.Connect (
			std::bind(&CstiISPMonitor::eventMonitorTimer, this)));

	//
	// Remove the file in case it is there.
	//
	unlink (g_szISPMonitorPipe);

	//
	// Create the fifo.
	//
	mkfifo (g_szISPMonitorPipe, 0600);

	stiOSStaticDataFolderGet(&m_ConfigurationFilename);
	m_ConfigurationFilename += g_szConfigurationFile;
}


/*! \brief Initializes the monitor task
 *
 */
stiHResult CstiISPMonitor::Initialize (
	CstiMonitorTask *pMonitorTask)
{
	stiLOG_ENTRY_NAME (CstiISPMonitor::Initialize);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (pMonitorTask != nullptr, stiRESULT_ERROR);

	m_pMonitorTask = pMonitorTask;

	m_pMonitorTask->RcuConnectedStatusGet(&m_bRcuConnected);

	m_signalConnections.push_back (m_pMonitorTask->usbRcuConnectionStatusChangedSignal.Connect (
		[this] ()
		{
			PostEvent (std::bind(&CstiISPMonitor::eventRcuConnectionStatusChanged, this));
		}));

	m_signalConnections.push_back (m_pMonitorTask->usbRcuResetSignal.Connect (
		[this] ()
		{
			PostEvent (std::bind(&CstiISPMonitor::eventRcuReset, this));
		}));

	m_pConfiguration = new CstiISPMonitor::SstiConfiguration;

	hResult = ConfigurationRead (m_ConfigurationFilename, m_pConfiguration);
	stiTESTRESULT ();

	if (m_bRcuConnected)
	{
		CameraOpen ();
		StaticSettingsSet ();
	}

	m_PipeFd = open (g_szISPMonitorPipe, O_RDWR | O_NONBLOCK);

	if (!FileDescriptorAttach (m_PipeFd, std::bind(&CstiISPMonitor::eventReadPipe, this)))
	{
		stiASSERTMSG(estiFALSE, __func__, "%s: Can't attach pipeFd %d\n", __func__, m_PipeFd);
	}

	// Near camera w/ Largan lens (9A)
	m_ambientLimits[OV640_LENS_TYPE_9513A_LARGAN].low = 0.99;
	m_ambientLimits[OV640_LENS_TYPE_9513A_LARGAN].midlow = 3.2;
	m_ambientLimits[OV640_LENS_TYPE_9513A_LARGAN].midhigh = 9.0;
	m_ambientLimits[OV640_LENS_TYPE_9513A_LARGAN].high = 11.0;

	// Far camera w/ Sunex lens (BA)
	m_ambientLimits[OV640_LENS_TYPE_DSL944_SUNEX].low = 0.64;
	m_ambientLimits[OV640_LENS_TYPE_DSL944_SUNEX].midlow = 1.62;
	m_ambientLimits[OV640_LENS_TYPE_DSL944_SUNEX].midhigh = 6.4;
	m_ambientLimits[OV640_LENS_TYPE_DSL944_SUNEX].high = 8.0;

	// Near camera w/ Demarren lens (9B)
	m_ambientLimits[OV640_LENS_TYPE_T3117_DEMARREN].low = 0.9;
	m_ambientLimits[OV640_LENS_TYPE_T3117_DEMARREN].midlow = 2.73;
	m_ambientLimits[OV640_LENS_TYPE_T3117_DEMARREN].midhigh = 10.0;
	m_ambientLimits[OV640_LENS_TYPE_T3117_DEMARREN].high = 11.0;

	// Far camera w/ Demarren lens (BB)
	m_ambientLimits[OV640_LENS_TYPE_CV0207A_DEMARREN].low = 0.93;
	m_ambientLimits[OV640_LENS_TYPE_CV0207A_DEMARREN].midlow = 2.62;
	m_ambientLimits[OV640_LENS_TYPE_CV0207A_DEMARREN].midhigh = 14.0;
	m_ambientLimits[OV640_LENS_TYPE_CV0207A_DEMARREN].high = 17.0;

	monitorTimerStart ();

STI_BAIL:

	return (hResult);

}//end CstiISPMonitor::Initialize ()


/*! \brief Destructor
 *
 * This is the default destructor for the CstiISPMonitor class.
 *
 * \return None
 */
CstiISPMonitor::~CstiISPMonitor ()
{
	stiLOG_ENTRY_NAME (CstiISPMonitor::~CstiISPMonitor);

	if (m_PipeFd > -1)
	{
		FileDescriptorDetach (m_PipeFd);
	}

	StopEventLoop ();

	if (m_pConfiguration)
	{
		delete m_pConfiguration;
		m_pConfiguration = nullptr;
	}

	unlink (g_szISPMonitorPipe);
}


void CstiISPMonitor::Startup ()
{
	StartEventLoop ();
}

/*! \brief shutdown the monitor task
 *
 *  \retval estiOK if success
 *  \retval estiERROR otherwise
 */
void CstiISPMonitor::Shutdown ()
{
	StopEventLoop ();
}


void PipeClear (
	int nFd)
{
	char szBuffer[4];

	for (;;)
	{
		int nLength = stiOSRead (nFd, szBuffer, sizeof (szBuffer) - 1);

		//
		// Did we reach the end of the file?
		//
		if (nLength <= 0)
		{
			break;
		}
	}
}


void CstiISPMonitor::monitorTimerStart ()
{
	if (m_bRcuConnected && !m_bHdmiPassthrough)
	{
		if (m_pConfiguration && m_pConfiguration->bValid)
		{
			m_monitorTimer.timeoutSet (m_pConfiguration->nSampleRate);
			m_monitorTimer.restart();
		}
	}
}

void CstiISPMonitor::eventMonitorTimer ()
{
	if (m_bRcuConnected && !m_bHdmiPassthrough)
	{
		DynamicSettingsSet ();
	}

	monitorTimerStart ();
}


void CstiISPMonitor::eventReadPipe ()
{
	//
	// Read all the data out of the fifo so we don't keep coming
	// back in here.
	//
	PipeClear (m_PipeFd);

	if (m_pConfiguration)
	{
		delete m_pConfiguration;
		m_pConfiguration = nullptr;
	}

	m_pConfiguration = new CstiISPMonitor::SstiConfiguration;

	stiHResult hResult = ConfigurationRead (m_ConfigurationFilename, m_pConfiguration);

	if (!stiIS_ERROR (hResult) && m_bRcuConnected && !m_bHdmiPassthrough)
	{
		StaticSettingsSet ();

		monitorTimerStart ();
	}
}


void CstiISPMonitor::ReadCurve (
	char *pCurrent,
	char *pEnd,
	std::vector<CstiISPMonitor::SstiCurveEntry> *pCurve,
	CstiISPMonitor::ECurveVariable *peCurveVariable)
{
	char *pEndOfLine = pEnd;

	//
	// Read the value the curve is based on: G or J.
	//

	//
	// Skip past any whitespace
	//
	pCurrent = SkipWhiteSpace (pCurrent);

	//
	// Read the token until a white space character is read
	//
	pEnd = SkipUntilWhiteSpace (pCurrent);

	*pEnd = '\0';


	if (strcmp (pCurrent, "G") == 0)
	{
		*peCurveVariable = eG;
	}
	else if (strcmp (pCurrent, "J") == 0)
	{
		*peCurveVariable = eJ;
	}
	else if (strcmp (pCurrent, "Y") == 0)
	{
		*peCurveVariable = eY;
	}
	else
	{
		stiASSERT (false);
	}

	pCurrent = pEnd + 1;

	//
	// Read the curve values
	//
	for (;;)
	{
		//
		// Skip past any whitespace
		//
		pCurrent = SkipWhiteSpace (pCurrent);

		if (pCurrent >= pEndOfLine)
		{
			break;
		}

		// todo: verify we are now on a '('.

		//
		// We should now be on a parenthesis. Skip until we find the end parenthesis.
		// Read the token until a white space character is read
		//
		pEnd = SkipUntilCharacter (pCurrent, ')');

		// todo: verify we are now on a ')'.

		//
		// Move past the paranthesis and terminate the string.
		//
		++pEnd;
		*pEnd = '\0';

		float fValue;
		float fPosition;

		sscanf (pCurrent, "(%f, %f)", &fPosition, &fValue);

		pCurve->push_back(CstiISPMonitor::SstiCurveEntry(fValue, fPosition));

		//
		// Look for more entries
		//
		pCurrent = pEnd + 1;

		if (pCurrent >= pEndOfLine)
		{
			break;
		}
	}
}


void CstiISPMonitor::ReadTable (
	char *pCurrent,
	char *pEnd,
	std::vector<uint8_t> *pTable)
{
	char *pEndOfLine = pEnd;

	//
	// Read the table
	//
	for (;;)
	{
		//
		// Skip past any whitespace
		//
		pCurrent = SkipWhiteSpace (pCurrent);

		if (pCurrent >= pEndOfLine)
		{
			break;
		}

		//
		// Read the token until a white space character is read
		//
		pEnd = SkipUntilWhiteSpace (pCurrent);

		*pEnd = '\0';

		uint32_t un32Value;

		sscanf (pCurrent, "%x", &un32Value);

		pTable->push_back(un32Value);

		//
		// Look for more entries
		//
		pCurrent = pEnd + 1;

		if (pCurrent >= pEndOfLine)
		{
			break;
		}
	}
}


void CstiISPMonitor::PrintCurve (
	const std::vector<CstiISPMonitor::SstiCurveEntry> &Curve)
{
	for (unsigned int i = 0; i < Curve.size (); i++)
	{
		stiTrace ("(%f, %f)\n", Curve[i].fPosition, Curve[i].fValue);
	}
}


stiHResult CstiISPMonitor::ConfigurationRead (
	const std::string &FileName,
	CstiISPMonitor::SstiConfiguration *pCfg)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	FILE *pFile = nullptr;
	char *pBuffer = nullptr;

	pCfg->bValid = false;

	pFile = fopen (FileName.c_str (), "r");

	if (pFile)
	{
		//
		// Read the entire file into memory.
		//
		fseek (pFile, 0, SEEK_END);

		int nFileSize = ftell (pFile);

		fseek (pFile, 0, SEEK_SET);

		pBuffer = new char[nFileSize + 1];

		char *pCurrent = pBuffer;
		char *pEndOfFile = &pBuffer[nFileSize];
		char *pToken = nullptr;

		fread (pBuffer, nFileSize, sizeof (char), pFile);
		pBuffer[nFileSize] = '\0';

		fclose (pFile);
		pFile = nullptr;

		//
		// Parse the file
		//
		for (;;)
		{
			//
			// Are we at the end?
			//
			if (*pCurrent == '\0')
			{
				break;
			}

			//
			// Skip past any white space
			//
			pCurrent = SkipWhiteSpace (pCurrent);

			//
			// Are we at the end?
			//
			if (*pCurrent == '\0')
			{
				// Yes

				break;
			}

			char *pEnd = nullptr;
			char *pNextLine = nullptr;

			//
			// If the current character is a '#' character
			// then skip the rest of the line
			//
			if (*pCurrent == '#')
			{
				pNextLine = SkipToEndOfLine (pCurrent + 1) + 1;
			}
			else
			{
				//
				// Read the token until a non-alphanumeric character is read
				//
				pEnd = pCurrent + 1;

				while (*pEnd != '\0' && isalnum (*pEnd))
				{
					pEnd++;
				}

				//
				// NULL terminate the current token
				//
				*pEnd = '\0';

				pToken = pCurrent;

				pCurrent = pEnd + 1;

				if (pCurrent >= pEndOfFile)
				{
					break;
				}

				//
				// Read until the end of the line.
				//
				pEnd = SkipToEndOfLine (pCurrent + 1);

				*pEnd = '\0';

				pNextLine = pEnd + 1;

				//
				// Find and null terminate at the start of a comment
				//
				pEnd = SkipUntilCharacter (pCurrent, '#');

				*pEnd = '\0';

				stiDEBUG_TOOL (g_stiISPDebug,
					stiTrace ("Command: %s, Parameters: %s\n", pToken, pCurrent);
				);
				
				//
				// Skip any whitespace
				//
				pCurrent = SkipWhiteSpace (pCurrent);

				if (strcmp (pToken, "MaxGain") == 0)
				{
					sscanf (pCurrent, "%d", &pCfg->nMaxGain);
					
					stiDEBUG_TOOL (g_stiISPDebug,
						stiTrace ("Max Gain = %d\n", pCfg->nMaxGain);
					);
				}
				else if (strcmp (pToken, "GainWeight") == 0)
				{
					sscanf (pCurrent, "%f", &pCfg->fGainWeight);

					stiDEBUG_TOOL (g_stiISPDebug,
						stiTrace ("Gain Weight = %f\n", pCfg->fGainWeight);
					);
				}
				else if (strcmp (pToken, "ExposureWeight") == 0)
				{
					sscanf (pCurrent, "%f", &pCfg->fExposureWeight);

					stiDEBUG_TOOL (g_stiISPDebug,
						stiTrace ("Exposure Weight = %f\n", pCfg->fExposureWeight);
					);
				}
				else if (strcmp (pToken, "LumaWeight") == 0)
				{
					sscanf (pCurrent, "%f", &pCfg->fLumaWeight);

					stiDEBUG_TOOL (g_stiISPDebug,
						stiTrace ("Luma Weight = %f\n", pCfg->fLumaWeight);
					);
				}
				else if (strcmp (pToken, "SampleRate") == 0)
				{
					sscanf (pCurrent, "%d", &pCfg->nSampleRate);
					
					stiDEBUG_TOOL (g_stiISPDebug,
						stiTrace ("Sample Rate = %d\n", pCfg->nSampleRate);
					);
				}
				else if (strcmp (pToken, "Saturation") == 0)
				{
					ReadCurve (pCurrent, pEnd, &pCfg->SaturationCurve, &pCfg->eSaturationVariable);
				}
				else if (strcmp (pToken, "YOffset") == 0)
				{
					ReadCurve (pCurrent, pEnd, &pCfg->YOffsetCurve, &pCfg->eYOffsetVariable);
				}
				else if (strcmp (pToken, "GDenoise") == 0)
				{
					ReadCurve (pCurrent, pEnd, &pCfg->GDenoiseCurve, &pCfg->eGDenoiseVariable);
				}
				else if (strcmp (pToken, "BRDenoise") == 0)
				{
					ReadCurve (pCurrent, pEnd, &pCfg->BRDenoiseCurve, &pCfg->eBRDenoiseVariable);
				}
				else if (strcmp (pToken, "UVDenoise") == 0)
				{
					ReadCurve (pCurrent, pEnd, &pCfg->UVDenoiseCurve, &pCfg->eUVDenoiseVariable);
				}
				else if (strcmp (pToken, "Brightness") == 0)
				{
					ReadCurve (pCurrent, pEnd, &pCfg->BrightnessCurve, &pCfg->eBrightnessVariable);
				}
				else if (strcmp (pToken, "Contrast") == 0)
				{
					ReadTable (pCurrent, pEnd, &pCfg->ContrastTable);
				}
				else if (strcmp (pToken, "Write") == 0)
				{
					SstiRegisterEntry SRegister;
					unsigned int unValue;

					sscanf (pCurrent, "%x %x", &SRegister.un32Address, &unValue);

					SRegister.un8Value = unValue;

					pCfg->RegisterList.push_back(SRegister);
				}
				else if (strcmp (pToken, "Include") == 0)
				{
					std::string IncludeFilename;

					stiOSStaticDataFolderGet(&IncludeFilename);
					IncludeFilename += pCurrent;

					stiDEBUG_TOOL (g_stiISPDebug,
						stiTrace ("Reading include file %s.\n", IncludeFilename.c_str ());
					);

					ConfigurationRead (IncludeFilename, pCfg);

					stiDEBUG_TOOL (g_stiISPDebug,
						stiTrace ("Done reading include file.\n");
					);
				}
				else
				{
					stiASSERTMSG (false, "Unknown token: %s", pToken);
				}
			}

			pCurrent = pNextLine;

			if (pCurrent >= pEndOfFile)
			{
				break;
			}
		}

		pCfg->bValid = true;

		stiDEBUG_TOOL (g_stiISPDebug,
			stiTrace ("Saturation Curve\n");
			PrintCurve (pCfg->SaturationCurve);

			stiTrace ("Green Denoise Curve\n");
			PrintCurve (pCfg->GDenoiseCurve);

			stiTrace ("Blue/Red Denoise Curve\n");
			PrintCurve (pCfg->BRDenoiseCurve);

			stiTrace ("U/V Denoise Curve\n");
			PrintCurve (pCfg->UVDenoiseCurve);

			stiTrace ("Brightness Curve\n");
			PrintCurve (pCfg->BrightnessCurve);
		);
	}
	else
	{
		stiASSERTMSG (false, "Could not read configuration file: %s\n", FileName.c_str());
	}

	if (pBuffer)
	{
		delete [] pBuffer;
		pBuffer = nullptr;
	}

	return hResult;
}


stiHResult CstiISPMonitor::StaticSettingsSet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_pConfiguration && m_pConfiguration->bValid)
	{
		//
		// Set the maximum gain
		//
		ValueSet (0x80140150, 0x80140151, 14, m_pConfiguration->nMaxGain * 16);

		//
		// Set the contrast table entries
		//
		std::vector<uint8_t>::const_iterator c;
		uint32_t un32Address = 0x801404c0;
		
		stiDEBUG_TOOL (g_stiISPDebug,
			stiTrace ("Setting contrast table.\n");
		);
		
		for (c = m_pConfiguration->ContrastTable.begin (); c != m_pConfiguration->ContrastTable.end (); ++c)
		{
			stiDEBUG_TOOL (g_stiISPDebug,
				stiTrace ("0x%0x to 0x%0x\n", un32Address, *c);
			);
			
			ValueSet (un32Address, 8, *c);

			un32Address += 1;
		}

		//
		// Set values from register list.
		//
		std::vector<CstiISPMonitor::SstiRegisterEntry>::const_iterator i;

		for (i = m_pConfiguration->RegisterList.begin (); i != m_pConfiguration->RegisterList.end (); ++i)
		{
			//stiTrace ("Setting register 0x%0x to 0x%x\n", (*i).un32Address, (*i).un8Value);
			ValueSet ((*i).un32Address, 8, (*i).un8Value);
		}
	}

	return hResult;
}


float Interpolate (
	float fCurrentValue,
	float fMinimum,
	float fMaximum,
	float fValueAtMinimum,
	float fValueAtMaximum)
{
	float fValue = 0;

	if (fCurrentValue < fMinimum)
	{
		fValue = fValueAtMinimum;
	}
	else if (fCurrentValue > fMaximum)
	{
		fValue = fValueAtMaximum;
	}
	else
	{
		float fPct = (fCurrentValue - fMinimum) / (fMaximum - fMinimum);
		fValue = fValueAtMinimum + (fValueAtMaximum - fValueAtMinimum) * fPct;
	}

	return fValue;
}


float CstiISPMonitor::FindCurveValue (
	float fInputValue,
	const std::vector<CstiISPMonitor::SstiCurveEntry> &Curve)
{
	unsigned int i = 0;
	float fValue = 0;

	if (fInputValue <= Curve[0].fPosition)
	{
		fValue = Curve[0].fValue;
	}
	else
	{
		//
		// Iterate through the curve to find the right entry to interpolate across
		//
		for (i = 1; i < Curve.size (); i++)
		{
			if (fInputValue <= Curve[i].fPosition)
			{
				fValue = Interpolate (fInputValue,
						Curve[i - 1].fPosition,
						Curve[i].fPosition,
						Curve[i - 1].fValue,
						Curve[i].fValue);

				break;
			}
		}

		//
		// If we ran through the entire curve then set the saturation value to the
		// last entry in the curve.
		//
		if (i >= Curve.size ())
		{
			fValue = Curve[Curve.size () - 1].fValue;
		}
	}

	return fValue;
}


void CstiISPMonitor::SaturationSet (
	float fInputValue,
	const std::vector<CstiISPMonitor::SstiCurveEntry> &SaturationCurve,
	int *pnCurrentSaturation)
{
	float fSaturation = FindCurveValue (fInputValue, SaturationCurve);

	//
	// Compute the new saturation value and see if it needs to be set.
	//
	int nNewSaturation  = ((int)(fSaturation * 0x80));

	if (nNewSaturation != *pnCurrentSaturation)
	{
		stiDEBUG_TOOL (g_stiISPDebug,
			stiTrace ("fSat = %0.04f, nSat = 0x%02x\n", fSaturation, nNewSaturation);
		);

		ValueSet (0x801404eb, 8, nNewSaturation); // Maximum Saturation
		ValueSet (0x801404ec, 8, nNewSaturation); // Minimum Saturation

		*pnCurrentSaturation = nNewSaturation;
	}
}


void CstiISPMonitor::YOffsetSet (
	float fInputValue,
	const std::vector<CstiISPMonitor::SstiCurveEntry> &YOffsetCurve,
	int *pnCurrentYOffset)
{
	float fYOffset = FindCurveValue (fInputValue, YOffsetCurve);

	//
	// Compute the new saturation value and see if it needs to be set.
	//
	auto  nNewYOffset  = (int)(fYOffset + 0.5f);

	if (nNewYOffset != *pnCurrentYOffset)
	{
		stiDEBUG_TOOL (g_stiISPDebug,
			stiTrace ("fYOffset = %0.04f, nYOffset = 0x%02x\n", fYOffset, nNewYOffset);
		);

		ValueSet (0x80215d16, 8, nNewYOffset); // Maximum Saturation

		*pnCurrentYOffset = nNewYOffset;
	}
}


void CstiISPMonitor::GDenoiseSet (
	float fInputValue,
	const std::vector<CstiISPMonitor::SstiCurveEntry> &GDenoiseCurve,
	int *pnCurrentGDenoise)
{
	float fGDenoise = FindCurveValue (fInputValue, GDenoiseCurve);

	//
	// Compute the new saturation value and see if it needs to be set.
	//
	auto  nNewGDenoise  = (int)(fGDenoise + 0.5f);

	if (nNewGDenoise != *pnCurrentGDenoise)
	{
		stiDEBUG_TOOL (g_stiISPDebug,
			stiTrace ("fGDenoise = %0.04f, nGDenoise = 0x%02x\n", fGDenoise, nNewGDenoise);
		);

		ValueSet (0x8021551a, 8, nNewGDenoise);
		ValueSet (0x8021551b, 8, nNewGDenoise);
		ValueSet (0x8021551c, 8, nNewGDenoise);
		ValueSet (0x8021551d, 8, nNewGDenoise);
		ValueSet (0x8021551e, 8, nNewGDenoise);
		ValueSet (0x8021551f, 8, nNewGDenoise);
		ValueSet (0x80215520, 8, nNewGDenoise);

		*pnCurrentGDenoise = nNewGDenoise;
	}
}


void CstiISPMonitor::BRDenoiseSet (
	float fInputValue,
	const std::vector<CstiISPMonitor::SstiCurveEntry> &BRDenoiseCurve,
	int *pnCurrentBRDenoise)
{
	float fBRDenoise = FindCurveValue (fInputValue, BRDenoiseCurve);

	//
	// Compute the new saturation value and see if it needs to be set.
	//
	auto  nNewBRDenoise  = (int)(fBRDenoise + 0.5f);

	if (nNewBRDenoise != *pnCurrentBRDenoise)
	{
		stiDEBUG_TOOL (g_stiISPDebug,
			stiTrace ("fBRDenoise = %0.04f, nBRDenoise = 0x%02x\n", fBRDenoise, nNewBRDenoise);
		);

		ValueSet (0x80215522, 0x80215523, 9, nNewBRDenoise);
		ValueSet (0x80215524, 0x80215525, 9, nNewBRDenoise);
		ValueSet (0x80215526, 0x80215527, 9, nNewBRDenoise);
		ValueSet (0x80215528, 0x80215529, 9, nNewBRDenoise);
		ValueSet (0x8021552a, 0x8021552b, 9, nNewBRDenoise);
		ValueSet (0x8021552c, 0x8021552d, 9, nNewBRDenoise);
		ValueSet (0x8021552e, 0x8021552f, 9, nNewBRDenoise);

		*pnCurrentBRDenoise = nNewBRDenoise;
	}
}


void CstiISPMonitor::UVDenoiseSet (
	float fInputValue,
	const std::vector<CstiISPMonitor::SstiCurveEntry> &UVDenoiseCurve,
	int *pnCurrentUVDenoise)
{
	float fUVDenoise = FindCurveValue (fInputValue, UVDenoiseCurve);

	//
	// Compute the new saturation value and see if it needs to be set.
	//
	auto  nNewUVDenoise  = (int)(fUVDenoise + 0.5f);

	if (nNewUVDenoise != *pnCurrentUVDenoise)
	{
		stiDEBUG_TOOL (g_stiISPDebug,
			stiTrace ("fUVDenoise = %0.04f, nUVDenoise = 0x%02x\n", fUVDenoise, nNewUVDenoise);
		);

		ValueSet (0x80215c00, 5, nNewUVDenoise);
		ValueSet (0x80215c01, 5, nNewUVDenoise);
		ValueSet (0x80215c02, 5, nNewUVDenoise);
		ValueSet (0x80215c03, 5, nNewUVDenoise);
		ValueSet (0x80215c04, 5, nNewUVDenoise);
		ValueSet (0x80215c05, 5, nNewUVDenoise);

		*pnCurrentUVDenoise = nNewUVDenoise;
	}
}


void CstiISPMonitor::BrightnessSet (
	float fInputValue,
	const std::vector<CstiISPMonitor::SstiCurveEntry> &BrightnessCurve,
	int *pnCurrentBrightness)
{
	float fBrightness = FindCurveValue (fInputValue, BrightnessCurve);

	//
	// Compute the new saturation value and see if it needs to be set.
	//
	auto  nNewBrightness  = (int)(fBrightness * 128 + 0.5f);

	if (nNewBrightness != *pnCurrentBrightness)
	{
		stiDEBUG_TOOL (g_stiISPDebug,
			stiTrace ("fBrightness = %0.04f, nBrightness = 0x%02x\n", fBrightness, nNewBrightness);
		);

		ValueSet (0x80215d0c, 0x80215d0d, 9, (uint16_t)nNewBrightness);

		*pnCurrentBrightness = nNewBrightness;
	}
}


stiHResult CstiISPMonitor::DynamicSettingsSet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	//
	// Get current gain
	//
	float fGain = ValueGet (0x80140170, 0x80140171, 14) / 16.0f;

	//
	// Get current exposure
	//
	int nExposure = ValueGet (0x8014016c, 0x8014016d, 16);

	//
	// Get current meam luma
	//
	int nMeanLuma = ValueGet (0x8014075e, 8);

	//
	// Compute the J value
	//
	m_jValue = (nMeanLuma * m_pConfiguration->fLumaWeight) / (fGain * m_pConfiguration->fGainWeight * nExposure * m_pConfiguration->fExposureWeight);

	//
	// VP Ambiance Debug Trace printouts
	// nISPTargetLow value is an ISP register modified when Brightness is set in self-view Video Settings
	// Setting g_stiISPDebug trace flag == 2 will print name and data pairs
	// Setting g_stiISPDebug trace flag > 2 will print data only
	//
	uint32_t nISPTargetLow = 0;
	stiDEBUG_TOOL (g_stiISPDebug == 2, 
		nISPTargetLow = ValueGet(0x80140146, 8); 
		stiTrace ("nISPTargetLow = %d, m_nDenoise = %d, nMeanLuma = %d, fGain = %0.4f, m_jValue = %0.4f\n", nISPTargetLow, m_nGDenoise, nMeanLuma, fGain, m_jValue);
	);
	stiDEBUG_TOOL (g_stiISPDebug > 2,  
		nISPTargetLow = ValueGet(0x80140146, 8); 
		stiTrace ("%d,%d,%d,%0.4f,%0.4f\n", nISPTargetLow, m_nGDenoise, nMeanLuma, fGain, m_jValue);
	);
		
	//
	// If there is a saturation curve then adjust saturation using the curve.
	//
	if (!m_pConfiguration->SaturationCurve.empty())
	{
//					stiTrace ("Finding Saturation\n");

		if (m_pConfiguration->eSaturationVariable == eJ)
		{
			SaturationSet (m_jValue, m_pConfiguration->SaturationCurve, &m_nSaturation);
		}
		else
		{
			SaturationSet (fGain, m_pConfiguration->SaturationCurve, &m_nSaturation);
		}
	}

	//
	// If there is a Y Offset curve then adjust the Y offset using the curve.
	//
	if (!m_pConfiguration->YOffsetCurve.empty())
	{
//					stiTrace ("Finding YOffset\n");

		if (m_pConfiguration->eYOffsetVariable == eJ)
		{
			YOffsetSet (m_jValue, m_pConfiguration->YOffsetCurve, &m_nYOffset);
		}
		else
		{
			YOffsetSet (fGain, m_pConfiguration->YOffsetCurve, &m_nYOffset);
		}
	}

	//
	// If there is a G Denoise cruve then adjust the G denoise using the curve.
	//
	if (!m_pConfiguration->GDenoiseCurve.empty())
	{
//					stiTrace ("Finding GDenoise\n");

		if (m_pConfiguration->eGDenoiseVariable == eJ)
		{
			GDenoiseSet (m_jValue, m_pConfiguration->GDenoiseCurve, &m_nGDenoise);
		}
		else
		{
			GDenoiseSet (fGain, m_pConfiguration->GDenoiseCurve, &m_nGDenoise);
		}
	}

	//
	// If there is a BR Denoise curve then adjust the BR denoise using the curve.
	//
	if (!m_pConfiguration->BRDenoiseCurve.empty())
	{
//					stiTrace ("Finding BRDenoise\n");

		if (m_pConfiguration->eBRDenoiseVariable == eJ)
		{
			BRDenoiseSet (m_jValue, m_pConfiguration->BRDenoiseCurve, &m_nBRDenoise);
		}
		else
		{
			BRDenoiseSet (fGain, m_pConfiguration->BRDenoiseCurve, &m_nBRDenoise);
		}
	}

	//
	// If there is a UV Denoise curve then adjust the UV denoise using the curve.
	//
	if (!m_pConfiguration->UVDenoiseCurve.empty())
	{
//					stiTrace ("Finding UVDenoise\n");

		if (m_pConfiguration->eUVDenoiseVariable == eJ)
		{
			UVDenoiseSet (m_jValue, m_pConfiguration->UVDenoiseCurve, &m_nUVDenoise);
		}
		else
		{
			UVDenoiseSet (fGain, m_pConfiguration->UVDenoiseCurve, &m_nUVDenoise);
		}
	}

	//
	// If there is a Brightness curve then adjust the Brightness using the curve.
	//
	if (!m_pConfiguration->BrightnessCurve.empty())
	{
		if (m_pConfiguration->eBrightnessVariable == eY)
		{
			BrightnessSet (nMeanLuma, m_pConfiguration->BrightnessCurve, &m_nBrightness);
		}
		else
		{
			stiASSERT (false);
		}
	}

	return hResult;
}


void CstiISPMonitor::eventRcuConnectionStatusChanged ()
{
	bool bRcuConnected = false;
	m_pMonitorTask->RcuConnectedStatusGet (&bRcuConnected);

	//
	// If the RCU wasn't connected before but now is then
	// go ahead and apply the static settings.
	//
	if (bRcuConnected)
	{
		if (!m_bRcuConnected && !m_bHdmiPassthrough)
		{
			CameraOpen ();
			StaticSettingsSet ();
		}
	}
	else
	{
		if (m_bRcuConnected)
		{
			CameraClose ();
		}
	}

	m_bRcuConnected = bRcuConnected;

	monitorTimerStart ();
}


void CstiISPMonitor::eventRcuReset ()
{
	if (m_bRcuConnected)
	{
		StaticSettingsSet ();
	}
}


stiHResult CstiISPMonitor::HdmiPassthroughSet(
	bool bHdmiPassthrough)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	if (bHdmiPassthrough)
	{
		if (!m_bHdmiPassthrough)
		{
			m_bHdmiPassthrough = true;
			CameraClose();

			m_monitorTimer.stop ();
		}
	}
	else
	{
		if (m_bHdmiPassthrough)
		{
			m_bHdmiPassthrough = false;

			if (m_bRcuConnected)
			{
				CameraOpen();
				hResult = StaticSettingsSet ();
				stiTESTRESULT();

				monitorTimerStart ();
			}
		}
	}

STI_BAIL:

	Unlock ();

	return(hResult);
}

stiHResult CstiISPMonitor::ambientLightGet (
	IVideoInputVP2::AmbientLight *ambiance,
	float *rawVal)
{
	Lock();
	
	stiHResult hResult = stiRESULT_SUCCESS;
	
	int lensType = 0;
	
	stiTESTCOND(ambiance, stiRESULT_ERROR);
	
	m_pMonitorTask->RcuLensTypeGet(&lensType);
	
	stiTESTCOND(m_ambientLimits.find(lensType) != m_ambientLimits.end(), stiRESULT_ERROR);

	if (m_jValue <= m_ambientLimits[lensType].low)
	{
		*ambiance = IVideoInputVP2::AmbientLight::LOW;
	}
	else if (m_jValue >= m_ambientLimits[lensType].high)
	{
		*ambiance = IVideoInputVP2::AmbientLight::HIGH;
	}
	else if ((m_prevAmbiance == IVideoInputVP2::AmbientLight::LOW && m_jValue >= m_ambientLimits[lensType].midlow) ||
			 (m_prevAmbiance == IVideoInputVP2::AmbientLight::HIGH && m_jValue <= m_ambientLimits[lensType].midhigh))
	{
		*ambiance = IVideoInputVP2::AmbientLight::MEDIUM;
	}
	else
	{
		*ambiance = m_prevAmbiance;
	}
	
	m_prevAmbiance = *ambiance;

	if (rawVal)
	{
		*rawVal = m_jValue;
	}
	
STI_BAIL:

	Unlock();
	
	return hResult;
}
