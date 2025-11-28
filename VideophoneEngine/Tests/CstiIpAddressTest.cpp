// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "CstiIpAddress.h"

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Message.h>
#include <sys/time.h>

class CstiIpAddressTest;


CPPUNIT_TEST_SUITE_REGISTRATION (CstiIpAddressTest);


class CstiIpAddressTest : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CstiIpAddressTest );
	CPPUNIT_TEST (Comparison);
	CPPUNIT_TEST (ValidateIpv6);
	CPPUNIT_TEST (ValidateIpv4);
	CPPUNIT_TEST (FormatIpv6);
	CPPUNIT_TEST (FormatIpv4);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp();
	void tearDown();

	void Comparison();
	void ValidateIpv6();
	void ValidateIpv4();
	void Mask();
	void FormatIpv6();
	void FormatIpv4();

private:
};


void CstiIpAddressTest::setUp ()
{
}

void CstiIpAddressTest::tearDown ()
{
}

void CstiIpAddressTest::ValidateIpv4 ()
{
	CstiIpAddress parser;
	
	//
	// Test valid IP addresses.
	//
	static const char *ValidStrings[] =
	{
		"0.0.0.0",
		"0.0.0.0:1",
		"255.255.255.255:65535",
		"255.255.255.255"
	};
	const int nNumValidStrings = sizeof(ValidStrings) / sizeof(ValidStrings[0]);

	char szBuffer[512];
	
	for (int i = 0; i < nNumValidStrings; i++)
	{
		stiHResult hResult = parser.assign(ValidStrings[i]);
		if (stiIS_ERROR (hResult))
		{
			sprintf (szBuffer, "Testing valid ip address %d = \"%s\"\n", i, ValidStrings[i]);
			CPPUNIT_ASSERT_MESSAGE(szBuffer, !stiIS_ERROR (hResult));
		}
	}

	//
	// Test invalid IP addresses.
	//
	static const char *InvalidStrings[] =
	{
		"-1.0.0.0:0",
		"0.-1.0.0:0",
		"0.0.-1.0:0",
		"0.0.0.-1:0",
		"0.0.0.0:-1",
		"256.255.255.255:65535",
		"255.256.255.255:65535",
		"255.255.256.255:65535",
		"255.255.255.256:65535",
		"255.255.255.255:65536",
		"",
		nullptr,
		"a.b.c.d.e:f",
		"garbage",
		"1",
		"1.2",
		"1.2.3",
		"1.2.3.4:",
		"1.2.3.4.",
		"1.2.3.4.5",
		"1.2.3.4.5:128",
		"1.2.3.5.6.7.8.9.10.11.12.13.14.15:128",
		".",
		"..",
		"...",
		"....",
		"....."
	};
	const int nNumInvalidStrings = sizeof(InvalidStrings) / sizeof(InvalidStrings[0]);

	for (int i = 0; i < nNumInvalidStrings; i++)
	{
		stiHResult hResult = parser.assign(InvalidStrings[i]);
		
		if (!stiIS_ERROR (hResult))
		{
			sprintf (szBuffer, "Testing invalid ip address %d = \"%s\"\n", i, InvalidStrings[i]);
			CPPUNIT_ASSERT_MESSAGE (szBuffer, stiIS_ERROR (hResult));
		}
	}
}

void CstiIpAddressTest::ValidateIpv6 ()
{
	CstiIpAddress parser;
	
	//
	// Test valid IP addresses.
	//
	static const char *ValidStrings[] =
	{
		"::1",
		"1::",
		"ffff::",
		"[::1]:65535",
		"aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa",
		"[aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa]:65535",
		"aaaa::aaaa",
		"1::a1",
		"[aaaa::aaaa]",
		"[aaaa::aaaa]:65535"
	};
	const int nNumValidStrings = sizeof(ValidStrings) / sizeof(ValidStrings[0]);

	char szBuffer[256];
	
	for (int i = 0; i < nNumValidStrings; i++)
	{
		stiHResult hResult = parser.assign (ValidStrings[i]);
		
		if (stiIS_ERROR (hResult))
		{
			sprintf (szBuffer, "Testing valid ip address %d = \"%s\"\n", i, ValidStrings[i]);
			CPPUNIT_ASSERT_MESSAGE(szBuffer, !stiIS_ERROR(hResult));
		}
	}

	//
	// Test invalid IP addresses.
	//
	static const char *InvalidStrings[] =
	{
		"",
		nullptr,
		"garbage",
		"-1::-1",
		"aaaa::1.5",
		"aaaa::aaaa::aaaa",
		"aaaa::aaaa:65535",
		"[aaaa::aaaa]:65536",
		"[aaaa::aaaa]:-1",
		"[aaaa::aaaa]:",
		"aaaa::aaag",
		"aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:",
		"aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa", // too few
		"aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa", // too many
		"::"
	};
	const int nNumInvalidStrings = sizeof(InvalidStrings) / sizeof(InvalidStrings[0]);

	for (int i = 0; i < nNumInvalidStrings; i++)
	{
		stiHResult hResult = parser.assign (InvalidStrings[i]);
			
		if (!stiIS_ERROR (hResult))
		{
			sprintf (szBuffer, "Testing invalid ip address %d = \"%s\"\n", i, InvalidStrings[i]);
			CPPUNIT_ASSERT_MESSAGE(szBuffer, stiIS_ERROR (hResult));
		}
	}
}


