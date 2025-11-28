///////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	SorensonMP4.h
//
//  Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#ifndef SORENSONMP4_H
#define SORENSONMP4_H

//
// Sorenson includes.
//
#include "SMTypes.h"
#include "SMMemory.h"
#include "stiSVX.h"
#include "CstiSignal.h"

typedef struct Track_t Track_t;
typedef struct Media_t Media_t;
typedef struct Movie_t Movie_t;

typedef int64_t MP4TimeValue;
typedef int32_t MP4TimeScale;

//The Windows version from MMSystem.h is opposite endian of this version!!
#undef MAKEFOURCC
#define MAKEFOURCC(a, b, c, d)  (((uint32_t)((uint8_t)(a)) << 24) | ((uint32_t)((uint8_t)(b)) << 16) | ((uint32_t)((uint8_t)(c)) << 8) | (uint32_t)((uint8_t)(d)))

//
// Track reference types.
//
enum
{
	eMP4ODTrackRefType = MAKEFOURCC('m', 'p', 'o', 'd'),
	eMP4DependRefType = MAKEFOURCC('d', 'p', 'n', 'd'),
	eMP4SyncRefType = MAKEFOURCC('s', 'y', 'n', 'c'),
	eMP4HintRefType = MAKEFOURCC('h', 'i', 'n', 't'),
	eMP4IpirRefType = MAKEFOURCC('i', 'p', 'i', 'r'),
};


//
// Sample description types.
//
enum
{
	eAudioSampleDesc = MAKEFOURCC('m', 'p', '4', 'a'),
	eVisualSampleDesc = MAKEFOURCC('m', 'p', '4', 'v'),
	eMP4SampleDesc = MAKEFOURCC('m', 'p', '4', 's'),
	eRTPSampleDesc = MAKEFOURCC('r', 't', 'p', ' '),
	eAVCSampleDesc = MAKEFOURCC('a', 'v', 'c', '1'),
	eH263SampleDesc = MAKEFOURCC('h', '2', '6', '3'),
};


//
// Media types.
//
enum
{
	eMediaTypeClockRef = MAKEFOURCC('c', 'r', 's', 'm'),
	eMediaTypeVisual = MAKEFOURCC('v', 'i', 'd', 'e'),
	eMediaTypeAudio = MAKEFOURCC('s', 'o', 'u', 'n'),
	eMediaTypeMPEG7 = MAKEFOURCC('m', '7', 's', 'm'),
	eMediaTypeScene = MAKEFOURCC('s', 'd', 's', 'm'),
	eMediaTypeObject = MAKEFOURCC('o', 'd', 's', 'm'),
	eMediaTypeObjectContent = MAKEFOURCC('o', 'c', 's', 'm'),
	eMediaTypeIPMP = MAKEFOURCC('i', 'p', 's', 'm'),
	eMediaTypeMPEGJ = MAKEFOURCC('m', 'j', 's', 'm'),
	eMediaTypeHint = MAKEFOURCC('h', 'i', 'n', 't'),
};

