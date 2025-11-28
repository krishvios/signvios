#include "CropScaleBinBase.h"
#include "stiDebugTools.h"
#include "stiError.h"
#include "stiTrace.h"

CropScaleBinBase::CropScaleBinBase(std::string name, bool enableScale, bool enableConvert)
:
	GStreamerBin(name + "_cropscale_bin", false),
	binName(name + "_cropscale_bin"),
	m_enableScale(enableScale),
	m_enableConvert(enableConvert)
{

}
