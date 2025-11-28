#ifndef CSTIAUDIOOUTPUT_H
#define CSTIAUDIOOUTPUT_H

// Includes
//

#include <sstream>
#include <memory>

#include "stiSVX.h"
#include "stiError.h"
#include "stiOSMutex.h"
#include "BaseAudioOutput.h"
#include "BaseAudioInput.h"


//
// Constants
//

const int AUDIO_LATENCY_MILLIS = 125; // about 1/8 of a second
const int AUDIO_LATENCY_PACKETS = 8000*AUDIO_LATENCY_MILLIS/1000;
//
// Typedefs
//
typedef struct {
	int16_t *mData;
	uint32_t mCount; // number of packets
	uint32_t mCapacity; // -> not currently being used -> perhaps this whole struct is unecessary.
} SstiAudioBuffer;

//
// Forward Declarations
//


//
// Globals
//

//
// Class Declaration
//
class CstiAudio :  public BaseAudioOutput, public BaseAudioInput
{
private:
    
public:
	CstiAudio ();
	
	stiHResult Initialize ();
	stiHResult Uninitialize();

// playback methods
	virtual stiHResult AudioPlaybackStart ();
	virtual stiHResult AudioPlaybackStop ();
	virtual stiHResult AudioPlaybackCodecSet (
		EstiAudioCodec eAudioCodec);
	virtual stiHResult AudioPlaybackPacketPut (
		SsmdAudioFrame * pAudioFrame) ;
    virtual stiHResult AudioOut (
        EstiSwitch eSwitch);
	virtual void DtmfReceived (EstiDTMFDigit eDtmfDigit);
    
    virtual int GetDeviceFD ();
	
	bool AudioEnabledGet() { return m_playbackEnabled || m_recordEnabled; }
	
// record methods

	stiHResult AudioRecordStart ();
	stiHResult AudioRecordStop ();
	stiHResult AudioRecordPacketGet(
		SsmdAudioFrame * pAudioFrame);
	stiHResult AudioRecordCodecSet (
		EstiAudioCodec eAudioCodec);
	stiHResult AudioRecordSettingsSet (
		const SstiAudioRecordSettings * pAudioRecordSettings);
	stiHResult EchoModeSet (
		EsmdEchoMode eEchoMode);
	stiHResult PrivacySet (
		EstiBool bEnable);
	stiHResult PrivacyGet (
		EstiBool *pbEnabled) const ;
    void AudioFrameAvailable(const char *audioSendBuffer, int length);

	int GetDeviceFD () const;
	
	//IstiAudioInput.h
	virtual stiHResult CodecsGet (
		std::list<EstiAudioCodec> *pCodecs,
		EstiAudioCodec ePreferredCodec);
	//IstiAudioOutput.h
	virtual stiHResult CodecsGet (std::list<EstiAudioCodec> *pCodecs);

	void Reinit();
	
private:
// The file descriptor for the signal to be used
	STI_OS_SIGNAL_TYPE m_fdPSignal;
	stiMUTEX_ID m_audioSessionMutex;
	stiMUTEX_ID m_playbackMutex;
	stiMUTEX_ID m_recordMutex;
	
	bool m_audioOpened;
	bool m_playbackEnabled;
	bool m_recordEnabled;
	bool m_recordPrivacy;
	bool m_bAudioOutEnabled;
		
		
	// #include <sstream>

	class fastbytes {

		private:
        std::stringstream data;

		public:
        fastbytes() {}
        ~fastbytes() {}

        size_t size() { return data.tellp()-data.tellg(); } 
        void read( char* dst, size_t size ) {
           data.read(dst,size);
        }
        void write( const char* bytes, size_t size) {
           data.write(bytes,size); 
        }
				void skip( size_t size) {
					data.seekg( data.tellg() + (std::streampos)size );
				}

	};
	
	fastbytes m_playbackQueue;
	fastbytes m_recordQueue;
	
};

using CstiAudioSharedPtr = std::shared_ptr<CstiAudio>;

#endif //#ifndef CSTIAUDIOOUTPUT_H
