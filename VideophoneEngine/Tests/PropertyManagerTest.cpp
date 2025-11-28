// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "PropertyManager.h"
#include "stiError.h"

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Message.h>
#include <regex.h>
#include <sys/stat.h>
#include <sys/types.h>

static const char g_szTEST_PROP1[] = "TestProperty";
static const std::string g_TEST_PROP2 = "TestProperty2";

class PropertyManagerTest;


CPPUNIT_TEST_SUITE_REGISTRATION( PropertyManagerTest);


// This test fixture doesn't operate on existing tables
class PropertyManagerTest : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( PropertyManagerTest );
	CPPUNIT_TEST (NewPersistentStringSet);
	CPPUNIT_TEST (NewPersistentIntSet);
	CPPUNIT_TEST (NewPersistentInt64Set);
	CPPUNIT_TEST (NewPersistentOldTempStringSet);
	CPPUNIT_TEST (NewPersistentOldTempIntSet);
	CPPUNIT_TEST (StringGetSet);
	CPPUNIT_TEST (PeristentPropertyReplacesTemporaryString);
	CPPUNIT_TEST (PeristentPropertyReplacesTemporaryInt);
	CPPUNIT_TEST (PersistentGetOnTemporaryOnlyRemovesTemporaryInt);
	CPPUNIT_TEST (RemovePropertyRestoresDefaultString);
	CPPUNIT_TEST (RemovePropertyRestoresDefaultInt);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp();
	void tearDown();

	void StringGetSet();
	void NewPersistentStringSet ();
	void NewPersistentIntSet ();
	void NewPersistentInt64Set ();
	void NewPersistentOldTempStringSet ();
	void NewPersistentOldTempIntSet ();
	void PeristentPropertyReplacesTemporaryString ();
	void PeristentPropertyReplacesTemporaryInt ();
	void PersistentGetOnTemporaryOnlyRemovesTemporaryInt ();
	void RemovePropertyRestoresDefaultString ();
	void RemovePropertyRestoresDefaultInt ();
	
private:
};


#if 0
static void BackupFile (
	const char *pszFile)
{
	std::string temp = pszFile;
	temp += ".testbackup";
	
	rename (pszFile, temp.c_str());
}


static void RestoreFile (
	const char *pszFile)
{
	std::string temp = pszFile;
	temp += ".testbackup";
	
	rename (temp.c_str(), pszFile);
}
#endif


void PropertyManagerTest::setUp()
{
	//
	// Make sure paths are created
	//
	mkdir ("./data", 0700);
	mkdir ("./data/pm", 0700);

	unlink ("./data/pm/PropertyTable.xml");

	stiOSDynamicDataFolderSet ("./data/");
	stiOSStaticDataFolderSet (STATIC_DATA);

	WillowPM::PropertyManager::getInstance()->init("M.m.r.b");
	WillowPM::PropertyManager::getInstance ()->PersistentSaveSet (true);
}


void PropertyManagerTest::tearDown()
{
	///
	/// Restore the property table from the copy.
	///
	WillowPM::PropertyManager::getInstance ()->PersistentSaveSet (false);

	WillowPM::PropertyManager::getInstance ()->uninit ();
}


void PropertyManagerTest::StringGetSet()
{
	std::string value;

	WillowPM::PropertyManager::getInstance()->propertySet(g_szTEST_PROP1, "TestValue");

	WillowPM::PropertyManager::getInstance()->propertyGet(g_szTEST_PROP1, &value);

	CPPUNIT_ASSERT_MESSAGE("Property Manager get/set string.", value == "TestValue");
}


