#include "SslCertificateHandler.h"
#include "stiTools.h"
#include "Poco/Net/CertificateHandlerFactory.h"
#include "Poco/Net/SSLManager.h"

SslCertificateHandler::SslCertificateHandler(bool server) : InvalidCertificateHandler(server)
{
	Poco::Net::SSLManager::instance().certificateHandlerFactoryMgr().setFactory(SslCertificateHandlerName, new Poco::Net::CertificateHandlerFactoryImpl<SslCertificateHandler>());
}

void SslCertificateHandler::onInvalidCertificate(const void *, Poco::Net::VerificationErrorArgs &errorCert)
{
	auto &certificate = errorCert.certificate();

	auto errorNumber = errorCert.errorNumber();
	if (X509_V_ERR_CERT_HAS_EXPIRED == errorNumber)
	{
		errorCert.setIgnoreError(true);
	}
	else
	{
		stiASSERTMSG(false, "Issuer=\"%s\" Subject=\"%s\" Error=\"%s\" ErrorNumber=%d ErrorDepth=%d\n", certificate.issuerName().c_str(), certificate.subjectName().c_str(), errorCert.errorMessage().c_str(), errorNumber, errorCert.errorDepth());
		errorCert.setIgnoreError(false);
	}
}
