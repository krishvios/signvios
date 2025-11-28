#pragma once
#include <gst/gst.h>
#include <gst/pbutils/encoding-profile.h>
#include <sstream>
#include "GStreamerCaps.h"

class GStreamerEncodingVideoProfile
{
public:
	GStreamerEncodingVideoProfile() = default;

	GStreamerEncodingVideoProfile(GStreamerCaps &inputCaps, GStreamerCaps &outputCaps)
	{
		m_profile = gst_encoding_video_profile_new(outputCaps.get(), nullptr, inputCaps.get(), 0);
	}

	GStreamerEncodingVideoProfile(const GStreamerEncodingVideoProfile &other) = delete;
	GStreamerEncodingVideoProfile &operator=(GStreamerEncodingVideoProfile &other) = delete;

	GStreamerEncodingVideoProfile(GStreamerEncodingVideoProfile &&other)
	{
		if (other.m_profile)
		{
			m_profile = other.m_profile;
			other.m_profile = nullptr;
		}
	}

	GStreamerEncodingVideoProfile &operator = (GStreamerEncodingVideoProfile &&other)
	{
		if (this != &other)
		{
			clear();
			if (other.m_profile)
			{
				m_profile = other.m_profile;
				other.m_profile = nullptr;
			}
		}
		return *this;
	}

	GstEncodingVideoProfile *get()
	{
		return m_profile;
	}

	~GStreamerEncodingVideoProfile()
	{
		clear();
	}

	void clear()
	{
		if (m_profile)
		{
			gst_encoding_profile_unref(GST_ENCODING_PROFILE(m_profile));
			m_profile = nullptr;
		}
	}

private:
	GstEncodingVideoProfile *m_profile { nullptr };
};
