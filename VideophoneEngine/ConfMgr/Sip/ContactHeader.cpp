#include "./ContactHeader.h"

namespace vpe
{
namespace sip
{
	std::string ContactHeader::otherParamsGet()
	{
		RvUint length {48};
		std::vector<RvChar> buffer;

		buffer.resize(length);
		RvStatus rvStatus = RvSipContactHeaderGetOtherParams (m_contactHeader, buffer.data(), length, &length);
		if (RV_ERROR_INSUFFICIENT_BUFFER == rvStatus)
		{
			buffer.resize(length);
			rvStatus = RvSipContactHeaderGetOtherParams (m_contactHeader, buffer.data(), length, &length);
		}

		if (rvStatus == RV_OK)
		{
			buffer.resize(length - 1);
			return {buffer.begin(), buffer.end()};
		}

		return {};
	}
}
}
