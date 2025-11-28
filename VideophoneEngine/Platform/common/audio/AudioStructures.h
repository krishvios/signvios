#pragma once
#include <stdint.h>

namespace vpe
{
	namespace Audio
	{
		struct Packet
		{
			uint8_t* data{};
			uint32_t maxLength{};
			uint32_t dataLength{};
			uint32_t sampleRate{};
			uint32_t channels{};
			uint32_t sampleCount{};
			~Packet ()
			{
				if (data)
				{
					delete[] data;
				}
			}
		};

		enum class BufferStatus : uint32_t
		{
			None = 0x00,
			FrameComplete = 0x01,
		};

		inline BufferStatus operator & (BufferStatus lhs, BufferStatus rhs)
		{
			return BufferStatus (static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
		}
	}
}