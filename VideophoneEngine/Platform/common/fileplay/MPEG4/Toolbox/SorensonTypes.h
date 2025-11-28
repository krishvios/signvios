//////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	File Name:	SorensonTypes.h
//
//  Abstract:	File contains typedefs that span all core codecs (MPEG-4, SV3,
//				etc) and all architecture interfaces (QT, VFW, etc).  This
//				header file can be included anywhere and should not contain
//				any architecture interface or core codec specific items.
//				Items other than typedefs (type_t) probably belong in another
//				header file: SMCommon.h
//
//////////////////////////////////////////////////////////////////////////////
#ifndef SORENSONTYPES_H
#define SORENSONTYPES_H

#include <cinttypes>

typedef long bool_t;

typedef struct vec2_s
{
	int nX;
	int nY;
} vec2_t;

typedef int32_t SMResult_t;

#define kSorensonY420CodecType	'syuv'

#ifdef WIN32
	#define MAX_FILE_NAME	260
#else
	#define MAX_FILE_NAME	64
#endif // WIN32


#define MAX_MEDIA_KEY_LENGTH	255

enum CompressionSpeeds_e
{
	eNormalSpeed = 0,
	eFastSpeed = 1,
};

enum GMCEnabling_t
{
	eGMCDisabled = 0,
	eGMCInternalEstimation = 1,
	eGMCExternalEstimation = 2,
};

enum AlphaType_e
{
	eRectangular   = 0,	// Regular rectangular video
	eBinary		   = 1,	// Binary shape coding AND regular video
	eBinaryOnly	   = 2,	// Binary shape coding only, no texture in foreground
	eGrayscale	   = 3,	// Not used
	eGrayscaleOnly = 4,	// Not used
	eAlphaTypeUnknown = (unsigned long) 0x10000000 	//to make sure enums always intUnknown = (unsigned long) 0x10000000 	//to make sure enums always int 
};

enum AlphaTypeExt_e
{
	eNoExt			 = 0,
	eScreenToTexture = 1,	// colorkey the background, compress regular video in foreground
	eScreenToGray	 = 2,	// Not used
	eScreenToBinary  = 3,	// colorkey the background only, no compression of texture
	eAlphaTypeExtUnknown = (unsigned long) 0x10000000 	//to make sure enums always intUnknown = (unsigned long) 0x10000000 	//to make sure enums always int 
};

// Structure that represents a FSSpec 
typedef struct smfsspec
{
	short vRefNum;						// Drive ID
	long parID;							// Directory ID
	unsigned char name[MAX_FILE_NAME];	// File name
} SMFSSpec;

typedef struct
{
	int AvgScreenColor[3];	// avgscreencolor[] and spread determined upon analysis of screen capture
	double dKeySpread;		// user-set, see Readme.txt
	int nWidth;				// width of color screen capture
	int nHeight;			// height of capture
	bool_t bUserSuppliedScreenSwatch; // did the user give a screen capture to scan? If no, then codec expects an RGB-vector to be entered
	SMFSSpec SwatchFSSpec;	//Contains information about the screen capture file
} ColorKey_t;

//
//	Color formats.
//
enum
{
	eCF_RGB = 0,				// Interleaved components in this order in memory: R, G, B
	eCF_ARGB = 1,				// Interleaved components in this order in memory: A, R, G, B
	eCF_BGR = 2,				// Interleaved components in this order in memory, B, G, R
	eCF_BGRA = 3,				// Interleaved components in this order in memory, B, G, R, A
	eCF_RGB565 = 4,			
	eCF_RGB555 = 5,
	eCF_PlanarYUV420 = 6,
	eCF_PlanarYUV9 = 7,
	eCF_InterleavedYUYV = 8,
	eCF_InterleavedYUYVs = 9,
	eCF_AlphaOnly = 10,			// ARGB where R, G and B are zero.
	eCF_InterleavedUYVY = 11,
};


typedef struct
{
	uint32_t unFormat;
	uint32_t unWidth;
	uint32_t unHeight;
	bool_t bBottomUp;

	//
	// Row byte values
	//
	union
	{
		unsigned int unRowBytes;
		unsigned int unYRowBytes;
		unsigned int unRRowBytes;
	};

	union
	{
		unsigned int unUVRowBytes;
		unsigned int unURowBytes;
		unsigned int unGRowBytes;
	};

	union
	{
		unsigned int unVRowBytes;
		unsigned int unBRowBytes;
	};

	unsigned int unARowBytes;

	//
	// Pointers to the image data
	//
	union
	{
		unsigned char *pImage;
		unsigned char *pY;
		unsigned char *pR;
	};

	union
	{
		unsigned char *pU;
		unsigned char *pG;
	};

	union
	{
		unsigned char *pV;
		unsigned char *pB;
	};

	unsigned char *pA;
} SMImage_t;


//
// Frame types
//
enum
{
	eFT_UNKNOWN_FRAME = -1,
	eFT_KEY_FRAME = 0,
	eFT_PRED_FRAME = 1,
	eFT_DISP_FRAME = 2,
	eFT_BIDIRECT_FRAME = 3,
	eFT_DROPPED_FRAME = 4,
	eFT_PENDING_FRAME = 5,
	eFT_SGMC_FRAME = 6
};

//
// Frame information structure used in retrieving the reconstructed frame
//
typedef struct
{
	uint32_t unComponentSize;
	void *pY;
	void *pU;
	void *pV;
	void *pA;
	int nYRowBytes;
	int nUVRowBytes;
	int nARowBytes;
	int nImageWidth;
	int nImageHeight;
} FrameInfo_t;


typedef struct
{
	uint32_t unFrameType;
	uint8_t *pBitstream;
	uint32_t unBitstreamLength;
} CompressedFrame_t;

typedef struct
{
	unsigned int unNumFrames;
	CompressedFrame_t Frames[1];
} PendingFrames_t;


#endif // SORENSONTYPES_H
