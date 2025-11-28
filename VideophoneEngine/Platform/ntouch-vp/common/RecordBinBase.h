#pragma once

#include <gst/gst.h>
#include "stiDefs.h"
#include "stiSVX.h"
#include "CstiTimer.h"
#include "GStreamerBin.h"
#include "CstiEventQueue.h"

class RecordBinBase : public CstiEventQueue, public GStreamerBin
{
public:

	RecordBinBase ();
	~RecordBinBase () override;

	RecordBinBase (const RecordBinBase &other) = delete;
	RecordBinBase (RecordBinBase &&other) = delete;
	RecordBinBase &operator= (const RecordBinBase &other) = delete;
	RecordBinBase &operator= (RecordBinBase &&other) = delete;

	void create ();

	void recordStart();
	void recordStop();
	stiHResult recordPositionGet(
		gint64 *position,
		gint64 *timeScale);
	void initialize(
		bool recordFastStartFile);

private:
	GstClockTime m_recordStartTime {0};

	static gboolean recordBusCallback (
		GstBus *pGstBus,
		GstMessage *pGstMessage,
		gpointer pUserData);

	void eventCleanupFileSink();
	void eventRecordUpdateTimeout();

	GStreamerElement m_mp4Parse;
	GStreamerElement m_mp4FileMux;
//	GStreamerPad m_mp4FileMuxSrcPad;
	GStreamerElement m_mp4FileSink;
	GstClock *m_pipelineClock {nullptr};

	bool m_recordingData {false};
	const int RECORD_UPDATE_TIMEOUT_MS = 250;
	CstiTimer m_recordUpdateTimer {RECORD_UPDATE_TIMEOUT_MS};

	CstiSignalConnection::Collection m_signalConnections;
	std::string m_recordFilePath;
	std::string m_recordFile;
};
