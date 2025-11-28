#pragma once

#include "Poco/Net/NetSSL.h"
#include "Poco/Net/InvalidCertificateHandler.h"

class NetSSL_API SslCertificateHandler : public Poco::Net::InvalidCertificateHandler
{
public:
	SslCertificateHandler(bool server);
	~SslCertificateHandler() override = default;

	SslCertificateHandler(const SslCertificateHandler &) = delete;
	SslCertificateHandler(SslCertificateHandler &&other) = delete;
	SslCertificateHandler& operator= (SslCertificateHandler &) = delete;
	SslCertificateHandler& operator= (SslCertificateHandler &&other) = delete;

	void onInvalidCertificate(const void *pSender, Poco::Net::VerificationErrorArgs &errorCert) override;
private:
	const std::string SslCertificateHandlerName{ "SslCertificateHandler" };
};

