#pragma once
#include <memory>
#include <functional>

class ServiceCallbackContextBase
{
public:
	virtual size_t contentTypeIdGet () = 0;

protected:
	virtual ~ServiceCallbackContextBase() = default;
};

template<typename T>
class ServiceCallbackContext : public ServiceCallbackContextBase
{
public:
	std::function<void (std::shared_ptr<T>)> successCallback;
	std::function<void (std::shared_ptr<T>)> errorCallback;
	size_t contentTypeIdGet () override { return typeid(T).hash_code (); }
};
