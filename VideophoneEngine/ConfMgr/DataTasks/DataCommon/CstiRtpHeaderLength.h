////////////////////////////////////////////////////////////////////////////////
//
//  Sorenson Communications Inc. Confidential. -- Do not distribute
//  Copyright 2000 - 2012 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: CstiRtpHeaderLength
//
//  File Name:  CstiRtpHeaderLength.h
//
//  Abstract:
//	See CstiRtpHeaderLength.cpp for abstract.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTIRTPHEADERLENGTH_H
#define CSTIRTPHEADERLENGTH_H


class CstiRtpHeaderLength
{
public:
	CstiRtpHeaderLength () = default;

	void MultiplexIDLengthSet (unsigned int unLen);	///< The portion that is for the multiplex ID
	void RtpHeaderLengthSet (unsigned int unLen);	///< The portion that is for the traditional portion of the RTP header
	unsigned int RtpHeaderOffsetLengthGet () const  ///< Return the combined RTP and Multiplex ID lengths.
		{ return m_unRtpHeaderOffset; };
	
private:
	unsigned int m_unRtpHeaderOffset{0};	// The combined RTP and Multiplex ID lengths
	unsigned int m_unMultiplexIDLen{0};	// The portion of the header for the multiplex ID
	unsigned int m_unRtpHeaderLen{0};		// The portion of the header for RTP info
};


#endif // #ifndef CSTIRTPHEADERLENGTH_H