///\brief This test writes a new string to the property table and checks to see if it is
/// actually in the file.
///
void PropertyManagerTest::NewPersistentStringSet ()
{
	int nResult;
	std::string value;

	//
	// Make sure our test string is not already there.
	//
	nResult = WillowPM::PropertyManager::getInstance()->propertyGet("RandomTestString", &value,
																WillowPM::PropertyManager::Persistent);
	
	CPPUNIT_ASSERT_MESSAGE("Test property is already in the file.", nResult == WillowPM::PM_RESULT_NOT_FOUND);

	//
	// Save the value persistently and force a write.
	//
	WillowPM::PropertyManager::getInstance()->propertySet("RandomTestString", "TestValue",
																WillowPM::PropertyManager::Persistent);
	
	WillowPM::PropertyManager::getInstance ()->saveToPersistentStorage ();
	
	//
	// Cause the property manager to reload the file.
	//
	WillowPM::PropertyManager::getInstance ()->init ("M.m.r.b");
	
	//
	// Look for the string.
	//
	nResult = WillowPM::PropertyManager::getInstance()->propertyGet("RandomTestString", &value,
																WillowPM::PropertyManager::Persistent);
	
	CPPUNIT_ASSERT_MESSAGE("Property manager failed to write new property.", nResult != WillowPM::PM_RESULT_NOT_FOUND);

	CPPUNIT_ASSERT_MESSAGE("Property has incorrect value.", value == "TestValue");
}


///\brief This test writes a new string to the property table while there is an existing temporary
/// value and checks to see if it gets stored to the file.
///
void PropertyManagerTest::NewPersistentOldTempStringSet ()
{
	int nResult;
	std::string value;

	//
	// Make sure our test string is not already there.
	//
	nResult = WillowPM::PropertyManager::getInstance()->propertyGet("RandomTestString", &value,
																WillowPM::PropertyManager::Persistent);
	
	CPPUNIT_ASSERT_MESSAGE("Test property is already in the file.", nResult == WillowPM::PM_RESULT_NOT_FOUND);

	//
	// Save the value temporarily.
	//
	WillowPM::PropertyManager::getInstance()->propertySet("RandomTestString", "TestValue",
																WillowPM::PropertyManager::Temporary);
																
	//
	// Save the value persistently and force a write.
	//
	WillowPM::PropertyManager::getInstance()->propertySet("RandomTestString", "TestValue",
																WillowPM::PropertyManager::Persistent);
	
	WillowPM::PropertyManager::getInstance ()->saveToPersistentStorage ();
	
	//
	// Cause the property manager to reload the file.
	//
	WillowPM::PropertyManager::getInstance ()->init ("M.m.r.b");
	
	//
	// Look for the string.
	//
	nResult = WillowPM::PropertyManager::getInstance()->propertyGet("RandomTestString", &value,
																WillowPM::PropertyManager::Persistent);
	
	CPPUNIT_ASSERT_MESSAGE("Property manager failed to write new property.", nResult != WillowPM::PM_RESULT_NOT_FOUND);

	CPPUNIT_ASSERT_MESSAGE("Property has incorrect value.", value == "TestValue");
}


///\brief This test writes a new int to the property table and checks to see if it is
/// actually in the file.
///
void PropertyManagerTest::NewPersistentIntSet ()
{
	int nResult;
	int nValue;
	
	//
	// Make sure our test string is not already there.
	//
	nResult = WillowPM::PropertyManager::getInstance()->propertyGet("RandomTestInt", &nValue,
																WillowPM::PropertyManager::Persistent);
	
	CPPUNIT_ASSERT_MESSAGE("Test property is already in the file.", nResult == WillowPM::PM_RESULT_NOT_FOUND);

	//
	// Save the value persistently and force a write.
	//
	WillowPM::PropertyManager::getInstance()->propertySet("RandomTestInt", 0,
																WillowPM::PropertyManager::Persistent);
	
	WillowPM::PropertyManager::getInstance ()->saveToPersistentStorage ();
	
	WillowPM::PropertyManager::getInstance ()->init ("M.m.r.b");

	//
	// Look for the property.
	//
	nResult = WillowPM::PropertyManager::getInstance()->propertyGet("RandomTestInt", &nValue,
																WillowPM::PropertyManager::Persistent);
	
	CPPUNIT_ASSERT_MESSAGE("Property manager failed to write new property.", nResult != WillowPM::PM_RESULT_NOT_FOUND);

	CPPUNIT_ASSERT_MESSAGE("Property has incorrect value.", nValue == 0);
}

