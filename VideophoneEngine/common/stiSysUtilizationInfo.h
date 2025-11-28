/*!
 * \file stiSysUtilizationInfo.h
 * \brief See stiSysUtilizationInfo.cpp.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef STISYSUTILIZATIONINFO_H
#define STISYSUTILIZATIONINFO_H


//
// Includes
//
#ifdef stiLINUX

//
// Constants
//

#define NUM_AVERAGES    3
#define NCPUSTATES 4
#define NMEMSTATES 6

//
// Typedefs
//

typedef struct SstiSysUtilizationInfo
{
	int last_pid;
	double load_avg[NUM_AVERAGES];
	int *cpustates;
	int *memory;

	const char **cpustate_names;
	const char **memory_names;
} SstiSysUtilizationInfo;

typedef struct SstiThreadUtilizationInfo
{
	int nTaskId;
	int nTime;
	int nOldTime;
	int nPercentage;

} SstiThreadUtilizationInfo;

//
// Forward Declarations
//

//
// Globals
//

//
// Function Declarations
//

#ifdef __cplusplus
extern "C" {
#endif

int stiSystemInfoInit (void);
void stiSystemInfoGet (struct SstiSysUtilizationInfo * pstInfo);
void stiSystemThreadInfoGet(SstiThreadUtilizationInfo *pstInfo);

#ifdef __cplusplus
}
#endif

#endif // #ifdef stiLINUX
#endif // #define STISYSUTILIZATIONINFO_H
