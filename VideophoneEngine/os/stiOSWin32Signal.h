/*!
 * \file stiWin32Signal.h
 *
 * \brief The Win32 OS abstraction for signal access.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef stiOSWIN32SIGNAL_H
#define stiOSWIN32SIGNAL_H

//
// Typedefs
//

typedef struct stiSIGNAL_ID_s *STI_OS_SIGNAL_TYPE;


EstiResult stiOSSignalCreate (
	STI_OS_SIGNAL_TYPE *pPipe);

EstiResult stiOSSignalClose (
    IN STI_OS_SIGNAL_TYPE *p_fd);

EstiResult stiOSSignalSet (
    IN STI_OS_SIGNAL_TYPE fd);

EstiResult stiOSSignalClear (
    IN STI_OS_SIGNAL_TYPE fd);

int stiOSSignalDescriptor (
    IN STI_OS_SIGNAL_TYPE fd);



#endif /* stiOSWIN32XSIGNAL_H */
/* end file stiOSWin32Signal.h */