//
// Atoms
enum
{
	eAtom4CCmvhd = MAKEFOURCC('m', 'v', 'h', 'd'),
	eAtom4CCiods = MAKEFOURCC('i', 'o', 'd', 's'),
	eAtom4CCesds = MAKEFOURCC('e', 's', 'd', 's'),
	eAtom4CCavcC = MAKEFOURCC('a', 'v', 'c', 'C'),
	eAtom4CCtims = MAKEFOURCC('t', 'i', 'm', 's'),
	eAtom4CCudta = MAKEFOURCC('u', 'd', 't', 'a'),
	eAtom4CCtref = MAKEFOURCC('t', 'r', 'e', 'f'),
	eAtom4CCtkhd = MAKEFOURCC('t', 'k', 'h', 'd'),
	eAtom4CCmdia = MAKEFOURCC('m', 'd', 'i', 'a'),
	eAtom4CCmdhd = MAKEFOURCC('m', 'd', 'h', 'd'),
	eAtom4CCelst = MAKEFOURCC('e', 'l', 's', 't'),
	eAtom4CCtrak = MAKEFOURCC('t', 'r', 'a', 'k'),
	eAtom4CCminf = MAKEFOURCC('m', 'i', 'n', 'f'),
	eAtom4CCstbl = MAKEFOURCC('s', 't', 'b', 'l'),
	eAtom4CCstts = MAKEFOURCC('s', 't', 't', 's'),
	eAtom4CCctts = MAKEFOURCC('c', 't', 't', 's'),
	eAtom4CCstsz = MAKEFOURCC('s', 't', 's', 'z'),
	eAtom4CCstsc = MAKEFOURCC('s', 't', 's', 'c'),
	eAtom4CCstco = MAKEFOURCC('s', 't', 'c', 'o'),
	eAtom4CCstss = MAKEFOURCC('s', 't', 's', 's'),
	eAtom4CCstsd = MAKEFOURCC('s', 't', 's', 'd'),
	eAtom4CChdlr = MAKEFOURCC('h', 'd', 'l', 'r'),
	eAtom4CCfree = MAKEFOURCC('f', 'r', 'e', 'e'),
	eAtom4CCmdat = MAKEFOURCC('m', 'd', 'a', 't'),
	eAtom4CCmoov = MAKEFOURCC('m', 'o', 'o', 'v'),
	eAtom4CCftyp = MAKEFOURCC('f', 't', 'y', 'p'),
	eAtom4CCvmhd = MAKEFOURCC('v', 'm', 'h', 'd'),
	eAtom4CCsmhd = MAKEFOURCC('s', 'm', 'h', 'd'),
	eAtom4CChmhd = MAKEFOURCC('h', 'm', 'h', 'd'),
	eAtom4CCnmhd = MAKEFOURCC('n', 'm', 'h', 'd'),
	eAtom4CCdref = MAKEFOURCC('d', 'r', 'e', 'f'),
	eAtom4CCco64 = MAKEFOURCC('c', 'o', '6', '4'),
	eAtom4CCurl = MAKEFOURCC('u', 'r', 'l', ' '),
	eAtom4CCurn = MAKEFOURCC('u', 'r', 'n', ' '),
	eAtom4CCdinf = MAKEFOURCC('d', 'i', 'n', 'f'),
	eAtom4CCedts = MAKEFOURCC('e', 'd', 't', 's'),
	eAtom4CCstdp = MAKEFOURCC('s', 't', 'd', 'p'),
	eAtom4CCskip = MAKEFOURCC('s', 'k', 'i', 'p'),
	eAtom4CCwide = MAKEFOURCC('w', 'i', 'd', 'e'),
	eAtom4CCmpod = MAKEFOURCC('m', 'p', 'o', 'd'),
	eAtom4CCdpnd = MAKEFOURCC('d', 'p', 'n', 'd'),
	eAtom4CCsync = MAKEFOURCC('s', 'y', 'n', 'c'),
	eAtom4CChint = MAKEFOURCC('h', 'i', 'n', 't'),
	eAtom4CCipir = MAKEFOURCC('i', 'p', 'i', 'r'),
};

//
// Sample descriptions
//
#pragma pack(1)

typedef struct SampleDescr_t
{
	uint32_t unSize;						// size of the structure including additional data
	uint32_t unDescrType;					// set to sample descrition type
	uint8_t aucReserved[6];					// set to 0
	uint16_t usDataRefIndex;				// set to 0
	uint8_t unData[1];
} SampleDescr_t;


typedef struct RTPSampleDescr_t
{
	uint32_t unSize;						// size of the structure
	uint32_t unDescrType;					// set to 'rtp '
	uint8_t aucReserved[6];					// set to 0
	uint16_t usDataRefIndex;				// set to 0
	uint16_t usHintTrackVersion;			// set to 1
	uint16_t usHighestCompatibleVersion;	// set to 1
	uint32_t unMaxPacketSize;				// set to max packet size
	uint32_t unReserved2;					// set to 12
	uint32_t unReserved3;					// set to 'tims'
	uint32_t unTimeScale;					// set to time scale for RTP
} RTPSampleDescr_t;


typedef struct VisualSampleDescr_t
{
	uint32_t unSize;						// size of the structure including extended information such as ES descriptor
	uint32_t unDescrType;					// set to 'mp4v'
	uint8_t aucReserved[6];				// set to 0
	uint16_t usDataRefIndex;				// set to 0
	uint32_t aunReserved1[4];				// set to 0
	uint16_t usWidth;
	uint16_t usHeight;
	uint32_t unHorizRes;					// set to 0x00480000
	uint32_t unVertRes;					// set to 0x00480000
	uint32_t unReserved5;					// set to 0
	uint16_t usFrameCount;					// set to 1
	uint8_t aucCompressorName[32];			// set to 0
	uint16_t usDepth;						// set to 24
	int16_t sReserved9;					// set to -1
	uint32_t unESSize;						// set to length of remainder of structure including ES descriptor
	uint32_t unReserved10;					// set to 'esds'
	uint32_t unReserved11;					// set to 0
	uint8_t aucESDescriptor[1];			// contains encoded ES descriptor
} VisualSampleDescr_t;


