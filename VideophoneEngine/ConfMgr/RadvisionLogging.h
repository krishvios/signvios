#pragma once
#include "rvinterfacesdefs.h"
#include "RvSipCommonTypes.h"
#include "rvrtplogger.h"

namespace vpe
{
	class RadvisionLogging
	{
	private:

#ifndef RV_LOGLEVEL
#define RV_LOGLEVEL (RV_LOGLEVEL_EXCEP | RV_LOGLEVEL_ERROR | RV_LOGLEVEL_WARNING)
#endif

	public:

		static int iceLogFilterGet ()
		{
			return RV_LOGLEVEL;
		}

		static RvSipLogFilters sipLogFilterGet ()
		{
			int result {0};

			if (RV_LOGLEVEL & RV_LOGLEVEL_EXCEP)
			{
				result |= RVSIP_LOG_EXCEP_FILTER;
			}
			if (RV_LOGLEVEL & RV_LOGLEVEL_ERROR)
			{
				result |= RVSIP_LOG_ERROR_FILTER;
			}
			if (RV_LOGLEVEL & RV_LOGLEVEL_WARNING)
			{
				result |= RVSIP_LOG_WARN_FILTER;
			}
			if (RV_LOGLEVEL & RV_LOGLEVEL_INFO)
			{
				result |= RVSIP_LOG_INFO_FILTER;
			}
			if (RV_LOGLEVEL & RV_LOGLEVEL_DEBUG)
			{
				result |= RVSIP_LOG_DEBUG_FILTER;
			}
			if (RV_LOGLEVEL & RV_LOGLEVEL_ENTER)
			{
				result |= RVSIP_LOG_ENTER_FILTER;
			}
			if (RV_LOGLEVEL & RV_LOGLEVEL_LEAVE)
			{
				result |= RVSIP_LOG_LEAVE_FILTER;
			}
			return static_cast<RvSipLogFilters>(result);
		}

		static RvRtpLoggerFilterBit rtpLogFiterGet ()
		{
			return static_cast<RvRtpLoggerFilterBit>(sipLogFilterGet());
		}
	};
}
