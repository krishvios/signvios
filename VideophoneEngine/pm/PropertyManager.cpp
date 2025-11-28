/*!
*  \file PropertyManager.cpp
*  \brief Implementation of the Property Manager class
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*/

#include <cstdio>
#include "PropertyListItem.h"
#include "XMLList.h"
#include "PropertyManager.h"
#include "CstiPropertySender.h"
#include "stiTrace.h"
#include <fstream>
#include <sys/stat.h>
#include <sstream>
#include <cinttypes>


#define MAX_PROPERTIES_TO_SEND 30
#define MAX_TIME_TO_WAIT (5 * 1000) // 5 seconds, time is in milliseconds
#define MAX_ERROR_TIME_TO_WAIT (5 * 60 * 1000) // 5 minutes, time is in milliseconds



using namespace std;
using namespace WillowPM;

// forward declarations

//
// Constants
//
#if APPLICATION != APP_NTOUCH_VP2 && APPLICATION != APP_NTOUCH_VP3 && APPLICATION != APP_NTOUCH_VP4
#ifdef WIN32
#define BASE_DIR "C:"
#elif defined __APPLE__
#define BASE_DIR "/tmp"
#elif APPLICATION == APP_NTOUCH_ANDROID
#define BASE_DIR "/data/data/com.sorenson.mvrs.android/files/"
#else
#error Unknown application
#endif

// Local Persistent XML file
const char WillowPM::LOCAL_PERSIST_XML[] = BASE_DIR "/PropertyTable.xml";

// Persistent XML backup file (previous version)
const char WillowPM::LOCAL_PERSIST_BACKUP_XML[]   = BASE_DIR "/PropertyTable.xml.bck";
#endif

// Local Persistent DEFAULTS XML file
#if APPLICATION != APP_NTOUCH_VP2 && APPLICATION != APP_NTOUCH_VP3 && APPLICATION != APP_NTOUCH_VP4
const char DEFAULT_TEMP_XML[]           = "/usr/pm/DefaultProperties.xml";

// Forced Update XML file
const char UPDATE_XML[]                 = "/usr/pm/UpdateProperties.xml";
#endif

// Property String Constants used with updating the XML and the firmware
const char pmUPDATE_XML_COMPLETED[]     = "pmUpdateXmlCompleted";

// Version Constant String for lookup
const char SW_VERSION_NUMBER[] = "swVersionNumber";


// private constructor
PropertyManager::PropertyManager()
{
	stiDEBUG_TOOL (g_stiPMDebug,
		stiTrace ("PropertyManager::PropertyManager:%s\n",
			"Constructor called");
	);

#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
	std::string DynamicDataFolder;

	stiOSDynamicDataFolderGet (&DynamicDataFolder);

	std::stringstream PropertyTableFile;
	std::stringstream BackupPropertytTableFile;

	PropertyTableFile << DynamicDataFolder << "pm/PropertyTable.xml";
	BackupPropertytTableFile << DynamicDataFolder << "pm/PropertyTable.xml.bck";

	SetFilename(PropertyTableFile.str ());
	SetBackup1Filename(BackupPropertytTableFile.str ());
#else
	SetFilename(LOCAL_PERSIST_XML);
	SetBackup1Filename(LOCAL_PERSIST_BACKUP_XML);
#endif
	
#if APPLICATION == APP_NTOUCH_VP2
	SetDefaultFilename("/usr/share/ntouchvp2/DefaultProperties.xml");
	SetUpdateFilename("/usr/share/ntouchvp2/UpdateProperties.xml");
#elif APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
	SetDefaultFilename("/usr/share/lumina/DefaultProperties.xml");
	SetUpdateFilename("/usr/share/lumina/UpdateProperties.xml");
#else
	SetDefaultFilename(DEFAULT_TEMP_XML);
	SetUpdateFilename(UPDATE_XML);
#endif
	
	
}


void PropertyManager::uninit ()
{
	// delete the property sender
	if (nullptr != m_pPropertySender)
	{
		delete m_pPropertySender;
		m_pPropertySender = nullptr;
	}

	// unregister property listeners
	m_oPropertyChangeCallBacks.clear();

	XMLManager::uninit ();

	m_bInitialized = false;
}


