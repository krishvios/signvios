#pragma once

#include <cppunit/extensions/HelperMacros.h>

class SignalTests; // forward declaration
CPPUNIT_TEST_SUITE_REGISTRATION (SignalTests);

class SignalTests : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( SignalTests );
	CPPUNIT_TEST (ConnectionTest);
	CPPUNIT_TEST (ConnectionCollectionTest);
	CPPUNIT_TEST (DisconnectTest);
	CPPUNIT_TEST (ConnectionAssignmentTest);
	CPPUNIT_TEST (ConnectionAssignmentTest);
	CPPUNIT_TEST (ConnectionReassignmentTest);
	CPPUNIT_TEST (ConnectionReassignmentTest2);
	CPPUNIT_TEST_SUITE_END();

public:

	void ConnectionTest();
	void ConnectionCollectionTest();
	void DisconnectTest();
	void ConnectionAssignmentTest();
	void ConnectionReassignmentTest();
	void ConnectionReassignmentTest2();

private:
};
