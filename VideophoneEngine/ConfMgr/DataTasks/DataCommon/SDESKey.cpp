
#include "SDESKey.h"

#include "Random.h"
#include "stiBase64.h"

#include <ostream>

namespace vpe
{

const std::array<SDESKey::Suite, 4> SDESKey::Suites = {
	Suite::AES_CM_128_HMAC_SHA1_80,
	Suite::AES_CM_128_HMAC_SHA1_32,
	Suite::AES_256_CM_HMAC_SHA1_80,
	Suite::AES_256_CM_HMAC_SHA1_32
};

#define ADD_ENUM_CASE(enum_namespace, value) case enum_namespace::value: ret = #value; break;

std::string SDESKey::toString(Suite suite)
{
	std::string ret;

	switch(suite)
	{
		ADD_ENUM_CASE(Suite, AES_CM_128_HMAC_SHA1_80);
		ADD_ENUM_CASE(Suite, AES_CM_128_HMAC_SHA1_32);
		ADD_ENUM_CASE(Suite, AES_256_CM_HMAC_SHA1_80);
		ADD_ENUM_CASE(Suite, AES_256_CM_HMAC_SHA1_32);
	}

	return ret;
}

/*!
 * \brief convert string to suite
 *
 * \param suiteStr string to parse
 * \param error will be set if the string was invalid
 *
 * \return Suite enum type
 */
SDESKey::Suite SDESKey::fromString(const std::string &suiteStr, bool &error)
{
	Suite ret(Suite::AES_CM_128_HMAC_SHA1_80); // TODO: use boost::optional<SDESKey::Suite>?
	error = true;

	for(const auto &suite : Suites)
	{
		if(toString(suite) == suiteStr)
		{
			ret = suite;
			error = false;
			break;
		}
	}

	return ret;
}

KeyExchangeMethod SDESKey::suiteToKeyExchangeMethod (Suite suite)
{
	switch (suite)
	{
		case vpe::SDESKey::Suite::AES_CM_128_HMAC_SHA1_80:
			return KeyExchangeMethod::SDES_AES_CM_128_HMAC_SHA1_80;

		case vpe::SDESKey::Suite::AES_256_CM_HMAC_SHA1_80:
			return KeyExchangeMethod::SDES_AES_256_CM_HMAC_SHA1_80;

		default:
			break;
	}

	stiASSERTMSG (false, "Unsupported SDES suite %d", suite);
	return KeyExchangeMethod::None;
}

SDESKey::Sizes SDESKey::sizesGet(const Suite &suite)
{
	Sizes ret;

	switch(suite)
	{
	case Suite::AES_CM_128_HMAC_SHA1_80:
		ret.masterKey  = 16;
		ret.masterSalt = 14;
		ret.key        = 16;
		ret.salt       = 14;
		ret.authKey    = 20;
		ret.authTag    = 10;
		break;
	case Suite::AES_256_CM_HMAC_SHA1_80:
		ret.masterKey  = 32;
		ret.masterSalt = 14;
		ret.key        = 32;
		ret.salt       = 14;
		ret.authKey    = 20;
		ret.authTag    = 10;
		break;
	case Suite::AES_CM_128_HMAC_SHA1_32:
		ret.masterKey  = 16;
		ret.masterSalt = 14;
		ret.key        = 16;
		ret.salt       = 14;
		ret.authKey    = 20;
		ret.authTag    =  4;
		break;
	case Suite::AES_256_CM_HMAC_SHA1_32:
		ret.masterKey  = 32;
		ret.masterSalt = 14;
		ret.key        = 32;
		ret.salt       = 14;
		ret.authKey    = 20;
		ret.authTag    =  4;
		break;
	}

	return ret;
}

/*! \brief convenience constructor
 */
SDESKey::SDESKey(Suite suite)
{
	generate(suite);
}

/*!
 * \brief SDESKey generator
 *
 * \param suite to use to generate the keys
 *
 * Once function is complete, each of the key, salt, and base64
 * encoded tree will be available
 */
void SDESKey::generate(Suite suite)
{
	m_suite = suite;

	RandomBytesEngine engine; // also seeds the engine

	Sizes sizes = sizesGet(suite);

	m_key  = engine.bytesGet(sizes.masterKey);
	m_salt = engine.bytesGet(sizes.masterSalt);

	// Base64 encode
	m_base64KeyParams = base64Encode(m_key + m_salt);

	m_valid = true;
}

/*!
 * \brief SDESKey decode
 *
 * \param suite
 * \param base64 encoded key
 *
 * Once function is complete, each of the key, salt, and base64
 * encoded tree will be available
 */
SDESKey SDESKey::decode(const std::string &suiteStr, const std::string &base64key)
{
	SDESKey key;
	bool error = false;
	std::string decoded;

	SDESKey::Suite suite = fromString(suiteStr, error);

	if(!error)
	{
		key.m_suite = suite;
		
		// It is possible for there to be optional parameters for lifetime and MKI
		// to be after the key seperated by '|' we need to get just the key when passing to
		// base64Decode in order for the size check to succeed.
		auto optionalParameters = base64key.find ("|");
		
		if (optionalParameters != std::string::npos)
		{
			decoded = base64Decode(base64key.substr(0,optionalParameters));
		}
		else
		{
			decoded = base64Decode(base64key);
		}

		Sizes sizes = sizesGet(suite);

		if(sizes.masterKey + sizes.masterSalt == decoded.size()) // size sanity check
		{
			key.m_key  = decoded.substr(0, sizes.masterKey);
			key.m_salt = decoded.substr(sizes.masterKey);

			// Store the encoded key
			key.m_base64KeyParams = base64key;

			key.m_valid = true;
		}
	}

	return key;
}

bool operator==(const SDESKey &lhs, const SDESKey &rhs)
{
	return
		lhs.validGet()           == rhs.validGet() &&
		lhs.suiteGet()           == rhs.suiteGet() &&
		lhs.methodGet()          == rhs.methodGet() &&
		lhs.keyGet()             == rhs.keyGet() &&
		lhs.saltGet()            == rhs.saltGet() &&
		lhs.base64KeyParamsGet() == rhs.base64KeyParamsGet()
		;
}

std::ostream & operator<<(std::ostream& stream, const SDESKey& key)
{
	if(key.validGet())
	{
		stream
			<< vpe::SDESKey::toString(key.suiteGet())
			<< " " << key.methodGet() << ":"
			<< key.base64KeyParamsGet();
	}

	return stream;
}

} // vpe namespace