// public static getInstance: retrieves the instance of the PropertyManager
PropertyManager* PropertyManager::getInstance()
{
	 // Meyers singleton: a local static variable (cleaned up automatically at exit)
	static PropertyManager localPropManager;

	return &localPropManager;
}

// Compare the image version to the stored version in persistent data
bool PropertyManager::versionsMatch (
	const std::string &applicationVersion)
{
	bool match = false;
	std::string propertyTableVersion;

	if (XM_RESULT_NOT_FOUND == getPropertyString(SW_VERSION_NUMBER, &propertyTableVersion, Persistent))
	{
		// Version hasn't ever been set in the PropertyTable.  Treat it as
		// though the versions match.
		stiDEBUG_TOOL (g_stiPMDebug,
			stiTrace ("PropertyManager::versionsMatch - Version info not found.\n");
		);

		match = true;
	} // end if
	else
	{
		if (applicationVersion == propertyTableVersion)
		{
			match = true;
		}

		stiDEBUG_TOOL (g_stiPMDebug,
			stiTrace ("PropertyManager::versionsMatch - Property version %s %s match image version.\n",
				propertyTableVersion.c_str (), match ? "DOES" : "DOES NOT");
		);

	} // end else

	return match;
}

// check for and process property updates
void PropertyManager::updateProperties ()
{
	// Check to see if we have some forced update values (if we haven't already
	// done so.
	int bUpdateXmlCompleted = 0;

	getPropertyInt(pmUPDATE_XML_COMPLETED, &bUpdateXmlCompleted, Persistent);

	if (!bUpdateXmlCompleted)
	{
		stiDEBUG_TOOL (g_stiPMDebug,
			stiTrace ("Performing UPDATE of XML properties\n");
		);
		IXML_Document* pUpdateDoc = nullptr;
		IXML_Node* pUpdateRootNode;
		IXML_Node* pCurNode;
		IXML_NamedNodeMap* pstMap = nullptr;
		const DOMString szTag;
		const DOMString szType;
		const DOMString szValue;
		int ret = ixmlLoadDocumentEx(GetUpdateFilename().c_str (), &pUpdateDoc);
		if ((nullptr != pUpdateDoc) && (0 == ret))
		{
			pUpdateRootNode = ixmlNode_getFirstChild((IXML_Node*)pUpdateDoc);
			pCurNode = ixmlNode_getFirstChild (pUpdateRootNode);

			// Traverse each value and force it to the persistent table
			// NOTE:
			//  The following code is structured for a simple xml file like the
			//  following example
			//  <root>
			//  <name1 type="int">1</name1>
			//  <name2 type="enum">2</name2>
			//  <name3 type="string">This is the string</name3>
			//  </root>
			//
			//  The only things that will vary is the number of entries, the type
			//  will be either "int", "enum", or "string", the name will be any name,
			//  and the value associated with this particular entry as in "1", "2",
			//  and "This is the string" which were used in the example file above.
			while (nullptr != pCurNode)
			{
				pstMap = ixmlNode_getAttributes (pCurNode);
				if (nullptr != pstMap)
				{
					szTag = ixmlNode_getLocalName (pCurNode);
					szType = ixmlNode_getNodeValue (ixmlNamedNodeMap_getNamedItem (pstMap, (DOMString)"type"));
					szValue = ixmlNode_getNodeValue (ixmlNode_getFirstChild (pCurNode));
					stiDEBUG_TOOL (g_stiPMDebug,
						stiTrace ("Data for Node <%s>\n\tType <%s>, Value = %s\n",
							szTag, szType, szValue);
						stiTrace ("\tSaving updated value to the persistent table\n");
					);

					// Update the value in the persistent table
					setPropertyValue(szTag, szType, (char*)szValue, Persistent);

					ixmlNamedNodeMap_free(pstMap);
				} // end if

				// Get the next value node to update
				pCurNode = ixmlNode_getNextSibling (pCurNode);
			} // end while

			// Set a flag denoting the update has been processed.
			setPropertyInt(pmUPDATE_XML_COMPLETED, 1, Persistent);

			// free up the UPDATE dom table
			ixmlDocument_free(pUpdateDoc);
			pUpdateDoc = nullptr;
		}
	} // end if
}