typedef struct AVCSampleDescr_t
{
	uint32_t unSize;						// size of the structure including extended information such as ES descriptor
	uint32_t unDescrType;					// set to 'avc1'
	uint8_t aucReserved[6];				// set to 0
	uint16_t usDataRefIndex;				// set to 0
	uint32_t aunReserved1[4];				// set to 0
	uint16_t usWidth;
	uint16_t usHeight;
	uint32_t unHorizRes;					// set to 0x00480000
	uint32_t unVertRes;					// set to 0x00480000
	uint32_t unReserved5;					// set to 0
	uint16_t usFrameCount;					// set to 1
	uint8_t aucCompressorName[32];			// set to "\012AVC Coding"
	uint16_t usDepth;						// set to 24
	int16_t sReserved9;					// set to -1
	uint32_t unConfigSize;					// set to length of remainder of structure including AVC configuration record
	uint32_t unConfigType;					// set to 'avcC'
	uint8_t aucConfig[1];					// contains AVC configuration record
} AVCSampleDescr_t;

typedef struct AVCDecoderConfigRecord_t
{
	uint8_t unConfigVersion;				// size of the structure including extended information such as ES descriptor
	uint8_t unAVCProfileIndication;		// profile code
	uint8_t unProfileCompatability;		// byte between profile_IDC and level_IDC in a sequence parameter set
	uint8_t unAVCLevelIndication;			// level code
	uint8_t unLengthSizeMinusOne;			// length in bytes of the NALUnitLength field
	uint8_t aucParameterSets[1];			// 
} AVCDecoderConfigRecord_t;


typedef struct AudioSampleDescr_t
{
	uint32_t unSize;						// size of the structure including extended information such as ES descriptor
	uint32_t unDescrType;					// set to 'mp4a'
	uint8_t aucReserved[6];				// set to 0
	uint16_t usDataRefIndex;				// set to 0
	uint32_t aunReserved1[2];				// set to 0
	uint16_t usChannelCount;				// set to 2
	uint16_t usSampleSize;					// set to 16
	uint32_t unReserved4;					// set to 0
	uint16_t usSampleRate;					// set to track's time scale
	uint16_t usReserved5;					// set to 0
	uint32_t unESSize;						// set to length of remainder of structure including ES descriptor
	uint32_t unReserved10;					// set to 'esds'
	uint32_t unReserved11;					// set to 0
	uint8_t aucESDescriptor[1];			// contains encoded ES descriptor
} AudioSampleDescr_t;


typedef struct MPEGSampleDescr_t
{
	uint32_t unSize;						// size of the structure including extended information such as ES descriptor
	uint32_t unDescrType;					// set to 'mp4v'
	uint8_t aucReserved[6];					// set to 0
	uint16_t usDataRefIndex;				// set to 0
	uint32_t unESSize;						// set to length of remainder of structure including ES descriptor
	uint32_t unReserved10;					// set to 'esds'
	uint32_t unReserved11;					// set to 0
	uint8_t aucESDescriptor[1];				// contains encoded ES descriptor
} MPEGSampleDescr_t;

#pragma pack()


//
// Object Descriptor Tags
//
enum
{
	eObjectDescrTag = 0x01,
	eInitialObjectDescrTag = 0x02,
	eES_DescrTag = 0x03,
	eDecoderConfigTag = 0x04,
	eDecSpecificInfoTag = 0x05,
	eSLConfigDescrTag = 0x06,
	eContentIdentDescrTag = 0x07,
	eSupplContentIdentDescrTag = 0x08,
	eIPI_DescrPointerTag = 0x09,
	eIPMP_DescrPointerTag = 0x0A,
	eIPMP_DescrTag = 0x0B,
	eQoS_DescrTag = 0x0C,
	eRegistrationDescrTag = 0x0D,
	eES_ID_IncTag = 0x0E,
	eES_ID_RefTag = 0x0F,
	eMP4_IOD_Tag = 0x10,
	eMP4_OD_Tag = 0x11,
	eIPL_DescrPointerRefTag = 0x12,
	eExtendedProfileLevelDescrTag = 0x013,
	eProfileLevelIndicationIndexDescrTag = 0x14,
};

//
// Stream types
//
enum
{
	eObjectDescriptorStream = 0x01,
	eClockReferenceStream = 0x02,
	eSceneDescriptionStream = 0x03,
	eVisualStream = 0x04,
	eAudioStream = 0x05,
	eMPEG7Stream = 0x06,
	eIPMPStream = 0x07,
	eObjectContentInfoStream = 0x08,
	eMPEGJStream = 0x09
};


