// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "stiTools.h"

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Message.h>
#include <sys/time.h>

#include <boost/format.hpp>

class stiToolsTest;


CPPUNIT_TEST_SUITE_REGISTRATION (stiToolsTest);


class stiToolsTest : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( stiToolsTest );
	CPPUNIT_TEST (IPAddressValidateEx);
	CPPUNIT_TEST (TimevalSubtract);
	CPPUNIT_TEST (TimevalAdd);
	CPPUNIT_TEST (TimespecSubtract);
	CPPUNIT_TEST (TimespecAdd);
	CPPUNIT_TEST (CanonicalNumberValidate);
	CPPUNIT_TEST (E164AliasValidate);
	CPPUNIT_TEST (EmailAddressValidate);
	CPPUNIT_TEST (TimeDifferenceInDays);
	CPPUNIT_TEST (TimeDiffToString);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp();
	void tearDown();

	void IPAddressValidateEx ();
	void IPAddressToIntegerConvert ();
	void TimevalSubtract ();
	void TimevalAdd ();
	void TimespecSubtract ();
	void TimespecAdd ();
	void CanonicalNumberValidate ();
	void E164AliasValidate ();
	void EmailAddressValidate ();
	void TimeDifferenceInDays ();
	void TimeDiffToString ();

private:
};


void stiToolsTest::setUp ()
{
}

void stiToolsTest::tearDown ()
{
}

void stiToolsTest::IPAddressValidateEx ()
{
	//
	// Test valid IP addresses.
	//
	static std::vector<const char *> ValidStrings =
	{
		"0.0.0.0:1", // We consider a port of zero invalid
		"255.255.255.255:65535",
		"0.0.0.0",
		"255.255.255.255",
		"::1",
		"[::1]:65535",
		"aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa",
		"[aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa]:65535",
		"aaaa::aaaa",
		"1::a1",
		"[aaaa::aaaa]",
		"[aaaa::aaaa]:65535"
	};

	for (const auto &address : ValidStrings)
	{
		bool result = ::IPAddressValidateEx (address);

		CPPUNIT_ASSERT_MESSAGE ( (boost::format ("Invalid address: %1%\n") % address).str (), result);
	}

	//
	// Test invalid IP addresses.
	//
	static std::vector<const char *> InvalidStrings =
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
		".....",
		"-1::-1",
		"aaaa::1.5",
		"aaaa::aaaa::aaaa",
		"aaaa::aaaa:65535",
		"aaaa::aaaa:65536",
		"[aaaa::aaaa]:",
		"aaaa::aaag",
		"aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:",
	};

	for (const auto &address : InvalidStrings)
	{
		bool result = ::IPAddressValidateEx (address);

		CPPUNIT_ASSERT_MESSAGE ( (boost::format ("Valid (should be invalid) address: %1%\n") % address).str (), !result);
	}
}


#define MAKEINT(a, b, c, d) ((a << 24) | (b << 16) | (c << 8) | d)


void stiToolsTest::TimevalSubtract ()
{
	struct TimeTests
	{
		timeval T1;
		timeval T2;
		timeval Result;
		int nSign;
	};
	
	const TimeTests Times [] =
	{
		{{9, 5}, {10, 6}, {1, 1}, 1},	// less, less
		{{9, 5}, {10, 5}, {1, 0}, 1},	// less, equal
		{{9, 6}, {10, 5}, {0, 999999}, 1},	// less, greater
		{{10, 5}, {10, 6}, {0, 1}, 1},	// equal, less
		{{10, 5}, {10, 5}, {0, 0}, 0},	// equal, equal
		{{10, 6}, {10, 5}, {0, 1}, 0},	// equal, greater
		{{10, 5}, {9, 6}, {0, 999999}, 0},	// greater, less
		{{10, 5}, {9, 5}, {1, 0}, 0},	// greater, equal
		{{10, 6}, {9, 5}, {1, 1}, 0},	// greater, greater
	};
	const int nNumTests = sizeof (Times) / sizeof (Times[0]);
	
	for (int i = 0; i < nNumTests; i++)
	{
		timeval Result;
		char szTestNum[8];
		
		int nSign = timevalSubtract (&Result, &Times[i].T1, &Times[i].T2);
		
		sprintf (szTestNum, "%d", i);
		
		CPPUNIT_ASSERT_MESSAGE (std::string("timevalSubtract Failed: Test index ")
								+ szTestNum,
								Result.tv_sec == Times[i].Result.tv_sec
								&& Result.tv_usec == Times[i].Result.tv_usec
								&& nSign == Times[i].nSign);
	}
}



