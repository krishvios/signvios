#ifndef __OVUTILS_H
#define __OVUTILS_H

#include "stiError.h"
#include <stdio.h>
#include <inttypes.h>

extern const char g_szCameraDeviceName[];

void ValueSet (
	uint32_t un32Address1,
	uint32_t un32NumBits,
	uint32_t un32Value);

void ValueSet (
	uint32_t un32Address1,
	uint32_t un32Address2,
	uint32_t un32NumBits,
	uint32_t un32Value);

uint32_t ValueGet (
	uint32_t un32Address1,
	uint32_t un32NumBits);

uint32_t ValueGet (
	uint32_t un32Address1,
	uint32_t un32Address2,
	uint32_t un32NumBits);

uint32_t ValueGet (
	uint32_t un32Address1,
	uint32_t un32Address2,
	uint32_t un32Address3,
	uint32_t un32NumBits);

stiHResult Ov640RegisterSet (
	uint32_t un32Register,
	uint8_t un8RegValue);

stiHResult Ov640RegisterGet (
	uint32_t un32Register,
	uint8_t *pun8RegValue);

char *SkipWhiteSpace (
	char *pCurrent);

char *SkipUntilWhiteSpace (
	char *pCurrent);

char *SkipUntilCharacter (
	char *pCurrent,
	char Character);

char *SkipToEndOfLine (
	char *pCurrent);

int xioctl (
	int nIoctlFd,
	int nCommand,
	void *pParam);

int CameraOpen ();

void CameraClose ();

#endif // __OVUTILS_H
