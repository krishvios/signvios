#pragma once
#include "ISerializer.h"

#include <Poco/JSON/Parser.h>
#include <string>
#include <iostream>

namespace vpe
{
	namespace Serialization
	{
		class JsonFileSerializer : public ISerializer
		{
		public:
			JsonFileSerializer (const std::string& fileName);

			void serialize (ISerializable* serializable) override;
			void deserialize (ISerializable* serializable) override;

		private:
			std::string filePathGet ();
			void serialize (ISerializable* serializable, Poco::JSON::Object& json);
			void deserialize (Poco::JSON::Object::Ptr json, ISerializable* serializable);

			std::string m_fileName;
			bool m_encrypt;
		};
	}
}
