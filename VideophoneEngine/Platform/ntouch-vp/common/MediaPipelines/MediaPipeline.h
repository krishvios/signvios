/*!\brief A base class for media pipelines.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "GStreamerPipeline.h"
#include "CstiSignal.h"
#include <functional>
#include <map>
#include <string>


class MediaPipeline : public GStreamerPipeline
{
public:

	MediaPipeline () = default;

	MediaPipeline (
		const std::string &name)
	:
		GStreamerPipeline (name)
	{
	}

	~MediaPipeline () override = default;

	MediaPipeline (const MediaPipeline &other) = delete;
	MediaPipeline (MediaPipeline &&other) = delete;
	MediaPipeline &operator= (const MediaPipeline &other) = delete;
	MediaPipeline &operator= (MediaPipeline &&other) = default;

	void busWatchEnable ()
	{
		busWatchAdd (busCallback, this);
	}

	CstiSignal<> pipelineErrorSignal;
	CstiSignal<> cameraErrorSignal;
	CstiSignal<> dspResetSignal;

protected:

	static gboolean busCallback (
		GstBus *pGstBus,
		GstMessage *pGstMessage,
		gpointer pUserData);

	virtual void busCallbackHandle (GstMessage *pGstMessage);

	void registerMessageCallback (
		GstMessageType messageType,
		std::function<void(GstMessage &message)> callback);

	void registerElementMessageCallback (
		const std::string &messageName,
		std::function<void(const GstStructure &)> callback);

private:

	std::map<GstMessageType, std::function<void(GstMessage &)>> m_messageCallback;
	std::map<std::string, std::function<void(const GstStructure &)>> m_elementMessageCallback;
};
