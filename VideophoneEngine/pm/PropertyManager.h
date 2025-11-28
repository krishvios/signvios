
/*!
*  \file PropertyManager.h
*  \brief The Property Manager
*
*  Class declaration for the Willow/Redwood Property Manager.  Provides storage
*  and access for persitent (both local and remote) and temporary data.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*/

#ifndef PROPERTYMANAGER_H
#define PROPERTYMANAGER_H

#include "ixml.h"
#include "XMLManager.h"
#include <string>
#include <map>
#include "PropertyListItem.h"
#include "stiError.h"

class CstiCoreService;
class CstiCoreResponse;
class CstiPropertySender;

// for Profiles CallLists, and other remote properties

namespace WillowPM
{
class PropertyListItem;

// General Defines for use with PropertyManager
#define pmFIRMWARE_UPDATED "pmFirmwareUpdated"

// Keep the old "PM_" names so we don't break the old code
const int PM_RESULT_SUCCESS = XM_RESULT_SUCCESS ;       //!< no error
const int PM_RESULT_NOT_FOUND = XM_RESULT_NOT_FOUND;       //!< property not found in the PropertyManager
const int PM_RESULT_TYPE_MISMATCH = XM_RESULT_TYPE_MISMATCH;       //!< existing type doesn't match the request
const int PM_RESULT_IXML_ERROR = XM_RESULT_IXML_ERROR;       //!< problem reading/writing/accessing underlying XML
const int PM_RESULT_VALUE_CHANGED = XM_RESULT_VALUE_CHANGED;      //!< internal message: this property already in table

typedef void (stiSTDCALL *PropertyChangeNotifyFuncPtr)(
	const char *pszProperty,
	void *pCallbackParam);

typedef struct
{
	PropertyChangeNotifyFuncPtr pfnFunction;
	void *pParam;
} PropertyChangeCallbackInfo;

const int PM_MAX_PROPERTY_STRING_LENGTH = 255;

extern const char LOCAL_PERSIST_XML[];
extern const char LOCAL_PERSIST_BACKUP_XML[];

/*!
*  \brief Property Manager Class
*
*  Outlines the public (and private) APIs and member variables
*/

class PropertyManager : public XMLManager
{
public:

	void init(
		const std::string &applicationVersion);

	void uninit () override;
	
	~PropertyManager() override = default;
	PropertyManager (const PropertyManager &other) = delete;
	PropertyManager (PropertyManager &&other) = delete;
	PropertyManager &operator= (const PropertyManager &other) = delete;
	PropertyManager &operator= (PropertyManager &&other) = delete;

	static PropertyManager* getInstance();

	int PropertyTableBackup () { return backup(); }

	void CoreServiceSet (
		CstiCoreService *pCoreService);
		
	void PropertySenderEnable (EstiBool bUsePropertySender);

	enum EStorageLocation
	{
		Persistent = PropertyListItem::eLOC_PERSISTENT,
		Temporary = PropertyListItem::eLOC_TEMPORARY
	};

	int propertyGet (
		const std::string &propertyName,
		std::string *value,
		EStorageLocation eLocation = Temporary);

	int propertySet (
		const std::string &propertyName,
		const std::string &value,
		EStorageLocation eLocation = Temporary);

	void propertyDefaultSet (
		const std::string &propertyName,
		const std::string &value);

	template<typename T,
		typename = typename std::enable_if<
		std::is_enum<T>::value ||
		std::is_integral<T>::value>::type>
	int propertyGet (
		const std::string &propertyName,
		T *value,
		EStorageLocation eLocation = Temporary)
	{
		int64_t intValue;

		auto result = getPropertyInt (propertyName, &intValue, eLocation);

		if (result == PM_RESULT_SUCCESS || result == PM_RESULT_NOT_FOUND)
		{
			*value = static_cast<T>(intValue);

			stiASSERTMSG(intValue == static_cast<int64_t>(*value),
				"Precision lost when retrieving property value: %d %d\n", intValue, *value);
		}

		return result;
	}

	int propertyGet (
		const std::string &propertyName,
		bool *value,
		EStorageLocation eLocation = Temporary);


