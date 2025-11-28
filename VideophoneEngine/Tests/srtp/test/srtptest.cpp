
#include "SDESKey.h"
#include "CstiRtpSession.h"

#include "rvnettypes.h"
#include "rtcp.h"
#include "rvrtplogger.h"

#include <boost/program_options.hpp>
#include <boost/format.hpp>

#include <iostream>
#include <thread>

/*!
 * \brief generate test byte streams to be able to verify
 */
template <class ItemType = int>
class TestBufferCounter
{
public:
	TestBufferCounter(size_t itemCount) :
		m_itemCount(itemCount)
	{
	}

	/*!
	 * \brief Generate data
	 *
	 * NOTE: using iterators for flexibility to fill mid-buffer
	 */
	void fillBuffer(const std::string::iterator &begin, const std::string::iterator &end)
	{
		// TODO: do padding check?  (ie: end - begin % m_itemSize == 0)

		for(auto iter = begin; iter != end; iter += m_itemSize)
		{
			memcpy(&(*iter), &m_currentCount, m_itemSize);

			// TODO: may have to handle endianness

			// let it roll over naturally
			++m_currentCount;
		}
	}

	/*!
	 * \brief consume/validate data
	 *
	 * NOTE: using iterators for flexibility to validate mid-buffer
	 */
	bool validateBuffer(const std::string::iterator &begin, const std::string::iterator &end)
	{
		if(std::distance(begin, end) != static_cast<int>(m_itemCount * m_itemSize))
		{
			std::cout << "Passed in buffer was unexpected size\n";
			return false;

		}

		ItemType item;

		for(auto iter = begin; iter != end; iter += m_itemSize)
		{
			memcpy(&item, &(*iter), m_itemSize);

			// TODO: may have to handle endianness

			if(item != m_currentCount)
			{
				std::cout << (boost::format("Expecting %1%, received %2%\n") % m_currentCount % item).str();
				return false;
			}

			// let it roll over naturally
			++m_currentCount;
		}

		return true;
	}

	size_t bufferSize()
	{
		return m_itemCount * m_itemSize;
	}

private:

	// Doesn't change after initialization
	size_t m_itemCount = 0;

	ItemType m_currentCount = 0;
	size_t m_itemSize = sizeof(ItemType);
};

class SrtpTest
{
public:
	SrtpTest(const vpe::RtpSession::SessionParams &rtp_params) :
		m_session(rtp_params),
		m_sendCounter(128),
		m_receiveCounter(128)
	{
		// TODO: awkward api for setting the mki and enabling encryption
		if(rtp_params.encryptKey.validGet() && rtp_params.decryptKey.validGet())
		{
			m_session.SDESKeysSet(rtp_params.encryptKey, rtp_params.decryptKey);
		}
	}

	void connect(const std::string &host, int port)
	{
		RvAddress rtpAddress = toRvAddress(host, port);

		// Port just above
		RvAddress rtcpAddress = toRvAddress(host, port + 1);

		bool changed;

		// Open the rtp and rtcp ports (for sending and receiving)
		std::cout << "Opening session\n";
		m_session.open();

		// sleep 5 seconds to wait for other side to set up
		// TODO: better way to trigger this?

		std::cout << "Waiting to set remote addresses\n";
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));

		// This is what appears to connect to the remote side
		// the srtp doc says this should be after setting up srtp
		// and before sending packets
		std::cout << "Setting remote addresses...\n";
		m_session.remoteAddressSet(&rtpAddress, &rtcpAddress, &changed);
	}

	void sendPacket()
	{
		std::cout << "Sending packet\n";
		std::string buffer;

		// Get size of authentication tag
		int authTagSize = 0;
		vpe::SDESKey encryptKey = m_session.paramsGet().encryptKey;
		if(encryptKey.validGet())
		{
			// Setting this seems to add an additional 10 bytes for the auth tag size
			//authTagSize = vpe::SDESKey::sizesGet(encryptKey.suiteGet()).authTag;
		}

		buffer.resize(m_sendCounter.bufferSize() + m_rtpHeaderSize + authTagSize);

		m_sendCounter.fillBuffer(buffer.begin() + m_rtpHeaderSize, buffer.end() - authTagSize);

		RvRtpParam rtpParam;

		// initialize the rtp param structure
		RvRtpParamConstruct (&rtpParam);

		rtpParam.sByte = m_rtpHeaderSize;

		// NOTE: does *not* contain the auth tag
		rtpParam.len = m_sendCounter.bufferSize();

		std::cout << "Sending packet with payload size: " << m_sendCounter.bufferSize() << "\n";

		// Send the data to the stack
		RvStatus rvStatus = RvRtpWrite (
			m_session.rtpGet(),
			&buffer[0],
			buffer.size(),
			&rtpParam);

		if(RV_OK != rvStatus)
		{
			std::cout << "Error in RvRtpWrite: " << rvStatus << "\n";
		}

		std::cout << "Done sending packet\n";
	}

	void receivePacket()
	{
		std::cout << "Receiving packet\n";

		std::string buffer;

		// Get size of authentication tag
		int authTagSize = 0;
		vpe::SDESKey decryptKey = m_session.paramsGet().decryptKey;
		if(decryptKey.validGet())
		{
			authTagSize = vpe::SDESKey::sizesGet(decryptKey.suiteGet()).authTag;
		}

		// Take the rtp header size into account
		buffer.resize(m_receiveCounter.bufferSize() + m_rtpHeaderSize + authTagSize);

		RvRtpParam rtpParam;

		// initialize the rtp param structure
		RvRtpParamConstruct (&rtpParam);

		int status = m_session.RtpRead(
			&buffer[0],
			buffer.size(),
			&rtpParam
			);

		if(0 == status)
		{
			if(!m_receiveCounter.validateBuffer(buffer.begin() + m_rtpHeaderSize, buffer.end() - authTagSize))
			{
				std::cout << "Received buffer, but it's not what was expected\n";
			}
			else
			{
				std::cout << "Validated received packet...\n";
			}
		}
		else
		{
			std::cout << "Error reading packet\n";
		}

		std::cout << "Done receiving packet\n";
	}

	void processPackets()
	{
		while(true)
		{
			// NOTE: even sending two and receiving two doesn't help unblock

			sendPacket();

			receivePacket();

			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}

private:

	RvAddress toRvAddress(const std::string &ip, int port)
	{
		RvAddress address;
		std::stringstream ss;

		ss << ip << ":" << port;
		RvStatus status = RvAddressConstructFromString(&address, ss.str().c_str());

		if(RV_OK != status)
			throw; // TODO throw string description

		return address;
	}

	vpe::RtpSession m_session;

	TestBufferCounter<> m_sendCounter;
	TestBufferCounter<> m_receiveCounter;

	size_t m_rtpHeaderSize = 12; // static... ?
};

