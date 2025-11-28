// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef CstiVideoOutput_H
#define CstiVideoOutput_H

#include "CstiVideoOutputBase2.h"
#include <sstream>
#include <list>
#include "CstiVideoInput.h"
#include "CstiVideoPlaybackFrame.h"

#if APPLICATION == APP_NTOUCH_VP2
#define MIN_DISPLAY_WIDTH 128
#define MIN_DISPLAY_HEIGHT 96
#define MAX_DISPLAY_WIDTH 1920
#define MAX_DISPLAY_HEIGHT 1080
#endif

#if APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
#define MAX_DISPLAY_WIDTH 1920
#define MAX_DISPLAY_HEIGHT 1080
#endif

class CstiMonitorTask;
class CstiCECControl;


class CstiVideoOutput : public CstiVideoOutputBase2
{
public:

	CstiVideoOutput ();

	CstiVideoOutput (const CstiVideoOutput &other) = delete;
	CstiVideoOutput (CstiVideoOutput &&other) = delete;
	CstiVideoOutput &operator= (const CstiVideoOutput &other) = delete;
	CstiVideoOutput &operator= (CstiVideoOutput &&other) = delete;

	~CstiVideoOutput () override = default;
	
	stiHResult Initialize (
		CstiMonitorTask *monitorTask,
		CstiVideoInput *pVideoInput,
		std::shared_ptr<CstiCECControl> cecControl);

	std::string decodeElementName(EstiVideoCodec eCodec) override;

#if APPLICATION == APP_NTOUCH_VP2
	void DisplaySettingsGet (
		SstiDisplaySettings * pDisplaySettings) const override;

	stiHResult DisplaySettingsSet (
		const SstiDisplaySettings *pDisplaySettings) override;

	stiHResult DisplayModeCapabilitiesGet (
		uint32_t *pun32DisplayModeCapabilitiesBitMask) override;

	stiHResult DisplayModeSet (
		EDisplayModes eDisplayMode) override;
#endif

	bool CECSupportedGet () override;
	bool DisplayPowerGet () override;
	stiHResult DisplayPowerSet (bool bOn) override;	

	stiHResult VideoCodecsGet (
		std::list<EstiVideoCodec> *pCodecs) override;

	stiHResult CodecCapabilitiesGet (
		EstiVideoCodec eCodec,
		SstiVideoCapabilities *pstCaps) override;

private:

	void platformVideoDecodePipelineCreate () override;

	std::string m_FileName;
	unsigned int m_unFileCounter {0};
	FILE *m_pOutputFile {nullptr};

	stiHResult CECStatusCheck();
	static EstiPowerStatus CECDisplayPowerGet();
	
#if APPLICATION == APP_NTOUCH_VP2
	void EventDisplaySettingsSet (
		std::shared_ptr<SstiDisplaySettings> displaySettings);
	
	bool ImageSettingsValidate (
		const IstiVideoOutput::SstiImageSettings *pImageSettings);
	unsigned int maxDisplayWidthGet ()
	{
		return MAX_DISPLAY_WIDTH;
	}

	unsigned int minDisplayWidthGet ()
	{
		return MIN_DISPLAY_WIDTH;
	}

	unsigned int maxDisplayHeightGet ()
	{
		return MAX_DISPLAY_HEIGHT;
	}

	unsigned int minDisplayHeightGet ()
	{
		return MIN_DISPLAY_HEIGHT;
	}
	
#endif
	
	//
	// Event Handlers
	//
	void eventDisplayResolutionSet (uint32_t, uint32_t) override;

	CstiMonitorTask *m_pMonitorTask {nullptr};

	vpe::EventTimer m_delayDecodePiplineCreationTimer;
	CstiSignalConnection::Collection m_videoOutputConnections;
};

#endif // CstiVideoOutput_H
