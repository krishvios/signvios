#include "BasePlatform.h"

ISignal<const std::string&>& BasePlatform::geolocationChangedSignalGet ()
{
	return geolocationChangedSignal;
}

ISignal<>& BasePlatform::geolocationClearSignalGet ()
{
	return geolocationClearSignal;
}

ISignal<int>& BasePlatform::hdmiInStatusChangedSignalGet ()
{
	return hdmiInStatusChangedSignal;
}

ISignal<const PlatformCallStateChange&>& BasePlatform::callStateChangedSignalGet ()
{
	return callStateChangedSignal;
}