void CstiIpAddressTest::Comparison()
{
	CstiIpAddress parser;
	char szBuffer[512];
	bool bResult;
	stiHResult hResult;
	
	const struct { const char *pszValue1; const char *pszValue2; bool bExpectedResult; } tests[] =
	{
		{"127.0.0.1", "127.0.0.1", true},
		{"127.0.0.1", "127.0.0.2", false},
		{"::1", "::1", true},
		{"::1", "::2", false},
		{"127.0.0.1", "::1", false},
		{"127.0.0.1:1234", "127.0.0.1:1234", true},
		{"127.0.0.1:1234", "127.0.0.1", false},
		{"127.0.0.1:1234", "127.0.0.1:4321", false},
		{"[::1]:1234", "[::1]:1234", true},
		{"[::1]:1234", "::1", false},
		{"[::1]:1234", "[::1]:4321", false},
		{"::1%1", "::1%1", true},
		{"::1%1", "::1", false},
		{"::1%1", "::1%2", false},
		{"[::1%1]:1234", "[::1%1]:1234", true},
		{"[::1%1]:1234", "::1%1", false},
		{"[::1%1]:1234", "[::1%1]:4321", false},
		{"[::1%1]:1234", "[::1%2]:1234", false},
		{"::1", "gibberish", false},
	};
	
	const int nNumTests = sizeof(tests) / sizeof(tests[0]);
	for (int i = 0; i < nNumTests; i++)
	{
		hResult = parser.assign (tests[i].pszValue1);
		if (stiIS_ERROR(hResult))
		{
			sprintf (szBuffer, "Test error parsing ip %d = \"%s\"\n", i, tests[i].pszValue1);
			printf("%s", szBuffer);
			CPPUNIT_ASSERT_MESSAGE (szBuffer, false);
		}
		bResult = (parser == tests[i].pszValue2);
		if (bResult != tests[i].bExpectedResult)
		{
			sprintf (szBuffer, "Incorrect comparison. \"%s\"==\"%s\" should return %d\n", tests[i].pszValue1, tests[i].pszValue2, tests[i].bExpectedResult);
			printf("%s", szBuffer);
			CPPUNIT_ASSERT_MESSAGE (szBuffer, false);
		}
	}
}


