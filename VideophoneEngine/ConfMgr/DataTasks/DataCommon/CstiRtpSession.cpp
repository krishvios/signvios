/*!
* \file RtpSession.cpp
* \brief Represents all RTP/RTCP session information that is
*        shared by a playback/record pair for a given media strea 
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2017 Sorenson Communications, Inc. -- All rights reserved
*/

#include "stiTrace.h"
#include "CstiRtpSession.h"
#include "rvrtcpfb.h"
#include "payload.h"
#include "rtcp.h"
#include "rvsrtpconfig.h"


#ifdef stiTUNNEL_MANAGER
#include "tsc_socket_api.h"
#include "rvsockettun.h"
#include "rvrtpseli.h"
#include "rvsocket.h"
#include "rvares.h"
#include "rtp.h"
#endif

#include <sstream>
#include <iomanip>

namespace vpe
{

/*!
 * \brief Constructor
 * \param params - initial session parameters
 */
RtpSession::RtpSession (
	const RtpSession::SessionParams &params)
:
	m_params (params)
{
	RvInt ipType = params.ipv6Enabled ? RV_ADDRESS_TYPE_IPV6 : RV_ADDRESS_TYPE_IPV4;
	
	RvAddressConstruct (ipType, &m_remoteRtpAddress);
	RvAddressConstruct (ipType, &m_remoteRtcpAddress);

	// Setup the default feedback handlers.
	m_defaultFeedbackHandlers.fbEventSLI = [](RvRtcpSession, RvUint32, RvUint32, RvUint32, RvUint32) { return RV_OK; };
	m_defaultFeedbackHandlers.fbEventPLI = [](RvRtcpSession, RvUint32) { return RV_OK; };
	m_defaultFeedbackHandlers.fbEventRPSI = [](RvRtcpSession, RvUint32, RvUint32, RvUint8*, RvUint32) { return RV_OK; };
	m_defaultFeedbackHandlers.fbEventAFB = [](RvRtcpSession, RvUint32, RvUint32*, RvUint32) { return RV_OK; };
	m_defaultFeedbackHandlers.fbEventNACK = [](RvRtcpSession, RvUint32, RvUint16, RvUint16) { return RV_OK; };
	m_defaultFeedbackHandlers.fbEventUnknown = [](RvRtcpSession, RvUint32, RvUint32*, RvUint32) { return RV_OK; };
	m_defaultFeedbackHandlers.fbEventFIR = [](RvRtcpSession, RvUint32, RvUint32) { return RV_OK; };
	m_defaultFeedbackHandlers.fbEventTMMBR = [](RvRtcpSession, RvUint32, RvUint32, RvUint32) { return RV_OK; };
	m_defaultFeedbackHandlers.fbEventTMMBN = [](RvRtcpSession, RvUint32, RvUint32, RvUint32) { return RV_OK; };

	m_feedbackHandlers = m_defaultFeedbackHandlers;
}


/*!
 * \brief Destructor
 */
RtpSession::~RtpSession ()
{
	close ();
}


/*!
 * \brief Open the rtp/rtcp sessions
 */
stiHResult RtpSession::open ()
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	auto hResult = stiRESULT_SUCCESS;

	// Don't proceed if the session is already open
	stiTESTCONDMSG (m_rtp == nullptr, stiRESULT_ERROR, "RtpSession already open\n");

	// If there were transports passed in, use them
	// Otherwise try to find ports on our own
	if (m_transports.RtpTransportGet() &&
		m_transports.RtcpTransportGet())
	{
		return openFromTransports ();
	}

	return openFromPorts ();

STI_BAIL:

	return hResult;
}


/*!\brief Sets the remove handler to reject any attempts to remove the remote address due to inactivity
 *
 * Radvision will remove the remote address of the RTCP session if it is not getting reports from the remote
 * endpoint (after about 25 seconds). We don't want this to happen as it could be due to a temporary interruption
 * in the network.
 */
void rtcpRemoveHandlerSet(
	RvRtcpSession rtcp)
{
	RvRtcpSetSsrcRemoveEventHandler (rtcp, [](RvRtcpSession, void *, RvUint32) { return RV_FALSE; }, nullptr);
}


/*!
 * \brief Helper method for opening from transports
 */
stiHResult RtpSession::openFromTransports ()
{
	auto hResult = stiRESULT_SUCCESS;

	if (m_dtlsContext == nullptr)
	{
		stiASSERT (!m_rtp);
		stiASSERT (!m_rtcp);

		stiTrace ("Opening RTP and RTCP sessions using transports %p and %p\n",
			m_transports.RtpTransportGet (),
			m_transports.RtcpTransportGet ());
		
		if (!m_transports.RtcpTransportGet () && !m_transports.isRtcpMux ())
		{
			stiASSERTMSG (false, "There is no rtcp transport this will cause open to fail. Attempting to use rtcp-mux");
			m_transports.isRtcpMuxSet (true);
		}
		
		if (m_transports.isRtcpMux ())
		{
			RvMultiplexerConfig mxCfg{};

			mxCfg.flags = RV_MULTIPLEXER_CREATE_RTP_TRANSPORT | RV_MULTIPLEXER_CREATE_RTCP_TRANSPORT;
			mxCfg.underlyingTransport = m_transports.RtpTransportGet ();

			mxCfg.localAddress = const_cast<RvAddress*>(m_transports.RtpLocalAddressGet());

			auto created = RvMultiplexerCreateTransports(&mxCfg);
			stiASSERTMSG(created == RV_TRUE, "openFromTransports failed to create multiplex connection.");
			
			m_transports.RtpTransportSet (mxCfg.rtpTransport, true);
			m_transports.RtcpTransportSet (mxCfg.rtcpTransport, true);
		}

		m_rtp = RvRtpOpenEx2 (
			m_transports.RtpTransportGet (),
			1,
			0xff,
			(char*)"",
			m_transports.RtcpTransportGet ());

		stiTESTCOND (m_rtp, stiRESULT_ERROR);

		// That also should have started rtcp, but we do need a handle to it.
		m_rtcp = RvRtpGetRTCPSession (m_rtp);
		stiTESTCOND (m_rtcp, stiRESULT_ERROR);

		rtcpRemoveHandlerSet (m_rtcp);

		stiASSERT (m_rtpEventHandler != nullptr);
		stiASSERT (m_rtpEventHandlerContext != nullptr);

		RvRtpSetEventHandler (m_rtp, m_rtpEventHandler, m_rtpEventHandlerContext);

		if (m_params.srtpEncrypted)
		{
			hResult = sdesSrtpAesEncryptionBegin ();
		}

		remoteAddressSet (&m_remoteRtpAddress, &m_remoteRtcpAddress);

		// Construct the rtcp feedback plugin if necessary
		constructRtcpFeedbackPlugin ();
	}

STI_BAIL:
	return hResult;
}


/*!
 * \brief Helper method for opening the session with custom ports
 */
