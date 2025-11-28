////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2019 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: CstiCameraControl
//
//  File Name:	CstiCameraControl.h
//
//  Owner:		Ting-Yu Yang
//
//	Abstract:	
//		See CstiCameraControl.ccp. 
//				
//
////////////////////////////////////////////////////////////////////////////////


#pragma once

//
// Includes
//
#include "CstiEventQueue.h"
#include "stiTrace.h"

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//
namespace WillowPM
{
	class PropertyManager;
};

//
// Globals
//

class CstiCameraControl :  public CstiEventQueue
{
public:

	CstiCameraControl (
		int captureWidth,
		int captureHeight,
		int minZoomLevel,
		int defaultSelfViewWidth,
		int defaultSelfViewHeight);

	CstiCameraControl (const CstiCameraControl &other) = delete;
	CstiCameraControl (CstiCameraControl &&other) = delete;
	CstiCameraControl &operator= (const CstiCameraControl &other) = delete;
	CstiCameraControl &operator= (CstiCameraControl &&other) = delete;

	~CstiCameraControl () override;

	virtual stiHResult Initialize ();

	virtual stiHResult Startup ();

	void Shutdown ();
		
	//
	//	Task functions
	//

	EstiResult CameraMove(
		uint8_t un8ActionBitMask);

	EstiResult CameraPTZGet (
		uint32_t *pun32HorzPercent,
		uint32_t *pun32VertPercent,
		uint32_t *pun32ZoomPercent,
		uint32_t *pun32ZoomWidthPercent,
		uint32_t *pun32ZoomHeightPercent);

	void selfViewSizeRestore ();

	void RemoteDisplayOrientationChanged (
		int width,
		int height);

	/*!
	 * \struct SstiPtzSettings
	 *
	 * \brief This structure defines the settings for the internal
	 *        camera pan/tilt/zoom.
	 */
	struct SstiPtzSettings
	{
		int horzPos;	//!< horizontal pan
		int horzZoom;	//!< horizontal zoom
		int vertPos;	//!< vertical pan
		int vertZoom;	//!< vertical zoom
	};

	SstiPtzSettings ptzSettingsGet () const;

	int defaultSelfViewWidthGet () const;
	int defaultSelfViewHeightGet () const;

	// Event Signals
	CstiSignal<const SstiPtzSettings &> cameraMoveSignal;
	CstiSignal<> cameraMoveCompleteSignal;

private:

	//
	// Event Handlers
	//
	void eventCameraControl (uint8_t actionBitBask);

	CstiSignalConnection::Collection m_signalConnections;

	EstiResult CameraSelfViewSizeSet (
		int horzSize,
		int vertSize,
		bool restore);

private:

	SstiPtzSettings m_stPtzSettings{};

	int m_nHorzCenter{0};
	int m_nVertCenter{0};

	WillowPM::PropertyManager * m_poPM{nullptr};

	vpe::EventTimer m_ptzSettingsTimer;
	vpe::EventTimer m_localMovementTimer;

	uint32_t m_un32CameraCommandTimeout{0};
	uint8_t m_un8CameraStep{0};
	int m_nTimeoutCounter{0};
	uint8_t m_un8CurrActionBitMask{0};
	bool m_bPTZChanged{false};
	int m_nZoomLevelNumerator{256};
	int m_selfViewWidth{0};
	int m_selfViewHeight{0};

	int m_captureWidth {0};
	int m_captureHeight {0};
	int m_minZoomLevel {128};
	const int m_defaultSelfViewWidth;
	const int m_defaultSelfViewHeight;

	void StartLocalMovement ();
	void StopLocalMovement ();

	void zoomIn (
		SstiPtzSettings *pPtzSettings);

	void zoomOut (
		SstiPtzSettings *pPtzSettings);

	void panRight (
		SstiPtzSettings *pPtzSettings);

	void panLeft (
		SstiPtzSettings *pPtzSettings);

	void tiltDown (
		SstiPtzSettings *pPtzSettings);

	void tiltUp (
		SstiPtzSettings *pPtzSettings);

	EstiResult CameraMove ();

	void SavePTZSettings ();

	void SelfViewZoomDimensionsCompute (
		int *horzZoom,
		int *vertZoom);

	void SelfViewPositionCompute (
		SstiPtzSettings *pPtzSettings);

	void ResetSelfViewProperties ();
};