//
// Object Descriptor Commands
//
enum
{
	eObjectDescriptorUpdate = 1,
	eObjectDescriptorRemove = 2,
	eES_DescriptorUpdate = 3,
	eES_DescriptorRemove = 4,
	eIPMP_DescriptorUpdate = 5,
	eIPMP_DescriptorRemove = 6,
};


//
// Generic object
//
class Object_i
{
public:

	virtual ~Object_i ();
	
	Object_i (const Object_i &other) = delete;
	Object_i (Object_i &&other) = delete;
	Object_i &operator= (const Object_i &other) = delete;
	Object_i &operator= (Object_i &&other) = delete;

	virtual uint32_t GetTag() = 0;

	virtual void SetTag(
		uint32_t unTag) = 0;

	virtual void AddRef() = 0;

	virtual void Release() = 0;
};

//
// Decoder specific descriptor
//
class DecoderSpecificDescr_i : public Object_i
{
public:
	
	virtual SMResult_t SetData(
		const uint8_t *pData,
		uint32_t unLength) = 0;

	virtual void GetData(
		uint8_t **ppData,
		uint32_t *punLength) = 0;
};


DecoderSpecificDescr_i *CreateDecoderSpecific();


//
// Decoder configuration descriptor
//
class DecoderConfigDescr_i : public Object_i
{
public:

	virtual uint32_t GetObjectType() = 0;

	virtual void SetObjectType(
		uint32_t unObjectType) = 0;

	virtual uint32_t GetStreamType() = 0;

	virtual void SetStreamType(
		uint32_t unStreamType) = 0;

	virtual bool_t GetUpStreamFlag() = 0;

	virtual void SetUpStreamFlag(
		bool_t bUpStream) = 0;

	virtual uint32_t GetBufferSize() = 0;

	virtual void SetBufferSize(
		uint32_t unBufferSize) = 0;

	virtual uint32_t GetMaxBitRate() = 0;

	virtual void SetMaxBitRate(
		uint32_t unMaxBitRate) = 0;

	virtual uint32_t GetAvgBitRate() = 0;

	virtual void SetAvgBitRate(
		uint32_t unAvgBitRate) = 0;

	virtual DecoderSpecificDescr_i *GetDecoderSpecific() = 0;

	virtual void SetDecoderSpecific(
		DecoderSpecificDescr_i *pDecoderSpecific) = 0;
};


DecoderConfigDescr_i *CreateDecoderConfig();


//
// Sync Layer configuration descriptor
//
class SLConfigDescriptor_i : public Object_i
{
public:

	virtual uint32_t GetPredefined() = 0;

	virtual void SetPredefined(
		uint32_t unPredefined) = 0;

	virtual bool_t GetDurationFlag() = 0;

	virtual bool_t GetUseTimeStampsFlag() = 0;

	virtual void SetUseTimeStampsFlag(
		bool_t bUseTimeStamps) = 0;

	virtual void SetTimeStampResolution(
		uint32_t unResolution) = 0;

	virtual void SetTimeStampLength(
		uint32_t unLength) = 0;

	virtual uint32_t GetStartCompositionTimeStamp() = 0;

	virtual uint32_t GetStartDecodingTimeStamp() = 0;

	virtual void SetStartCompositionTimeStamp(
		uint32_t unTimeStamp) = 0;

	virtual void SetStartDecodingTimeStamp(
		uint32_t unTimeStamp) = 0;
};


SLConfigDescriptor_i *CreateSLConfig();


//
// ES descriptor
//
class ESDescriptor_i : public Object_i
{
public:

	virtual uint32_t GetESID() = 0;

	virtual void SetESID(
		uint32_t unESID) = 0;

	virtual void GetStreamDependence(
		bool_t *pbStreamDependent,
		uint32_t *punESID) = 0;

	virtual void SetStreamDependence(
		bool_t bStreamDependent,
		uint32_t unESID) = 0;

	virtual SMResult_t SetUrl(
		const char *pszUrl,
		uint32_t unLength) = 0;

	virtual const char *GetUrl() = 0;

	virtual uint32_t GetStreamPriority() = 0;

	virtual void SetStreamPriority(
		uint32_t unStreamPriority) = 0;

	virtual void GetOCRStream(
		bool_t *pbUseOCRStream,
		uint32_t *punESID) = 0;

	virtual void SetOCRStream(
		bool_t bUseOCRStream,
		uint32_t unESID) = 0;

	virtual DecoderConfigDescr_i *GetDecoderConfig() = 0;

