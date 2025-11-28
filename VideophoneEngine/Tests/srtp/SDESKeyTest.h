#pragma once

#include <cppunit/extensions/HelperMacros.h>

#include "SDESKey.h"

class SDESKeyTests; // forward declaration
CPPUNIT_TEST_SUITE_REGISTRATION(SDESKeyTests);

class SDESKeyTests: public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE(SDESKeyTests);
	CPPUNIT_TEST(SDESKeyTest1);
	CPPUNIT_TEST(SDESKeyRandom);
	CPPUNIT_TEST(SDESKeyRoundtrip);
	CPPUNIT_TEST_SUITE_END();

public:

	void SDESKeyTest1();
	void SDESKeyRandom();
	void SDESKeyRoundtrip();

private:

	void keyRoundTrip(vpe::SDESKey::Suite suite);

};