stiHResult RtpSession::openFromPorts ()
{
	auto hResult = stiRESULT_SUCCESS;
	if (m_dtlsContext == nullptr)
	{
		RvNetAddress netAddr;
		memset (&netAddr, 0, sizeof netAddr);
		RvNetIpv4 netIp;
		RvNetIpv6 netIp6{};
		memset (&netIp, 0, sizeof netIp);

		// TODO:  This is inheriting an empty ip address for unspecified
		// local address.  According to the srtp doc, page 9-ish I don't
		// think this is allowed.
		if (m_params.ipv6Enabled == true)
		{
			RvNetCreateIpv6 (&netAddr, &netIp6);
		}
		else
		{
			RvNetCreateIpv4 (&netAddr, &netIp);
		}

#ifdef stiTUNNEL_MANAGER
		if (m_params.tunneled)
		{
			// Set callback to enable DDT so UDP tunnel is used if available
			rtpTransportProvideCallbackSet (RtpSession::rtpTransportProvideCallback);
		}
#endif

		// Open the rtp session with a port of 0 to have Radvision select
		// a port from the range we set on the rtp instance.
		m_rtp = RvRtpOpenEx (&netAddr, 1, 0xff, (char*)"");
		stiTESTCOND (m_rtp, stiRESULT_ERROR);

#ifdef stiTUNNEL_MANAGER
		if (m_params.tunneled)
		{
			// This seems really strange to me given that we just
			// set the callback above... Maybe it's because sessions
			// for video, audio, and text all need to use the callback
			// mechanism, but it's global?
			rtpTransportProvideCallbackSet (nullptr);

			stiASSERT (m_rtpEventHandler != nullptr);
			stiASSERT (m_rtpEventHandlerContext != nullptr);

			RvRtpSetEventHandler (m_rtp, m_rtpEventHandler, m_rtpEventHandlerContext);
		}
#endif

		// Open the rtcp session
		m_rtcp = RvRtpGetRTCPSession (m_rtp);
		stiTESTCOND (m_rtcp, stiRESULT_ERROR);
		rtcpRemoveHandlerSet (m_rtcp);

		// Radvision docs say to init struct before calling open,
		// but you need a session before being able to open
		if (m_params.srtpEncrypted)
		{
			hResult = sdesSrtpAesEncryptionBegin ();
		}

		remoteAddressSet (&m_remoteRtpAddress, &m_remoteRtcpAddress);

		// Construct the rtcp feedback plugin if necessary
		constructRtcpFeedbackPlugin ();
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		// If there was an error and we still have an RTP session close it.
		if (m_rtp)
		{
			RvRtpClose (m_rtp);
			m_rtp = nullptr;
		}
	}
	return hResult;
}


/*!
* \brief Ends the current RTP/RTCP session
*/
stiHResult RtpSession::close ()
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	stiLOG_ENTRY_NAME (vpe::RtpSession::close);

	stiDEBUG_TOOL (g_stiRtpSessionDebug,
		if (m_rtp)
		{
		   stiTrace ("<RtpSession::close> Shutting down rtp for port %d\n", RvRtpGetPort (m_rtp));
		}
	);

	bool rtpRtcpLinked = false;
	RvStatus rvStatus{ RV_OK };

	if (m_rtcp)
	{
		rvStatus = RvRtcpStop (m_rtcp);
		stiASSERT (rvStatus == RV_OK);

		RvRtcpRemoveAllRemoteAddresses (m_rtcp);

		destructRtcpFeedbackPlugin ();
	}
	
	if (m_dtlsSrtpSession)
	{
		RvDtlsSrtpSessionDelete (m_dtlsSrtpSession);
		m_dtlsSrtpSession = nullptr;
	}

	if (m_rtp)
	{
		rvStatus = RvRtpRemoveAllRemoteAddresses (m_rtp);
		stiASSERT (rvStatus == RV_OK);

		srtpAesEncryptionEnd ();

		// Register an event handler to handle any residual data
		RvRtpSetEventHandler (m_rtp, rtpDataThrowAwayCallback, nullptr);

		// See if the RTCP session is linked with the RTP session.
		// If it is then we don't need to close the RTCP session ourselves.
		if (m_rtcp && m_rtcp == RvRtpGetRTCPSession (m_rtp))
		{
			rtpRtcpLinked = true;
		}

		rvStatus = RvRtpResume (m_rtp);
		stiASSERT (rvStatus == RV_OK);

		rvStatus = RvRtpClose (m_rtp);
		stiASSERT (rvStatus == RV_OK);

		m_rtp = nullptr;
	}

	if (m_rtcp)
	{
		if (!rtpRtcpLinked)
		{
			rvStatus = RvRtcpClose (m_rtcp);
			stiASSERT (rvStatus == RV_OK);
		}

		m_rtcp = nullptr;
	}

	m_dtlsContext = nullptr;

	// We set the transports to null to ensure that CstiTransport
	// is destructed now causing RvTransportRelease to be called if needed.
	m_transports.RtpTransportSet (nullptr, false);
	m_transports.RtcpTransportSet (nullptr, false);

	// Since we ended the session, clear the remote address information
	RvAddressConstruct (RV_ADDRESS_TYPE_IPV4, &m_remoteRtpAddress);
	RvAddressConstruct (RV_ADDRESS_TYPE_IPV4, &m_remoteRtcpAddress);

	return stiRESULT_SUCCESS;
}


/*!
 * \brief Determines if the session is open or not
 */
bool RtpSession::isOpen ()
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	return (m_rtp != nullptr);
}


/*!
* \brief Registers a throwaway callback for unwanted media during a hold
* \return stiRESULT_SUCCESS
*/
stiHResult RtpSession::hold ()
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	stiLOG_ENTRY_NAME (vpe::RtpSession::hold);

	// If we an rtp session but we are not using transports then set the
	// event handler to throw away packets
	if (m_rtp && !rtpTransportGet () && !tunneledGet())
	{
		RvRtpSetEventHandler (m_rtp, rtpDataThrowAwayCallback, nullptr);
	}

	return stiRESULT_SUCCESS;
}


/*!
* \brief Call for playback channels to unregister the throwaway callback used during a hold.
* \return stiRESULT_SUCCESS
*/
stiHResult RtpSession::resume ()
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	stiLOG_ENTRY_NAME (vpe::RtpSession::resume);

	// If we an rtp session but we are not using transports then set the
	// event handler to allow the data task to retrieve data on the port.
	if (m_rtp && !rtpTransportGet () && !tunneledGet())
	{
		RvRtpSetEventHandler (m_rtp, nullptr, nullptr);
	}

	return stiRESULT_SUCCESS;
}


/*!
 * \brief Gets the low level rtp session handle
 * \return RvRtpSession handle
 */
RvRtpSession RtpSession::rtpGet () const
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	return m_rtp;
}

/*!
 * \brief Gets the low level rtcp session handle
 * \return RvRtcpSession handle
 */
RvRtcpSession RtpSession::rtcpGet () const
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	return m_rtcp;
}

/*!
 * \brief Gets the low level rtp transport handle
 * \return RvTransport for RTP
 */
RvTransport RtpSession::rtpTransportGet () const
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	return m_transports.RtpTransportGet ();
}


