#pragma once
#include "ISerializable.h"

namespace vpe
{
	namespace Serialization
	{
		class ISerializer
		{
		public:
			virtual void serialize (ISerializable* serializable) = 0;
			virtual void deserialize (ISerializable* serializable) = 0;

			ISerializer () = default;
			virtual ~ISerializer () = default;
			ISerializer (const ISerializer& other) = delete;
			ISerializer (ISerializer&& other) = delete;
			ISerializer& operator= (const ISerializer& other) = delete;
			ISerializer& operator= (ISerializer&& other) = delete;
		};
	}
}