void stiToolsTest::TimevalAdd ()
{
	struct TimeTests
	{
		timeval T1;
		timeval T2;
		timeval Result;
	};
	
	const TimeTests Times [] =
	{
		{{1, 0}, {1, 1}, {2, 1}},		// no carry
		{{1, 999999}, {1, 1}, {3, 0}},	// carry
		{{1, 999999}, {1, 2}, {3, 1}},	// carry
	};
	const int nNumTests = sizeof (Times) / sizeof (Times[0]);
	
	for (int i = 0; i < nNumTests; i++)
	{
		timeval Result;
		char szTestNum[8];
		
		timevalAdd (&Result, &Times[i].T1, &Times[i].T2);
		
		sprintf (szTestNum, "%d", i);
		
		CPPUNIT_ASSERT_MESSAGE (std::string("timevalAdd Failed: Test index ")
								+ szTestNum,
								Result.tv_sec == Times[i].Result.tv_sec
								&& Result.tv_usec == Times[i].Result.tv_usec);
	}
}


void stiToolsTest::TimespecSubtract ()
{
	struct TimeTests
	{
		timespec T1;
		timespec T2;
		timespec Result;
		int nSign;
	};
	
	const TimeTests Times [] =
	{
		{{9, 5}, {10, 6}, {1, 1}, 1},	// less, less
		{{9, 5}, {10, 5}, {1, 0}, 1},	// less, equal
		{{9, 6}, {10, 5}, {0, 999999999}, 1},	// less, greater
		{{10, 5}, {10, 6}, {0, 1}, 1},	// equal, less
		{{10, 5}, {10, 5}, {0, 0}, 0},	// equal, equal
		{{10, 6}, {10, 5}, {0, 1}, 0},	// equal, greater
		{{10, 5}, {9, 6}, {0, 999999999}, 0},	// greater, less
		{{10, 5}, {9, 5}, {1, 0}, 0},	// greater, equal
		{{10, 6}, {9, 5}, {1, 1}, 0},	// greater, greater
	};
	const int nNumTests = sizeof (Times) / sizeof (Times[0]);
	
	for (int i = 0; i < nNumTests; i++)
	{
		timespec Result;
		char szTestNum[8];
		
		int nSign = timespecSubtract (&Result, Times[i].T1, Times[i].T2);
		
		sprintf (szTestNum, "%d", i);
		
		CPPUNIT_ASSERT_MESSAGE (std::string("timevalSubtract Failed: Test index ")
								+ szTestNum,
								Result.tv_sec == Times[i].Result.tv_sec
								&& Result.tv_nsec == Times[i].Result.tv_nsec
								&& nSign == Times[i].nSign);
	}
}


void stiToolsTest::TimespecAdd ()
{
	struct TimeTests
	{
		timespec T1;
		timespec T2;
		timespec Result;
	};
	
	const TimeTests Times [] =
	{
		{{1, 0}, {1, 1}, {2, 1}},		// no carry
		{{1, 999999999}, {1, 1}, {3, 0}},	// carry
		{{1, 999999999}, {1, 2}, {3, 1}},	// carry
	};
	const int nNumTests = sizeof (Times) / sizeof (Times[0]);
	
	for (int i = 0; i < nNumTests; i++)
	{
		timespec Result;
		char szTestNum[8];
		
		timespecAdd (&Result, Times[i].T1, Times[i].T2);
		
		sprintf (szTestNum, "%d", i);
		
		CPPUNIT_ASSERT_MESSAGE (std::string("timevalAdd Failed: Test index ")
								+ szTestNum,
								Result.tv_sec == Times[i].Result.tv_sec
								&& Result.tv_nsec == Times[i].Result.tv_nsec);
	}
}