// Check to see if a firmware update just occurred, if so, change the version
bool PropertyManager::firmwareUpdate (
	const std::string &applicationVersion)
{
	int firmwareUpdated = 0;

	// Get the firmware updated flag from the PropertyTable
	getPropertyInt(pmFIRMWARE_UPDATED, &firmwareUpdated, Persistent);

	// Did a firmware update occur?
	if (firmwareUpdated)
	{
		stiDEBUG_TOOL (g_stiPMDebug,
			stiTrace ("Firmware update has occurred.  Updating version numbers.\n");
		);

		// Now set the old version numbers in the active table to match the
		// current image.
		setPropertyString(SW_VERSION_NUMBER, applicationVersion, Persistent);

		// Set a flag to cause the system to look for updated properties.
		setPropertyInt(pmUPDATE_XML_COMPLETED, 0, Persistent);

	} // end if

	return (bool)firmwareUpdated;
}

/**
* \brief Initializes the PropertyManager object by loading XML documents
*
*/
void PropertyManager::init(
	const std::string &applicationVersion)
{
	if (!m_bInitialized)
	{
		// lock the PropertyManager during the initialize
		lock();

		if (m_pPropertySender == nullptr)
		{
			m_pPropertySender = new CstiPropertySender(MAX_PROPERTIES_TO_SEND, MAX_TIME_TO_WAIT, MAX_ERROR_TIME_TO_WAIT);
		}

		XMLManager::init();

		// Assert on any duplicate properties
		m_pdoc->DuplicatesCheck();

		// Call firmwareUpdate to kick off updating properties if needed
		bool bFirmwareUpdated = firmwareUpdate (applicationVersion);

#ifndef stiDISABLE_PM_VERSION_CHECK
		// Check to see if we just completed an update of the firmware.
		if (!bFirmwareUpdated)
		{
			// If we didn't just complete a firmware update ...
			// Check to see if the version stored in the PropertyTable matches the
			// image version that is running.
			if (!versionsMatch (applicationVersion))
			{
				// The versions don't match.  We need to attempt to restore the
				// version that matches the running system.  This will be the case
				// if U-Boot loads an older version because (for example) the latest
				// image is corrupt.
				loadPreviousPersistentTable ();

				// Check again to see if the version stored matches the image
				// version that is running.
				if (!versionsMatch (applicationVersion))
				{
					// They still don't match.  Close the current one.  Delete it
					// from disk and load again (which will simply load the default
					// values).
					m_pdoc->ListFree();
					remove (GetFilename().c_str ());
					loadPersistentTable ();
				} // end if
			} // end if
		} // end if
#else
		stiUNUSED_ARG (bFirmwareUpdated);
		stiDEBUG_TOOL (g_stiPMDebug,
			stiTrace ("PropertyManager::init -- SKIPPED VERSION CHECKING\n");
		);
		bFirmwareUpdated = false;
#endif

		// Check to see if we have some forced update values to incorporate.
		updateProperties ();

		m_bInitialized = true;

		// unlock the PM
		unlock();
	}

	return;
}

//
// Disable the PropertyManager so no one can write to it
//
void PropertyManager::Disable()
{
	lock();

	m_bEnabled = estiFALSE;

	unlock();
}

//
// Re-enable the PropertyManager so that it can be written to
//
void PropertyManager::Enable()
{
	lock();

	m_bEnabled = estiTRUE;

	unlock();
}

//
// Sets the core service object.
//
void PropertyManager::CoreServiceSet (
	CstiCoreService *pCoreService)
{
	m_pPropertySender->CoreServiceSet (pCoreService);
}


void PropertyManager::PropertySenderEnable (
	EstiBool bUsePropertySender)
{
	// If the property sender was just disabled, check to see
	// if there are any properties waiting to be sent and if so,
	// remove them.
	if (bUsePropertySender != m_bUsePropertySender)
	{
		stiDEBUG_TOOL (g_stiPSDebug,
			stiTrace ("<PM::PropertySenderEnable> Changing to %suse PropertySender\n", bUsePropertySender ? "" : "NOT ");
		);
		if (!bUsePropertySender)
		{
			m_pPropertySender->PropertySendCancel ();
		}
		m_bUsePropertySender = bUsePropertySender;
	}
}

