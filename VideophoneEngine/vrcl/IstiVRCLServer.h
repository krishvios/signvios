// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef ISTIVRCLSERVER_H
#define ISTIVRCLSERVER_H


class IstiVRCLServer
{
public:

	/*!
	 * \brief Set server port
	 *
	 * \param un16Port
	 *
	 * \return stiHResult
	 */
	virtual stiHResult PortSet (
		uint16_t un16Port) = 0;

	/*!
	 * \brief Send Image 
	 * 
	 * \param nWidth 
	 * \param nHeight 
	 * \param pchRawData 
	 * 
	 * \return EstiResult 
	 */
	virtual EstiResult B64ImageSend (
		int nWidth,
		int nHeight,
		const char * pchRawData) = 0;

protected:

	virtual ~IstiVRCLServer () {}

};

#endif // ISTIVRCLSERVER_H