/*!
 * \brief Gets the low level rtcp transport handle
 * \return RvTransport for RTCP
 */
RvTransport RtpSession::rtcpTransportGet () const
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	return m_transports.RtcpTransportGet ();
}


/*!
 * \brief Sets the transports used by this session and will close the current session
 *        and open a new session if the transports are different that the current ones
 * \param transports - RTP/RTCP transports
 * \return
 */
stiHResult RtpSession::transportsSet (
	const CstiMediaTransports &transports)
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	auto hResult = stiRESULT_SUCCESS;

	if (m_transports.RtpTransportGet() != transports.RtpTransportGet() &&
		m_transports.RtcpTransportGet() != transports.RtcpTransportGet())
	{
		bool open = isOpen ();

		if (m_transports.RtpTransportGet () != nullptr || m_transports.RtcpTransportGet () != nullptr)
		{
			close (); // If there already is a connection, end it now.
		}

		m_transports = transports;

		RvAddressCopy (m_transports.RtpRemoteAddressGet(), &m_remoteRtpAddress);
		RvAddressCopy (m_transports.RtcpRemoteAddressGet(), &m_remoteRtcpAddress);

		m_dtlsRenegotiationRequired = true;

		if (open)
		{
			hResult = openFromTransports ();
			stiTESTRESULT ();
		}
	}
	else
	{
		stiASSERTMSG (
				false,
				"RTP and RTCP transports already in use: %p and %p\n",
				transports.RtpTransportGet (),
				transports.RtcpTransportGet ());
	}

STI_BAIL:
	if (stiIS_ERROR (hResult))
	{
		close ();
	}

	return hResult;
}

/*!
* \brief Sets the remote address/port for the session
* \return none
*/
void RtpSession::remoteAddressSet (
	const RvAddress *rtpAddress,
	const RvAddress *rtcpAddress)
{
	// Do not call SetRemoteAddress methods when using DTLS
	if (m_dtlsContext == nullptr)
	{
		std::lock_guard<std::recursive_mutex> lock (m_mutex);

		RvAddressCopy (rtpAddress, &m_remoteRtpAddress);
		RvNetAddress rtpNetAddress;
		RvAddressToRvNetAddress (&m_remoteRtpAddress, &rtpNetAddress);
		RvRtpSetRemoteAddress (m_rtp, &rtpNetAddress);

		RvAddressCopy (rtcpAddress, &m_remoteRtcpAddress);
		RvNetAddress rtcpNetAddress;
		RvAddressToRvNetAddress (&m_remoteRtcpAddress, &rtcpNetAddress);
		RvRtcpSetRemoteAddress (m_rtcp, &rtcpNetAddress);

		if (m_srtpAesPluginStarted)
		{
			destinationContextRemove ();
			destinationContextAdd ();
		}
	}
}

void RtpSession::destinationContextAdd ()
{
	RvNetAddress rtpNetAddress;
	RvAddressToRvNetAddress (&m_remoteRtpAddress, &rtpNetAddress);

	auto rvStatus = RvSrtpAddDestinationContext (
		m_rtp,
		&rtpNetAddress,
		RvSrtpStreamTypeSRTP,
		(RvUint8*)m_params.encryptKey.mkiGet ().c_str (),
		RV_FALSE
	);
	stiASSERTMSG (RV_OK == rvStatus, "Error adding srtp destination context");

	RvNetAddress rtcpNetAddress;
	RvAddressToRvNetAddress (&m_remoteRtcpAddress, &rtcpNetAddress);

	rvStatus = RvSrtpAddDestinationContext (
		m_rtp,
		&rtcpNetAddress,
		RvSrtpStreamTypeSRTCP,
		(RvUint8*)m_params.encryptKey.mkiGet ().c_str (),
		RV_FALSE
	);
	stiASSERTMSG (RV_OK == rvStatus, "Error adding srtcp destination context");
}

void RtpSession::destinationContextRemove ()
{
	RvNetAddress rtpNetAddress;
	RvAddressToRvNetAddress (&m_remoteRtpAddress, &rtpNetAddress);

	auto rvStatus = RvSrtpDestinationClearAllKeys (
		m_rtp,
		&rtpNetAddress);
	stiASSERTMSG (RV_OK == rvStatus, "Error clearing srtp destination context");

	RvNetAddress rtcpNetAddress;
	RvAddressToRvNetAddress (&m_remoteRtcpAddress, &rtcpNetAddress);

	rvStatus = RvSrtpDestinationClearAllKeys (
		m_rtp,
		&rtcpNetAddress);
	stiASSERTMSG (RV_OK == rvStatus, "Error clearing srtcp destination context");
}


/*!
* \brief Sets the remote address/port for the session
* \param addressChanged Out parameter that is set to true if address changed,
*        false otherwise
* \return none
*/
void RtpSession::remoteAddressSet (
	const RvAddress *rtpAddress,
	const RvAddress *rtcpAddress,
	bool *addressChanged)
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	bool changed = false;

	if (!RvAddressCompare (&m_remoteRtpAddress,  rtpAddress,  RV_ADDRESS_FULLADDRESS) ||
		!RvAddressCompare (&m_remoteRtcpAddress, rtcpAddress, RV_ADDRESS_FULLADDRESS))
	{
		remoteAddressSet(rtpAddress, rtcpAddress);
		changed = true;
	}

	if (addressChanged)
	{
		*addressChanged = changed;
	}
}


/*!
 * \brief Compares remote RTP and RTCP address to those currently set to see if they have changed.
 */
bool RtpSession::remoteAddressCompare (
	const RvAddress &rtpAddress,
	const RvAddress &rtcpAddress)
{
	return (!RvAddressCompare (&m_remoteRtpAddress, &rtpAddress, RV_ADDRESS_FULLADDRESS)
		|| !RvAddressCompare (&m_remoteRtcpAddress, &rtcpAddress, RV_ADDRESS_FULLADDRESS));
}


/*!
 * \brief Gets the remote address/port for rtp and rtcp
 */
stiHResult RtpSession::RemoteAddressGet (
	std::string *rtpAddress,
	int *rtpPort,
	std::string *rtcpAddress,
	int *rtcpPort)
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	auto hResult = stiRESULT_SUCCESS;

	RvChar addrTxt1[stiIP_ADDRESS_LENGTH + 1];

	if (rtpAddress)
	{
		*rtpAddress = RvAddressGetString (&m_remoteRtpAddress, sizeof (addrTxt1), addrTxt1);
	}

	if (rtpPort)
	{
		*rtpPort = RvAddressGetIpPort (&m_remoteRtpAddress);
	}

	if (rtcpAddress)
	{
		*rtcpAddress = RvAddressGetString (&m_remoteRtcpAddress, sizeof (addrTxt1), addrTxt1);
	}

	if (rtcpPort)
	{
		*rtcpPort = RvAddressGetIpPort (&m_remoteRtcpAddress);
	}

	return hResult;
}


/*!
 * \brief Gets the local address/port for rtp and rtcp
 */