void PropertyManager::PropertySend (
	const std::string& name,
	int nType)
{
	if (m_pPropertySender && m_bUsePropertySender)
	{
		m_pPropertySender->PropertySend(name, nType);
	}
}

void PropertyManager::SendProperties()
{
	m_pPropertySender->SendProperties();
}


void PropertyManager::PropertySendWait (int timeoutMilliseconds)
{
	m_pPropertySender->PropertySendWait (timeoutMilliseconds);
}


void PropertyManager::ResponseCheck(
	const CstiCoreResponse *poResponse)
{
	if (m_pPropertySender)
	{
		m_pPropertySender->ResponseCheck(poResponse);
	}
}


/*!
* \brief Gets a property as an integer value
*
* Gets the property value as an integer.  By default, will check in the temporary
* table first, and then the persistent table next.  If the fromTemp parameter is
* set to false, this function will first _remove_ the property from the temporary
* table (so future calls will not see it in the temporary table), then checks the
* persistent table and retrieves it from there.
*
* \param propertyName - name of property to retrieve
* \param intVal - output: holds the retrieved integer property value
* \param defaultValue - returns this value on error (default is DEFAULT_INT if unspecified)
* \param eLocation - storaage location for poperty (persistent or temporary memory)
* \retval An integer error code, 0 == SUCCESS
*/
int PropertyManager::getPropertyInt(
	const std::string &propertyName,
	int* intVal,
	int defaultValue,
	EStorageLocation eLocation)
{
	int64_t int64Val;
	int code = intGet(propertyName, &int64Val, eLocation, defaultValue);
	*intVal = static_cast<int>(int64Val);

	stiASSERTMSG(int64Val == *intVal,
		"Precision lost when retrieving 32bit property value");

	return code;
}

int PropertyManager::getPropertyInt(
	const std::string &propertyName,
	int* intVal,
	EStorageLocation eLocation)
{
	int64_t int64Val;
	auto code = intGet(propertyName, &int64Val, eLocation);
	*intVal = static_cast<int>(int64Val);

	stiASSERTMSG(int64Val == *intVal,
		"Precision lost when retrieving 32bit property value");

	return code;
}



int PropertyManager::getPropertyInt(
	const std::string &propertyName,
	int64_t* intVal,
	EStorageLocation eLocation)
{
	return intGet(propertyName, intVal, eLocation);
}

/*!
* \brief Sets a property with an integer value
*
* Sets the property with the specified value.  When the toTemp parameter is true,
* sets to the temporary table, whose values are lost when phone shuts down.  To
* save properties in the persistent table, set toTemp = false.
*
* \param pszPropertyName - name of property to set
* \param value - integer value to set in the property
* \param eLocation - storaage location for poperty (persistent or temporary memory)
* \retval An integer error code, 0 == SUCCESS
*/
int PropertyManager::setPropertyInt(
	const std::string &propertyName,
	int64_t value,
	EStorageLocation eLocation)
{
	// lock the XMLManager during the set
	lock();

	int errCode = XM_RESULT_NOT_FOUND;

	if (m_bEnabled)
	{
		bool changed = false;
		XMLListItemSharedPtr item;

		errCode = itemGet(propertyName, &item);

		if (errCode == XM_RESULT_SUCCESS)
		{
			int64_t oldValue = 0;
			int64_t newValue = 0;

			item->IntGet (oldValue, eLocation);
			item->IntSet(value, eLocation);
			item->IntGet (newValue, eLocation);

			changed = (oldValue != newValue);
		}
		else
		{
			auto newItem = std::make_shared<PropertyListItem> ();
			newItem->NameSet(propertyName);
			newItem->IntSet(value, eLocation);
			m_pdoc->ItemAdd(newItem);

			// If the value is different than the standard default value
			changed = (value != g_IntegerDefault);
		}

		// unlock the PM
		unlock();

		if (changed)
		{
			// Start the SaveData timer
			if (eLocation == Persistent)
			{
				startSaveDataTimer();
			}

			// Notify all callbacks that a property has changed
			callPropertyChangeNotifyFunc (propertyName.c_str ());

			errCode = XM_RESULT_SUCCESS;
		}
	}
	else
	{
		unlock();
	}

	return errCode;
}

