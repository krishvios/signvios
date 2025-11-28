/*!
 * \file CstiNvmImageCache.h
 * \brief This class provides an image cache that is stored in non-volatile
 * memory (NVM) such as a hard disk.  The cache maintains a configurable fixed
 * size maximum image count.  Images are prioritized based on most recently used
 * ordering so that the least used images are purged first when the cache needs
 * to evict something.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

#include "CstiNvmImageCache.h"

#include <dirent.h>

#include <fstream>
#include <set>

#include <cstdio>

namespace
{
	const char* MANIFEST_FILENAME = ".manifest";
	const char MANIFEST_DELIMITER = '|';
}

CstiNvmImageCache::CstiNvmImageCache(
	const std::string& path, unsigned int capacity):
	m_PATH(path),
	m_CAPACITY(capacity),
	m_cacheMutex(stiOSMutexCreate()),
	m_manifestDirty(false)
{
}

CstiNvmImageCache::~CstiNvmImageCache()
{
	WriteManifestIfDirty();
	stiOSMutexDestroy(m_cacheMutex);
}

void CstiNvmImageCache::Initialize()
{
	LoadManifest();
}

void CstiNvmImageCache::Add(const std::string& guid,
							const std::string& timestamp)
{
	stiOSMutexLock(m_cacheMutex);

	m_cache.push_front(Entry(guid, timestamp));

	if (m_cache.size() > m_CAPACITY)
	{
		std::remove(FilePath(m_cache.back().guid).c_str());
		m_cache.pop_back();
	}

	m_manifestDirty = true;

	stiOSMutexUnlock(m_cacheMutex);

}

void CstiNvmImageCache::Purge(const std::string& guid)
{
	stiOSMutexLock(m_cacheMutex);

	for (auto suspect = m_cache.begin();
		 suspect != m_cache.end();
		 ++suspect)
	{
		if (suspect->guid == guid)
		{
			m_cache.erase(suspect);
			std::remove(FilePath(guid).c_str());
			m_manifestDirty = true;
			break;
		}
	}

	stiOSMutexUnlock(m_cacheMutex);
}

void CstiNvmImageCache::PurgeAll()
{
	stiOSMutexLock(m_cacheMutex);

	if (!m_cache.empty())
	{
		for (auto image = m_cache.begin();
			 image != m_cache.end();
			 ++image)
		{
			std::remove(FilePath(image->guid).c_str());
		}

		m_cache.clear();

		m_manifestDirty = true;
	}

	stiOSMutexUnlock(m_cacheMutex);
}

void CstiNvmImageCache::PurgeIfOlderThan(const std::string& guid,
										 const std::string& timestamp)
{
	stiOSMutexLock(m_cacheMutex);

	for (auto suspect = m_cache.begin();
		 suspect != m_cache.end();
		 ++suspect)
	{
		if (suspect->guid == guid)
		{
			if (suspect->timestamp != timestamp)
			{
				Purge(guid);
			}
			break;
		}
	}

	stiOSMutexUnlock(m_cacheMutex);
}

bool CstiNvmImageCache::Contains(const std::string& guid) const
{
	bool result = false;

	stiOSMutexLock(m_cacheMutex);

	for (auto suspect = m_cache.begin();
		 suspect != m_cache.end();
		 ++suspect)
	{
		if (suspect->guid == guid)
		{
			result = true;
			break;
		}
	}

	stiOSMutexUnlock(m_cacheMutex);

	return result;
}

void CstiNvmImageCache::MoveToFront(const std::string& guid)
{
	stiOSMutexLock(m_cacheMutex);

	for (auto image = m_cache.begin();
		 image != m_cache.end();
		 ++image)
	{
		if (image->guid == guid)
		{
			m_cache.splice(m_cache.begin(), m_cache, image);
			m_manifestDirty = true;
			break;
		}
	}

	stiOSMutexUnlock(m_cacheMutex);
}

std::string CstiNvmImageCache::FilePath(const std::string& guid) const
{
	return m_PATH + "/" + guid;
}

void CstiNvmImageCache::WriteManifestIfDirty()
{
	stiOSMutexLock(m_cacheMutex);

	if (m_manifestDirty)
	{
		WriteManifest();
	}

	stiOSMutexUnlock(m_cacheMutex);
}

void CstiNvmImageCache::LoadManifest()
{
	//--------------------------------------------------------------------------
	// See what's on the filesystem
	//--------------------------------------------------------------------------
	typedef std::set<std::string> CacheFiles;
	CacheFiles cached_image_files;

	DIR *pDir = opendir(m_PATH.c_str());

	if (pDir)
	{
		struct dirent *entry;

		// NOTE: readdir_r is deprecated as of glibc 2.24.
		// that version of the manpage (READDIR(3)) suggests readdir()
		// is threadsafe on modern implementations when different
		// directory streams are used, which is the case here
		while ((entry = readdir(pDir)) != nullptr)
		{
			if (   strcmp(".", entry->d_name) != 0
				&& strcmp("..", entry->d_name) != 0
				&& strcmp(MANIFEST_FILENAME, entry->d_name) != 0)
			{
				cached_image_files.insert(entry->d_name);
			}
		}

		closedir(pDir);
	}

	//--------------------------------------------------------------------------
	// Read the manifest file building the actual cache out of things that are
	// actually on the filesystem
	//--------------------------------------------------------------------------
	std::ifstream cache_manifest(
		(m_PATH + "/" + MANIFEST_FILENAME).c_str(),
		std::ifstream::in);

	stiOSMutexLock(m_cacheMutex);

	m_cache.clear();

	if (cache_manifest)
	{
		while (!cache_manifest.eof() && !cache_manifest.fail())
		{
			Entry cache_entry;

			std::getline(
				cache_manifest,
				cache_entry.guid,
				MANIFEST_DELIMITER);

			std::getline(
				cache_manifest,
				cache_entry.timestamp);

			if (!cache_manifest.fail() && !cache_manifest.eof())
			{
				auto cached_file =
					cached_image_files.find(cache_entry.guid);

				if (cached_file != cached_image_files.end())
				{
					m_cache.push_back(cache_entry);
					cached_image_files.erase(cached_file);
				}
			}
		}

		cache_manifest.close();
	}

	//--------------------------------------------------------------------------
	// Remove unaccounted for images from NVM
	//--------------------------------------------------------------------------
	for (auto cached_file = cached_image_files.begin();
		 cached_file != cached_image_files.end();
		 ++cached_file)
	{
		std::remove(FilePath(*cached_file).c_str());
	}

	//--------------------------------------------------------------------------
	// Make sure the cache size limit hasn't been exceeded
	//--------------------------------------------------------------------------
	while (m_cache.size() > m_CAPACITY)
	{
		std::remove(FilePath(m_cache.back().guid).c_str());
		m_cache.pop_back();
	}

	stiOSMutexUnlock(m_cacheMutex);
}

void CstiNvmImageCache::WriteManifest()
{
	std::ofstream cache_manifest(
		(m_PATH + "/" + MANIFEST_FILENAME).c_str(),
		std::ofstream::out | std::ofstream::trunc);

	if (cache_manifest)
	{
		stiOSMutexLock(m_cacheMutex);

		for (auto cache_entry = m_cache.cbegin();
			 cache_entry != m_cache.cend();
			 ++cache_entry)
		{
			cache_manifest << cache_entry->guid
						   << MANIFEST_DELIMITER
						   << cache_entry->timestamp
						   << std::endl;
		}

		m_manifestDirty = false;

		stiOSMutexUnlock(m_cacheMutex);

		cache_manifest.close();
	}
}
