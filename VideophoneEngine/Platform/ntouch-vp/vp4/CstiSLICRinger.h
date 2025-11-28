// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "CstiSLICRingerBase.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <netdb.h>
#include <sys/un.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define slicRingerCrc unsigned char

#define RING_START_CMD 'r'
/* [ r ] [ 5 ] [ d ] [  ] [slicRingerCrc] */
#define DURATION_MSG 'd'
#define DURATION_LEN 5
/* [ r ] [ 8 ] [ u ] [freq] [cad_on] [cad_off] [duration] 
	freq table: 15 20 25 30 35 40 45 50
	cad_on/off: 0 - 255 (0 - 8.19 s, step 8.19/255 s)
	duration: 0 - 255 s */
#define RING_OPTS_MSG 'o'
#define RING_OPTS_LEN 8
/* manual cadence
	[ r ] [ 8 ] [ m ] [freq] [cad_on] [cad_off] [duration] */
#define RING_OPTS_MSG2 'u'

#define RING_STOP_CMD  's'
#define INIT_CMD 'i'
#define QUIT_CMD 'q'

#define SET_STATE_CMD 'S'
#define GET_STATE_CMD 'G'

#define SERVER_PORT     10000
//#define SERVER_NAME     "localhost"
#define SERVER_NAME     "127.0.0.1"

/*! 
 * \class  CstiSLICRinger
 *  
 * \brief SLIC Ringer 
 *  
 */
class CstiSLICRinger : public CstiSLICRingerBase
{
public:

	CstiSLICRinger ();
	~CstiSLICRinger () override;

	CstiSLICRinger (const CstiSLICRinger &other) = delete;
	CstiSLICRinger (CstiSLICRinger &&other) = delete;
	CstiSLICRinger &operator = (const CstiSLICRinger &other) = delete;
	CstiSLICRinger &operator = (CstiSLICRinger &&other) = delete;

private:

	void eventRingerStart () override;
	void eventRingerStop () override;
	void eventDeviceDetect () override;
	void ringSlicDevice ();

	void crcInit();
	slicRingerCrc crcFast(unsigned char const message[], int nBytes);
	slicRingerCrc crcTable[256];

	stiHResult SLICPacketSend(unsigned char *pBuff, int nPktSize);

	stiHResult SocketOpen();
	void SocketClose();

	int m_sockFd = 0;

	vpe::EventTimer m_ringTimer;
};