stiHResult RtpSession::LocalAddressGet (
	std::string *rtpAddress,
	int *rtpPort,
	std::string *rtcpAddress,
	int *rtcpPort)
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	auto hResult = stiRESULT_SUCCESS;

	if (rtpTransportGet ())
	{
		hResult = m_transports.LocalAddressesGet (rtpAddress, rtpPort, rtcpAddress, rtcpPort);
	}
	else if (m_rtp)
	{
		if (rtpPort)
		{
			*rtpPort = RvRtpGetPort (m_rtp);
		}

		if (rtcpPort)
		{
			RvAddress outAddress;
			auto rvStatus = RvRtcpGetLocalAddress (m_rtcp, &outAddress);
			stiTESTRVSTATUS ();
			*rtcpPort = outAddress.data.ipv4.port;
		}
	}
STI_BAIL:

	return hResult;
}


/*!
* \brief Handler for incoming RTP data from RadVision stack and discards it
*/
void RtpSession::rtpDataThrowAwayCallback (
	RvRtpSession rtp,
	void *callbackParam)
{
	stiLOG_ENTRY_NAME (vpe::RtpSession::rtpDataThrowAwayCallback);

	RvRtpParam rtpParam;
	RvRtpParamConstruct(&rtpParam);

	char buf[2000] = { 0 };
	RvRtpRead (rtp, buf, sizeof(buf), &rtpParam);
}


/*!
* \brief Return the remote rtcp ip address
*/
unsigned int RtpSession::rtcpRemoteIpGet () const
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	stiLOG_ENTRY_NAME (vpe::RtpSession::RtcpRemoteIpGet);
	return m_rtcpRemoteIp;
}


/*!
* \brief Store the remote rtcp ip address
* \param unIP - The remote rtcp address
*/
void RtpSession::rtcpRemoteIpSet (unsigned int ipAddress)
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	stiLOG_ENTRY_NAME (vpe::RtpSession::rtcpRemoteIpSet);
	m_rtcpRemoteIp = ipAddress;
}


/*!
* \brief Return the remote rtcp port for this channel
* \return The remote rtcp port
*/
unsigned int RtpSession::rtcpRemotePortGet () const
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	stiLOG_ENTRY_NAME (vpe::RtpSession::rtcpRemotePortGet);
	return m_rtcpRemotePort;
}

/*!
* \brief Store the remote rtcp port for this channel
* \param unPort - The remote rtcp port
*/
void RtpSession::rtcpRemotePortSet (unsigned int port)
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	stiLOG_ENTRY_NAME (vpe::RtpSession::rtcpRemotePortSet);
	m_rtcpRemotePort = port;
}


/*!
 * \brief Gets the tunneled flag
 * \return true if tunneling is used, false otherwise
 */
bool RtpSession::tunneledGet () const
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	return m_params.tunneled;
}


/*!
 * \brief RtpSession::tunneledSet - sets tunneling to be used for this session
 * \param tunneled - determines whether or not tunneling is used
 */
void RtpSession::tunneledSet (bool tunneled)
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	m_params.tunneled = tunneled;
}


/*!
 * \brief Determines if FIR is enabled for at least one payload type
 */
bool RtpSession::rtcpFeedbackFirEnabledGet () const
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	return m_params.rtcpFeedbackFirEnabled;
}


/*!
 * \brief Sets the FIR flag, indicating that this session supports FIR feedback via RTCP
 *        for at least one payload type
 */
void RtpSession::rtcpFeedbackFirEnabledSet (bool enabled)
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	m_params.rtcpFeedbackFirEnabled = enabled;
}


/*!
 * \brief Determines if PLI is enabled for at least one payload type
 */
bool RtpSession::rtcpFeedbackPliEnabledGet () const
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	return m_params.rtcpFeedbackPliEnabled;
}


/*!
 * \brief Sets the PLI flag, indicating that this session supports PLI feedback via RTCP
 *        for at least one payload type
 */
void RtpSession::rtcpFeedbackPliEnabledSet (bool enabled)
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	m_params.rtcpFeedbackPliEnabled = enabled;
}


/*!
 * \brief Determines if AFB is enabled for at least one payload type
 */
bool RtpSession::rtcpFeedbackAfbEnabledGet () const
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	return m_params.rtcpFeedbackAfbEnabled;
}


/*!
 * \brief Sets the AFB flag, indicating that this session supports AFB feedback via RTCP
 *        for at least one payload type
 */
void RtpSession::rtcpFeedbackAfbEnabledSet (bool enabled)
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	m_params.rtcpFeedbackAfbEnabled = enabled;
}


/*!
 * \brief Determines if TMMBR is enabled for at least one payload type
 */
bool RtpSession::rtcpFeedbackTmmbrEnabledGet () const
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	return m_params.rtcpFeedbackTmmbrEnabled;
}


/*!
 * \brief Sets the TMMBR flag, indicating that this session supports TMMBR feedback via RTCP
 *        for at least one payload type
 */
void RtpSession::rtcpFeedbackTmmbrEnabledSet (bool enabled)
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	m_params.rtcpFeedbackTmmbrEnabled = enabled;
}
	

/*!
 * \brief Sets the NACK/RTX flag, indicating that this session supports NACK feedback via RTCP
 *        for at least one payload type and accepts retransmission of missing data.
 */
void RtpSession::rtcpFeedbackNackRtxEnabledSet (bool enabled)
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	m_params.rtcpFeedbackNackRtxEnabled = enabled;
}


/*!
 * \brief Determines if NACK and RTX is enabled for at least one payload type
 */
bool RtpSession::rtcpFeedbackNackRtxEnabledGet () const
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	return m_params.rtcpFeedbackNackRtxEnabled;
}


bool RtpSession::decryptKeyChanged (const vpe::SDESKey &decryptKey)
{
	return (!m_params.decryptKey.keyGet().empty() &&
			(m_params.decryptKey.keyGet() != decryptKey.keyGet()));
}

void RtpSession::createNewSsrc ()
{
	auto newSsrc = RvRtpRegenSSRC(m_rtp);
	RvRtpStream *rtpStream = RvRtpSessionFindStreamNL (m_rtp, newSsrc, false);
	rtpStream->roc = 0;
}


/*!
 * \brief Sets the SDES keys used for encryption/decryption
 */

void RtpSession::SDESKeysSet (const vpe::SDESKey &encryptKey, const vpe::SDESKey &decryptKey)
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);

	// only set mki if these are new keys
	if(m_params.encryptKey.keyGet() != encryptKey.keyGet() || m_params.encryptKey.mkiGet().empty())
	{
		auto mki = uniqueMkiGet();
		m_params.encryptKey = encryptKey;
		m_params.encryptKey.mkiSet(mki);
	}

	// The same for the decryption key
	if(m_params.decryptKey.keyGet() != decryptKey.keyGet() || m_params.decryptKey.mkiGet().empty())
	{
		auto mki = uniqueMkiGet();
		m_params.decryptKey = decryptKey;
		m_params.decryptKey.mkiSet(mki);
	}

	m_params.srtpEncrypted = true;

	auto keyExchangeMethod = SDESKey::suiteToKeyExchangeMethod (encryptKey.suiteGet ());
	m_keyExchangeMethod = keyExchangeMethod;

	// NOTE: this seems like an odd place to call this
	// But, without ice, this appears to be the only place that it gets called
	// TODO: Maybe, we have construction step, and then enable/add keys... ?
	sdesSrtpAesEncryptionBegin ();
}