/*!
* \brief Sets a property with an integer value
*
* Sets the property with the specified value if it doesn't already exist.  When the eLocation
* parameter is temporary, sets to the temporary table, whose values are lost when phone shuts down.
* To save properties in the persistent table, set eLocation to persistent.
*
* \param pszPropertyName - name of property to set
* \param value - integer value to set in the property
* \param eLocation - storage location for poperty (persistent or temporary memory)
* \retval An integer error code, 0 == SUCCESS
*/
void PropertyManager::setPropertyIntDefault(
	const std::string &propertyName,
	int64_t value)
{
	// lock the XMLManager during the get
	lock();

	if (m_bEnabled)
	{
		XMLListItemSharedPtr item;
		bool changed = false;

		auto errCode = itemGet(propertyName, &item);

		if (errCode == XM_RESULT_SUCCESS)
		{
			int64_t oldValue = 0;
			int64_t newValue = 0;

			// Changing the default of property may or may not change the value. If the
			// property does not have an assigned value yet then changing the default
			// will change its value. If the property already has an assigned value then
			// changing the default won't change the current value. Retrieve the value of
			// the property before and after setting the default to see if the value changes.
			item->IntGet (oldValue, Temporary);
			std::static_pointer_cast<PropertyListItem>(item)->defaultSet(value);
			item->IntGet (newValue, Temporary);

			changed = (oldValue != newValue);
		}
		else
		{
			auto newItem = std::make_shared<PropertyListItem> ();
			newItem->NameSet(propertyName);
			newItem->defaultSet(value);
			m_pdoc->ItemAdd(newItem);

			// If the new default value is different than the standard default value
			changed = (value != g_IntegerDefault);
		}

		unlock ();

		if (changed)
		{
			// Notify all callbacks that a property has changed
			callPropertyChangeNotifyFunc (propertyName.c_str ());
		}
	}
	else
	{
		unlock();
	}
}

/*!
* \brief Gets a property as a string value
*
* Gets the property value as a string.  By default, will check in the temporary
* table first, and then the persistent table next.  If the fromTemp parameter is
* set to false, this function will first _remove_ the property from the temporary
* table (so future calls will not see it in the temporary table), then checks the
* persistent table and retrieves it from there.
*
* \param propertyName - name of property to retrieve
* \param propertyVal - output: holds the retrieved string property value
* \param defaultVal - default value returned on error (default is NULL if unspecified)
* \param eLocation - storage location for poperty (persistent or temporary memory)
* \retval An integer error code, 0 == SUCCESS
*/
int PropertyManager::getPropertyString(
	const std::string &propertyName,
	std::string *propertyVal,
	const std::string &defaultVal,
	EStorageLocation eLocation)
{
	return stringGet(propertyName, *propertyVal, eLocation, defaultVal);
}


int PropertyManager::getPropertyString(
	const std::string &propertyName,
	std::string *propertyVal,
	EStorageLocation eLocation)
{
	return stringGet(propertyName, *propertyVal, eLocation);
}


int PropertyManager::propertyGet(
	const std::string &propertyName,
	std::string *propertyVal,
	EStorageLocation eLocation)
{
	return stringGet(propertyName, *propertyVal, eLocation);
}


int PropertyManager::propertySet (
	const std::string &propertyName,
	const std::string &value,
	EStorageLocation eLocation)
{
	return setPropertyString (propertyName, value, eLocation);
}


int PropertyManager::propertyGet (
	const std::string &propertyName,
	bool *value,
	EStorageLocation eLocation)
{
	int64_t intValue;

	auto result = getPropertyInt (propertyName, &intValue, eLocation);

	if (result == PM_RESULT_SUCCESS || result == PM_RESULT_NOT_FOUND)
	{
		*value = intValue != 0;
	}

	return result;
}

