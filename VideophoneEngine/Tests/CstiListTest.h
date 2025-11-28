#pragma once

#include <cppunit/extensions/HelperMacros.h>

class CstiListTests; // forward declaration
CPPUNIT_TEST_SUITE_REGISTRATION (CstiListTests);

class CstiListTests : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE(CstiListTests);
	CPPUNIT_TEST (SizeWithoutListTest);
	CPPUNIT_TEST_SUITE_END();

public:

	void SizeWithoutListTest ();

private:

};
