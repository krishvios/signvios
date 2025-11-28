#ifndef CSTIVIDEOINPUT_H
#define CSTIVIDEOINPUT_H

// Includes
//

#include "stiSVX.h"
#include "stiError.h"
#include "CstiOsTaskMQ.h"
#include "CstiEvent.h"
#include "stiEventMap.h"
#include "CstiVideoCallback.h"
#include "BaseVideoInput.h"
#include "IPlatformVideoInput.h"
#include "IstiVideoPacket.h"
#include <queue>
#include <unordered_map>

using namespace Common;

//
// Constants
//

//
// Class Declaration
//

class CstiVideoInput : public CstiOsTaskMQ, public IPlatformVideoInput, public BaseVideoInput
{
public:

	CstiVideoInput ();
	CstiVideoInput(uint32_t nCallIndex);
	~CstiVideoInput() override;
	
	stiHResult Initialize();
	
	stiHResult VideoRecordStart() override;

	stiHResult VideoRecordStop() override;

	EstiVideoCodec VideoRecordCodecGet() override;

	stiHResult VideoRecordSettingsSet(const SstiVideoRecordSettings *pVideoRecordSettings) override;

	stiHResult VideoRecordFrameGet(SstiRecordVideoFrame * pVideoFrame) override;

	stiHResult PrivacySet(EstiBool bEnable) override;

	stiHResult PrivacyGet(EstiBool *pbEnable) const override;

	stiHResult KeyFrameRequest() override;

	stiHResult VideoCodecsGet(std::list<EstiVideoCodec> *pCodecs) override;

	stiHResult CodecCapabilitiesGet(EstiVideoCodec eCodec, SstiVideoCapabilities *pstCaps) override;

	bool NextFrame(Common::VideoPacket * packet) override;

	static std::unordered_map<uint32_t, std::shared_ptr<CstiVideoInput>> m_videoInputs;

protected:

private:
	std::queue<Common::VideoPacket*> m_qPackets;

	enum EEventType
	{
		estiVIDEOINPUT_NEXT_FRAME = estiEVENT_NEXT,
	};

	stiMUTEX_ID m_EncoderMutex;
	stiMUTEX_ID m_QueueMutex;

	uint32_t m_nCallIndex;
	EstiBool m_bIsRecording;
	EstiBool m_bPrivacy;
	static stiMUTEX_ID m_LockMutex; // Used to secure access to VideoInputList
	
	SstiVideoRecordSettings m_stVideoRecordSettings;

	void instanceRelease();
	static void Lock();
	static void Unlock();

	int Task();

	stiDECLARE_EVENT_MAP(CstiVideoInput);
	stiDECLARE_EVENT_DO_NOW(CstiVideoInput);

	stiHResult stiEVENTCALL ShutdownHandle(CstiEvent* poEvent);  // The event to be handled

}; //CstiVideoInput

#endif //#ifndef CSTIVIDEOINPUT_H

