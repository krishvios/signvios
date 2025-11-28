#pragma once
#include <string>
#include <functional>

class CstiVPServiceResponse;
using CoreResponseCallback = const std::function<void (std::shared_ptr<CstiCoreResponse>)>;

class IUserAccountManager
{
public:
	virtual void userPinSet (const std::string& pin) = 0;
	virtual void userPinSet (const std::string& pin, CoreResponseCallback coreResponseCallback) = 0;

	virtual void profileImageIdSet (const std::string& imageID) = 0;
	virtual void profileImageIdSet (const std::string& imageID, CoreResponseCallback coreResponseCallback) = 0;

	virtual void signMailEmailPreferencesSet (bool enableSignMailNotifications, const std::string& emailPrimary, const std::string& emailSecondary) = 0;
	virtual void signMailEmailPreferencesSet (bool enableSignMailNotifications, const std::string& emailPrimary, const std::string& emailSecondary, CoreResponseCallback coreResponseCallback) = 0;

	virtual void userRegistrationInfoSet (const std::string& birthDate /* MM/dd/yyyy */, const std::string& lastFourSSN, bool hasIdentification) = 0;
	virtual void userRegistrationInfoSet (const std::string& birthDate /* MM/dd/yyyy */, const std::string& lastFourSSN, bool hasIdentification, CoreResponseCallback coreResponseCallback) = 0;

protected :
	virtual ~IUserAccountManager() = default;
};
