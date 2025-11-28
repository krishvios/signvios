/*!
 * \file IVideoInputVP2.h
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2018-2020 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "BaseVideoInput.h"
#include "PropertyRange.h"

class IVideoInputVP2 : public BaseVideoInput
{
public:

	enum class AmbientLight
	{
		UNKNOWN = 0,
		LOW,
		MEDIUM,
		HIGH
	};

	
	/*!
	 * \brief Get the focus range
	 *
	 * \param pun32Min - The minimum value.
	 * \param pun32Max - The maximum value.
	 *
	 * \return stiRESULT_SUCCESS or an error code.
	 */
	virtual stiHResult FocusRangeGet (
		PropertyRange *range) const = 0;

	/*!
	 *\ brief Force single shot focus on the current display window
	 */
	virtual stiHResult SingleFocus() = 0;

	/*!
	 *\ brief set the focus at the specified range
	 *
	 * return stiRESULT_SUCCESS, or error
	 */
	virtual stiHResult FocusPositionSet(int nPosition) = 0;

	/*!
	 *\ brief get the current focus range
	 *
	 * return the current range value for the focus (-1 on error)
	 */
	virtual int FocusPositionGet() = 0;

	virtual stiHResult contrastLoopStart (std::function<void(int contrast)> callback) = 0;
	virtual stiHResult contrastLoopStop () = 0;

	/*!
	 * \brief Get the brightness range
	 *
	 * \param minBrightness - The minimum brightness.
	 * \param maxBrightness - The maximum brightness.
	 *
	 * \return stiRESULT_SUCCESS or an error code.
	 */
	virtual stiHResult BrightnessRangeGet (
		PropertyRange *range) const = 0;

	/*!
	 * \brief Get the default brightness
	 *
	 * \return The default brightness.
	 */
	virtual int BrightnessGet () const = 0;

	/*!
	 * \brief Set the brightness to be used
	 *
	 * \param brightness - the brightness to be used by the Video Input driver.
	 *
	 * \return stiRESULT_SUCCESS or an error code.
	 */
	virtual stiHResult BrightnessSet (
		int brightness) = 0;

	/*!
	 * \brief Get the saturation range being used by the driver.
	 *
	 * \param minSaturation - the minimum saturation in use.
	 * \param maxSaturation - the maximum saturation in use.
	 *
	 * \return stiRESULT_SUCCESS or an error code.
	 */
	virtual stiHResult SaturationRangeGet (
		PropertyRange *range) const = 0;

	/*!
	 * \brief Get the default staturation
	 *
	 * \return the default saturation.
	 */
	virtual int SaturationGet () const = 0;

	/*!
	 * \brief Set the saturation to be used by the driver.
	 *
	 * \param saturation - the saturation to be used by the driver.
	 *
	 * \return stiRESULT_SUCCESS or an error code.
	 */
	virtual stiHResult SaturationSet (
		int saturation) = 0;

	/*!
	 * \brief Get RCU connected status
	 * \remarks This method will only be implemented for APP_NTOUCH_VP2
	 *
	 * \return estiTRUE or estiFALSE indicating the RCU is connected or not.
	 */
	virtual EstiBool RcuConnectedGet () const = 0;

	/*!
	* \brief Get Advanced Status
	* \remarks This method returns the advanced status of the driver.
	*
	* \param pSettings - a pointer to a structure that will be updated by the driver.
	*
	* \return stiRESULT_SUCCESS or an error code.
	*/
	virtual stiHResult AdvancedStatusGet (
		SstiAdvancedVideoInputStatus &) const = 0;

	//
	// Camera methods
	//
	virtual EstiResult cameraMove(
		uint8_t un8ActionBitMask) = 0;

	virtual EstiResult cameraPTZGet (
		uint32_t *pun32HorzPercent,
		uint32_t *pun32VertPercent,
		uint32_t *pun32ZoomPercent,
		uint32_t *pun32ZoomWidthPercent,
		uint32_t *pun32ZoomHeightPercent) = 0;

	/*!
	 * \brief Gets the current ambient light reading.
	 *
	 */
	virtual stiHResult ambientLightGet (
			AmbientLight *ambiance,
			float *rawVal) = 0;

	virtual void videoRecordToFile() = 0;

#if APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
	//TODO: what about vp4? 
	/*!
	 *\ Set self view widget
	 *
	 * return stiRESULT_SUCCESS, or error
	 */
	virtual stiHResult SelfViewWidgetSet (
		void * QQuickItem) = 0;

	/*!
	 *\ Tell self view to play
	 *
	 * return stiRESULT_SUCCESS, or error
	 */
	virtual stiHResult PlaySet (
		bool play) = 0;

#endif
	virtual stiHResult EncodePipelineReset ( ) = 0;
	virtual stiHResult EncodePipelineDestroy ( ) = 0;

	virtual float aspectRatioGet () const = 0;
	
	virtual void selfViewEnabledSet (
		bool enabled) = 0;

	virtual void systemSleepSet (
		bool enabled) = 0;
	
	// TODO: Intel bug. This is needed because gstreamer/icamerasrc/libcamhal
	// have a problem when we set system time. (libcamhal timeout)
	virtual void currentTimeSet (
		time_t currentTime) = 0;
};
