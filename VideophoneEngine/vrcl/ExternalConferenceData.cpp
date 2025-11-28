#include "ExternalConferenceData.h"

ExternalConferenceData::ExternalConferenceData(std::string conferenceURI, std::string conferenceType, std::string publicIP, std::string securityToken)
{
	this->conferenceURI = conferenceURI;
	this->conferenceType = conferenceType;
	this->publicIP = publicIP;
	this->securityToken = securityToken;
}

ExternalConferenceData::ExternalConferenceData()
{
	conferenceURI = "";
	conferenceType = "";
	publicIP = "";
	securityToken = "";
}
