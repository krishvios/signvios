
#include "SDESKeyTest.h"

void SDESKeyTests::SDESKeyTest1()
{
	vpe::SDESKey key1(vpe::SDESKey::Suite::AES_CM_128_HMAC_SHA1_80);
	// NOTE: not using CPPUNIT_ASSERT_EQUALS because the types must match,
	// and the size_t types are platform dependent
	CPPUNIT_ASSERT(key1.base64KeyParamsGet().size() == 40);
	std::cout << key1.base64KeyParamsGet() << '\n';

	vpe::SDESKey key2(vpe::SDESKey::Suite::AES_256_CM_HMAC_SHA1_80);
	CPPUNIT_ASSERT(key2.base64KeyParamsGet().size() == 64);
	std::cout << key2.base64KeyParamsGet() << '\n';
}

void SDESKeyTests::SDESKeyRandom()
{
	// Make sure the keys are really random
	// (ie: we don't generate the same keys over and over)

	vpe::SDESKey key1(vpe::SDESKey::Suite::AES_CM_128_HMAC_SHA1_80);
	vpe::SDESKey key2(vpe::SDESKey::Suite::AES_CM_128_HMAC_SHA1_80);

	CPPUNIT_ASSERT(key1.base64KeyParamsGet() != key2.base64KeyParamsGet());
}

void SDESKeyTests::keyRoundTrip(vpe::SDESKey::Suite suite)
{
	vpe::SDESKey key1(suite);
	vpe::SDESKey key2;

	key2 = vpe::SDESKey::decode(vpe::SDESKey::toString(key1.suiteGet()), key1.base64KeyParamsGet());

	CPPUNIT_ASSERT_EQUAL(key1, key2);
}

void SDESKeyTests::SDESKeyRoundtrip()
{
	keyRoundTrip(vpe::SDESKey::Suite::AES_CM_128_HMAC_SHA1_32);
	keyRoundTrip(vpe::SDESKey::Suite::AES_256_CM_HMAC_SHA1_32);
}
