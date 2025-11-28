// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2025 Sorenson Communications, Inc. -- All rights reserved

#include "CstiAudioHandler.h"
#include <BluetoothAudio.h>
#include "stiTrace.h"
#include "AudioPlaybackPipelineVP4.h"
#include "AudioRecordPipelineVP4.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

const std::string AUDIO_INITIALIZATION_STRING {
	"amixer -c " MIC_CARD " cset name='AEC MIC GAIN' 3" REDIRECT_SPEC};

#define EDID_PATH "/sys/devices/platform/display-subsystem/drm/card0/card0-HDMI-A-1/edid"
#define MAX_EDID_SIZE 512
#define EDID_BASE_SIZE 0x80
#define AUDIO_BLOCK 1


/*!
 * \brief Initialize
 * Usually called once for object
 *
 * \return stiHResult
 */
stiHResult CstiAudioHandler::Initialize (
	EstiAudioCodec eAudioCodec,
	CstiMonitorTask *monitor,
	const nonstd::observer_ptr<vpe::BluetoothAudio> &bluetooth)
{
	stiHResult hResult = CstiAudioHandlerBase::Initialize(eAudioCodec, monitor, bluetooth);
	stiTESTRESULT();

	if (system (AUDIO_INITIALIZATION_STRING.c_str ()))
	{
		stiASSERTMSG (false, "Error: Failed to initialize audio system\n");
	}

	m_signalConnections.push_back(m_pMonitor->hdmiHotPlugStateChangedSignal.Connect( [this] ()
		{
			if (m_pMonitor->HdmiHotPlugGet())
			{
				PostEvent([this] { hdmiAudioSupportedUpdate(); });
			}
		}
	));

	m_signalConnections.push_back(m_recordPipeline.dspResetSignal.Connect([this] ()
		{
			if (m_recordPipeline.m_inputType == InputType::internal)
			{
				PostEvent([this] { dspReset(); });
			}
		}
	));

STI_BAIL:

	return hResult;
}

void CstiAudioHandler::dspReset()
{
	int i = system("hbi_test -d 0 -w 0x0006 0xFFFA");
	if (i < 0)
	{
		stiASSERTMSG (false, "error: Can't reset dsp state\n");
	}
}

/*
 * check the edid and see if the audio extension data block exists.  If it does
 * audio is supported
 */
bool CstiAudioHandler::doesEdidSupportHdmiAudio()
{

	char edid[512];			// actually 256 should probably be enough

	int fd;
	int size;			

	bool retVal = false;

	stiDEBUG_TOOL(g_stiAudioHandlerDebug,
		vpe::trace(__func__, ": enter\n");
	);

	if ((fd = open(EDID_PATH, O_RDONLY)) > -1)
	{
		size = read(fd, edid, MAX_EDID_SIZE);
		close(fd);

/*
 * if we don't have anything more than the basic EDID information none of
 * this is necessary
 */
		if (size > EDID_BASE_SIZE)
		{
			uint8_t checksum = 0;
			for (int i = 1; i < EDID_BASE_SIZE; i++)
			{
				checksum += edid[i];
			}

			if (checksum == 0)
			{
/*
 * checksum of the basic EDID block was good.
 * if there are extended blocks (CEA EDID TIMING) the data will continue.
 * byte 0 of the further data is the extension tag.  this is 0x2 for CEA EDID data.
 * byte 1 is the revision number.  the current revision is 0x3.
 * byte 2 contains the offset within this block where DTD information begins.
 * byte 3 contains a bitmask of various supported DTD information.
 * byte 4 thru the offset contains the extension blocks.  the DTD blocks follow immediately
 * after the extension blocks.  AS A RESULT if the offset in byte 2 has a value of 4, the
 * DTD blocks start immediately and there are no extension blocks.  if the value is greater
 * than 4 there are extension blocks, but they end not at the end of this entire block, but
 * at the offset specified in byte 3, and again, the DTD blocks follow immediately.
 * byte 3 has a bit to specify if basic audio is supported.  if this bit isn't set, we
 * still can have an audio extension block that defined audio capabilities.
 */ 
				char *ptr = edid+EDID_BASE_SIZE;
	
				int tag = ptr[0];
				int revision = ptr[1];
				int offset = ptr[2];

				struct bitmask {
					unsigned int formats:4;
					unsigned int YCbCr_4_2_2:1;
					unsigned int YCbCr_4_4_4:1;
					unsigned int basicAudio:1;
					unsigned int underscan:1;
				} *bitMask;

				bitMask = (struct bitmask *)(ptr+3);

				if (tag == 2 && revision == 3)
				{
					if (bitMask->basicAudio != 0)
					{
						retVal = true;
					}
					else
					{
						uint8_t tmp;
						uint8_t code;
						int len;

						ptr += 4;
						offset += EDID_BASE_SIZE;

						while (ptr < edid + offset)
						{
							tmp = *ptr;
							len = tmp & 0x1f;
							code = (tmp >> 5) & 0x7;

							if (code == AUDIO_BLOCK)
							{
								retVal = true;
								break;
							}
							len++;
							ptr += len;
						}
					}
				}
			}
		}
	}

	stiDEBUG_TOOL(g_stiAudioHandlerDebug,
		vpe::trace(__func__, ": returns ", retVal, "\n");
	);
	return retVal;
}
