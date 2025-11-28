#include "BaseAudioInput.h"

ISignal<bool>& BaseAudioInput::packetReadyStateSignalGet ()
{
return packetReadyStateSignal;
}

ISignal<bool>& BaseAudioInput::audioPrivacySignalGet ()
{
		return audioPrivacySignal;
}
