/*!
* \file CDeviceController.h
* \brief This file defines a base class for the device controller.
*
* Class declaration a base class for the Willow/Redwood device controller. 
* The class for video/audio device controller will be a derived class of this class
*
*
* \author Ting-Yu Yang
*
*  Copyright (C) 2003-2004 by Sorenson Media, Inc.  All Rights Reserved
*/

#ifndef CDEVICECONTROLLER_H
#define CDEVICECONTROLLER_H

//
// Includes
//
#ifndef WIN32
#include <unistd.h>
#endif
#include <fcntl.h>
#include "error.h"
#include "stiOS.h"
#include "stiSVX.h"

//
// Constants
//
const int nMaxSettingBufSize = 256;
const int nMaxRequestBufSize = 256;
//
// Typedefs
//
typedef struct SsmdIndicatorSettings
{
	EstiToneMode eSlicMode;
//	EstiSwitch eRingSwitch;
	EstiToneMode eBuzzerMode;
	EstiSwitch ePowerLED;
	EstiSwitch eStatusLED;
	EstiSwitch eCameraActiveLED;
	uint8_t un8LightRingBitMask;      
} SsmdIndicatorSettings;

typedef enum EsmdTintChannel
{
  esmdTINT_BLUE,
  esmdTINT_RED,
} EsmdTintChannel;

typedef struct SsmdCameraParam
{
	uint8_t un8Camera;		// video source number (0 - 15)

	// TODO: camera parameter set
} SsmdCameraParam;

typedef struct SsmdVideoSize
{
	uint32_t unRows;				// number of row pixels
	uint32_t unColumns;			// number of column pixels
} SsmdVideoSize;

typedef enum EsmdControlMsg
{
	esmdCTRLMSG_NONE = 0,
	esmdCTRLMSG_HANDSET_ON,
	esmdCTRLMSG_HANDSET_OFF,
	esmdCTRLMSG_KEYFRAME,
	esmdCTRLMSG_RESET_TO_DEFAULT,
	esmdCTRLMSG_KEY_0,
	esmdCTRLMSG_KEY_1,
	esmdCTRLMSG_KEY_2,
	esmdCTRLMSG_KEY_3,
	esmdCTRLMSG_KEY_4,
	esmdCTRLMSG_KEY_5,
	esmdCTRLMSG_KEY_6,
	esmdCTRLMSG_KEY_7,
	esmdCTRLMSG_KEY_8,
	esmdCTRLMSG_KEY_9,
	esmdCTRLMSG_KEY_S,	// start sign
	esmdCTRLMSG_KEY_P,	// pound sign
} EsmdControlMsg;



// 
// Forward Declarations
//

//
// Globals
//

/*!
*  \brief Device Controller Class
*
*  Gives the public (and private) APIs and member variables
*/
class CDeviceController
{

public:
	CDeviceController ();
	virtual ~CDeviceController ();

	/*!
	* \brief Open a device driver
	* \param const char * pDeviceName - name of device to open
	* \retval 0 - success, otherwize error occured
	*/
	virtual HRESULT DeviceOpen ();
	
	/*!
	* \brief Close the device driver
	* \retval 0 - success, otherwize error occured
	*/
	HRESULT DeviceClose ();


	/*!
	* \brief Get the device's file description
	* \retval positive number - success, otherwise error occured
	*/
	int GetDeviceFD ();

protected:
	
	stiHResult Lock ();
	void Unlock ();

	// File descriptor for device driver
	int m_nfd;
	stiMUTEX_ID m_LockMutex;

	unsigned char aSettingBuf[nMaxSettingBufSize];
	unsigned char aRequestBuf[nMaxRequestBufSize];  

};


#endif