void RtpSession::dtlsContextSet(std::shared_ptr<DtlsContext> dtlsContext)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);
	m_dtlsContext = dtlsContext;
	m_keyExchangeMethod = KeyExchangeMethod::DTLS;
}

void RtpSession::encryptionDisabled ()
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);

	m_dtlsContext = nullptr;
	m_params.srtpEncrypted = false;

	m_keyExchangeMethod = KeyExchangeMethod::None;
	if (m_encryptionState != vpe::EncryptionState::NONE)
	{
		m_encryptionState = vpe::EncryptionState::NONE;
		encryptionStateChangedSignal.Emit(m_encryptionState);
	}
}

void RtpSession::onDtlsCompleteCallbackSet (const std::function<void ()>& callback)
{
	m_onDtlsCompleteCallback = callback;
}


void RtpSession::dtlsBegin ()
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	if (m_dtlsSrtpSession == nullptr)
	{
		dtlsSrtpAesEncryptionBegin ();
	}
	else if (m_dtlsRenegotiationRequired)
	{
		auto rvStatus = RvDtlsSrtpSessionRenegotiate (m_dtlsSrtpSession, RV_DTLS_SRTP_RENEG_BOTH);
		stiASSERT (rvStatus == RV_OK);
	}
	m_dtlsRenegotiationRequired = false;
}

/*!
 * \brief sets flag once rtcp stream has been configured for srtp
 */
void RtpSession::rtcpSrtpStreamAddedSet()
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	m_params.rtcpStreamAdded = true;
}


/*!
 * \brief Gets a copy of the session parameters
 * \return
 */
RtpSession::SessionParams RtpSession::paramsGet () const
{
	std::lock_guard<std::recursive_mutex> lock (m_mutex);
	return m_params;
}

/*!
 * \brief wrap RvRtpRead, but also handle the possibility of decryption
 *
 * NOTE: this code was suggested by radvision R&D in order to do
 * late binding.  Put in base class to share across implementations.
 *
 */
int RtpSession::RtpRead(void *buf, int len, RvRtpParam *p)
{
	stiHResult hResult {stiRESULT_SUCCESS}; stiUNUSED_ARG(hResult);

	RvRtpSession rtp = rtpGet();
	RvRtpParamConstruct (p);

	RvStatus rvStatus = RvRtpRead(rtp, buf, len, p);
	stiDEBUG_TOOL (g_stiRtpSessionDebug,
		stiTrace("%s: called RvRtpRead (rtp: %d), with result %d\n", __FUNCTION__, rtp, rvStatus);
		)

	RtpSession::SessionParams rtpParams = paramsGet();

	// If we are expecting to do encryption and the packet read failed,
	// Set up the encryption
	if(rtpParams.srtpEncrypted && rvStatus != RV_OK && p->len > 0)
	{
		stiDEBUG_TOOL (g_stiRtpSessionDebug,
			stiTrace("Unable to decrypt packet, attempting 'late binding'\n");
			)

		/* Probably read fails because new SSRC appears
		 * In this case bufferToReceive still holds encrypted data.
		 */
		/* Extract SSRC and seqNum from packet header */
		rvStatus = RvRtpUnpack(rtp, buf, p->len, p);
		stiTESTRVSTATUS();

		bool ssrcSet = false;
		for (auto &ssrc : m_encryptedSSRC)
		{
			if(ssrc == p->sSrc)
			{
				ssrcSet = true;
			}
		}

		if (!ssrcSet && p->len)
		{
			
			m_encryptedSSRC.push_back(p->sSrc);
			/* Sets remote source encryption contexts for RTP and for RTCP */
			rvStatus = RvSrtpRemoteSourceAddRtp(rtp, p->sSrc, 0, p->sequenceNumber);
			stiTESTRVSTATUS();
			
			RvSrtpMasterKeyAdd (m_rtp,
				(RvUint8*)m_params.decryptKey.mkiGet().c_str(),
				(RvUint8*)m_params.decryptKey.keyGet().c_str(),
				(RvUint8*)m_params.decryptKey.saltGet().c_str()
				);

			destinationContextAdd ();

			rvStatus = RvSrtpRemoteSourceChangeKey(
				rtp,
				p->sSrc,
				RvSrtpStreamTypeSRTP,
				(RvUint8*)rtpParams.decryptKey.mkiGet().c_str(),
				RV_FALSE
				);
			stiTESTRVSTATUS();

			/*
			 * do it for RTCP only if has not been done before (when
			 * the rtpOnNewRtcpStream callback was called
			 */
			if (!rtpParams.rtcpStreamAdded && rtcpGet())
			{
				rvStatus = RvSrtpRemoteSourceAddRtcp(rtp, p->sSrc, 0);
				stiTESTRVSTATUS();

				stiDEBUG_TOOL (g_stiRtpSessionDebug,
					stiTrace("%s: key %s, mki %s\n", __FUNCTION__, rtpParams.decryptKey.base64KeyParamsGet().c_str(),
						rtpParams.decryptKey.mkiGet().c_str()
						);
					)

				rvStatus  = RvSrtpRemoteSourceChangeKey(
					rtp,
					p->sSrc,
					RvSrtpStreamTypeSRTCP,
					(RvUint8*)rtpParams.decryptKey.mkiGet().c_str(),
					RV_FALSE
					);
				stiTESTRVSTATUS();
			}

			m_ssrc = p->sSrc;

			/* Try to decrypt the packet again */
			p->bDontRead = RV_TRUE;

			rvStatus = RvRtpRead(rtp, buf, len, p);
			stiTESTRVSTATUS();
		}
	}
	
	// Fixes bug #31181 If this is an empty packet or keepalive just ignore it.
	// We also set len to 0 so it will not be processed.
	if (!(p->len - p->sByte)
		|| p->payload == RTP_KEEPALIVE_PAYLOAD_ID
		|| p->payload == OLD_RTP_KEEPALIVE_PAYLOAD_ID)
	{
		p->len = 0;
	}

STI_BAIL:

	return rvStatus;
}

void RtpSession::rtpEventHandlerSet (RvRtpEventHandler_CB rtpEventHandler, void* rtpEventHandlerContext)
{
	m_rtpEventHandler = rtpEventHandler;
	m_rtpEventHandlerContext = rtpEventHandlerContext;
	if (m_rtp != nullptr)
	{
		RvRtpSetEventHandler (m_rtp, m_rtpEventHandler, m_rtpEventHandlerContext);
	}
}


/*!
 * \brief Updates the encrypted state when the AES plug-in's initialize callback is raised
 */