void stiToolsTest::CanonicalNumberValidate()
{
	EstiBool bValid;
	struct PhoneNumberTest
	{
		const char *pszPhoneNumber;
		EstiBool bExpectedResult;
	};
	
	const PhoneNumberTest aTests [] =
	{
		{nullptr, estiFALSE},
		{"", estiFALSE},
		{"+18005551212", estiTRUE},
		{"18005551212", estiTRUE},
		{"+1(800) 555-1212", estiTRUE},
		{"1 (800) 555-1212", estiTRUE},
		{"ABCDEFGHIJK", estiFALSE},
		{"+1ABCDEF", estiFALSE},
	};
	
	for (unsigned int i = 0; i < sizeof (aTests) / sizeof (aTests[0]); i++)
	{
		char szTestNum[11];
		
		sprintf (szTestNum, "%d", i);
		
		bValid = ::CanonicalNumberValidate(aTests[i].pszPhoneNumber);
		CPPUNIT_ASSERT_MESSAGE (std::string("CanonicalNumberValidate Failed: Test index ") + szTestNum,
								bValid == aTests[i].bExpectedResult);
	}
}


void stiToolsTest::E164AliasValidate()
{
	EstiBool bValid;
	struct PhoneNumberTest
	{
		const char *pszPhoneNumber;
		EstiBool bExpectedResult;
	};
	
	const PhoneNumberTest aTests [] =
	{
		{nullptr, estiFALSE},
		{"", estiFALSE},
		{"18005551212", estiTRUE},
		{"ABCDEFGHIJK", estiFALSE},
		{"+1ABCDEF", estiFALSE},
	};
	
	for (unsigned int i = 0; i < sizeof (aTests) / sizeof (aTests[0]); i++)
	{
		char szTestNum[11];
		
		sprintf (szTestNum, "%d", i);
		
		bValid = ::E164AliasValidate(aTests[i].pszPhoneNumber);
		CPPUNIT_ASSERT_MESSAGE (std::string("E164AliasValidate Failed: Test index ") + szTestNum,
								bValid == aTests[i].bExpectedResult);
	}
}


void stiToolsTest::EmailAddressValidate()
{
	EstiBool bValid;
	struct EmailTest
	{
		const char *pszAddress;
		EstiBool bExpectedResult;
	};
	
	const EmailTest aTests [] =
	{
		{nullptr, estiFALSE},
		{"", estiFALSE},
		{"@", estiFALSE},
		{"@.", estiFALSE},
		{"abc@", estiFALSE},
		{"abc@def", estiFALSE},
		{"abc@def.", estiFALSE},
		{"abc@.def", estiFALSE},
		{"abc@@def", estiFALSE},
		{"abc@def.ghi", estiTRUE},
		{"@def.ghi", estiFALSE},
		{"@.ghi", estiFALSE},
		{"@def.", estiFALSE},
		{"@def", estiFALSE},
	};
	
	for (unsigned int i = 0; i < sizeof (aTests) / sizeof (aTests[0]); i++)
	{
		char szTestNum[11];
		
		sprintf (szTestNum, "%d", i);
		
		bValid = ::EmailAddressValidate(aTests[i].pszAddress);
		CPPUNIT_ASSERT_MESSAGE (std::string("EmailAddressValidate Failed: Test index ") + szTestNum,
								bValid == aTests[i].bExpectedResult);
	}
}


void stiToolsTest::TimeDifferenceInDays()
{
	struct TimeTest
	{
		const char *pszDate1;
		const char *pszDate2;
		EstiRounding eRoundMethod;
		int nDays;
	};
	const TimeTest aTests [] = 
	{
		{"1/1/2001 12:00:00", "1/2/2001 12:00:01", estiTRUNCATE, 1},
		{"1/1/2001 12:00:00", "1/2/2001 12:00:01", estiROUND_UP, 2},
		{"1/1/2001 12:00:00", "1/2/2001 11:59:59", estiTRUNCATE, 0},
		{"1/1/2001 12:00:00", "1/2/2001 11:59:59", estiROUND_UP, 1},
	};
	
	for (unsigned int i = 0; i < sizeof (aTests) / sizeof (aTests[0]); i++)
	{
		char szTestNum[11];
		time_t t1;
		time_t t2;
		int nDays;
		
		sprintf (szTestNum, "%d", i);
		
		TimeConvert (aTests[i].pszDate1, &t1);
		TimeConvert (aTests[i].pszDate2, &t2);
		
		nDays = ::TimeDifferenceInDays (t2, t1, aTests[i].eRoundMethod);
		
		CPPUNIT_ASSERT_MESSAGE (std::string("TimeDifferenceInDays Failed: Test index ") + szTestNum,
								nDays == aTests[i].nDays);
	}
}