void initRadvision()
{
	RvRtpInit();

	RvRtcpInit();

#ifndef RV_CFLAG_NOLOG // these logging functions depend on this not being set
	// static so it doesn't go out of scope...
	static RvRtpLogger rvLogger;

	RvRtpCreateLogManager (&rvLogger);
	RvRtpSetLogModuleFilter (RVRTP_RTP_MODULE, 0xFF);
	RvRtpSetLogModuleFilter (RVRTP_SRTP_MODULE, 0xFF);
	// 		RVRTP_LOG_ERROR_FILTER | RVRTP_LOG_EXCEP_FILTER | RVRTP_LOG_DEBUG_FILTER | RVRTP_LOG_INFO_FILTER | RVRTP_LOG_ENTER_FILTER);
	// 	RvRtpSetLogModuleFilter (RVRTP_RTCP_MODULE, 0xFF);
	// 		RVRTP_LOG_ERROR_FILTER | RVRTP_LOG_EXCEP_FILTER | RVRTP_LOG_DEBUG_FILTER | RVRTP_LOG_INFO_FILTER);
	RvRtpSetLogModuleFilter (RVRTP_PAYLOAD_MODULE, 0xFF);
#endif
}

int main(int argc, char **argv)
{
	int localPort = 0;
	std::string localKey;

	std::string remoteHost;
	int remotePort = 0;
	std::string remoteKey;

	boost::program_options::options_description desc("Allowed options");
	desc.add_options()
		("local-port",  boost::program_options::value<int>        (&localPort)->required(),  "local port")
		("local-key",   boost::program_options::value<std::string>(&localKey)->required(),   "local key")
		("remote-host", boost::program_options::value<std::string>(&remoteHost)->required(), "remote host")
		("remote-port", boost::program_options::value<int>        (&remotePort)->required(), "remote port")
		("remote-key",  boost::program_options::value<std::string>(&remoteKey)->required(),  "remote key")
		("help", "produce help message")
		;

	boost::program_options::variables_map vm;

	bool error = false;
	try
	{
		boost::program_options::store(
			boost::program_options::command_line_parser(argc, argv)
			.options(desc)
			.run(),
			vm
			);

		boost::program_options::notify(vm);
	}
	catch(std::exception &e)
	{
		std::cout << "\n" << e.what() << "\n";
		error = true;
	}

	if (vm.count("help") || error)
	{
		std::cout << "\n" << desc << "\n";
		return 1;
	}

	initRadvision();

	vpe::RtpSession::SessionParams params;

	params.portBase = localPort;
	params.portRange = 12; // this is what vp2 uses?

	// Comment out to disable encryption
	params.encryptKey = vpe::SDESKey::decode("AES_CM_128_HMAC_SHA1_80", localKey);
	params.decryptKey = vpe::SDESKey::decode("AES_CM_128_HMAC_SHA1_80", remoteKey);

	SrtpTest srtp(params);

	srtp.connect(remoteHost, remotePort);

	srtp.processPackets();

	return 0;
}