///\brief This test writes a new int to the property table and checks to see if it is
/// actually in the file.
///
void PropertyManagerTest::NewPersistentInt64Set ()
{
	int nResult;
	int64_t nValue;

	//
	// Make sure our test string is not already there.
	//
	nResult = WillowPM::PropertyManager::getInstance()->propertyGet("RandomTestInt64", &nValue,
		WillowPM::PropertyManager::Persistent);

	CPPUNIT_ASSERT_MESSAGE("Test property is already in the file.", nResult == WillowPM::PM_RESULT_NOT_FOUND);

	//
	// Save the value persistently and force a write.
	//
	int64_t value64 = 34359738368;
	WillowPM::PropertyManager::getInstance()->propertySet("RandomTestInt64", value64,
		WillowPM::PropertyManager::Persistent);

	WillowPM::PropertyManager::getInstance ()->saveToPersistentStorage ();

	WillowPM::PropertyManager::getInstance ()->init ("M.m.r.b");

	//
	// Look for the property.
	//
	nResult = WillowPM::PropertyManager::getInstance()->propertyGet("RandomTestInt64", &nValue,
		WillowPM::PropertyManager::Persistent);

	CPPUNIT_ASSERT_MESSAGE("Property manager failed to write new property.", nResult != WillowPM::PM_RESULT_NOT_FOUND);

	CPPUNIT_ASSERT_MESSAGE("Property has incorrect value.", nValue == value64);
}


///\brief This test writes a new int to the property table when a temporary already exists
/// and checks to see if it is actually gets stored to the file.
///
void PropertyManagerTest::NewPersistentOldTempIntSet ()
{
	int nResult;
	int nValue;
	
	//
	// Make sure our test string is not already there.
	//
	nResult = WillowPM::PropertyManager::getInstance()->propertyGet("RandomTestInt", &nValue,
																WillowPM::PropertyManager::Persistent);
	
	CPPUNIT_ASSERT_MESSAGE("Test property is already in the file.", nResult == WillowPM::PM_RESULT_NOT_FOUND);

	//
	// Save the value temporarily.
	//
	WillowPM::PropertyManager::getInstance()->propertySet("RandomTestInt", 0,
																WillowPM::PropertyManager::Temporary);
	
	//
	// Save the value persistently and force a write.
	//
	WillowPM::PropertyManager::getInstance()->propertySet("RandomTestInt", 0,
																WillowPM::PropertyManager::Persistent);
	
	WillowPM::PropertyManager::getInstance ()->saveToPersistentStorage ();
	
	WillowPM::PropertyManager::getInstance ()->init ("M.m.r.b");

	//
	// Look for the property.
	//
	nResult = WillowPM::PropertyManager::getInstance()->propertyGet("RandomTestInt", &nValue,
																WillowPM::PropertyManager::Persistent);
	
	CPPUNIT_ASSERT_MESSAGE("Property manager failed to write new property.", nResult != WillowPM::PM_RESULT_NOT_FOUND);

	CPPUNIT_ASSERT_MESSAGE("Property has incorrect value.", nValue == 0);
}


