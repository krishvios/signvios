#import <Foundation/Foundation.h>
#if 0
#import <CoreServices/CoreServices.h>
#import <AppKit/AppKit.h>
#endif
#include "IstiVideoOutput.h"
#include "IstiVideoInput.h"
#include "IstiAudioInput.h"
#include "IstiAudioOutput.h"
#include "IstiAudibleRinger.h"
#include "IstiPlatform.h"
#include "CstiBSPInterface.h"

class CstiVideoOutput : public IstiVideoOutput
{
public:
    CstiVideoOutput() {}
    ~CstiVideoOutput() {}
    
#if APPLICATION == APP_NTOUCH
	virtual stiHResult FramePut (
                                 EDisplayWindow eWindow,
                                 const SstiFrameUpdate *pFrameUpdate) { return stiRESULT_SUCCESS; }
    
	virtual stiHResult DisplayModeCapabilitiesGet (
                                                   uint32_t *pun32CapBitMask)  { return stiRESULT_SUCCESS; }
    
	virtual stiHResult WindowModeSet (
                                      EDisplayWindow eWindow,
                                      EDisplayModes eDisplayMode)  { return stiRESULT_SUCCESS; }
    
	virtual stiHResult WindowEnable (
                                     EDisplayWindow eWindow,
                                     EstiBool bEnable)  { return stiRESULT_SUCCESS; }
    
	virtual stiHResult WindowPositionSet (
                                          EDisplayWindow eWindow,
                                          int nXPos,
                                          int nYPos)  { return stiRESULT_SUCCESS; }
#endif
	
	virtual stiHResult VideoOut (
								 EstiSwitch eSwitch) { return stiRESULT_SUCCESS; }
    
	virtual stiHResult CallbackRegister (
                                         PstiObjectCallback pfnVPCallback,
                                         size_t CallbackParam,
                                         size_t CallbackFromId)  { return stiRESULT_SUCCESS; }
    
	virtual stiHResult CallbackUnregister (
                                           PstiObjectCallback pfnVPCallback,
                                           size_t CallbackParam)  { return stiRESULT_SUCCESS; }
    
    
	virtual void DisplaySettingsGet (
                                     SstiDisplaySettings * pDisplaySettings) const  { }
	
	virtual stiHResult DisplaySettingsSet (
                                           const SstiDisplaySettings * pDisplaySettings)  { return stiRESULT_SUCCESS; }
	
	virtual stiHResult RemoteViewPrivacySet (
                                             EstiBool bPrivacy)  { return stiRESULT_SUCCESS; }
    
	virtual stiHResult RemoteViewHoldSet (
                                          EstiBool bHold)  { return stiRESULT_SUCCESS; }
    
	virtual stiHResult VideoPlaybackSizeGet (
                                             uint32_t *pun32Width,
                                             uint32_t *pun32Height) const  { return stiRESULT_SUCCESS; }
    
	virtual stiHResult VideoPlaybackCodecSet (
                                              EstiVideoCodec eVideoCodec)  { return stiRESULT_SUCCESS; }
    
	virtual stiHResult VideoPlaybackStart ()  { return stiRESULT_SUCCESS; }
    
	virtual stiHResult VideoPlaybackStop ()  { return stiRESULT_SUCCESS; }
    
	virtual stiHResult VideoPlaybackFramePut (
                                              SstiPlaybackVideoFrame *pVideoFrame)  { return stiRESULT_SUCCESS; }
    
	virtual stiHResult VideoPlaybackFrameGet (IstiVideoPlaybackFrame **ppVideoFrame) { return stiRESULT_SUCCESS; }
	
	virtual stiHResult VideoPlaybackFramePut (IstiVideoPlaybackFrame *pVideoFrame) { return stiRESULT_SUCCESS; }
	
	virtual stiHResult VideoPlaybackFrameDiscard (IstiVideoPlaybackFrame *pVideoFrame) { return stiRESULT_SUCCESS; }
	
	virtual int GetDeviceFD () const  { return 0; }

	virtual stiHResult VideoCodecsGet (
           std::list<int> *pCodecs,
           EstiVideoCodec ePreferredCodec)  { return stiRESULT_SUCCESS; }
    
