#pragma once
#include <string>
#include <functional>
#include <algorithm>

namespace vpe
{
	namespace Serialization
	{
		class ISerializable;

		enum class SerializationType
		{
			String,
			Integer,
			Array,
			// Other types could include floating point or objects
		};

		class PropertyBase
		{
		public:
			PropertyBase (std::string propertyName, SerializationType propertyType) :
				m_name (propertyName),
				m_type (propertyType) {}

			virtual ~PropertyBase () = default;
			PropertyBase (const PropertyBase& other) = delete;
			PropertyBase (PropertyBase&& other) = delete;
			PropertyBase& operator= (const PropertyBase& other) = delete;
			PropertyBase& operator= (PropertyBase&& other) = delete;

			std::string m_name;
			SerializationType m_type;
		};

		class StringProperty : public PropertyBase
		{
		public:
			StringProperty(std::string propertyName, std::string& propertyValue) :
				PropertyBase (propertyName, SerializationType::String),
				m_value(propertyValue)
			{}

			std::string& m_value;

			std::string stringGet ()
			{
				return m_value;
			}

			void stringSet (const std::string& value)
			{
				m_value = value;
			}
		};

		class IntProperty : public PropertyBase
		{
		public:
			IntProperty (std::string propertyName) :
				PropertyBase (propertyName, SerializationType::Integer)
			{}

			~IntProperty () override = default;
			IntProperty (const IntProperty& other) = delete;
			IntProperty (IntProperty&& other) = delete;
			IntProperty& operator= (const IntProperty& other) = delete;
			IntProperty& operator= (IntProperty&& other) = delete;

			virtual int64_t integerGet () = 0;
			virtual void integerSet (int64_t value) = 0;
		};

		template<typename T>
		struct IntegerProperty : IntProperty
		{
			IntegerProperty(std::string propertyName, T& propertyValue) :
				IntProperty (propertyName),
				m_value(propertyValue)
			{}

			~IntegerProperty () override = default;
			IntegerProperty (const IntegerProperty& other) = delete;
			IntegerProperty (IntegerProperty&& other) = delete;
			IntegerProperty& operator= (const IntegerProperty& other) = delete;
			IntegerProperty& operator= (IntegerProperty&& other) = delete;

			T& m_value;

			int64_t integerGet () override
			{
				return static_cast<int64_t>(m_value);
			}

			void integerSet (int64_t value) override
			{
				m_value = static_cast<T>(value);
			}
		};

		class ArrayPropertyBase : public PropertyBase
		{
		public:
			ArrayPropertyBase (std::string propertyName) :
				PropertyBase (propertyName, SerializationType::Array)
			{}

			~ArrayPropertyBase () override = default;
			ArrayPropertyBase (const ArrayPropertyBase& other) = delete;
			ArrayPropertyBase (ArrayPropertyBase&& other) = delete;
			ArrayPropertyBase& operator= (const ArrayPropertyBase& other) = delete;
			ArrayPropertyBase& operator= (ArrayPropertyBase&& other) = delete;

			virtual ISerializable* itemGet (size_t index) = 0;
			virtual int itemsCountGet () = 0;
			virtual void itemsClear () = 0;
			virtual void itemAdd (ISerializable* item) = 0;
		};

		template<typename ContainerType, typename ItemType>
		class ArrayProperty : public ArrayPropertyBase
		{
		public:
			ArrayProperty (std::string propertyName, ContainerType& propertyValue) :
				ArrayPropertyBase (propertyName),
				m_value(propertyValue)
			{}

			~ArrayProperty () override = default;
			ArrayProperty (const ArrayProperty& other) = delete;
			ArrayProperty (ArrayProperty&& other) = delete;
			ArrayProperty& operator= (const ArrayProperty& other) = delete;
			ArrayProperty& operator= (ArrayProperty&& other) = delete;

			ContainerType& m_value;

			ISerializable* itemGet (size_t index) override
			{
				if (index < m_value.size ())
				{
					auto it = m_value.begin ();
					std::advance (it, index);
					return *it;
				}

				return nullptr;
			}

			int itemsCountGet () override
			{
				return m_value.size ();
			}

			void itemsClear () override
			{
				m_value.clear ();
			}

			void itemAdd (ISerializable* item) override
			{
				m_value.push_back (static_cast<ItemType>(item));
			}
		};

		template<typename T>
		class CustomIntegerProperty : public IntProperty
		{
		public:
			CustomIntegerProperty (std::string propertyName, T& propertyValue, std::function<int64_t (T&)> getter, std::function<void (int64_t, T&)> setter) :
				IntProperty (propertyName),
				m_value(propertyValue),
				m_getter (getter),
				m_setter (setter)
			{}

			T& m_value;

			std::function<int64_t (T&)> m_getter;
			std::function<void (int64_t, T&)> m_setter;

			int64_t integerGet () override
			{
				return m_getter (m_value);
			}

			void integerSet (int64_t value) override
			{
				m_setter (value, m_value);
			}
		};
	}
}
