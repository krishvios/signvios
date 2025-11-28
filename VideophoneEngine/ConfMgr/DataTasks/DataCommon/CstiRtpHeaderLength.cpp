////////////////////////////////////////////////////////////////////////////////
//
//  Sorenson Communications Inc. Confidential. -- Do not distribute
//  Copyright 2000 - 2012 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: CstiRtpHeaderLength
//
//  File Name:  CstiRtpHeaderLength.cpp
//
//  Abstract:
//	This class is used to determine the length of the header that will be added
//	to the beginning of each packet that is sent.  The header is made up of
//	the traditional RTP header plus any extra space needed for a multiplex ID
//	(see H.460.19 for info about the multiplex ID).
//
////////////////////////////////////////////////////////////////////////////////

#include "CstiRtpHeaderLength.h"


void CstiRtpHeaderLength::MultiplexIDLengthSet (unsigned int unLen) 
{ 
	  m_unMultiplexIDLen = unLen;
	  m_unRtpHeaderOffset = m_unRtpHeaderLen + m_unMultiplexIDLen;
};


void CstiRtpHeaderLength::RtpHeaderLengthSet (unsigned int unLen) 
{
	m_unRtpHeaderLen = unLen;
	m_unRtpHeaderOffset = m_unRtpHeaderLen + m_unMultiplexIDLen;
};
