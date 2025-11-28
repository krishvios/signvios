// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

class DbusError
{
public:

	DbusError () = default;
	DbusError (const DbusError &other) = delete;
	DbusError (DbusError &&other) = delete;
	DbusError &operator= (const DbusError &other) = delete;
	DbusError &operator= (DbusError &&other) = delete;

	~DbusError ()
	{
		if (m_dbusError)
		{
			g_error_free(m_dbusError);
			m_dbusError = nullptr;
		}
	}

	std::string message ()
	{
		return m_dbusError->message;
	}

	GError **get ()
	{
		return &m_dbusError;
	}

	bool valid ()
	{
		return m_dbusError != nullptr;
	}

private:
	GError *m_dbusError {nullptr};
};

