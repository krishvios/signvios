#include "stiOVUtils.h"
#include <sys/ioctl.h>
#include <errno.h>
#include <media/ov640_v4l2.h>
#include <fcntl.h>
#include "stiDefs.h"


const char g_szCameraDeviceName[] = "/dev/video0";

int g_nCameraControlFd = -1;

char *SkipWhiteSpace (
	char *pCurrent)
{
	while (*pCurrent != '\0' && isspace (*pCurrent))
	{
		pCurrent++;
	}

	return pCurrent;
}


char *SkipUntilWhiteSpace (
	char *pCurrent)
{
	char *pEnd = pCurrent + 1;

	while (*pEnd != '\0' && !isspace (*pEnd))
	{
		pEnd++;
	}

	return pEnd;
}


char *SkipUntilCharacter (
	char *pCurrent,
	char Character)
{
	char *pEnd = pCurrent + 1;

	while (*pEnd != '\0' && *pEnd != Character)
	{
		pEnd++;
	}

	return pEnd;
}

char *SkipToEndOfLine (
	char *pCurrent)
{
	while (*pCurrent != '\0' && *pCurrent != '\n')
	{
		pCurrent++;
	}

	return pCurrent;
}

int xioctl (
	int nIoctlFd,
	int nCommand,
	void *pParam)
{
	int nReturnVal;

	do
	{
		nReturnVal = ioctl (nIoctlFd, nCommand, pParam);

	} while (nReturnVal == -1 && errno == EINTR);

	return (nReturnVal);
}


/*!\brief Sets a register on the OV640
 *
 * \return stiHResult
 */
stiHResult Ov640RegisterSet (
	uint32_t un32Register,
	uint8_t un8RegValue)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult = -1;
	struct v4l2_dbg_register regtr;

	memset (&regtr, 0, sizeof(regtr));

	regtr.reg  = un32Register;
	regtr.size = 1;
	regtr.val = un8RegValue;

	if (g_nCameraControlFd > -1)
	{
		nResult = xioctl (g_nCameraControlFd, VIDIOC_DBG_S_REGISTER, &regtr);
		stiTESTCOND (nResult >= 0, stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
}


/*!\brief Gets a register from the OV640
 *
 * \return stiHResult
 */
stiHResult Ov640RegisterGet (
	uint32_t un32Register,
	uint8_t *pun8RegValue)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult = -1;
	struct v4l2_dbg_register reg;

	memset (&reg, 0, sizeof(reg));

	reg.reg  = un32Register;
	reg.size = 1;

	if (g_nCameraControlFd > -1)
	{
		nResult = xioctl (g_nCameraControlFd, VIDIOC_DBG_G_REGISTER, &reg);
		stiTESTCOND (nResult >= 0, stiRESULT_ERROR);
	}

	if (nResult >= 0)
	{
		*pun8RegValue = reg.val;
	}

STI_BAIL:

	return hResult;
}


void ValueSet (
	uint32_t un32Address1,
	uint32_t un32NumBits,
	uint32_t un32Value)
{
	Ov640RegisterSet (un32Address1, un32Value & ((1 << un32NumBits) - 1));
}


void ValueSet (
	uint32_t un32Address1,
	uint32_t un32Address2,
	uint32_t un32NumBits,
	uint32_t un32Value)
{
	Ov640RegisterSet (un32Address1, (un32Value >> 8) & ((1 << (un32NumBits -  8)) - 1));
	Ov640RegisterSet (un32Address2, un32Value & 0xff);
}


uint32_t ValueGet (
	uint32_t un32Address1,
	uint32_t un32NumBits)
{
	uint8_t un8Register1;
	uint32_t un32Value;

	Ov640RegisterGet (un32Address1, &un8Register1);

	un32Value = (un8Register1) & ((1 << un32NumBits) - 1);

	return un32Value;
}


uint32_t ValueGet (
	uint32_t un32Address1,
	uint32_t un32Address2,
	uint32_t un32NumBits)
{
	uint8_t un8Register1;
	uint8_t un8Register2;
	uint32_t un32Value;

	Ov640RegisterGet (un32Address1, &un8Register1);
	Ov640RegisterGet (un32Address2, &un8Register2);

	un32Value = ((un8Register1 << 8) | un8Register2) & ((1 << un32NumBits) - 1);

	return un32Value;
}


uint32_t ValueGet (
	uint32_t un32Address1,
	uint32_t un32Address2,
	uint32_t un32Address3,
	uint32_t un32NumBits)
{
	uint8_t un8Register1;
	uint8_t un8Register2;
	uint8_t un8Register3;
	uint32_t un32Value;

	Ov640RegisterGet (un32Address1, &un8Register1);
	Ov640RegisterGet (un32Address2, &un8Register2);
	Ov640RegisterGet (un32Address3, &un8Register3);

	un32Value = ((un8Register1 << 16) | (un8Register2 << 8) | un8Register3) & ((1 << un32NumBits) - 1);

	return un32Value;
}

void CameraClose ()
{
	if (g_nCameraControlFd >= 0)
	{
		close (g_nCameraControlFd);
		g_nCameraControlFd = -1;
	}
}

int CameraOpen ()
{
	CameraClose ();
	
	g_nCameraControlFd = open (g_szCameraDeviceName, O_RDWR);
	
	return g_nCameraControlFd;
}