	virtual stiHResult CodecCapabilitiesGet (
                                             EstiVideoCodec eCodec,
                                             SstiVideoCapabilities *pstCaps)  { return stiRESULT_SUCCESS; }
	
	virtual void ScreenshotTake (EstiScreenshotType eType) { };
	
    virtual stiHResult OverlaySizeSet (int nWidth, int nHeight) { return stiRESULT_SUCCESS; }
    
};

class CstiVideoInput : public IstiVideoInput
{
public:
    
#ifdef stiHOLDSERVER
	static IstiVideoInput *InstanceCreate () { return NULL; }
	virtual void InstanceRelease () { }
	virtual EstiBool InUseGet () { return estiTRUE; }
	virtual void InUseSet () { }
	virtual stiHResult HSEndService () { return stiRESULT_SUCCESS; }
#endif
    
	virtual stiHResult CameraSet (
                                  EstiSwitch eSwitch) { return stiRESULT_SUCCESS; }
    
	virtual stiHResult CameraMove (
                                   const SstiPtzSettings *pPtzSettings) { return stiRESULT_SUCCESS; }
	
	virtual stiHResult VideoRecordStart () { return stiRESULT_SUCCESS; }
    
	virtual stiHResult VideoRecordStop () { return stiRESULT_SUCCESS; }
    
	virtual stiHResult VideoRecordCodecSet (
                                            EstiVideoCodec eVideoCodec) { return stiRESULT_SUCCESS; }
	
	virtual stiHResult VideoRecordSettingsSet (
                                               const SstiVideoRecordSettings *pVideoRecordSettings) { return stiRESULT_SUCCESS; }
    
	virtual stiHResult VideoRecordFrameGet (
                                            SstiRecordVideoFrame *pVideoFrame) { return stiRESULT_SUCCESS; }
    
	virtual stiHResult BrightnessRangeGet (
                                           uint32_t *pun32Min,
                                           uint32_t *pun32Max) const { return stiRESULT_SUCCESS; }
    
	virtual uint32_t BrightnessDefaultGet () const { return 0; }
    
	virtual stiHResult BrightnessSet (
                                      uint32_t un32Brightness) { return stiRESULT_SUCCESS; }
    
	virtual stiHResult SaturationRangeGet (
                                           uint32_t *pun32Min,
                                           uint32_t *pun32Max) const { return stiRESULT_SUCCESS; }
    
	virtual uint32_t SaturationDefaultGet () const { return 0; }
    
	virtual stiHResult SaturationSet (
                                      uint32_t un32Saturation) { return stiRESULT_SUCCESS; }
    
	virtual stiHResult PrivacySet (
                                   EstiBool bEnable) { return stiRESULT_SUCCESS; }
    
	virtual stiHResult PrivacyGet (
                                   EstiBool *pbEnabled) const { return stiRESULT_SUCCESS; }
    
	virtual stiHResult MirroredSet (
                                    EstiBool bMirrored) { return stiRESULT_SUCCESS; }
    
	virtual stiHResult MirroredGet (
                                    EstiBool *pbMirrored) const { return stiRESULT_SUCCESS; }
    
	virtual stiHResult KeyFrameRequest () { return stiRESULT_SUCCESS; }
    
	virtual stiHResult CallbackRegister (
                                         PstiObjectCallback pfnVPCallback,
                                         size_t CallbackParam,
                                         size_t CallbackFromId)  { return stiRESULT_SUCCESS; }
    
	virtual stiHResult CallbackUnregister (
                                           PstiObjectCallback pfnVPCallback,
                                           size_t CallbackParam) { return stiRESULT_SUCCESS; }
    
	virtual stiHResult PacketizationSchemesGet (
		std::list<EstiPacketizationScheme> *pPacketizationSchemes) { return stiRESULT_SUCCESS; }
	
	virtual EstiBool RcuConnectedGet () { return estiTRUE; }

#if APPLICATION == APP_VP200
	virtual int GetDeviceFD () const { return 0; }
#endif
    
	CstiVideoInput () {}
	virtual ~CstiVideoInput () {}
	
};

class CstiAudioInput : public IstiAudioInput
{
public:
    CstiAudioInput() {}
    ~CstiAudioInput() {}
    
