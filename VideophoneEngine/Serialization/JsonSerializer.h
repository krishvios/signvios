#pragma once

#include <Poco/JSON/Parser.h>

namespace vpe
{
	namespace Serialization
	{
		class JsonSerializable
		{
		public:

			JsonSerializable () = default;
			virtual ~JsonSerializable () = default;

			JsonSerializable (const JsonSerializable &other) = default;
			JsonSerializable (JsonSerializable &&other) = default;
			JsonSerializable &operator= (const JsonSerializable &other) = default;
			JsonSerializable &operator= (JsonSerializable &&other) = default;

			virtual operator Poco::JSON::Object () const = 0;
		};

		class JsonSerializer
		{
		public:

			JsonSerializer () = default;
			virtual ~JsonSerializer () = default;

			JsonSerializer (const JsonSerializer &other) = delete;
			JsonSerializer (JsonSerializer &&other) = delete;
			JsonSerializer &operator= (const JsonSerializer &other) = delete;
			JsonSerializer &operator= (JsonSerializer &&other) = delete;

			template<typename T,
				typename = typename std::enable_if<
				std::is_enum<T>::value ||
				std::is_integral<T>::value>::type>
			void serialize (const std::string &name, T &data)
			{
				m_json.set (name, data);
			}

			void serialize (const std::string &name, const JsonSerializable &data)
			{
				m_json.set (name, static_cast<Poco::JSON::Object>(data));
			}

			operator Poco::JSON::Object () { return m_json; };

		private:

			Poco::JSON::Object m_json;
		};
	}
}