	virtual void SetDecoderConfig(
		DecoderConfigDescr_i *pDecoderConfigDescr) = 0;

	virtual SLConfigDescriptor_i *GetSLConfig() = 0;

	virtual void SetSLConfig(
		SLConfigDescriptor_i *pSLConfigDescr) = 0;
};


ESDescriptor_i *CreateESDescriptor();


//
// Object descriptor
//
class ObjectDescriptor_i : public Object_i
{
public:

	virtual uint32_t GetODID() = 0;

	virtual void SetODID(
		uint32_t unODID) = 0;

	virtual SMResult_t SetUrl(
		const char *pszUrl,
		uint32_t unLength) = 0;

	virtual const char *GetUrl() = 0;

	virtual SMResult_t AddESDescriptor(
		ESDescriptor_i *pESDescriptor) = 0;

	virtual uint32_t GetNumESDescriptors() = 0;
	
	virtual ESDescriptor_i *GetESDescriptor(
		uint32_t unIndex) = 0;

	virtual SMResult_t SetESDescriptor(
		ESDescriptor_i *pESDescriptor,
		uint32_t unIndex) = 0;

	virtual SMResult_t RemoveESDescriptor(
		uint32_t unIndex) = 0;
};


ObjectDescriptor_i *CreateObjectDescriptor();

//
// Initial Object
//
class InitialObjectDescr_i : public ObjectDescriptor_i
{
public:

	virtual bool_t GetIncludeInlineProfile() = 0;

	virtual void SetIncludeInlineProfile(
		bool_t bIncludeInlineProfile) = 0;

	virtual void SetProfiles(
		uint32_t unODProfileLevel,
		uint32_t unSceneProfileLevel,
		uint32_t unAudioProfileLevel,
		uint32_t unVisualProfileLevel,
		uint32_t unGraphicsProfileLevel) = 0;

	virtual void GetProfiles(
		uint32_t *punODProfileLevel,
		uint32_t *punSceneProfileLevel,
		uint32_t *punAudioProfileLevel,
		uint32_t *punVisualProfileLevel,
		uint32_t *punGraphicsProfileLevel) = 0;

};


InitialObjectDescr_i *CreateInitialObjectDescriptor();



class ObjectList_i
{
public:

	virtual ~ObjectList_i ();
	
	ObjectList_i (const ObjectList_i &other) = delete;
	ObjectList_i (ObjectList_i &&other) = delete;
	ObjectList_i &operator= (const ObjectList_i &other) = delete;
	ObjectList_i &operator= (ObjectList_i &&other) = delete;

	virtual uint32_t GetNumObjects() = 0;
	
	virtual SMResult_t AddObject(
		Object_i *pObject) = 0;

	virtual Object_i *GetObject(
		uint32_t unIndex) = 0;

	virtual SMResult_t SetObject(
		Object_i *pObject, 
		uint32_t unIndex) = 0;

	virtual SMResult_t RemoveObject(
		uint32_t unIndex) = 0;

	virtual void AddRef() = 0;

	virtual void Release() = 0;
};


ObjectList_i *CreateObjectList();


//
// OD Command
//
class ODCommand_i : public ObjectList_i
{
public:

	virtual uint32_t GetCommandID() = 0;

};


ODCommand_i *CreateODCommand(
	uint32_t unCommandID);


//
// OD Frame
//
class ODFrame_i
{
public:

	virtual ~ODFrame_i ();
	
	ODFrame_i (const ODFrame_i &other) = delete;
	ODFrame_i (ODFrame_i &&other) = delete;
	ODFrame_i &operator= (const ODFrame_i &other) = delete;
	ODFrame_i &operator= (ODFrame_i &&other) = delete;

	virtual uint32_t GetNumCommands() = 0;

	virtual SMResult_t AddCommand(
		ODCommand_i *pCommand) = 0;

	virtual ODCommand_i *GetCommand(
		uint32_t unIndex) = 0;

	virtual SMResult_t RemoveCommand(
		uint32_t unIndex) = 0;

	virtual void AddRef() = 0;

	virtual void Release() = 0;
};


ODFrame_i *CreateODFrame();


//
// Object decoder.
//
class ObjectDecoder_i
{
public:

	virtual ~ObjectDecoder_i ();
	
	ObjectDecoder_i (const ObjectDecoder_i &other) = delete;
	ObjectDecoder_i (ObjectDecoder_i &&other) = delete;
	ObjectDecoder_i &operator= (const ObjectDecoder_i &other) = delete;
	ObjectDecoder_i &operator= (ObjectDecoder_i &&other) = delete;

