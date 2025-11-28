#include "CstiPlatformInterface.h"

static std::unique_ptr<CstiPlatformInterface> g_platformInterface;

IstiPlatform *IstiPlatform::InstanceGet ()
{
	if (g_platformInterface == nullptr)
	{
		g_platformInterface = std::make_unique<CstiPlatformInterface>();
	}

	return g_platformInterface.get();
}

IstiAudibleRinger *IstiAudibleRinger::InstanceGet ()
{
	return &g_platformInterface->audibleRinger;
}

IstiVideoOutput *IstiVideoOutput::InstanceGet ()
{
	return &g_platformInterface->videoOutput;
}

IstiVideoInput *IstiVideoInput::InstanceGet ()
{
	return &g_platformInterface->videoInput;
}

IstiAudioInput *IstiAudioInput::InstanceGet ()
{
	return &g_platformInterface->audioInput;
}

IstiAudioOutput *IstiAudioOutput::InstanceGet ()
{
	return &g_platformInterface->audioOutput;
}

IstiNetwork *IstiNetwork::InstanceGet ()
{
	return &g_platformInterface->network;
}

IstiMessageViewer* IstiMessageViewer::InstanceGet ()
{
	return &g_platformInterface->filePlay;
}

stiHResult CstiPlatformInterface::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	auto success = videoInput.StartEventLoop ();
	stiASSERT (success);

	success = videoOutput.StartEventLoop ();
	stiASSERT (success);

	hResult = audioOutput.Initialize ();
	stiTESTRESULT ();

	hResult = filePlay.Initialize ();
	stiTESTRESULT ();

	hResult = filePlay.Startup ();
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult CstiPlatformInterface::Uninitialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = audioOutput.Uninitialize ();
	stiASSERT (!stiIS_ERROR (hResult));
	videoOutput.StopEventLoop ();

	hResult = filePlay.Shutdown ();
	stiASSERT (!stiIS_ERROR (hResult));

	filePlay.StopEventLoop ();

	g_platformInterface = nullptr;

	return hResult;
}

stiHResult CstiPlatformInterface::RestartSystem (EstiRestartReason eRestartReason)
{
	std::stringstream RestartFile;
	std::string DynamicDataFolder;
	stiOSDynamicDataFolderGet (&DynamicDataFolder);
	RestartFile << DynamicDataFolder << "RestartReason";
	FILE *pFile = fopen (RestartFile.str().c_str(), "a");
	if (pFile)
	{
		fprintf (pFile, "%d\n", eRestartReason);
		fclose (pFile);
	}
	//exit(0);
	return stiRESULT_SUCCESS;
}

stiHResult CstiPlatformInterface::RestartReasonGet (EstiRestartReason *peRestartReason)
{
	stiHResult hResult = stiRESULT_SUCCESS; int nRestartReason = estiRESTART_REASON_UNKNOWN;
	std::stringstream RestartFile;
	std::string DynamicDataFolder;
	stiOSDynamicDataFolderGet (&DynamicDataFolder);
	RestartFile << DynamicDataFolder << "RestartReason";
	FILE *pFile = fopen (RestartFile.str().c_str(), "r");
	if (pFile)
	{
		fscanf (pFile, "%d", &nRestartReason);
		fclose (pFile);
		pFile = NULL;
		unlink (RestartFile.str().c_str());
	}
	*peRestartReason = (EstiRestartReason)nRestartReason;
	return hResult;
}

template<typename T>
void callStatusPropertyWrite(std::stringstream& callStatus, const std::string& name, boost::optional<T> value)
{
	callStatus << "\t" << name << " = ";
	if (value.has_value())
	{
		if (typeid(T) == typeid(std::string))
		{
			callStatus << "\"" << value.value() << "\"";
		}
		else
		{
			callStatus << value.value();
		}
	}
	else
	{
		callStatus << "N/A";
	}
	callStatus << "\n";
}

void CstiPlatformInterface::callStatusAdd (std::stringstream & callStatus)
{
	callStatus << "\nCamera Information:\n";
	callStatusPropertyWrite(callStatus, "Description", m_currentCameraInfo.Description);
	callStatusPropertyWrite(callStatus, "Width", m_currentCameraInfo.Width);
	callStatusPropertyWrite(callStatus, "Height", m_currentCameraInfo.Height);
	callStatusPropertyWrite(callStatus, "FrameRate", m_currentCameraInfo.FrameRate);
	callStatusPropertyWrite(callStatus, "Saturation", m_currentCameraInfo.Saturation);
	callStatusPropertyWrite(callStatus, "Brightness", m_currentCameraInfo.Brightness);
	callStatusPropertyWrite(callStatus, "Zoom", m_currentCameraInfo.Zoom);
	callStatusPropertyWrite(callStatus, "Pan", m_currentCameraInfo.Pan);
	callStatusPropertyWrite(callStatus, "Tilt", m_currentCameraInfo.Tilt);
	callStatusPropertyWrite(callStatus, "Roll", m_currentCameraInfo.Roll);
	callStatusPropertyWrite(callStatus, "Exposure", m_currentCameraInfo.Exposure);
	callStatusPropertyWrite(callStatus, "Iris", m_currentCameraInfo.Iris);
	callStatusPropertyWrite(callStatus, "Focus", m_currentCameraInfo.Focus);
	callStatusPropertyWrite(callStatus, "Contrast", m_currentCameraInfo.Contrast);
	callStatusPropertyWrite(callStatus, "Hue", m_currentCameraInfo.Hue);
	callStatusPropertyWrite(callStatus, "Sharpness", m_currentCameraInfo.Sharpness);
	callStatusPropertyWrite(callStatus, "Gamma", m_currentCameraInfo.Gamma);
	callStatusPropertyWrite(callStatus, "ColorEnable", m_currentCameraInfo.ColorEnable);
	callStatusPropertyWrite(callStatus, "WhiteBalance", m_currentCameraInfo.WhiteBalance);
	callStatusPropertyWrite(callStatus, "BacklightCompensation", m_currentCameraInfo.BacklightCompensation);
	callStatusPropertyWrite(callStatus, "Gain", m_currentCameraInfo.Gain);
}

void CstiPlatformInterface::ringStart ()
{
}

void CstiPlatformInterface::ringStop ()
{
}

void CstiPlatformInterface::cameraInformationSet(const CameraInformation& cameraInformation)
{
	m_currentCameraInfo = cameraInformation;
}
