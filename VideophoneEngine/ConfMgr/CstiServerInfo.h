/*!
 * \file CstiServerInfo
 * \brief Defines the class that contains server information
 * 
 * This class is used to hold server information such as address, username and password
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2012 – 2012 Sorenson Communications, Inc. -- All rights reserved
 */

class CstiServerInfo
{
public:

	CstiServerInfo() = default;

	CstiServerInfo (
		const char *address,
		const char *username,
		const char *password)
	:
		m_address (address),
		m_username (username),
		m_password (password)
	{
	}

	bool operator== (
		const CstiServerInfo &Other)
	{
		bool result = false;

		if(m_address == Other.m_address
			&& m_username == Other.m_username
			&& m_password == Other.m_password)
		{
			result = true;
		}

		return result;
	}


	void AddressSet (
		const char *address)
	{
		m_address = address;
	}

	const char *AddressGet () const
	{
		// TODO: would be nice to just return std::string copy to not expose
		// internals, but let's not change the interface just yet
		return m_address.c_str();
	}

	void UsernameSet (
		const char *username)
	{
		m_username = username;
	}

	const char *UsernameGet () const
	{
		return m_username.c_str();
	}

	void PasswordSet (
		const char *password)
	{
		m_password = password;
	}

	const char *PasswordGet () const
	{
		return m_password.c_str();
	}

private:

	std::string m_address;
	std::string m_username;
	std::string m_password;
};


