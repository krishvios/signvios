#include "RecordBinBase.h"
#include "IstiVideoInput.h"
#include <sys/stat.h>
#include <sys/types.h>

#define DEFAULT_FILE_NAME "recordedFile.h264"
#define DEFAULT_RECORD_DIR "VideoRecord/"

RecordBinBase::RecordBinBase()
:
	CstiEventQueue ("RecordBinBase"),
	GStreamerBin ("recordPipeline_leg")
{
	m_recordUpdateTimer.TypeSet(CstiTimer::Type::Repeat);

	m_signalConnections.push_back (m_recordUpdateTimer.timeoutEvent.Connect(
		[this]() {
			PostEvent([this]{eventRecordUpdateTimeout();});
		}));
}

RecordBinBase::~RecordBinBase()
{
	CstiEventQueue::StopEventLoop();

	// Make sure the timer is shutdown.
	m_recordUpdateTimer.Stop();

	// Unref the pipelineClock if we have one.
	if (m_pipelineClock)
	{
		gst_object_unref(m_pipelineClock);
		m_pipelineClock = nullptr;
	}
}

void RecordBinBase::create ()
{
	auto hResult = stiRESULT_SUCCESS;
	auto result = false;
	std::string location;
	GStreamerPad sinkPad;

	// Make sure we have the record directory.
	stiOSDynamicDataFolderGet(&m_recordFilePath);
	m_recordFilePath += DEFAULT_RECORD_DIR;
	mkdir(m_recordFilePath.c_str(), 0644);
 
	result = CstiEventQueue::StartEventLoop();
	stiTESTCOND(result, stiRESULT_ERROR);

	m_mp4Parse = GStreamerElement ("h264parse", "h264_parse");
	stiTESTCOND(m_mp4Parse.get (), stiRESULT_ERROR);

	m_mp4FileMux = GStreamerElement ("mp4mux", "mp4_mux");
	stiTESTCOND(m_mp4FileMux.get (), stiRESULT_ERROR);

	elementAdd(m_mp4Parse);
	elementAdd(m_mp4FileMux);

	sinkPad = m_mp4Parse.staticPadGet("sink");
	stiTESTCOND(sinkPad.get(), stiRESULT_ERROR);

	result = ghostPadAdd ("sink", sinkPad);
	stiTESTCOND(result, stiRESULT_ERROR);

	result = m_mp4Parse.link(m_mp4FileMux);
	stiTESTCOND(result, stiRESULT_ERROR);

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		clear ();
	}
}


void RecordBinBase::recordStart()
{
	auto hResult = stiRESULT_SUCCESS;

	m_recordUpdateTimer.Start();

	if (!m_pipelineClock)
	{
		m_pipelineClock = clockGet();
		stiTESTCOND(m_pipelineClock, stiRESULT_ERROR);
	}
	m_recordStartTime = gst_clock_get_time(m_pipelineClock);

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		IstiVideoInput::InstanceGet()->videoRecordErrorSignalGet().Emit();
	}

}


void RecordBinBase::recordStop()
{
	eventSend(gst_event_new_eos());
	
	// Stop the record timer.
	m_recordUpdateTimer.Stop();
	
	if (m_pipelineClock)
	{
		gst_object_unref(m_pipelineClock);
		m_pipelineClock = nullptr;
	}
	m_recordStartTime = 0;
}


stiHResult RecordBinBase::recordPositionGet (
	gint64 *position,
	gint64 *timeScale)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	gint64 pos = -1;

	GstClockTime currentTime = gst_clock_get_time(m_pipelineClock);

	if (currentTime > m_recordStartTime)
	{
		pos = currentTime - m_recordStartTime;
	}

	*position = pos;
	*timeScale = 1000000000;

	return hResult;
}

void RecordBinBase::initialize(
	bool recordFastStartFile)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	GstStateChangeReturn stateChangeResult = GST_STATE_CHANGE_SUCCESS;
	std::string location;
	bool result = true;
	GStreamerPad sinkPad;

	// If we already have a fileSink we can't update the file location and 
	// are probably in a bad state.
	stiTESTCOND(!m_mp4FileSink.get (), stiRESULT_ERROR);

	m_mp4FileSink = GStreamerElement("filesink", "mp4_fileSink");
	stiTESTCOND(m_mp4FileSink.get (), stiRESULT_ERROR);

	m_mp4FileSink.propertySet("async", FALSE);

	elementAdd(m_mp4FileSink);

	result = m_mp4FileMux.link(m_mp4FileSink);
	stiTESTCOND(result, stiRESULT_ERROR);

	// Add a probe to watch for the EOS message.
	sinkPad = m_mp4FileSink.staticPadGet("sink");
	stiTESTCOND(sinkPad.get (), stiRESULT_ERROR);
	
	sinkPad.addProbe(
		static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM),
		[this](GStreamerPad pad, GstPadProbeInfo *probeInfo)
		{
			if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (probeInfo)) == GST_EVENT_EOS)
			{
				PostEvent([this]{ eventCleanupFileSink(); });
			}

			return GST_PAD_PROBE_PASS;
		});

	location = m_recordFilePath;
	location += DEFAULT_FILE_NAME;
	m_mp4FileSink.propertySet("location", location);

	m_mp4FileMux.propertySet("faststart", recordFastStartFile);

	stateChangeResult = stateSet(GST_STATE_PLAYING);
	stiTESTCOND(stateChangeResult == GST_STATE_CHANGE_SUCCESS, stiRESULT_ERROR);

STI_BAIL:
	if (stiIS_ERROR(hResult))
	{
		IstiVideoInput::InstanceGet()->videoRecordErrorSignalGet().Emit();
	}
}


void RecordBinBase::eventCleanupFileSink()
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG(hResult);

	// Set the pipeline state to NULL.
	stateSet(GST_STATE_NULL);

	// Remove the sink so we can create a new one next time we need to record.
	elementRemove(m_mp4FileSink);
	m_mp4FileSink.clear();

	// Create a new file name.
	clock_t clockTicks = clock();
	std::string newFileName = "recordedMsg_" + std::to_string(clockTicks) + ".h264";
	
	// Rename the file.
	std::string command = "mv ";
	command += m_recordFilePath + DEFAULT_FILE_NAME + " " + m_recordFilePath + newFileName;

	if (system(command.c_str()))
	{
		// Even by doing nothing, this gets rid of a compiler warning.
	}

	IstiVideoInput::InstanceGet()->videoRecordFinishedSignalGet().Emit(m_recordFilePath, newFileName);
}


void RecordBinBase::eventRecordUpdateTimeout()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	gint64 position = 0;
	gint64 timeScale = 0;

	hResult = recordPositionGet (&position, 
			  					 &timeScale);
	stiTESTRESULT ();

	if (position != -1)
	{
		// Time scale is in nano seconds.
		IstiVideoInput::InstanceGet()->videoRecordPositionSignalGet().Emit(position, timeScale);
	}

STI_BAIL:

	return;
}


