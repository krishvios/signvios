#include "BaseAudioOutput.h"

ISignal<bool>& BaseAudioOutput::readyStateChangedSignalGet ()
{
	return readyStateChangedSignal;
}

ISignal<EstiDTMFDigit>& BaseAudioOutput::dtmfReceivedSignalGet ()
{
	return dtmfReceivedSignal;

}