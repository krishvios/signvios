
#include "HMACSHA1Test.h"

#include "CstiAesEncryptionContext.h"
#include "stiBase64.h"

void HMACSHA1Tests::Test1()
{
	doTest("Jefe", "what do ya want for nothing?", "effcdf6ae5eb2fa2d27416d5f184df9c259a7c79");
}

void HMACSHA1Tests::doTest(const std::string &key, const std::string &input, const std::string &expectedHex)
{
	std::string output;
	output.resize(20); // is this a fixed size? srtp uses 10... ?

	ShaStatus status = Sha1Calculate(
		(RvUint8*)&key[0], key.size(),
		(RvUint8*)&input[0], input.size(),
		(RvUint8*)"", 0,
		(RvUint8*)&output[0], output.size());

	std::cout << "hmac sha1Test status: " << status << '\n';

	std::string outputHex = vpe::hexEncode(output);

	std::cout << "Expected: " << expectedHex << '\n';
	std::cout << "Actual: " << outputHex << '\n';

	CPPUNIT_ASSERT_EQUAL(outputHex, expectedHex);
}
