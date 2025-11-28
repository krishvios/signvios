#pragma once
#include "SerializedProperty.h"
#include <vector>
#include <string>
#include <memory>

namespace vpe
{
	namespace Serialization
	{
		using MetaData = std::vector<std::shared_ptr<PropertyBase>>;

		class ISerializable
		{
		public:
			ISerializable () = default;
			virtual ~ISerializable () = default;
			ISerializable (const ISerializable& other) = delete;
			ISerializable (ISerializable&& other) = delete;
			ISerializable& operator= (const ISerializable& other) = delete;
			ISerializable& operator= (ISerializable&& other) = delete;

			virtual std::string typeNameGet () = 0;
			virtual MetaData& propertiesGet () = 0;
			virtual ISerializable* instanceCreate (const std::string& typeName) = 0;
		};
	}
}