///\brief This test verifies that when a property is written persistently that it replaces
/// the temporary value.  Also tests that when a persistent value is retrieved it removes
/// the temporary value.
///
void PropertyManagerTest::PeristentPropertyReplacesTemporaryString ()
{
	std::string value;

	//
	// Write the persistent property value.
	//
	WillowPM::PropertyManager::getInstance()->propertySet(g_szTEST_PROP1, "PValue1",
																WillowPM::PropertyManager::Persistent);
	
	//
	// Write the temporary property value.
	//
	WillowPM::PropertyManager::getInstance()->propertySet(g_szTEST_PROP1, "TValue1",
																WillowPM::PropertyManager::Temporary);
	
	//
	// Get the value and make sure it is the last temporary value.
	//
	WillowPM::PropertyManager::getInstance()->propertyGet(g_szTEST_PROP1, &value,
																WillowPM::PropertyManager::Temporary);

	CPPUNIT_ASSERT_MESSAGE("Temporary value was not saved.", value == "TValue1");

	//
	// Write the persistent property value.
	//
	WillowPM::PropertyManager::getInstance()->propertySet(g_szTEST_PROP1, "PValue2",
																WillowPM::PropertyManager::Persistent);

	//
	// Get the value and make sure it is the last persistent value.
	//
	WillowPM::PropertyManager::getInstance()->propertyGet(g_szTEST_PROP1, &value,
																WillowPM::PropertyManager::Temporary);

	CPPUNIT_ASSERT_MESSAGE("Temporary value was not replaced.", value == "PValue2");

	//
	// Write another temporary property value.
	//
	WillowPM::PropertyManager::getInstance()->propertySet(g_szTEST_PROP1, "TValue2",
																WillowPM::PropertyManager::Temporary);

	//
	// Get the value and make sure it is the last temporary value.
	//
	WillowPM::PropertyManager::getInstance()->propertyGet(g_szTEST_PROP1, &value,
																WillowPM::PropertyManager::Temporary);

	CPPUNIT_ASSERT_MESSAGE("Temporary value was not saved.", value == "TValue2");

	//
	// Get the value and make sure it is the last persistent value.
	//
	WillowPM::PropertyManager::getInstance()->propertyGet(g_szTEST_PROP1, &value,
																WillowPM::PropertyManager::Persistent);

	CPPUNIT_ASSERT_MESSAGE("Peristent value has incorrect value.", value == "PValue2");

	//
	// Get the value as a temporary and make sure it is the last persistent value.
	//
	WillowPM::PropertyManager::getInstance()->propertyGet(g_szTEST_PROP1, &value,
																WillowPM::PropertyManager::Temporary);

	CPPUNIT_ASSERT_MESSAGE("Temporary value was not replaced after getting the persistent value.", value == "PValue2");
}


///\brief This test verifies that when a property is written persistently that it replaces
/// the temporary value.  Also tests that when a persistent value is retrieved it removes
/// the temporary value.
///
void PropertyManagerTest::PeristentPropertyReplacesTemporaryInt ()
{
	int nValue;

	//
	// Write the persistent property value.
	//
	WillowPM::PropertyManager::getInstance()->propertySet(g_szTEST_PROP1, 1, WillowPM::PropertyManager::Persistent);
	
	//
	// Write the temporary property value.
	//
	WillowPM::PropertyManager::getInstance()->propertySet(g_szTEST_PROP1, 2, WillowPM::PropertyManager::Temporary);
	
	//
	// Get the value and make sure it is the last temporary value.
	//
	WillowPM::PropertyManager::getInstance()->propertyGet(g_szTEST_PROP1, &nValue,
															 WillowPM::PropertyManager::Temporary);

	CPPUNIT_ASSERT_MESSAGE("Temporary value was not saved.", nValue == 2);

	//
	// Write the persistent property value.
	//
	WillowPM::PropertyManager::getInstance()->propertySet(g_szTEST_PROP1, 3,
															 WillowPM::PropertyManager::Persistent);

	//
	// Get the value and make sure it is the last persistent value.
	//
	WillowPM::PropertyManager::getInstance()->propertyGet(g_szTEST_PROP1, &nValue,
															 WillowPM::PropertyManager::Temporary);

	CPPUNIT_ASSERT_MESSAGE("Temporary value was not replaced.", nValue == 3);

	//
	// Write another temporary property value.
	//
	WillowPM::PropertyManager::getInstance()->propertySet(g_szTEST_PROP1, 4,
															 WillowPM::PropertyManager::Temporary);

	//
	// Get the value and make sure it is the last temporary value.
	//
	WillowPM::PropertyManager::getInstance()->propertyGet(g_szTEST_PROP1, &nValue,
															 WillowPM::PropertyManager::Temporary);

	CPPUNIT_ASSERT_MESSAGE("Temporary value was not saved.", nValue == 4);

	//
	// Get the value and make sure it is the last persistent value.
	//
	WillowPM::PropertyManager::getInstance()->propertyGet(g_szTEST_PROP1, &nValue,
															 WillowPM::PropertyManager::Persistent);

	CPPUNIT_ASSERT_MESSAGE("Peristent value has incorrect value.", nValue == 3);

	//
	// Get the value as a temporary and make sure it is the last persistent value.
	//
	WillowPM::PropertyManager::getInstance()->propertyGet(g_szTEST_PROP1, &nValue,
															 WillowPM::PropertyManager::Temporary);

	CPPUNIT_ASSERT_MESSAGE("Temporary value was not replaced after getting the persistent value.",
						   nValue == 3);

}