/*!
* \brief Sets a property with a string value
*
* Sets the property with the specified value.  When the toTemp parameter is true,
* sets to the temporary table, whose values are lost when phone shuts down.  To
* save properties in the persistent table, set toTemp = false.
*
* \param pszPropertyName - name of property to set
* \param value - String value to set in the property
* \param eLocation - storaage location for poperty (persistent or temporary memory)
* \retval An integer error code, 0 == SUCCESS
*/
int PropertyManager::setPropertyString(
	const std::string &propertyName,
	const std::string &value,
	EStorageLocation eLocation)
{
	// lock the XMLManager during the set
	lock();

	int errCode = XM_RESULT_NOT_FOUND;

	if (m_bEnabled)
	{
		XMLListItemSharedPtr item;
		bool changed = false;

		errCode = itemGet(propertyName, &item);

		if (errCode == XM_RESULT_SUCCESS)
		{
			std::string oldValue;
			std::string newValue;

			item->StringGet (oldValue, eLocation);
			item->StringSet(value, eLocation);
			item->StringGet (newValue, eLocation);

			changed = (oldValue != newValue);
		}
		else
		{
			auto newItem = std::make_shared<PropertyListItem> ();
			newItem->NameSet(propertyName);
			newItem->StringSet(value, eLocation);
			m_pdoc->ItemAdd(newItem);

			// If the value is different than the standard default value
			changed = !value.empty ();
		}

		// unlock the PM
		unlock();

		if (changed)
		{
			// Start the SaveData timer
			if (eLocation == Persistent)
			{
				startSaveDataTimer();
			}

			// Notify all callbacks that a property has changed
			callPropertyChangeNotifyFunc (propertyName.c_str ());

			errCode = XM_RESULT_SUCCESS;
		}
	}
	else
	{
		unlock();
	}

	return errCode;
}


void PropertyManager::propertyDefaultSet (
	const std::string &propertyName,
	const std::string &value)
{
	setPropertyStringDefault (propertyName, value);
}


/*!
* \brief Sets a property default value if it isn't already set
*
* Sets the property with the specified value if it doesn't already exist.  When the eLocation
* parameter is temporary, sets to the temporary table, whose values are lost when phone shuts down.
* To save properties in the persistent table, set eLocation to persistent.
*
* \param pszPropertyName - name of property to set
* \param value - string value to set in the property
* \param eLocation - storage location for poperty (persistent or temporary memory)
* \retval An integer error code, 0 == SUCCESS
*/
void PropertyManager::setPropertyStringDefault(
	const std::string &propertyName,
	const std::string &value)
{
	// lock the XMLManager during the get
	lock();

	if (m_bEnabled)
	{
		XMLListItemSharedPtr item;

		bool changed = false;
		auto errCode = itemGet(propertyName, &item);

		if (XM_RESULT_SUCCESS == errCode)
		{
			std::string oldValue;
			std::string newValue;

			// Changing the default of property may or may not change the value. If the
			// property does not have an assigned value yet then changing the default
			// will change its value. If the property already has an assigned value then
			// changing the default won't change the current value. Retrieve the value of
			// the property before and after setting the default to see if the value changes.
			item->StringGet (oldValue, Temporary);
			std::static_pointer_cast<PropertyListItem>(item)->defaultSet(value);
			item->StringGet (newValue, Temporary);

			changed = (oldValue != newValue);
		}
		else
		{
			auto newItem = std::make_shared<PropertyListItem> ();
			newItem->NameSet(propertyName);
			newItem->defaultSet(value);
			m_pdoc->ItemAdd(newItem);

			// If the new default value is different than the standard default value
			changed = !value.empty ();
		}

		// unlock the PM
		unlock();

		if (changed)
		{
			// Notify all callbacks that a property has changed
			callPropertyChangeNotifyFunc (propertyName.c_str ());
		}
	}
	else
	{
		unlock();
	}
}


