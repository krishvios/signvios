// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "PropertyManager.h"
#include "stiError.h"

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Message.h>

#include <boost/filesystem.hpp>

class PropertyManagerReadTest;

CPPUNIT_TEST_SUITE_REGISTRATION(PropertyManagerReadTest);

// TODO: this file is also defined in another test.  Figure out how to share this
const std::string TestPropertyTable("./PropertyTables/TestPropertyTable.xml");
const std::string LocalPropertyTable("./data/pm/PropertyTable.xml");


class PropertyManagerReadTest : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE (PropertyManagerReadTest);
	CPPUNIT_TEST (Int32Read);
	CPPUNIT_TEST (TypeRead);
	CPPUNIT_TEST_SUITE_END ();

public:
	void setUp ();
	void tearDown ();

	void Int32Read ();
	void TypeRead ();
private:

};


void PropertyManagerReadTest::setUp ()
{
	//
	// Make sure paths are created
	//
	boost::system::error_code ec;
	boost::filesystem::create_directories("./data/pm", ec);

	// Copy test file into place
	boost::filesystem::copy_file(TestPropertyTable, LocalPropertyTable,
		boost::filesystem::copy_option::overwrite_if_exists, ec);

	stiOSDynamicDataFolderSet ("./data/");
	stiOSStaticDataFolderSet (STATIC_DATA);

	WillowPM::PropertyManager::getInstance()->init("M.m.r.b");
	WillowPM::PropertyManager::getInstance ()->PersistentSaveSet (true);
}


void PropertyManagerReadTest::tearDown ()
{
	///
	/// Restore the property table from the copy.
	///
	WillowPM::PropertyManager::getInstance ()->PersistentSaveSet (false);

	WillowPM::PropertyManager::getInstance ()->uninit ();

	boost::system::error_code ec;
	boost::filesystem::remove(LocalPropertyTable, ec);
}


void PropertyManagerReadTest::Int32Read ()
{
	int result;
	int value;

	result = WillowPM::PropertyManager::getInstance()->propertyGet("BlockListEnabled", &value,
		WillowPM::PropertyManager::Temporary);

	CPPUNIT_ASSERT_MESSAGE("32bit read failed", 0 == result);
	CPPUNIT_ASSERT_MESSAGE("32bit integer read test failed", 1 == value);
}

void PropertyManagerReadTest::TypeRead ()
{
	// These properties are known to exist in the test property table
	std::map<std::string, std::string> propertyTypes = {
		{ "cmSignMailEnabled",      "int" },
		{ "SipAgentDomainOverride", "string" },
		{ "cmLocalInterfaceMode",   "enum" },
	};

	for (const auto &key_value : propertyTypes)
	{
		// Validate types from the property manager
		std::string type;
		std::string value;

		WillowPM::PropertyManager::getInstance()->getPropertyValue(key_value.first, &type, &value, WillowPM::PropertyManager::Temporary);

		CPPUNIT_ASSERT_EQUAL(key_value.second, type);
	}
}