void RtpSession::encryptionStateUpdateFromAesInitialized (int keyBitSize)
{
	auto encryptionState = EncryptionState::NONE;
	switch (keyBitSize)
	{
		case 128:
			encryptionState = EncryptionState::AES_128;
			break;

		case 256:
			encryptionState = EncryptionState::AES_256;
			break;
	}

	if (m_encryptionState != encryptionState)
	{
		stiASSERTMSG (encryptionState != EncryptionState::NONE, "Unexpected AES encryption key bit size (%d)\n", keyBitSize);
		m_encryptionState = encryptionState;
		encryptionStateChangedSignal.Emit (m_encryptionState);
	}
}


/*!
 * \brief Constructs the RTCP feedback plugin if any of the fb mechanism are enabled
 */
void RtpSession::constructRtcpFeedbackPlugin ()
{
	// Attempt to destroy the feedback plugin if it is already allocated
	// This avoids a memory leak
	destructRtcpFeedbackPlugin ();

	if (!m_rtcpFeedbackPluginConstructed)
	{
		auto rvStatus = RvRtcpFbPluginConstruct (m_rtcp);
		stiASSERTMSG (rvStatus == RV_OK, "Failed to construct rtcp feedback plugin\n");
		if (rvStatus == RV_OK)
		{
			m_rtcpFeedbackPluginConstructed = true;

			RvRtcpFbSetEventHandler (rtcpGet(), &m_feedbackHandlers);
		}
	}
}


/*!
 * \brief Destruct the RTCP feedback plugin if needed
 */
void RtpSession::destructRtcpFeedbackPlugin ()
{
	stiASSERTMSG (m_rtcp, "Attempting to destruct RTCP feedback plugin on an invalid rtcp handle\n");
	if (m_rtcpFeedbackPluginConstructed)
	{
		auto rvStatus = RvRtcpFbPluginDestruct (m_rtcp);
		stiASSERTMSG (rvStatus == RV_OK, "Failed to destruct rtcp feedback plugin\n");
		m_rtcpFeedbackPluginConstructed = false; //We should still set this to false even if it fails to deconstruct since it's likely in a bad state.
	}
}

/*!
 * \brief Sets the RvRtcpFbSetEventHandler safely
 */
void RtpSession::rtcpFbEventHandlerSetSafely ()
{
	if (m_rtcpFeedbackPluginConstructed)
	{
		RvRtcpFbSetEventHandler (rtcpGet (), &m_feedbackHandlers);
	}
	else
	{
		stiASSERTMSG (m_rtcpFeedbackPluginConstructed, "Attempting to use rtcp feedback plugin that was not constructed\n");
	}
}

/*!
 * \brief Sets the FIR feedback handler
 */
void RtpSession::rtcpFeedbackFirEventHandlerSet (RvRtcpFbEventFIR firEventHandler)
{
	m_feedbackHandlers.fbEventFIR = m_defaultFeedbackHandlers.fbEventFIR;

	if (firEventHandler)
	{
		m_feedbackHandlers.fbEventFIR = firEventHandler;
	}

	rtcpFbEventHandlerSetSafely ();
}


/*!
 * \brief Sets the PLI feedback handler
 */
void RtpSession::rtcpFeedbackPlilEventHandlerSet (RvRtcpFbEventPLI pliEventHandler)
{
	m_feedbackHandlers.fbEventPLI = m_defaultFeedbackHandlers.fbEventPLI;

	if (pliEventHandler)
	{
		m_feedbackHandlers.fbEventPLI = pliEventHandler;
	}

	rtcpFbEventHandlerSetSafely ();
}


/*!
 * \brief Sets the application layer feedback handler
 */
void RtpSession::rtcpFeedbackAfbEventHandlerSet (RvRtcpFbEventAFB afbEventHandler)
{
	m_feedbackHandlers.fbEventAFB = m_defaultFeedbackHandlers.fbEventAFB;

	if (afbEventHandler)
	{
		m_feedbackHandlers.fbEventAFB = afbEventHandler;
	}

	rtcpFbEventHandlerSetSafely ();
}


/*!
 * \brief Sets the TMMBR feedback handler
 */
void RtpSession::rtcpFeedbackTmmbrEventHandlerSet (RvRtcpFbEventTMMBR tmmbrEventHandler)
{
	m_feedbackHandlers.fbEventTMMBR = m_defaultFeedbackHandlers.fbEventTMMBR;

	if (tmmbrEventHandler)
	{
		m_feedbackHandlers.fbEventTMMBR = tmmbrEventHandler;
	}

	rtcpFbEventHandlerSetSafely ();
}


/*!
 * \brief Sets the NACK feedback handler
 */
void RtpSession::rtcpFeedbackNackEventHandlerSet (RvRtcpFbEventNACK nackEventHandler)
{
	m_feedbackHandlers.fbEventNACK = m_defaultFeedbackHandlers.fbEventNACK;
	
	if (nackEventHandler)
	{
		m_feedbackHandlers.fbEventNACK = nackEventHandler;
	}

	rtcpFbEventHandlerSetSafely ();
}


stiHResult RtpSession::dtlsSrtpAesEncryptionBegin ()
{
	auto hResult = stiRESULT_SUCCESS;
	RvDtlsSrtpSessionConfig config {};
	RvStatus rvStatus = RV_OK;
	std::string cname;

	stiTESTCOND (!m_srtpAesPluginStarted, stiRESULT_ERROR);
	stiTESTCOND (m_dtlsContext != nullptr, stiRESULT_ERROR);
	stiTESTCOND (m_rtp == nullptr, stiRESULT_ERROR);
	stiTESTCOND (rtpTransportGet () != nullptr, stiRESULT_ERROR);
	stiTESTCOND (rtcpTransportGet () != nullptr, stiRESULT_ERROR);

	rvStatus = RvRtpSetSha1EncryptionCallback (Sha1Calculate); // set encryption routine
	stiTESTRVSTATUS ();

	config.mode = m_dtlsContext->localKeyExchangeModeGet ();
	config.sslCtx = m_dtlsContext->sslContextGet ().get ();

	RvSrtpAesPlugInConstruct (
		&m_srtpAesPlugin,
		this,
		srtpAesDataConstructCB,
		srtpAesDataDestructCB,
		srtpAesInitializeECBModeCB,
		srtpAesInitializeCBCModeCB,
		srtpAesEncryptCB,
		srtpAesDataDecryptCB);

	config.aesPlugin = &m_srtpAesPlugin;
	config.sink = rtpTransportGet ();
	config.remoteAddr = &m_remoteRtpAddress;
	
	if (!m_transports.isRtcpMux ())
	{
		config.sinkRtcp = rtcpTransportGet ();
		config.remoteRtcpAddr = &m_remoteRtcpAddress;
	}
	else
	{
		config.sinkRtcp = nullptr;
		config.remoteRtcpAddr = nullptr;
	}


	cname = m_dtlsContext->localCNameGet ();
	config.cname = cname.c_str ();
	config.seli = nullptr;
	config.onReady = &RtpSession::dtlsRtpReadyCB;
	config.onReadyCtx = this;

	m_dtlsSrtpSession = RvDtlsSrtpSessionNew (&config);
	stiTESTCOND (m_dtlsSrtpSession != nullptr, stiRESULT_ERROR);

	if (m_dtlsContext->localKeyExchangeModeGet () == DTLS_KEY_EXCHANGE_ACTIVE ||
		(m_dtlsContext->localKeyExchangeModeGet () == DTLS_KEY_EXCHANGE_PASSACT && m_dtlsContext->remoteKeyExchangeModeGet () == DTLS_KEY_EXCHANGE_PASSIVE))
	{
		rvStatus = RvDtlsSrtpSessionRenegotiate (m_dtlsSrtpSession, RV_DTLS_SRTP_RENEG_BOTH);
		stiTESTRVSTATUS ();
	}

	m_srtpAesPluginStarted = true;

STI_BAIL:
	return hResult;
}