/*!
* \brief Removes a property from property tables
* \param propertyName - name of property to remove
* \param fromAll - if true, removes property from all tables,
*  else just from temp table (default is true)
*/
int PropertyManager::removeProperty(
	const std::string &propertyName,
	bool fromAll)
{
	// lock the PropertyManager during the remove operation
	lock();

	XMLListItemSharedPtr item;

	auto errCode = itemGet(propertyName, &item);

	if (XM_RESULT_SUCCESS == errCode)
	{
		std::string oldValue;
		std::string newValue;

		item->StringGet (oldValue, Temporary);

		if (fromAll)
		{
			std::static_pointer_cast<PropertyListItem>(item)->RemoveValues();
		}
		else
		{
			std::static_pointer_cast<PropertyListItem>(item)->RemoveTemp();
		}

		item->StringGet (newValue, Temporary);

		if (oldValue != newValue)
		{
			// Notify all callbacks that a property has changed
			callPropertyChangeNotifyFunc (propertyName.c_str ());

			errCode = PM_RESULT_VALUE_CHANGED;
		}
	}

	// unlock the PM
	unlock();

	return errCode;
}


/*!
* \brief Gets a property value
* \param pszPropertyName - name of property to retrieve
* \param ppszPropertyType - the property type being requested
* \param ppszValue - output: holds the retrieved string property value
* \param eLocation - storaage location for poperty (persistent or temporary memory)
* \retval An integer error code, 0 == SUCCESS
*/
int PropertyManager::getPropertyValue(
	const std::string &propertyName,
	std::string *pPropertyType,
	std::string *pValue,
	EStorageLocation eLocation)
{
	// lock the XMLManager during the get
	lock();

	pValue->clear ();
	pPropertyType->clear ();

	XMLListItemSharedPtr item;
	int errCode = itemGet(propertyName, &item);

	if (errCode == XM_RESULT_SUCCESS)
	{
		std::static_pointer_cast<PropertyListItem> (item)->TypeGet(*pPropertyType);
		errCode = item->StringGet(*pValue, eLocation);

		// If the return code is "not found", when retrieving the value,
		// then change it back to "success" as "not found" simply means
		// the default value was returned.
		if (errCode == XM_RESULT_NOT_FOUND)
		{
			errCode = XM_RESULT_SUCCESS;
		}
	}

	// unlock the PM
	unlock();

	return errCode;
}

/*!
* \brief Sets the value of the specified property
*
* If the Property exists, changes it, otherwise adds it.  If the propertyType
* being passed in doesn't match existing type, an XM_RESULT_TYPE_MISMATCH is returned
* as a warning, but the set proceeds.
* \param pszPropertyName - name of property to set
* \param pszPropertyType - the type of property being set
* \param pszPropValStr - String value to set in the property
* \param eLocation - storaage location for poperty (persistent or temporary memory)
* \retval An integer error code, 0 == XM_RESULT_SUCCESS
*/
int PropertyManager::setPropertyValue(
	const std::string &propertyName,
	const std::string &propertyType,
	const std::string &propValStr,
	EStorageLocation eLocation)
{
	stiDEBUG_TOOL (g_stiPMDebug,
		stiTrace ("PropertyManager::setPropertyValue - Values:\n%s\n%s\n%s\n%d\n",
			propertyName.c_str (), propertyType.c_str (), propValStr.c_str (), (int)eLocation);
	);

	// lock the XMLManager during the set
	lock();

	int errCode = XM_RESULT_NOT_FOUND;

	if (m_bEnabled)
	{
		errCode = valueSet(propertyName, propValStr, eLocation);

		if (errCode == XM_RESULT_NOT_FOUND)
		{
			auto item = std::make_shared<PropertyListItem> ();
			item->NameSet(propertyName);
			item->TypeSet(propertyType);
			item->ValueSet(propValStr, eLocation);
			m_pdoc->ItemAdd(item);
			errCode = XM_RESULT_VALUE_CHANGED;
		}

		if (eLocation == Persistent && errCode == XM_RESULT_VALUE_CHANGED)
		{
			startSaveDataTimer();
		}
	}

	// unlock the PM
	unlock();

	return errCode;
}