void PropertyManagerTest::PersistentGetOnTemporaryOnlyRemovesTemporaryInt ()
{
	int nValue;
	int nResultCode;
	WillowPM::PropertyManager *pPM = WillowPM::PropertyManager::getInstance ();
	std::string Type;
	std::string Value;

	//
	// First check to see if there is already a persistent value.  If there is, then fail the
	// test because we cannot proceed with the real test.
	//
	nResultCode = pPM->propertyGet(g_szTEST_PROP1, &nValue, WillowPM::PropertyManager::Persistent);
														
	CPPUNIT_ASSERT_MESSAGE ("Persistent value already in table.",
							nResultCode == WillowPM::PM_RESULT_NOT_FOUND);

	//
	// Set a temporary property value.
	//
	pPM->propertySet(g_szTEST_PROP1, 1, WillowPM::PropertyManager::Temporary);

	//
	// Now get a persistent value.  It should return as "not found".
	//
	nResultCode = pPM->propertyGet(g_szTEST_PROP1, &nValue, WillowPM::PropertyManager::Persistent);
															 
	CPPUNIT_ASSERT_MESSAGE ("Persistent value found in table.",
							nResultCode == WillowPM::PM_RESULT_NOT_FOUND);
							
	//
	// Try using getPropertyValue.  It should find the value and return the default.
	//
	nResultCode = pPM->getPropertyValue(g_szTEST_PROP1, &Type, &Value, 
										WillowPM::PropertyManager::Persistent);
															 
	CPPUNIT_ASSERT_MESSAGE ("Persistent value found in table.",
							nResultCode == WillowPM::PM_RESULT_SUCCESS);
							
	//
	// Now get the temporary value.  It should come back as not found because
	// getting the persistent value should have removed the non-persistent value.
	//
	nResultCode = pPM->propertyGet(g_szTEST_PROP1, &nValue, WillowPM::PropertyManager::Temporary);

	CPPUNIT_ASSERT_MESSAGE ("Temporary value found in table.",
							nResultCode == WillowPM::PM_RESULT_NOT_FOUND);
							
	//
	// Try using getPropertyValue.  It should find the value and return the default.
	//
	nResultCode = pPM->getPropertyValue(g_szTEST_PROP1, &Type, &Value, 
										WillowPM::PropertyManager::Temporary);
															 
	CPPUNIT_ASSERT_MESSAGE ("Temporary value found in table.",
							nResultCode == WillowPM::PM_RESULT_SUCCESS);
}