void CstiIpAddressTest::FormatIpv6()
{
	CstiIpAddress parser;
	char szBuffer[512];
	stiHResult hResult;
	std::string result;
	
	const struct { const char *pszInput; int nFormatMask; const char *pszExpectedOutput; } tests[] =
	{
		{"::1", CstiIpAddress::eFORMAT_IP, "::1"},  // check non-expanding
		{"::1", CstiIpAddress::eFORMAT_IP | CstiIpAddress::eFORMAT_NO_CONDENSE, "0000:0000:0000:0000:0000:0000:0000:0001"},  // check expanding
		{"1::", CstiIpAddress::eFORMAT_IP | CstiIpAddress::eFORMAT_NO_CONDENSE, "0001:0000:0000:0000:0000:0000:0000:0000"},  // check expanding
		{"[::1]:1234", CstiIpAddress::eFORMAT_IP, "::1"}, // drop port
		{"[::1]:1234", CstiIpAddress::eFORMAT_IP | CstiIpAddress::eFORMAT_ALLOW_ZONE | CstiIpAddress::eFORMAT_ALLOW_PORT, "[::1]:1234"}, // include port
		{"[::1]:1234", CstiIpAddress::eFORMAT_IP | CstiIpAddress::eFORMAT_NO_CONDENSE, "0000:0000:0000:0000:0000:0000:0000:0001"},  // check expanding without port
		{"[::1]:1234", CstiIpAddress::eFORMAT_IP | CstiIpAddress::eFORMAT_NO_CONDENSE | CstiIpAddress::eFORMAT_ALLOW_ZONE | CstiIpAddress::eFORMAT_ALLOW_PORT, "[0000:0000:0000:0000:0000:0000:0000:0001]:1234"},  // check expanding with port
		{"0:a::1", CstiIpAddress::eFORMAT_IP | CstiIpAddress::eFORMAT_NO_CONDENSE, "0000:000a:0000:0000:0000:0000:0000:0001"}, // check expanding
		{"0000:000a:0000:0000:0000:0000:0000:0001", CstiIpAddress::eFORMAT_IP, "0:a::1"}, // check condensing
		{"0000:000a:0000:000b:0000:000c:0000:000d", CstiIpAddress::eFORMAT_IP, "0:a:0:b:0:c:0:d"}, // this non-colon-condensable
		{"0000:0000:000a:0000:0000:0000:000b:000c", CstiIpAddress::eFORMAT_IP, "0:0:a::b:c"}, // always condense longest run
		{"::ABCD", CstiIpAddress::eFORMAT_IP, "::abcd"},  // should lower-case it
		{"::1%1", CstiIpAddress::eFORMAT_IP, "::1"}, // drop zone
		{"::1%1", CstiIpAddress::eFORMAT_IP | CstiIpAddress::eFORMAT_ALLOW_ZONE | CstiIpAddress::eFORMAT_ALLOW_PORT, "::1%1"}, // include zone
		{"[::1%1]:1234", CstiIpAddress::eFORMAT_IP, "::1"}, // drop zone and port
		{"[::1%1]:1234", CstiIpAddress::eFORMAT_IP | CstiIpAddress::eFORMAT_ALLOW_ZONE | CstiIpAddress::eFORMAT_ALLOW_PORT, "[::1%1]:1234"}, // include zone and port
		{"[::1%1]:1234", CstiIpAddress::eFORMAT_IP | CstiIpAddress::eFORMAT_NO_CONDENSE | CstiIpAddress::eFORMAT_ALLOW_ZONE | CstiIpAddress::eFORMAT_ALLOW_PORT, "[0000:0000:0000:0000:0000:0000:0000:0001%1]:1234"},  // check expanding with zone and port
		{"[0000:0000:0000:0000:0000:0000:0000:0001%1]:1234", CstiIpAddress::eFORMAT_IP | CstiIpAddress::eFORMAT_ALLOW_ZONE | CstiIpAddress::eFORMAT_ALLOW_PORT, "[::1%1]:1234"},  // check condensing with zone and port
		{"[::1]:1234", CstiIpAddress::eFORMAT_IP | CstiIpAddress::eFORMAT_ALWAYS_USE_BRACKETS | CstiIpAddress::eFORMAT_ALLOW_PORT, "[::1]:1234"},
		{"[::1]:1234", CstiIpAddress::eFORMAT_IP | CstiIpAddress::eFORMAT_ALWAYS_USE_BRACKETS, "[::1]"}
	};
	
	const int nNumTests = sizeof(tests) / sizeof(tests[0]);
	for (int i = 0; i < nNumTests; i++)
	{
		hResult = parser.assign (tests[i].pszInput);
		if (stiIS_ERROR(hResult))
		{
			sprintf (szBuffer, "Test error parsing ip %d = \"%s\"\n", i, tests[i].pszInput);
			CPPUNIT_ASSERT_MESSAGE (szBuffer, !stiIS_ERROR(hResult));
		}

		result = parser.AddressGet (tests[i].nFormatMask);
		if (result != tests[i].pszExpectedOutput)
		{
			sprintf (szBuffer, "Incorrect formatting %d from \"%s\" to \"%s\" instead of \"%s\"\n", i, tests[i].pszInput, result.c_str (), tests[i].pszExpectedOutput);
			CPPUNIT_ASSERT_MESSAGE (szBuffer, result == tests[i].pszExpectedOutput);
		}
	}
}

void CstiIpAddressTest::FormatIpv4()
{
	CstiIpAddress parser;
	char szBuffer[512];
	stiHResult hResult;
	std::string result;
	
	const struct { const char *pszInput; int nFormatMask; const char *pszExpectedOutput; } tests[] =
	{
		{"127.0.0.1", CstiIpAddress::eFORMAT_IP | CstiIpAddress::eFORMAT_NO_CONDENSE, "127.0.0.1"},
		{"127.0.0.1", CstiIpAddress::eFORMAT_IP, "127.0.0.1"},
		{"127.0.0.1:1234", CstiIpAddress::eFORMAT_IP | CstiIpAddress::eFORMAT_NO_CONDENSE, "127.0.0.1"},
		{"127.0.0.1:1234", CstiIpAddress::eFORMAT_IP | CstiIpAddress::eFORMAT_NO_CONDENSE | CstiIpAddress::eFORMAT_ALLOW_ZONE | CstiIpAddress::eFORMAT_ALLOW_PORT, "127.0.0.1:1234"},
	};
	
	const int nNumTests = sizeof(tests) / sizeof(tests[0]);
	for (int i = 0; i < nNumTests; i++)
	{
		hResult = parser.assign (tests[i].pszInput);
		if (stiIS_ERROR(hResult))
		{
			sprintf (szBuffer, "Test error parsing ip %d = \"%s\"\n", i, tests[i].pszInput);
			CPPUNIT_ASSERT_MESSAGE (szBuffer, !stiIS_ERROR(hResult));
		}

		result = parser.AddressGet (tests[i].nFormatMask);
		if (result != tests[i].pszExpectedOutput)
		{
			sprintf (szBuffer, "Incorrect formatting %d from \"%s\" to \"%s\" instead of \"%s\"\n", i, tests[i].pszInput, result.c_str (), tests[i].pszExpectedOutput);
			CPPUNIT_ASSERT_MESSAGE (szBuffer, result == tests[i].pszExpectedOutput);
		}
	}
}
