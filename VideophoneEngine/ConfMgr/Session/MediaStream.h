/*!
 * \file MediaStream.h
 * \brief contains MediaStream class
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2009 - 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once


class MediaStream
{
public:

	MediaStream (
		bool inboundSupported,
		bool outboundSupported)
	:
		m_inboundSupported(inboundSupported),
		m_outboundSupported(outboundSupported)
	{
	}

	bool inboundSupported () const
	{
		return m_inboundSupported;
	}

	bool outboundSupported () const
	{
		return m_outboundSupported;
	}

	bool supported () const
	{
		return inboundSupported () || outboundSupported ();
	}
	
	bool isActive () const
	{
		return m_mediaIsActive;
	}
	
	void setActive (bool active)
	{
		m_mediaIsActive = active;
	}

private:

	bool m_inboundSupported = true;
	bool m_outboundSupported = true;
	bool m_mediaIsActive = true;
};