//!
//\brief Copies a property from one property to another
//
int PropertyManager::copyProperty(
	const std::string &dstName,
	const std::string &srcName)
{
	int retVal = XM_RESULT_SUCCESS;
	std::string value;
	std::string dataType;

	//
	// Lookup the old value and create or update the new value with the same attributes.
	//
	retVal = valueGet(srcName, value, dataType, Persistent);

	if (retVal == XM_RESULT_SUCCESS)
	{
		retVal = setPropertyValue(dstName, dataType, value, Persistent);
	}

	return retVal;
}


/*!
* \brief Gets the first node/property from the table, with its values
*
* As part of the diagnostic tools functionality, this function returns the
* persistent and temporary values of the first property in the property table.
* It also returns a pointer to the IXML_Node it represents, so that the next
* property can be retrieved using getNextElementPropertyInformation().
*
* \param ePrimaryTable - specifies whether to get prop value first from Persistent and then Temporary, or vice versa
* \param pszPropertyName - name of the property
* \param pszPropertyType - the type of the property
* \param pszPropertyPrimaryValue - value of the property from the specified table
* \param pszPropertySecondaryValue - value of the property the opposite table
* \retval A pointer to this first node in the property table, NULL if none
*/
bool PropertyManager::getPropertyInformation(
	EStorageLocation ePrimaryTable,
	int nIndex,
	std::string &propertyName,
	std::string &propertyType,
	std::string &propertyPrimaryValue,
	std::string &propertySecondaryValue)
{
	auto item = std::static_pointer_cast<PropertyListItem> (m_pdoc->ItemGet(nIndex));

	if (item)
	{
		propertyName = item->NameGet();
		propertyType = item->TypeGet();
		item->StringGet(propertyPrimaryValue, ePrimaryTable);
		item->StringGet(propertySecondaryValue, !ePrimaryTable);

		return true;
	}

	return false;
}

/*!
* \brief Registers a callback function to watch a given piece of data for changes
*
* \param pszDataName - the name of the data to watch
* \param pfnCallBack - the function to call when it changes
* \param pCallbackParam - a parameter passed in the callback function
*/
void PropertyManager::registerPropertyChangeNotifyFunc (
	const char *pszDataName,
	PropertyChangeNotifyFuncPtr pfnCallBack,
	void *pCallbackParam)
{
	PropertyChangeCallbackInfo tCbInfo = {pfnCallBack, pCallbackParam};
	// NOT VS 2012 - m_oPropertyChangeCallBacks.insert (make_pair<string, PropertyChangeCallbackInfo>(pszDataName, tCbInfo));
	m_oPropertyChangeCallBacks.insert (make_pair(pszDataName, tCbInfo));
}


/*!
* \brief synchronously calls all the callbacks associated with a value name
*
* \param pszDataName - the name of the data which has changed
*/
void PropertyManager::callPropertyChangeNotifyFunc (
	const char *pszDataName)
{
	// define what our iterator looks like
	typedef multimap<string, PropertyChangeCallbackInfo>::const_iterator CIT;
	// get a range of iterators (all values with key pszDataName)
	pair<CIT, CIT> range = m_oPropertyChangeCallBacks.equal_range(pszDataName);
	// loop through that range, calling the callbacks
	for(auto it = range.first; it != range.second; ++it)
	{
		it->second.pfnFunction(pszDataName, it->second.pParam);
	}
}

XMLListItemSharedPtr PropertyManager::NodeToItemConvert(IXML_Node *pNode)
{
	const char *pName = ixmlNode_getNodeName(pNode);
	const char *pType = ixmlElement_getAttribute((IXML_Element *)pNode, (char *)ATT_TYPE);
	const auto *pValue = (const char *)ixmlNode_getNodeValue(ixmlNode_getFirstChild(pNode));

	if (pValue == nullptr)
	{
		pValue = "";
	}

	auto item = std::make_shared<PropertyListItem> ();
	item->NameSet(pName);
	item->TypeSet(pType);
	item->ValueSet(pValue, Persistent);

	return item;
}

void PropertyManager::each (const std::function<void(std::string, std::string)> &callback)
{
	m_pdoc->each(
		[callback](const XMLListItemSharedPtr &item)
		{
			auto property = std::static_pointer_cast<PropertyListItem> (item);

			auto name = property->NameGet();

			std::string value;
			item->StringGet (value, Temporary);

			callback(name, value);
		});
}
