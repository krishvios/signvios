////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	stiClientValidate
//
//	File Name:	stiClientValidate.h
//
//	Owner:		Scot L. Brooksby
//
//	Abstract:
//		See stiClientValidate.cpp
//
////////////////////////////////////////////////////////////////////////////////
#ifndef STICLIENTVALIDATE_H
#define STICLIENTVALIDATE_H

//
// Includes
//
#include "stiDefs.h"

//
// Constants
//
#define MAX_KEY			30
#define MAX_PASSWORD	30

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//

//
// Function Declaration
//

//Android has problems trying to use ifaddrs so we need to use the old method.
#if APPLICATION != APP_NTOUCH_ANDROID
EstiResult stiClientValidate (
	int nAddressFamily,
	const uint32_t *pun32ClientIP,
	const uint32_t *pun32ServerIP,
	const char *pszValidationKey);

void stiKeyCreate (
	int nAddressFamily,
	const uint32_t *pun32ClientIP,
	const uint32_t *pun32ServerIP,
	char *pszKey);
#else
EstiResult stiClientValidate (
	uint32_t un32ClientIP,
	uint32_t un32ServerIP,
	const char * szValidationKey);

void stiKeyCreate (
	uint32_t un32ClientIP,
	uint32_t un32ServerIP,
	char *pszKey);
#endif

void stiKeyCreate (
	uint32_t un32Seed,
	char *pszKey);

void stiPasswordCreate (
	const char * szKey1,
	const char * szKey2,
	char *pszPassword);

EstiResult stiPasswordValidate (
	const char * szValidationKey1,
	const char * szValidationKey2,
	const char * szPassword);

uint32_t stiTransform (
	uint32_t un32Seed);

#endif // STICLIENTVALIDATE_H
// end file stiClientValidate.h