void stiToolsTest::TimeDiffToString()
{
	struct TimeTest
	{
		const char *pszDate1;
		const char *pszDate2;
		int nMask;
		EstiRounding eRoundMethod;
		const char *pszExpectedResult;
	};
	const TimeTest aTests [] = 
	{
		{"1/1/2001 12:00:00", "1/2/2001 12:00:01", estiDAYS, estiTRUNCATE, "1 day"},
		{"1/1/2001 12:00:00", "1/2/2001 12:00:01", estiDAYS | estiHOURS, estiTRUNCATE, "1 day"},
		{"1/1/2001 12:00:00", "1/2/2001 12:00:01", estiDAYS | estiHOURS | estiMINUTES, estiTRUNCATE, "1 day"},
		{"1/1/2001 12:00:00", "1/2/2001 12:00:01", estiDAYS | estiHOURS | estiMINUTES | estiSECONDS, estiTRUNCATE, "1 day and 1 second"},
		{"1/1/2001 12:00:00", "1/2/2001 12:00:01", estiDAYS, estiROUND_UP, "2 days"},
		{"1/1/2001 12:00:00", "1/2/2001 12:00:01", estiDAYS | estiHOURS, estiROUND_UP, "1 day and 1 hour"},
		{"1/1/2001 12:00:00", "1/2/2001 12:00:01", estiDAYS | estiHOURS | estiMINUTES, estiROUND_UP, "1 day and 1 minute"},
		{"1/1/2001 12:00:00", "1/2/2001 12:00:01", estiDAYS | estiHOURS | estiMINUTES | estiSECONDS, estiROUND_UP, "1 day and 1 second"},
		{"1/1/2001 12:00:00", "1/2/2001 11:59:59", estiDAYS, estiTRUNCATE, "0 days"},
		{"1/1/2001 12:00:00", "1/2/2001 11:59:59", estiDAYS | estiHOURS, estiTRUNCATE, "23 hours"},
		{"1/1/2001 12:00:00", "1/2/2001 11:59:59", estiDAYS | estiHOURS | estiMINUTES, estiTRUNCATE, "23 hours and 59 minutes"},
		{"1/1/2001 12:00:00", "1/2/2001 11:59:59", estiDAYS | estiHOURS | estiMINUTES | estiSECONDS, estiTRUNCATE, "23 hours, 59 minutes and 59 seconds"},
		{"1/1/2001 12:00:00", "1/2/2001 11:59:59", estiDAYS, estiROUND_UP, "1 day"},
		{"1/1/2001 12:00:00", "1/2/2001 11:59:59", estiDAYS | estiHOURS, estiROUND_UP, "1 day"},
		{"1/1/2001 12:00:00", "1/2/2001 11:59:59", estiDAYS | estiHOURS | estiMINUTES, estiROUND_UP, "1 day"},
		{"1/1/2001 12:00:00", "1/2/2001 11:59:59", estiDAYS | estiHOURS | estiMINUTES | estiSECONDS, estiROUND_UP, "23 hours, 59 minutes and 59 seconds"},
	};
	
	for (unsigned int i = 0; i < sizeof (aTests) / sizeof (aTests[0]); i++)
	{
		char szTestNum[11];
		time_t t1;
		time_t t2;
		double diff;
		char szBuffer[512];
		
		sprintf (szTestNum, "%d", i);
		
		TimeConvert (aTests[i].pszDate1, &t1);
		TimeConvert (aTests[i].pszDate2, &t2);
		
		diff = difftime (t2, t1);
		
		::TimeDiffToString(diff, szBuffer, sizeof (szBuffer), aTests[i].nMask, aTests[i].eRoundMethod);
		
		CPPUNIT_ASSERT_MESSAGE (std::string("TimeDiffToString Failed: Test index ") + szTestNum,
								strcmp (szBuffer, aTests[i].pszExpectedResult) == 0);
	}
}
