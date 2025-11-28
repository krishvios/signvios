#pragma once

#include <cppunit/extensions/HelperMacros.h>

#include "SDESKey.h"

class HMACSHA1Tests; // forward declaration
CPPUNIT_TEST_SUITE_REGISTRATION(HMACSHA1Tests);

class HMACSHA1Tests: public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE(HMACSHA1Tests);
	CPPUNIT_TEST(Test1);
	CPPUNIT_TEST_SUITE_END();

public:

	void Test1();

private:

	static void doTest(const std::string &key, const std::string &input, const std::string &expectedHex);

};
