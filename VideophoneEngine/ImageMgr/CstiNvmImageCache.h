/*!
 * \file CstiNvmImageCache.h
 * \brief See CstiNvmImageCache.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

#ifndef CSTINVMIMAGECACHE_H
#define	CSTINVMIMAGECACHE_H

#include "stiOSMutex.h"

#include <list>
#include <string>

class CstiNvmImageCache
{
public:
	CstiNvmImageCache(const std::string& path, unsigned int capacity);
	~CstiNvmImageCache();

	void Initialize();
	void Add(const std::string& guid, const std::string& timestamp);
	void Purge(const std::string& guid);
	void PurgeAll();
	void PurgeIfOlderThan(const std::string& guid, const std::string& timestamp);
	bool Contains(const std::string& guid) const;
	void MoveToFront(const std::string& guid);
	std::string FilePath(const std::string& guid) const;
	void WriteManifestIfDirty();

private:
	struct Entry
	{
		std::string guid;
		std::string timestamp;

		Entry() = default;

		Entry(const std::string& guid, const std::string& timestamp):
			guid(guid),
			timestamp(timestamp)
		{
		}
	};

	typedef std::list<Entry> Cache;

	const std::string m_PATH;
	const unsigned int m_CAPACITY;

	Cache m_cache;
	stiMUTEX_ID m_cacheMutex;
	bool m_manifestDirty;

	void LoadManifest();
	void WriteManifest();
};

#endif
