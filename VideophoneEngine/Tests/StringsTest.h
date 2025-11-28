#pragma once

#include <cppunit/extensions/HelperMacros.h>

class StringsTests; // forward declaration
CPPUNIT_TEST_SUITE_REGISTRATION (StringsTests);

class StringsTests : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( StringsTests );
	CPPUNIT_TEST (Base64EncodeDecodeTest);
	CPPUNIT_TEST_SUITE_END();

public:

	void Base64EncodeDecodeTest();

private:

	static void base64Tester(const std::string &decoded, const std::string &encoded);
};
