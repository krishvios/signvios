/*!
 * \file IVideoOutputVP.h
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2020 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "BaseVideoOutput.h"

class IVideoOutputVP : public BaseVideoOutput
{
public:

	IVideoOutputVP (const IVideoOutputVP &other) = delete;
	IVideoOutputVP (IVideoOutputVP &&other) = delete;
	IVideoOutputVP &operator= (const IVideoOutputVP &other) = delete;
	IVideoOutputVP &operator= (IVideoOutputVP &&other) = delete;


	/*!
	 * \brief Get instance 
	 * 
	 * \return IstiVideoOutput* 
	 */
	static IVideoOutputVP *InstanceGet ();

	/*!
	 * \brief Loads a video 
	 */
	 virtual stiHResult videoFileLoad(
		 std::string server,
		 std::string fileGUID,
		 std::string clientToken,
		 int maxDownloadSpeed,
		 bool loopVideo) = 0;

	/*!
	 * \brief Starts playback or changes speed of playback 
	 */
	 virtual stiHResult videoFilePlay(
		 EPlayRate speed) = 0;

	/*!
	 * \brief Seeks to the specified location in the file. 
	 */
	 virtual stiHResult videoFileSeek(
		 uint32_t seconds) = 0;

	/*!
	 * \brief Starts the video screen saver
	 */
//	 virtual stiHResult ScreenSaverVideoFilePlay(
//		 const std::string filePath) = 0;

	/*!
	 * \brief Stops the video screen saver
	 */
	 virtual stiHResult videoFileStop() = 0;

	/*!
	 * \brief set the remote view widget for qml display
	 */
	virtual void RemoteViewWidgetSet (
		void *qquickItem) = 0;

	/*!
	 * \brief set the playback view widget for qml display
	 */
	virtual void playbackViewWidgetSet (
		void *qquickItem) = 0;

	virtual bool videoOutputRunning() = 0;

public:
	virtual ISignal<>& videoFileStartFailedSignalGet () = 0;
	virtual ISignal<>& videoFileCreatedSignalGet () = 0;
	virtual ISignal<>& videoFileReadyToPlaySignalGet () = 0;
	virtual ISignal<>& videoFileEndSignalGet () = 0;
	virtual ISignal<const std::string>& videoFileClosedSignalGet () = 0;
	virtual ISignal<uint64_t, uint64_t, uint64_t>& videoFilePlayProgressSignalGet () = 0;
	virtual ISignal<>& videoFileSeekReadySignalGet () = 0;

protected:

	IVideoOutputVP () = default;
	~IVideoOutputVP () override = default;

private:

};
