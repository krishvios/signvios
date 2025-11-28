#pragma once
#include "CstiCoreResponse.h"

namespace vpe
{
	enum class ResponseOrigin
	{
		VPServices,
		LocalCache,
	};

	template<typename T>
	class ServiceResponse
	{
	public:
		unsigned int requestId{};
		bool responseSuccessful{};
		CstiCoreResponse::ECoreError coreErrorNumber{};
		std::string coreErrorMessage;
		ResponseOrigin origin{};
		T content{};
	};
}

template<typename T>
using ServiceResponseSP = std::shared_ptr<vpe::ServiceResponse<T>>;