	template<typename T,
		typename = typename std::enable_if<
		std::is_enum<T>::value ||
		std::is_same<T, bool>::value ||
		std::is_integral<T>::value>::type>
	int propertySet (
		const std::string &propertyName,
		const T &value,
		EStorageLocation eLocation = Temporary)
	{
		return setPropertyInt (propertyName, static_cast<int64_t>(value), eLocation);
	}

	template<typename T,
		typename = typename std::enable_if<
		std::is_enum<T>::value ||
		std::is_same<T, bool>::value ||
		std::is_integral<T>::value>::type>
	void propertyDefaultSet (
		const std::string &propertyName,
		const T &value)
	{
		return setPropertyIntDefault (propertyName, static_cast<int64_t>(value));
	}


	int getPropertyInt(
		const std::string &propertyName,
		int *value,
		EStorageLocation eLocation = Temporary);

	int getPropertyInt(
		const std::string &propertyName,
		int *value,
		int defaultValue,
		EStorageLocation eLocation = Temporary);

	int getPropertyInt(
		const std::string &propertyName,
		int64_t *value,
		EStorageLocation eLocation = Temporary);

	int setPropertyInt(
		const std::string &propertyName,
		int64_t value,
		EStorageLocation eLocation = Temporary);

	void setPropertyIntDefault(
		const std::string &propertyName,
		int64_t value);

	int getPropertyString(
		const std::string &propertyName,
		std::string *propertyValue,
		EStorageLocation eLocation = Temporary);

	int getPropertyString(
		const std::string &propertyName,
		std::string *propertyValue,
		const std::string &defaultVal,
		EStorageLocation eLocation = Temporary);

	int setPropertyString(
		const std::string &propertyName,
		const std::string &value,
		EStorageLocation eLocation = Temporary);

	void setPropertyStringDefault(
		const std::string &propertyName,
		const std::string &value);

	int removeProperty(
		const std::string &propertyName,
		bool fromAll = true);

	int getPropertyValue(
		const std::string &propertyName,
		std::string *propertyType,
		std::string *value,
		EStorageLocation eLocation);

	int copyProperty(
		const std::string &dstName,
		const std::string &srcName);

	// Internal QA/diagnostics tools functions
	//
	bool getPropertyInformation(
		EStorageLocation ePrimaryTable,
		int nIndex,
		std::string &propertyName,
		std::string &propertyType,
		std::string &propertyPrimaryValue,
		std::string &propertySecondaryValue);

	void registerPropertyChangeNotifyFunc (
		const char *pszDataName,
		PropertyChangeNotifyFuncPtr pfnCallBack,
		void *pCallbackParam);

	void PropertySend (
		const std::string& name,
		int nType);
	
	//
	// This function forces all of the properties to be sent that are waiting to be sent.
	//
	void SendProperties();
	
	/// Block until any pending property send requests finish, for a maximum of timeoutMilliseconds milliseconds
	void PropertySendWait(int timeoutMilliseconds);

	void ResponseCheck(
		const CstiCoreResponse *poResponse);

	void Disable();
	void Enable();

	void each (const std::function<void(std::string, std::string)> &callback);

private:

	using XMLManager::init;

	/**
	*  \brief protected constructor for singleton purposes
	*/
	PropertyManager();

	CstiPropertySender *m_pPropertySender{nullptr};

	virtual int setPropertyValue(
		const std::string &propertyName,
		const std::string &propertyType,
		const std::string &propVal,
		EStorageLocation eLocation);

	bool firmwareUpdate (
		const std::string &applicationVersion);

	void updateProperties ();

	bool versionsMatch (
		const std::string &applicationVersion);

	std::multimap<std::string, PropertyChangeCallbackInfo> m_oPropertyChangeCallBacks;

	void callPropertyChangeNotifyFunc (
		const char *pszDataName);

	XMLListItemSharedPtr NodeToItemConvert(IXML_Node *pNode) override;

	EstiBool m_bUsePropertySender{estiTRUE};

	EstiBool m_bEnabled{estiTRUE};

	bool m_bInitialized{false};

}; // end PropertyManager class definition

}  // end namespace declaration

#endif // PROPERTYMANAGER_H
