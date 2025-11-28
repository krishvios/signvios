#include "JsonFileSerializer.h"
#include "stiOS.h"
#include "stiTrace.h"
#include "FileEncryption.h"

#include <fstream>
#include <sstream>

using namespace vpe::Serialization;

static constexpr const char* typeNameKey{ "__type_name__" };

JsonFileSerializer::JsonFileSerializer (const std::string& fileName) :
	m_fileName(fileName),
	m_encrypt(false)
{
#ifdef stiENCRYPT_XML_FILES
	m_encrypt = true;
#endif
}

void JsonFileSerializer::serialize (ISerializable* serializable)
{
	auto path = filePathGet ();
	std::fstream stream (path.c_str (), std::fstream::out);

	Poco::JSON::Object json;
	serialize (serializable, json);
	json.stringify (stream);

	stream.close ();
	
	if (m_encrypt)
	{
		FileEncryption enc (path);
		enc.encrypt ();
		remove (path.c_str ());
	}
}

void JsonFileSerializer::serialize (ISerializable* serializable, Poco::JSON::Object& json)
{
	json.set (typeNameKey, serializable->typeNameGet ());

	for (auto& property : serializable->propertiesGet ())
	{
		switch (property->m_type)
		{
			case SerializationType::String:
			{
				auto strProperty = std::static_pointer_cast<StringProperty>(property);
				json.set (property->m_name, strProperty->stringGet ());
				break;
			}
			case SerializationType::Integer:
			{
				auto intProperty = std::static_pointer_cast<IntProperty>(property);
				json.set (property->m_name, intProperty->integerGet ());
				break;
			}
			case SerializationType::Array:
			{
				Poco::JSON::Array arr;
				auto arrProperty = std::static_pointer_cast<ArrayPropertyBase>(property);
				for (int i = 0; i < arrProperty->itemsCountGet(); i++)
				{
					auto item = arrProperty->itemGet (i);
					Poco::JSON::Object child;
					serialize (item, child);
					arr.add (child);
				}
				json.set (property->m_name, arr);
				break;
			}

			default:
				break;
		}
	}
}

void JsonFileSerializer::deserialize (ISerializable* serializable)
{
	auto path = filePathGet ();
	if (m_encrypt)
	{
		FileEncryption encryption (path);
		auto decryptedFileName = encryption.decrypt ();
		if (!decryptedFileName.empty ())
		{
			path = decryptedFileName;
		}
	}
	std::fstream stream (path.c_str (), std::fstream::in);
	
	Poco::JSON::Parser parser;
	try
	{
		if (stream.peek() != EOF)
		{
			auto result = parser.parse(stream);
			if (!result.isEmpty())
			{
				auto json = result.extract<Poco::JSON::Object::Ptr>();
				deserialize(json, serializable);
			}
		}
	}
	catch (std::exception&)
	{
		stiTrace ("Failed to deserialize file: \n", m_fileName.c_str());
	}

	stream.close ();

	if (m_encrypt)
	{
		remove (path.c_str ());
	}
}

void JsonFileSerializer::deserialize (Poco::JSON::Object::Ptr json, ISerializable* serializable)
{
	if (serializable != nullptr)
	{
		for (auto& property : serializable->propertiesGet ())
		{
			auto item = json->get (property->m_name);
			if (!item.isEmpty ())
			{
				switch (property->m_type)
				{
					case SerializationType::Integer:
					{
						if (item.isInteger ())
						{
							auto intProperty = std::static_pointer_cast<IntProperty>(property);
							intProperty->integerSet (item.convert<int64_t> ());
						}
						break;
					}
					case SerializationType::String:
					{
						if (item.isString ())
						{
							auto strProperty = std::static_pointer_cast<StringProperty>(property);
							strProperty->stringSet (item.convert<std::string> ());
						}
						break;
					}
					case SerializationType::Array:
					{
						if (item.isArray ())
						{
							auto arrProperty = std::static_pointer_cast<ArrayPropertyBase>(property);
							arrProperty->itemsClear ();
							auto arr = item.extract<Poco::JSON::Array::Ptr> ();
							for (auto& element : *arr)
							{
								auto jsonListItem = element.extract<Poco::JSON::Object::Ptr> ();
								auto typeName = jsonListItem->get (typeNameKey);
								auto serListItem = serializable->instanceCreate (typeName);
								deserialize (jsonListItem, serListItem);
								arrProperty->itemAdd (serListItem);
							}
						}
						break;
					}
					default:
						break;
				}
			}
		}
	}
}

std::string JsonFileSerializer::filePathGet ()
{
	std::string dynamicDataFolder;
	stiOSDynamicDataFolderGet (&dynamicDataFolder);
	std::stringstream filePath;
	filePath << dynamicDataFolder << m_fileName;
	return filePath.str ();
}