void PropertyManagerTest::RemovePropertyRestoresDefaultString ()
{
	int nResultCode;
	WillowPM::PropertyManager *pPM = WillowPM::PropertyManager::getInstance ();
	std::string value;
	const std::string testDefaultValue{"testDefaultValue"};
	const std::string testValue{"testValue"};


	//
	// First remove the property from the property table just to make sure it isn't already there
	//
	pPM->removeProperty(g_TEST_PROP2);

	//
	// Set a default value
	//
	pPM->propertyDefaultSet (g_TEST_PROP2, testDefaultValue);

	//
	// Retrieve the property to make sure we get the default value.
	// The result code should be "not found".
	//
	nResultCode = pPM->propertyGet (g_TEST_PROP2, &value);
	CPPUNIT_ASSERT_MESSAGE ("Result code was not PM_RESULT_NOT_FOUND", nResultCode == WillowPM::PM_RESULT_NOT_FOUND);
	CPPUNIT_ASSERT_MESSAGE (std::string{"String was unexpected value: "} + value, value == testDefaultValue);

	//
	// Set a value and make sure we get the value and not the default
	//
	nResultCode = pPM->propertySet (g_TEST_PROP2, testValue);
	CPPUNIT_ASSERT_MESSAGE (std::string{"Error setting property value:"} + std::to_string(nResultCode), nResultCode == WillowPM::PM_RESULT_SUCCESS);

	nResultCode = pPM->propertyGet (g_TEST_PROP2, &value);
	CPPUNIT_ASSERT_MESSAGE (std::string{"Error getting property value:"} + std::to_string(nResultCode), nResultCode == WillowPM::PM_RESULT_SUCCESS);
	CPPUNIT_ASSERT_MESSAGE (std::string{"String was unexpected value: "} + value, value == testValue);

	//
	// Now remove the property and retreive the value.
	// The value should match the previously set default value.
	// When getting the property, the result code should be "not found".
	//
	nResultCode = pPM->removeProperty(g_TEST_PROP2);

	nResultCode = pPM->propertyGet (g_TEST_PROP2, &value);
	CPPUNIT_ASSERT_MESSAGE (std::string{"Error getting property value:"} + std::to_string(nResultCode), nResultCode == WillowPM::PM_RESULT_NOT_FOUND);
	CPPUNIT_ASSERT_MESSAGE (std::string{"String was unexpected value: "} + value, value == testDefaultValue);
}

void PropertyManagerTest::RemovePropertyRestoresDefaultInt ()
{
	int nResultCode;
	WillowPM::PropertyManager *pPM = WillowPM::PropertyManager::getInstance ();
	int value;
	const int testDefaultValue{100};
	const int testValue{200};


	//
	// First remove the property from the property table just to make sure it isn't already there
	//
	pPM->removeProperty(g_TEST_PROP2);

	//
	// Set a default value
	//
	pPM->propertyDefaultSet (g_TEST_PROP2, testDefaultValue);

	//
	// Retrieve the property to make sure we get the default value.
	// The result code should be "not found".
	//
	nResultCode = pPM->propertyGet (g_TEST_PROP2, &value);
	CPPUNIT_ASSERT_MESSAGE ("Result code was not PM_RESULT_NOT_FOUND", nResultCode == WillowPM::PM_RESULT_NOT_FOUND);
	CPPUNIT_ASSERT_MESSAGE (std::string{"Int was unexpected value: "} + std::to_string(value), value == testDefaultValue);

	//
	// Set a value and make sure we get the value and not the default
	//
	nResultCode = pPM->propertySet (g_TEST_PROP2, testValue);
	CPPUNIT_ASSERT_MESSAGE (std::string{"Error setting property value:"} + std::to_string(nResultCode), nResultCode == WillowPM::PM_RESULT_SUCCESS);

	nResultCode = pPM->propertyGet (g_TEST_PROP2, &value);
	CPPUNIT_ASSERT_MESSAGE (std::string{"Error getting property value:"} + std::to_string(nResultCode), nResultCode == WillowPM::PM_RESULT_SUCCESS);
	CPPUNIT_ASSERT_MESSAGE (std::string{"String was unexpected value: "} + std::to_string(value), value == testValue);

	//
	// Now remove the property and retreive the value.
	// The value should match the previously set default value.
	// When getting the property, the result code should be "not found".
	//
	nResultCode = pPM->removeProperty(g_TEST_PROP2);

	nResultCode = pPM->propertyGet (g_TEST_PROP2, &value);
	CPPUNIT_ASSERT_MESSAGE (std::string{"Error getting property value:"} + std::to_string(nResultCode), nResultCode == WillowPM::PM_RESULT_NOT_FOUND);
	CPPUNIT_ASSERT_MESSAGE (std::string{"String was unexpected value: "} + std::to_string(value), value == testDefaultValue);
}
