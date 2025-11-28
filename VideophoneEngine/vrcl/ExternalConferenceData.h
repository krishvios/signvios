#pragma once
#include <string>

class ExternalConferenceData
{
public:
std::string conferenceURI;
std::string conferenceType;
std::string publicIP;
std::string securityToken;

ExternalConferenceData(std::string conferenceURI, std::string conferenceType, std::string publicIP, std::string securityToken);

ExternalConferenceData();
};