/*!
* \brief Constructs Radvision plugin for AES encryption
*/
stiHResult RtpSession::sdesSrtpAesEncryptionBegin ()
{
	auto hResult = stiRESULT_SUCCESS;
	auto rvStatus = RV_OK;

	// NOTE: don't need to reinitialize encryption each time this is called

	if (!m_srtpAesPluginStarted && m_rtp)
	{
		rvStatus = RvSrtpConstruct(m_rtp);
		stiTESTCOND (rvStatus == RV_OK, stiRESULT_ERROR);

		// Call all related rvsrtpconfig functions (before RvSrtpInit, after RvSrtpConstruct)

		// TODO: are these anything but defaults?

		// NOTE: although using the encrypt key, these sizes should be the same
		// for both encrypt and decrypt.
		SDESKey::Sizes keySizes = SDESKey::sizesGet(m_params.encryptKey.suiteGet());

		rvStatus = RvSrtpSetMasterKeySizes(m_rtp,
			keySizes.mki,
			keySizes.masterKey,
			keySizes.masterSalt
			);
		//stiTESTCOND (rvStatus == RV_OK, stiRESULT_ERROR);

		rvStatus = RvSrtpSetRtpEncryption(
			m_rtp,
			RvSrtpEncryptionAlg_AESCM,
			RV_FALSE // don't include mki in packet (since we're using fake mki)
			);

		rvStatus = RvSrtpSetRtcpEncryption(
			m_rtp,
			RvSrtpEncryptionAlg_AESCM,
			RV_FALSE // don't include mki in packet (since we're using fake mki)
			);

		// TODO: defaults for now
		//rvStatus = RvSrtpSetKeyDerivation();

		rvStatus = RvSrtpSetRtpAuthentication(
			m_rtp,
			RvSrtpAuthenticationAlg_HMACSHA1,
			keySizes.authTag
			);

		rvStatus = RvSrtpSetRtcpAuthentication(
			m_rtp,
			RvSrtpAuthenticationAlg_HMACSHA1,
			keySizes.authTag
			);

		rvStatus = RvSrtpSetRtpKeySizes(
			m_rtp,
			keySizes.key,
			keySizes.authKey,
			keySizes.salt
			);

		rvStatus = RvSrtpSetRtcpKeySizes(
			m_rtp,
			keySizes.key,
			keySizes.authKey,
			keySizes.salt
			);

		// TODO? - It looks like these only need to be called
		// if one wants different behavior from the RFC
		// RvSrtpSetRtpReplayListSize()
		// RvSrtpSetRtcpReplayListSize()
		// RvSrtpSetRtpHistory()
		// RvSrtpSetRtcpHistory()

		rvStatus = RvSrtpInit (m_rtp, &m_srtpAesPlugin);
		stiTESTRVSTATUS ();

		RvSrtpAesPlugInConstruct (
					&m_srtpAesPlugin,
					this,
					srtpAesDataConstructCB,
					srtpAesDataDestructCB,
					srtpAesInitializeECBModeCB,
					srtpAesInitializeCBCModeCB,
					srtpAesEncryptCB,
					srtpAesDataDecryptCB);

		int retval = RvRtpSetSha1EncryptionCallback (Sha1Calculate); // set encryption routine
		stiTESTCOND (retval == 0, stiRESULT_ERROR);

		// Ugh... the casting... more c++ friendly way to do this?

		RvSrtpMasterKeyAdd (m_rtp,
			(RvUint8*)m_params.encryptKey.mkiGet().c_str(),
			(RvUint8*)m_params.encryptKey.keyGet().c_str(),
			(RvUint8*)m_params.encryptKey.saltGet().c_str()
			);

		RvSrtpMasterKeyAdd (m_rtp,
			(RvUint8*)m_params.decryptKey.mkiGet().c_str(),
			(RvUint8*)m_params.decryptKey.keyGet().c_str(),
			(RvUint8*)m_params.decryptKey.saltGet().c_str()
			);

		// NOTE: we don't set up the decryption context until we have the ssrc from
		// the remote site (ie: late binding).  This is because the ssrc isn't exchanged
		// in the signaling.  See MediaPlaybackRead::RtpRead() for more details.

		destinationContextRemove ();
		destinationContextAdd ();

		m_srtpAesPluginStarted = true;
	}

STI_BAIL:
	return hResult;
}

/*!
* \brief Destructs Radvision plugin for AES encryption
*/
stiHResult RtpSession::srtpAesEncryptionEnd ()
{
	if (m_srtpAesPluginStarted)
	{
		if (m_dtlsContext == nullptr)
		{
			RvSrtpRemoteSourceRemoveAll (m_rtp);
			destinationContextRemove ();
			RvSrtpMasterKeyRemoveAll (m_rtp);
		}

		RvSrtpAesPlugInDestruct (&m_srtpAesPlugin);
		RvSrtpDestruct (m_rtp);
		m_srtpAesPluginStarted = false;
	}

	m_params.srtpEncrypted = false;

	return stiRESULT_SUCCESS;
}

void RtpSession::srtpAesDataConstructCB (
		RvSrtpAesPlugIn * /*plugin*/,
		RvSrtpEncryptionPurpose /*purpose*/)
{
}

void RtpSession::srtpAesDataDestructCB (
		RvSrtpAesPlugIn * /*plugin*/,
		RvSrtpEncryptionPurpose /*purpose*/)
{
}

RvBool RtpSession::srtpAesInitializeECBModeCB (
		RvSrtpAesPlugIn * plugin,
		RvSrtpEncryptionPurpose purpose,
		RvUint8 direction,
		const RvUint8 *key,
		RvUint16 keyBitSize,
		RvUint16 blockBitSize)
{
	auto rtpSession = static_cast<RtpSession*>(plugin->userData);
	rtpSession->encryptionStateUpdateFromAesInitialized (keyBitSize);
	return rtpSession->encryptionContexts[purpose].ECBModeInit (
				(EstiEncryptionDirection)direction,
				key,
				keyBitSize,
				blockBitSize);
}

RvBool RtpSession::srtpAesInitializeCBCModeCB (
		RvSrtpAesPlugIn * plugin,
		RvSrtpEncryptionPurpose purpose,
		RvUint8 direction,
		const RvUint8 *key,
		RvUint16 keyBitSize,
		RvUint16 blockBitSize,
		const RvUint8 *iv)
{
	auto rtpSession = static_cast<RtpSession*>(plugin->userData);
	rtpSession->encryptionStateUpdateFromAesInitialized (keyBitSize);
	return rtpSession->encryptionContexts[purpose].CBCModeInit (
				(EstiEncryptionDirection)direction,
				key,
				keyBitSize,
				blockBitSize,
				iv);
}

