#ifndef STIVIDEODEFS_H
#define STIVIDEODEFS_H

// Includes
//

//
// Constants
//

//
// Typedefs
//
// This structure defines a display image
typedef struct SsmdImageSettings
{
	int bVisible;
	int bMirrored;
	int bHold;
	int bPrivacy;
	unsigned int unWidth;
	unsigned int unHeight;
	unsigned int unPosX;
	unsigned int unPosY;
} SsmdImageSettings;

//
// This structure defines the display settings
//
typedef enum EsmdTopView
{
	esmdSelfView 	= 0,
	esmdRemoteView	= 1
} EsmdTopView;

// This structure defines the display settings
typedef struct SsmdDisplaySettings
{
	SsmdImageSettings stSelfView;
	SsmdImageSettings stRemoteView;
	EsmdTopView eTopView;
} SsmdDisplaySettings;

#endif // STIVIDEODEFS_H