	virtual stiHResult AudioRecordStart () { return stiRESULT_SUCCESS; }
    
	virtual stiHResult AudioRecordStop () { return stiRESULT_SUCCESS; }
    
	virtual stiHResult AudioRecordPacketGet (
                                             SsmdAudioFrame * pAudioFrame) { return stiRESULT_SUCCESS; }
    
	virtual stiHResult AudioRecordCodecSet (
                                            EstiAudioCodec eAudioCodec) { return stiRESULT_SUCCESS; }
    
	virtual stiHResult AudioRecordSettingsSet (
                                               const SstiAudioRecordSettings * pAudioRecordSettings) { return stiRESULT_SUCCESS; }
    
	virtual stiHResult EchoModeSet (
                                    EsmdEchoMode eEchoMode) { return stiRESULT_SUCCESS; }
    
	virtual stiHResult PrivacySet (
                                   EstiBool bEnable) { return stiRESULT_SUCCESS; }
    
	virtual stiHResult PrivacyGet (
                                   EstiBool *pbEnabled) const { return stiRESULT_SUCCESS; }
    
	virtual stiHResult CallbackRegister (
                                         PstiObjectCallback pfnCallback,
                                         size_t CallbackParam,
                                         size_t CallbackFromId) { return stiRESULT_SUCCESS; }
    
	virtual stiHResult CallbackUnregister (
                                           PstiObjectCallback pfnCallback,
                                           size_t CallbackParam) { return stiRESULT_SUCCESS; }
    
    //#if APPLICATION == APP_VP200
	virtual int GetDeviceFD () const { return 0; }
    //#endif
};

class CstiAudioOutput : public IstiAudioOutput
{	
public:
	CstiAudioOutput () {};
	~CstiAudioOutput () {};

//	static IstiAudioOutput *InstanceGet ();

	virtual stiHResult AudioPlaybackStart () { return stiRESULT_SUCCESS; }
	
	virtual stiHResult AudioPlaybackStop () { return stiRESULT_SUCCESS; }

//#if APPLICATION == APP_NTOUCH
	virtual stiHResult AudioOut (
								 EstiSwitch eSwitch) { return stiRESULT_SUCCESS; }
//#endif
	virtual stiHResult AudioPlaybackCodecSet (
											  EstiAudioCodec eAudioCodec) { return stiRESULT_SUCCESS; }
	
	virtual stiHResult AudioPlaybackPacketPut (
											   SsmdAudioFrame * pAudioFrame) { return stiRESULT_SUCCESS; }
	virtual int GetDeviceFD () { return 0; }
	
protected:
		
private:
	
};


#if TARGET_OS_IPHONE
#include <AudioToolbox/AudioToolbox.h>
#endif

@interface MstiAudibleRinger : NSObject 
#if !TARGET_OS_IPHONE
<NSSoundDelegate>
#endif
{
#if !TARGET_OS_IPHONE
    NSSound* m_sound;
#else
    SystemSoundID m_sound;
#endif
    BOOL m_loopsound;
}

- (void)start;
- (void)stop;

@end

#if TARGET_OS_IPHONE
@interface MstiAudibleRinger (MstiAudibleRinger_Private) 
- (void)sound:(SystemSoundID)sound didFinishPlaying:(BOOL)aBool;
@end

static void myAudioServicesSystemSoundCompletionProc(SystemSoundID ssID, void* clientData) {
    [(MstiAudibleRinger*)clientData sound:ssID didFinishPlaying:YES];
}
#endif

@implementation MstiAudibleRinger

-(id)init {
    if ([super init]) {
    	NSString *defaultFile = [[NSBundle mainBundle] pathForResource:@"uniphone" ofType:@"wav"];
#if !TARGET_OS_IPHONE
    	m_sound = [[NSSound alloc] initWithContentsOfFile:defaultFile byReference:NO];
    	[m_sound setDelegate:self];
#else
    	(void) AudioServicesCreateSystemSoundID((CFURLRef)[NSURL fileURLWithPath:defaultFile isDirectory:NO], &m_sound);
        (void) AudioServicesAddSystemSoundCompletion(m_sound,CFRunLoopGetMain(),kCFRunLoopDefaultMode,myAudioServicesSystemSoundCompletionProc,self);
#endif
        m_loopsound = NO;
    }
    return self;
}