	virtual ObjectDescriptor_i *DecodeDescriptor(
		char *pBitstream,
		uint32_t unBitstreamLength,
		Track_t *pTrack,
		Movie_t *pMovie) = 0;

	virtual SMResult_t DecodeODFrame(
		char *pBitstream,
		uint32_t unBitstreamLength,
		Track_t *pTrack,
		Movie_t *pMovie,
		ODFrame_i **ppODFrame) = 0;

};


ObjectDecoder_i *CreateObjectDecoder();


void DestroyObjectDecoder(
	ObjectDecoder_i *pObjectDecoder);


//
// Object encoder.
//
class ObjectEncoder_i
{
public:
	
	virtual ~ObjectEncoder_i ();
	
	ObjectEncoder_i (const ObjectEncoder_i &other) = delete;
	ObjectEncoder_i (ObjectEncoder_i &&other) = delete;
	ObjectEncoder_i &operator= (const ObjectEncoder_i &other) = delete;
	ObjectEncoder_i &operator= (ObjectEncoder_i &&other) = delete;

	virtual SMResult_t EncodeDescriptor(
		Object_i *pObject,
		uint8_t *pBitstream,
		uint32_t *punLength) = 0;

	virtual SMResult_t EncodeODFrame(
		ODFrame_i *pODFrame,
		uint8_t *pBitstream,
		uint32_t *punLength) = 0;

};

