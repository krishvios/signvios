#pragma once
#include "stiSVX.h"
#include "stiConfDefs.h"
#include "RvSipMsg.h"
#include "RvSipContactHeader.h"

namespace vpe
{
namespace sip
{
	class ContactHeader
	{
	public:

		ContactHeader (RvSipContactHeaderHandle contactHeader)
		:
			m_contactHeader(contactHeader)
		{
		}

		std::string otherParamsGet();

		operator bool ()
		{
			return m_contactHeader != nullptr;
		}

	private:

		RvSipContactHeaderHandle m_contactHeader {nullptr};
	};
}
}
