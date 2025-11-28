/*!
 * \file stiOSSignal.h
 *
 * \brief The OS abstraction for signal access.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef stiOSLINUXSIGNAL_H
#define stiOSLINUXSIGNAL_H

struct stiSIGNAL_ID_s;        // Private implementation
typedef struct stiSIGNAL_ID_s *STI_OS_SIGNAL_TYPE;

EstiResult stiOSSignalCreate (
	STI_OS_SIGNAL_TYPE *pPipe);

EstiResult stiOSSignalClose (
	IN STI_OS_SIGNAL_TYPE *p_fd);

EstiResult stiOSSignalSet (
	IN STI_OS_SIGNAL_TYPE fd);

EstiResult stiOSSignalSet2 (
	STI_OS_SIGNAL_TYPE fd,
	unsigned int unCount);

EstiResult stiOSSignalClear (
	IN STI_OS_SIGNAL_TYPE fd);

int stiOSSignalDescriptor (
	IN STI_OS_SIGNAL_TYPE fd);

#endif /* stiOSLINUXSIGNAL_H */
/* end file stiOSLinuxSignal.h */
