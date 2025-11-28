#pragma once
#include <string>
#include <boost/optional/optional_io.hpp>

struct CameraInformation
{
	boost::optional<std::string> Description;
	boost::optional<unsigned long> Width;
	boost::optional<unsigned long> Height;
	boost::optional<unsigned long> FrameRate;
	boost::optional<long> Pan;
	boost::optional<long> Tilt;
	boost::optional<long> Roll;
	boost::optional<long> Zoom;
	boost::optional<long> Exposure;
	boost::optional<long> Iris;
	boost::optional<long> Focus;
	boost::optional<long> Brightness;
	boost::optional<long> Contrast;
	boost::optional<long> Hue;
	boost::optional<long> Saturation;
	boost::optional<long> Sharpness;
	boost::optional<long> Gamma;
	boost::optional<long> ColorEnable;
	boost::optional<long> WhiteBalance;
	boost::optional<long> BacklightCompensation;
	boost::optional<long> Gain;
};

class ICameraInformation
{
public:
	virtual void cameraInformationSet(const CameraInformation& cameraInfo) = 0;
};