void RtpSession::srtpAesEncryptCB (
		RvSrtpAesPlugIn * plugin,
		RvSrtpEncryptionPurpose purpose,
		const RvUint8 *srcBuffer,
		RvUint8 *dstBuffer,
		RvUint32 length)
{
	static_cast<RtpSession*>(plugin->userData)->encryptionContexts[purpose].Process (
				srcBuffer,
				dstBuffer,
				length);
}

void RtpSession::srtpAesDataDecryptCB (
		RvSrtpAesPlugIn * plugin,
		RvSrtpEncryptionPurpose purpose,
		const RvUint8 *srcBuffer,
		RvUint8 *dstBuffer,
		RvUint32 length)
{
	static_cast<RtpSession*>(plugin->userData)->encryptionContexts[purpose].Process (
				srcBuffer,
				dstBuffer,
				length);
}

RvBool RtpSession::dtlsRtpReadyCB (RvDtlsSrtpSession* dtlsSrtpSession, RvRtpSession hRtp, void* ctx)
{
	if (hRtp == nullptr)
	{
		stiASSERTMSG (false, "DTLS key exchange failed");
		return RV_FALSE;
	}

	auto session = static_cast<RtpSession*>(ctx);

	SSL* rtpSSL{};

	/** *Fetch ssl object from RvDtlsSrtpSession **/
	RvDtlsSrtpSessionGetSSL (dtlsSrtpSession, &rtpSSL, nullptr);

	/** *Get peer certificate from SSL object **/
	auto peerCert = SSL_get_peer_certificate (rtpSSL);

	auto fingerprint = vpe::SelfSignedCert::fingerprintGenerate (peerCert, session->m_dtlsContext->remoteFingerprintHashFuncGet ());

	if (fingerprint != session->m_dtlsContext->remoteFingerprintGet ())
	{
		stiASSERTMSG (false, "Fingerprint did not match");
		return RV_FALSE;
	}

	// We now have RTP/RTCP sessions
	session->m_rtp = hRtp;
	session->m_rtcp = RvRtpGetRTCPSession (hRtp);

	if (session->m_rtpEventHandler != nullptr && session->m_rtpEventHandlerContext != nullptr)
	{
		RvRtpSetEventHandler (session->m_rtp, session->m_rtpEventHandler, session->m_rtpEventHandlerContext);
	}

	session->constructRtcpFeedbackPlugin ();

	// Continue setting up media
	if (session->m_onDtlsCompleteCallback != nullptr)
	{
		session->m_onDtlsCompleteCallback ();
	}

	return RV_TRUE;
}


/*!
 * \brief get a unique mki to identify master key
 * \return the unique value
 *
 * NOTE: this isn't the mki from the SDES line, it's only to identify
 * the master key within the radvision stack
 *
 * NOTE2: it needs to always be the same length
 */
std::string RtpSession::uniqueMkiGet()
{
	static std::mutex mki_mutex;
	static int mki_counter = 0;
	// NOTE: was using an atomic int, but would like to control rollover...

	std::lock_guard<std::mutex> lock (mki_mutex);

	++mki_counter;

	// Limit mki to 4 digits (ie: 0-9999)
	if(mki_counter >= 10000)
	{
		mki_counter = 0;
	}

	std::stringstream ss;
	ss << std::setfill('0') << std::setw(4);
	ss << mki_counter;

	return ss.str();
}

#ifdef stiTUNNEL_MANAGER
/*!
 * \brief Callback for RTP transport
 */
void RVCALLCONV RtpSession::rtpTransportProvideCallback (
		void *appContext,
		RvUint32 rtpRtcp,
		RvUint32 timing,
		RvTransport transport,
		RvBool *bContinue)
{
	if ((rtpRtcp & RV_RTP_TRANSPORT_PROVIDE_RTP) &&
		(timing & RV_RTP_TRANSPORT_PROVIDE_BEFORE_BIND))
	{
		//printf("******* RvRtpTransportProvideCB(SOCK_DGRAM, SOL_UDP) Sorenson setting options TO_TSC_RTC = 1, SO_TSC_TUNNEL_TRANSPORT = datagram_preferred\n");

		auto  rvSocket = (RvSocket)RV_INVALID_SOCKET;

		RvTransportGetOption(transport, RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
							 RVTRANSPORT_OPT_SOCKET, (void*)&rvSocket);

		auto  rtpSocket = (RtpSocket)rvSocket;
		int tscSocket = RvSocketToTscSocket(rtpSocket);

		unsigned int rtc = 1;
		tsc_setsockopt(tscSocket, SOL_SOCKET, SO_TSC_RTC,(char *)&rtc, sizeof(unsigned int));
		
#if 0 // DDT tunnel has been providing stability issues for some time. Disabling for now to resolve bug # 27768.
//Change this to #ifdef TSC_DDT when the ACME code compiles without this define.
#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4 || defined(stiMVRS_APP)
		tsc_so_tunnel_transport tt = tsc_so_tunnel_transport_datagram_preferred;
		tsc_setsockopt(tscSocket, SOL_SOCKET, SO_TSC_TUNNEL_TRANSPORT, (char *)&tt, sizeof(tsc_so_tunnel_transport));
#endif
#endif
#ifdef stiENABLE_TUNNEL_REDUNDANCY
		tsc_so_redundancy redundancy;
		memset(&redundancy, 0, sizeof(tsc_so_redundancy));
		redundancy.redundancy_factor = 1;
		tsc_tunnel_socket_info tunnel_info;
		tsc_handle tunnel = tsc_get_tunnel(tscSocket);
		tsc_get_tunnel_socket_info(tunnel, &tunnel_info);

		//if (tunnel_info.transport == tsc_transport_udp || tunnel_info.transport == tsc_transport_dtls)
		{
			redundancy.redundancy_method = tsc_so_redundancy_method_udp_dtls;
			if (tsc_setsockopt(tscSocket, SOL_SOCKET, SO_TSC_REDUNDANCY, (char *)&redundancy, sizeof(tsc_so_redundancy)) == -1)
			{
				stiDEBUG_TOOL (g_stiRtpSessionDebug,
					stiTrace("<RtpSession::rtpTransportProvideCallback> Failed to set redundancy on socket.\n");
				)
			}
		}
#endif
	}

	*bContinue = RV_TRUE;
}

RvRtpTransportProvide RtpSession::m_rtpTransportProvide = {
	RtpSession::rtpTransportProvideCallback,
	RV_RTP_TRANSPORT_PROVIDE_RTP,
	RV_RTP_TRANSPORT_PROVIDE_BEFORE_BIND,
	nullptr };


void RtpSession::rtpTransportProvideCallbackSet (
		RvRtpTransportProvideCB callback)
{
	m_rtpTransportProvide.cb = callback;
	RvRtpSetTransportProvideData(&m_rtpTransportProvide);
}
#endif  // stiTUNNEL_MANAGER






}
