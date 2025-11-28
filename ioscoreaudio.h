
#pragma interface

// workaround to remove warnings, when including OSServiceProviders
#define __OPENTRANSPORTPROVIDERS__

// needed by lists.h of pwlib, unfortunately also defined in previous
// includes from Apple
#undef nil


// static loading of plugins
#define P_FORCE_STATIC_PLUGIN

#include <ptlib.h>
#include <ptlib/sound.h>


#define CA_DUMMY_DEVICE_NAME "iOS"
#define kAudioDeviceDummy kAudioDeviceUnknown



class PSoundChannelIOSAudio : public PSoundChannel
{
 public:
   PSoundChannelIOSAudio();
   PSoundChannelIOSAudio(const PString &device,
                PSoundChannel::Directions dir,
                unsigned numChannels,
                unsigned sampleRate,
                unsigned bitsPerSample);
	~PSoundChannelIOSAudio();
    
    virtual PBoolean Open(const PString & device,
                          Directions dir,
                          unsigned numChannels,
                          unsigned sampleRate,
                          unsigned bitsPerSample);
    
    static PString GetDefaultDevice(
		Directions dir    // Sound I/O direction
	);
    static PStringList GetDeviceNames(
		Directions direction,               ///< Direction for device (record or play)
		PPluginManager * pluginMgr = NULL   ///< Plug in manager, use default if NULL
	);
};