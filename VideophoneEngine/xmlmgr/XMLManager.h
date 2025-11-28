/*!
*  \file XMLManager.h
*  \brief The XML Manager
*
*  Class declaration for a generic XML Manager class.  Provides storage
*  and access for persitent(both local and remote) and temporary data.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*/

#ifndef XMLMANAGER_H
#define XMLMANAGER_H

#include "ixml.h"
#include "stiError.h"

// for WatchDog
#include "stiDefs.h"
#include "stiOS.h"

#include <memory>
#include <limits>

namespace WillowPM
{
class XMLList;
class XMLListItem;
using XMLListItemSharedPtr = std::shared_ptr<XMLListItem>;

#ifndef OUT
	#define OUT
#endif // OUT

#ifndef IN
	#define IN
#endif // IN



/**
* XML Value Types
*/

const char XT_STRING[] = "string";
const char XT_INT[] =    "int";
const char XT_ENUM[] =   "enum";

/**
* Error Codes
*/
const int XM_RESULT_SUCCESS =         0;       //!< no error
const int XM_RESULT_NOT_FOUND =       1;       //!< property not found in the PropertyManager
const int XM_RESULT_TYPE_MISMATCH =   2;       //!< existing type doesn't match the request
const int XM_RESULT_IXML_ERROR =      3;       //!< problem reading/writing/accessing underlying XML
const int XM_RESULT_VALUE_CHANGED = 9999;      //!< internal message: this property already in table

// default integer return value for the propertyGet
const int DEFAULT_INT =          std::numeric_limits<int>::max (); // 2^31-1
const int64_t DEFAULT_INT64 =    std::numeric_limits<int64_t>::max (); // 2^63-1
const char DEFAULT_STRING[] =    "zzzzzzzz";
const char szXM_EMPTY_STRING[] = "";
const char ATT_TYPE[] =  "type";

constexpr int g_IntegerDefault = 0;

/*!
*  \brief XML Manager Class
*
*  Outlines the public(and private) APIs and member variables
*/
class XMLManager
{
public:

	virtual ~XMLManager();
	XMLManager (const XMLManager &other) = delete;
	XMLManager (XMLManager &&other) = delete;
	XMLManager &operator= (const XMLManager &other) = delete;
	XMLManager &operator= (XMLManager &&other) = delete;

	virtual stiHResult init();
	virtual void uninit ();
	virtual stiHResult saveToPersistentStorage();

	int backup();

	/*!
	* Prevents the property table from saving out to file(used for specialized acceptance testing)
	*/
	virtual void PersistentSaveSet(bool bSet) { lock (); m_bPersistentSave = bSet; unlock ();}

	void SetFilename(const std::string &filename) { m_filename = filename; }
	void SetBackup1Filename(const std::string &filename) { m_backup1Filename = filename; }
	void SetDefaultFilename(const std::string &filename) { m_defaultFilename = filename; }
	void SetUpdateFilename(const std::string &filename) { m_updateFilename = filename; }

	virtual XMLListItemSharedPtr NodeToItemConvert(IXML_Node *pNode) = 0;

	void SaveDataTimerSet(bool Set) { m_bSaveDataTimerSet = Set; }

	/*!
	* Locks/Unlocks the XML manager
	*/
	void lock() const { stiOSMutexLock(m_mutex); }
	void unlock() const { stiOSMutexUnlock(m_mutex); }

protected:

	/**
	*  \brief protected constructor for singleton purposes
	*/
	XMLManager();

	stiHResult loadPersistentTable();
	void loadPreviousPersistentTable();

	int itemGet(const std::string &itemName, XMLListItemSharedPtr *item) const;
	int intGet(const std::string &itemName, int64_t *pIntVal, int Field, int64_t defaultVal) const;
	int intGet(const std::string &itemName, int64_t *pIntVal, int Field) const;

	int stringGet(
		const std::string &itemName,
		std::string &stringVal,
		int Field,
		const std::string &defaultVal) const;

	int stringGet(
		const std::string &itemName,
		std::string &stringVal,
		int Field) const;

	int valueGet(const std::string &name, std::string &value, std::string &typeString, int Field) const;

	int valueSet(const std::string &name, const std::string &value, int Field);

	void resetToDefaults();

	virtual int printTables();

	std::string GetFilename() { return m_filename; }
	std::string GetBackup1Filename() { return m_backup1Filename; }
	std::string GetDefaultFilename() { return m_defaultFilename; }
	std::string GetUpdateFilename() { return m_updateFilename; }

	XMLList* m_pdoc{nullptr};  //!< persistent data table

	void startSaveDataTimer();

	/**
	*  \brief boolean variable indicates if the save data watchdog timer is running
	*/
	bool m_bSaveDataTimerSet{false};

private:

	std::string GetPersistentFile();
	void writeToFile(const std::string &file);

	stiWDOG_ID m_wdSaveData{nullptr}; //!< watch dog timer for saving persistent data

	bool m_bPersistentSave{true};

	std::string m_filename;
	std::string m_backup1Filename;
	std::string m_defaultFilename;
	std::string m_updateFilename;

	stiMUTEX_ID m_mutex{nullptr};  //!< Provides thread saftey for the XMLManager
	unsigned int m_nWriteCount{0};

}; // end PropertyManager class definition




}

#endif
