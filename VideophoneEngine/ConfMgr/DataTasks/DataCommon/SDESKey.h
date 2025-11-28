#pragma once

#include <string>
#include <array>
#include "stiSVX.h"

namespace vpe
{

class SDESKey
{
public:

	// TODO: how many suites do we want to support?
	enum class Suite
	{
		AES_CM_128_HMAC_SHA1_80,
		AES_CM_128_HMAC_SHA1_32,
		AES_256_CM_HMAC_SHA1_80,
		AES_256_CM_HMAC_SHA1_32,
	};
	// Create list of items to be able to iterate through them
	// Would be nice to have an enum type to provide this without duplicating
	// the list, but oh well...
	// TODO: can't initialize here? ugh...
	static const std::array<SDESKey::Suite, 4> Suites;

	static std::string toString(Suite suite);
	static Suite fromString(const std::string &suiteStr, bool &error);

	static KeyExchangeMethod suiteToKeyExchangeMethod (Suite suite);

	// Size in bytes for a crypto suite
	// NOTE: values are defined in rfcs
	struct Sizes
	{
		unsigned int masterKey = 0;
		unsigned int masterSalt = 0;
		unsigned int mki = 4; // currently hard coded
		unsigned int key = 0; // derived
		unsigned int salt = 0; // derived
		unsigned int authKey = 0; // derived key
		unsigned int authTag = 0;
	};

	static Sizes sizesGet(const Suite &suite);

	SDESKey() = default;
	SDESKey(Suite suite);

	~SDESKey() = default;

	SDESKey (const SDESKey &other) = default;
	SDESKey (SDESKey &&other) = default;
	SDESKey &operator= (const SDESKey &other) = default;
	SDESKey &operator= (SDESKey &&other) = default;

	void generate(Suite suite);

	static SDESKey decode(const std::string &suiteStr, const std::string &base64key);

	std::string methodGet() const
	{
		return m_method;
	}

	std::string keyGet() const
	{
		return m_key;
	}

	std::string saltGet() const
	{
		return m_salt;
	}

	std::string base64KeyParamsGet() const
	{
		return m_base64KeyParams;
	}

	bool validGet() const
	{
		return m_valid;
	}

	Suite suiteGet() const
	{
		return m_suite;
	}

	std::string mkiGet() const
	{
		return m_mki;
	}

	void mkiSet(const std::string &mki)
	{
		m_mki = mki;
	}

private:

	bool m_valid {false};
	Suite m_suite {Suite::AES_256_CM_HMAC_SHA1_80};

	std::string m_method = "inline"; // always inline for now

	std::string m_key;
	std::string m_salt;

	std::string m_mki;

	std::string m_base64KeyParams;

};

// For convenience
bool operator==(const SDESKey &lhs, const SDESKey &rhs);
std::ostream & operator<<(std::ostream& stream, const SDESKey& key);

} // vpe namespace
