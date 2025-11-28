#ifndef CSTIVIDEOINPUT_H
#define CSTIVIDEOINPUT_H

// Includes
//

#include "stiSVX.h"
#include "stiError.h"
#include "CstiOsTaskMQ.h"
#include "CstiEvent.h"
#include "stiEventMap.h"

#include "BaseVideoInput.h"
#include "IPlatformVideoInput.h"
#include "stiVideoDefs.h"
#include "CstiMediaFile.h"
#include "MediumResolutionTimer.h"
#include <boost/optional.hpp>

#include <list>

//
// Constants
//
#define CstiRTPBufferSize 12
#define CstiRTPVideoClockRate 90  // RTP Video Clock rate is 90000 this constant is used to convert to milliseconds (1000)
#define CstiDefaultFrameGap 10 // Used to default the framegap to a value that is not 0.

//bU
// Typedefs
//

//
// Forward Declarations
//


//
// Globals
//

//
// Class Declaration
//

class CstiVideoInput : public CstiOsTaskMQ, public BaseVideoInput
{
public:
	CstiVideoInput ();
	~CstiVideoInput ();
	
	stiHResult Initialize ();
	
	virtual stiHResult VideoRecordStart ();

	virtual stiHResult VideoRecordStop ();
	
	virtual stiHResult VideoRecordSettingsSet (
		const SstiVideoRecordSettings *pVideoRecordSettings);

	virtual stiHResult VideoRecordFrameGet (
		SstiRecordVideoFrame * pVideoFrame);

	virtual stiHResult BrightnessRangeGet (
		uint32_t *pun32Min,
		uint32_t *pun32Max) const;
		
	virtual uint32_t BrightnessDefaultGet () const;
		
	virtual stiHResult BrightnessSet (
		uint32_t un32Brightness);

	virtual stiHResult SaturationRangeGet (
		uint32_t *pun32Min,
		uint32_t *pun32Max) const;
		
	virtual uint32_t SaturationDefaultGet () const;
		
	virtual stiHResult SaturationSet (
		uint32_t un32Saturation);
		
	virtual stiHResult PrivacySet (
		EstiBool bEnable);

	virtual stiHResult PrivacyGet (
		EstiBool *pbEnable) const;

	virtual stiHResult KeyFrameRequest ();

	virtual stiHResult VideoCodecsGet(
		std::list<EstiVideoCodec> *pCodecs);

	virtual stiHResult CodecCapabilitiesGet(
		EstiVideoCodec eCodec,
		SstiVideoCapabilities *pstCaps);

	stiHResult CallbackRegister (
		PstiObjectCallback pfnCallback,
		size_t CallbackParam,
		size_t CallbackFromId);

	stiHResult CallbackUnregister (
		PstiObjectCallback pfnCallback,
		size_t CallbackParam);

	virtual stiHResult PacketizationSchemesGet (
		std::list<EstiPacketizationScheme> *pPacketizationSchemes);

	virtual void ServiceStartStateSet(EstiMediaServiceSupportStates state);

protected:

private:
	Common::MediumResolutionTimer m_timer{};
	EstiBool m_bPrivacy;
	EstiBool m_bInUse;
	static std::list<CstiVideoInput *> m_VideoInputList;
	static std::recursive_mutex m_videoInputListLock; // Used to secure access to VideoInputList
	SstiVideoRecordSettings m_stVideoRecordSettings;
	EstiMediaServiceSupportStates m_eMediaServiceSupportState;
	UINT32 m_un32LastTimeStamp;
	CstiMediaFile *m_pMediaFile;

	/*!
	 * \brief release instance
	 */
	void instanceRelease ();

	void InUseSet();
	EstiBool InUseGet ();
	static std::list<CstiVideoInput*> VideoInputListGet () {return m_VideoInputList;};  // erc keep - still need?
	void Lock () = delete;
	void Unlock () = delete;

public:
	static void UninitList ();
	static stiHResult InitList (int nMaxCalls);

	/*!
	 * \brief Create instance
	 * 
	 * \return IstiVideoInput* 
	 */
	static IstiVideoInput *InstanceCreate ();

private:

	int Task ();

	stiDECLARE_EVENT_MAP (CstiVideoInput);
	stiDECLARE_EVENT_DO_NOW (CstiVideoInput);

	stiHResult stiEVENTCALL ShutdownHandle (
		CstiEvent* poEvent);  // The event to be handled

	/*!
	 * \brief 
	 * 
	 * \return stiHResult 
	 */
	stiHResult HSEndService(EstiMediaServiceSupportStates eMediaServiceSupportState);

	// State that the media file will start in during the first call to OpenByDescription
	boost::optional<EstiMediaServiceSupportStates> m_serviceStartState;

	int m_nHighResolutionTimerID;
	int m_nFrameTimeinMS;
	bool m_bIsStarted;
	// Timer version is used to cancel an older version of the timer when a new one is created
	int m_currentTimerVersion = 0;

	struct FrameTimerArgs
	{
		CstiVideoInput *videoInput;
		int timerVersion = 0;
	};

	void static CALLBACK TimerTick (PVOID param, BOOLEAN timerWaitOrFired)
	{
		FrameTimerArgs *args = (FrameTimerArgs *)(param);
		std::lock_guard<std::recursive_mutex> lock (CstiVideoInput::m_videoInputListLock);
		if (args->timerVersion == args->videoInput->m_currentTimerVersion)
		{
			if (args->videoInput->m_bIsStarted)
			{
				args->videoInput->packetAvailableSignalGet ().Emit ();
				args->videoInput->m_timer.OneShot (CstiVideoInput::TimerTick, args, args->videoInput->m_nFrameTimeinMS);
			}
		}
		else
		{
			// Since this timer is no longer used, clean up the args object
			delete args;
		}
	}

}; //CstiVideoInput

#endif //#ifndef CSTIVIDEOINPUT_H