extern "C" {

#if !defined(stiLINUX) && !defined(WIN32)
ObjectEncoder_i *CreateObjectEncoder();


void DestroyObjectEncoder(
	ObjectEncoder_i *pObjectEncoder);


SMResult_t ExecODFrame(
	ODFrame_i *pODFrame,
	ObjectList_i *pObjectList);


SMResult_t MP4NewRTPSampleDescription(
	SMHandle *phSampleDescription);


SMResult_t MP4NewAudioSampleDescription(
	ESDescriptor_i *pESDescriptor,
	SMHandle *phSampleDescription);


SMResult_t MP4NewMPEGSampleDescription(
	ESDescriptor_i *pESDescriptor,
	SMHandle *phSampleDescription);


SMResult_t MP4NewVisualSampleDescription(
	ESDescriptor_i *pESDescriptor,
	uint16_t usWidth,
	uint16_t usHeight,
	SMHandle *phSampleDescritpion);


SMResult_t MP4GetESDescriptor(
	SMHandle hSampleDescription,
	ESDescriptor_i **ppESDescriptor);


SMResult_t MP4SetESDescriptor(
	SMHandle hSampleDescription,
	ESDescriptor_i *pESDescriptor);
#endif


SMResult_t MP4NewAVCSampleDescription(
	const uint8_t *pConfigRecord,
	unsigned int unConfigRecordLength,
	uint16_t usWidth,
	uint16_t usHeight,
	SMHandle *phSampleDescription);

SMResult_t MP4NewAACSampleDescription(
	uint16_t sampleRate,
	uint32_t encodeBitrate,
	SMHandle hAudioSampleDescr);

SMResult_t MP4NewMovieTrack(
	Movie_t *pMovie,
	uint32_t unWidth,			// Fixed 16.16
	uint32_t unHeight,			// Fixed 16.16
	int16_t sVolume,			// Fixed 8.8
	Track_t **ppTrack);


SMResult_t MP4AddTrackReference(
	Track_t *pTrack,
	Track_t *pRefTrack,
	uint32_t unRefType,
	uint32_t *punRefIndex);


SMResult_t MP4NewTrackMedia(
	Track_t *pTrack,
	uint32_t unMediaType,
	Media_t **ppMedia);


SMResult_t MP4SetMovieTimeScale(
	Movie_t *pMovie,
	uint32_t unTimeScale);


SMResult_t MP4GetMovieTimeScale(
	Movie_t *pMovie,
	MP4TimeScale *punTimeScale);


SMResult_t MP4SetMediaTimeScale(
	Media_t *pMedia,
	uint32_t unTimeScale);


SMResult_t MP4GetMediaTimeScale(
	Media_t *pMedia,
	MP4TimeScale *punTimeScale);


SMResult_t MP4BeginMediaEdits(
	Media_t *pMedia);


SMResult_t MP4EndMediaEdits(
	Media_t *pMedia);

SMResult_t MP4UpdateMediaDuration(
	Media_t *pMedia);

SMResult_t MP4AddMediaSamples(
	Media_t *pMedia,
	SMHandle hSampleData, 
	SMHandle hSampleDescription,
	unsigned int unSampleCount,
	SMHandle hSampleDurations,
	SMHandle hSampleSizes,
	SMHandle hDecodingOffsets,
	SMHandle hSyncSamples);


SMResult_t MP4GetMediaSampleDescription(
	Media_t *pMedia,
	uint32_t unIndex,
	SMHandle hSampleDescr);


SMResult_t MP4SetMediaSampleDescription(
	Media_t *pMedia,
	uint32_t unIndex,
	SMHandle hSampleDescr);


SMResult_t MP4GetMovieDuration(
	Movie_t *pMovie,
	MP4TimeValue *pMovieDuration);


SMResult_t MP4GetMediaDuration(
	Media_t *pMedia,
	MP4TimeValue *pMediaDuration);


SMResult_t MP4GetMediaDataSize(
	Media_t *pMedia,
	MP4TimeValue StartTime,
	MP4TimeValue EndTime,
	uint64_t *pun64Size);


SMResult_t MP4InsertMediaIntoTrack(
	Track_t *pTrack,
	uint32_t unTrackStart,
	uint32_t unMediaTime,
	uint64_t un64MediaDuration,
	uint32_t unMediaRate);

#if !defined(stiLINUX) && !defined(WIN32)
SMResult_t MP4CreateMovieFile(
#ifdef __SMMac__
	short vRefNum,
	long parID,
	Str255 fileName,
#endif // __SMMac__
#ifdef WIN32
	const char *pszFileName,
#endif // WIN32
	uint32_t *pBrands,
	unsigned int unBrandLength,
	Movie_t **ppMovie);

SMResult_t MP4OpenMovieFile(
#ifdef __SMMac__
	short vRefNum,
	long parID,
	Str255 fileName,
#endif // __SMMac__
#ifdef WIN32
	const char *pszFileName,
#endif // WIN32
	Movie_t **ppMovie);
#endif // #if !defined(stiLINUX)

SMResult_t MP4OpenMovieFileAsync(
	const std::string &server,
	const std::string &fileGUID,
	uint64_t un16FileSize,
	const std::string  &clientToken,
	uint32_t un32DownloadSpeed,
	Movie_t **ppMovie);

SMResult_t MP4ResetMovieDownloadGUID(
	const std::string &fileGUILD,
	Movie_t *pMovie);

SMResult_t MP4MovieDownloadSpeedSet(
	uint32_t un32DownloadSpeed,
	Movie_t *pMovie);

SMResult_t MP4MovieDownloadResume(
	Movie_t *pMovie);

SMResult_t MP4UploadMovieFileAsync(
	const std::string &server,
	const std::string &uploadGUID,
	const std::string &uploadFileName,
	Movie_t **ppMovie);

SMResult_t MP4UploadOptimizedMovieFileAsync(
	const std::string &server,
	const std::string &uploadGUID,
	uint32_t un32Width,
	uint32_t un32Height,
	uint32_t un32Bitrate,
	uint32_t un32Duration,
	Movie_t *pMovie);

SMResult_t MP4MovieWriteFtypAtom(
	Movie_t *pMovie);

SMResult_t MP4MovieWriteMoovAtom(
	Movie_t *pMovie);

SMResult_t MP4GetMediaSampleAsync(
	Media_t *pMedia,
	uint32_t unSampleIndex,
	SMHandle hFrame);

SMResult_t MP4GetAudioForFrameIndexAsync(
	const Media_t *audioMedia,
	const Media_t *videoMedia,
	uint32_t sampleIndex,
	SMHandle audio,
	uint32_t lastAudioIndex);

SMResult_t MP4CloseMovieFileAsync(
	Movie_t *pMovie);

SMResult_t MP4CleanupMovieFileAsync(
	Movie_t *pMovie);

SMResult_t MP4MovieClosed(
	Movie_t *pMovie);

SMResult_t MP4GetMaxBufferingSize(
	Movie_t *pMovie,
	uint32_t *punMaxBufferingSize);

SMResult_t MP4ParseAtom(
	Movie_t *pMovie,
	uint64_t un64FileOffset,
	uint64_t *pun64DataParsed,
	uint32_t *pun32AtomType,
	uint32_t *pun32HeaderSize);

SMResult_t MP4SaveMovie(
	Movie_t *pMovie);


SMResult_t MP4CloseMovieFile(
	Movie_t *pMovie);


SMResult_t MP4DisposeMovie(
	Movie_t *pMovie);


SMResult_t MP4NewMovie(
	Movie_t **ppMovie);


SMResult_t MP4SetMovieBrands(
	Movie_t *pMovie,
	uint32_t *punBrands,
	unsigned int unBrandLength);


SMResult_t MP4GetTrackCount(
	Movie_t *pMovie,
	uint32_t *punTrackCount);


SMResult_t MP4GetTrackByIndex(
	Movie_t *pMovie,
	unsigned int unIndex,
	Track_t **ppTrack);


SMResult_t MP4GetTrackByID(
	Movie_t *pMovie,
	unsigned int unTrackID,
	Track_t **ppTrack);


SMResult_t MP4GetTrackID(
	Track_t *pTrack,
	uint32_t *punTrackID);


SMResult_t MP4GetTrackMedia(
	Track_t *pTrack,
	Media_t **ppMedia);


SMResult_t MP4GetMediaSyncSampleIndex(
	Media_t *pMedia,
	uint32_t desiredFrameIndex,
	uint32_t *pIFrameIndex);


SMResult_t MP4GetMediaHandlerType(
	Media_t *pMedia,
	uint32_t *punHandlerType);


SMResult_t MP4GetMediaSampleCount(
	Media_t *pMedia,
	uint32_t *punNumSamples);


SMResult_t MP4GetMediaSampleByIndex(
	Media_t *pMedia,
	uint32_t unSampleIndex,
	SMHandle hData,
	uint64_t *pun64SampleOffset,
	uint32_t *punSampleSize,
	bool_t *pbSyncSample,
	uint32_t *punSampleDescIndex,
	bool_t bReleaseBuffer = true);


SMResult_t MP4GetMediaPrevSync(
	Media_t *pMedia,
	unsigned int unIndex,
	bool_t bIncludeCurrent,
	uint32_t *punSyncIndex);


SMResult_t MP4GetMediaNextTime(
	Media_t *pMedia,
	MP4TimeValue Time,
	int32_t nDirection,
	MP4TimeValue *NextTime);


SMResult_t MP4GetSampleDuration(
	Media_t *pMedia,
	uint32_t unSampleIndex,
	uint32_t *punDuration);


SMResult_t MP4MediaTimeToSampleIndex(
	Media_t *pMedia,
	MP4TimeValue unTime,
	uint32_t *punIndex);


SMResult_t MP4SampleIndexToMediaTime(
	Media_t *pMedia,
	uint32_t unSampleIndex,
	MP4TimeValue *punTime,
	MP4TimeValue *punDuration);


SMResult_t MP4TrackTimeToMediaTime(
	Track_t *pTrack,
	MP4TimeValue TrackTime,
	MP4TimeValue *pMediaTime);


SMResult_t MP4GetTrackDuration(
	Track_t *pTrack,
	MP4TimeValue *pDuration);

SMResult_t MP4GetTrackDimensions(
	Track_t *pTrack,
	uint32_t *punWidth,
	uint32_t *punHeight);


SMResult_t MP4GetMediaEditTime(
	Media_t *pMedia,
	MP4TimeValue MediaTime,
	uint32_t unFlags,
	MP4TimeValue *pTime,
	MP4TimeValue *pDuration);


SMResult_t MP4GetTrackReference(
	Track_t *pTrack,
	uint32_t unType,
	uint32_t unIndex,
	uint32_t *punRef);


SMResult_t MP4HintMovie(
	Movie_t *pMovie,
	uint32_t unMaxPacketSize,			// Maximum packet size in bytes
	uint32_t unMaxPacketDuration);		// Maximum packet duration in milliseconds


SMResult_t MP4AddMovieUserData(
	Movie_t *pMovie,
	uint32_t unType,
	const uint8_t *pucData,
	uint32_t unLength);


SMResult_t MP4AddTrackUserData(
	Track_t *pTrack,
	uint32_t unType,
	const uint8_t *pucData,
	uint32_t unLength);

#if !defined(stiLINUX) && !defined(WIN32)
SMResult_t MP4SetMovieIOD(
	Movie_t *pMovie,
	InitialObjectDescr_i *pIOD);


SMResult_t MP4GetMovieIOD(
	Movie_t *pMovie,
	InitialObjectDescr_i **ppIOD);
#endif

SMHandle MP4NewHandle(
	unsigned int unSize);


SMResult_t MP4SetHandleSize(
	SMHandle hHandle,
	unsigned int unSize);


unsigned int MP4GetHandleSize(
	SMHandle hHandle);


void MP4DisposeHandle(
	SMHandle hHandle);

SMResult_t MP4OptimizeAndWriteMovieFile(
	Movie_t *pMovie);

SMResult_t  MP4ReadyForDownloadUpdate(
	Media_t *pMedia);

void MP4LogInfoToSplunk (
	Movie_t *movie);
}

#endif // SORENSONMP4_H


