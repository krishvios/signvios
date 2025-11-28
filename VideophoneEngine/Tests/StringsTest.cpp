
#include "StringsTest.h"

#include "stiBase64.h"

void StringsTests::Base64EncodeDecodeTest()
{
	// Empty case
	base64Tester("", "");

	// all cases of padding: 1, none, 2
	base64Tester("01234567",   "MDEyMzQ1Njc=");
	base64Tester("012345678",  "MDEyMzQ1Njc4");
	base64Tester("0123456789", "MDEyMzQ1Njc4OQ==");
}

void StringsTests::base64Tester(const std::string &decoded, const std::string &encoded)
{
	std::string test_encoded(vpe::base64Encode(decoded));
	std::string test_decoded(vpe::base64Decode(encoded));

	CPPUNIT_ASSERT_EQUAL(test_encoded, encoded);
	CPPUNIT_ASSERT_EQUAL(encoded.size(), test_encoded.size());

	CPPUNIT_ASSERT_EQUAL(test_decoded, decoded);
	CPPUNIT_ASSERT_EQUAL(decoded.size(), test_decoded.size());
}