- (void)dealloc {
#if !TARGET_OS_IPHONE
    [m_sound release];
#else
    AudioServicesRemoveSystemSoundCompletion(m_sound);                          
    AudioServicesDisposeSystemSoundID(m_sound);
#endif
    m_sound = NULL;
    [super dealloc];
}

- (void)start {
#if !TARGET_OS_IPHONE
    [m_sound play];
#else
    AudioServicesPlayAlertSound(m_sound);
#endif
    m_loopsound = YES;
}

- (void)stop {
#if !TARGET_OS_IPHONE
    [m_sound stop];
#endif
    m_loopsound = NO;
}

#if !TARGET_OS_IPHONE
- (void)sound:(NSSound *)sound didFinishPlaying:(BOOL)aBool {
#else
- (void)sound:(SystemSoundID)sound didFinishPlaying:(BOOL)aBool {
#endif
    if (aBool && m_loopsound)
        [self start];
}

@end

class CstiAudibleRinger : public IstiAudibleRinger
{
public:
    CstiAudibleRinger() /*: m_sound(NULL)*/ {
//        NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
//        m_sound = [MstiAudibleRinger new];
//        [pool release];
    }
    ~CstiAudibleRinger() {
//        NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
//        [m_sound release];
//        [pool release];
    }
    
    virtual stiHResult Start() {
//        NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
//        [m_sound start];
//        [pool release];
        return stiRESULT_SUCCESS;
    }
    virtual stiHResult Stop() {
//        NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
//        [m_sound stop];
//        [pool release];
        return stiRESULT_SUCCESS;
    }
    
    virtual stiHResult TonesSet (
         EstiTone eTone,
         const SstiTone *pstiTone,	/// Pointer to the array of tones structures
         unsigned int unCount,		/// Number of entries in the array of tones
         unsigned int unRepeatCount	/// Number of times the array of tones should be played before stopping
    ) { return stiRESULT_SUCCESS; }
    
protected:
//    MstiAudibleRinger *m_sound;
};


//class CstiPlatform : public IstiPlatform
//{
//	static IstiPlatform *InstanceGet ();
//	
//	virtual stiHResult Initialize ()  { return stiRESULT_SUCCESS; }	
//	virtual stiHResult Uninitialize ()  { return stiRESULT_SUCCESS; }
//	
//#if APPLICATION == APP_NTOUCH || APPLICATION == APP_VP200
//	virtual stiHResult RestartSystem ()  { return stiRESULT_SUCCESS; }
//#endif
//};

IstiVideoOutput *IstiVideoOutput::InstanceGet ()
{
	static CstiVideoOutput g_oDisplayTask;
	return &g_oDisplayTask;
}

IstiVideoInput *IstiVideoInput::InstanceGet ()
{
	static CstiVideoInput g_oVideoEncodeTask;
 	return &g_oVideoEncodeTask;
}

IstiAudioInput *IstiAudioInput::InstanceGet ()
{
	static CstiAudioInput g_pAudioInput;
	return &g_pAudioInput;
}
	
IstiAudioOutput *IstiAudioOutput::InstanceGet ()
{
	static CstiAudioOutput g_pAudioOutput;
	return &g_pAudioOutput;
}

IstiAudibleRinger *IstiAudibleRinger::InstanceGet ()
{
	static CstiAudibleRinger g_pAudibleRinger;
	return &g_pAudibleRinger;
}
	
IstiPlatform *IstiPlatform::InstanceGet ()
{
	static CstiPlatform g_pPlatform;
	return &g_pPlatform;
}

CstiPlatform::CstiPlatform ()
//	:
//m_pDisplayTask (NULL)
{
}


CstiPlatform::~CstiPlatform()
{
}

stiHResult CstiPlatform::Initialize ()
{
	return stiRESULT_SUCCESS;
}
stiHResult CstiPlatform::Uninitialize ()
{
    return stiRESULT_SUCCESS;
}
stiHResult CstiPlatform::RestartSystem ()
{
    return stiRESULT_SUCCESS;
}
stiHResult CstiPlatform::DisplayTaskStartup ()
{
    return stiRESULT_SUCCESS;
}
