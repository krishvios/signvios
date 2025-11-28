 // Sorenson Communications Inc. Confidential. --  Do not distribute
 // Copyright 2020 Sorenson Communications, Inc. -- All rights reserved
 #pragma once

 #include "MediaPipeline.h"
 #include <chrono>


 class DecodePipeline : public MediaPipeline
 {
public:
	DecodePipeline () = default;

	DecodePipeline (
		const std::string &name)
	:
		MediaPipeline (name)
	{
	}

	~DecodePipeline () override = default;

	DecodePipeline (const DecodePipeline &other) = delete;
	DecodePipeline (DecodePipeline &&other) = delete;
	DecodePipeline &operator= (const DecodePipeline &other) = delete;
	DecodePipeline &operator= (DecodePipeline &&other) = default;

	void registerBusCallbacks();

	CstiSignal<> decodeErrorSignal;
	CstiSignal<> decodeStreamingErrorSignal;

	std::chrono::time_point<std::chrono::steady_clock> m_lastKeyframeRequestTime {};
 };
