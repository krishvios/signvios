// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef CPP_UNIT_TEST_WRAPPER
#define CPP_UNIT_TEST_WRAPPER

#include <string>

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/TextTestProgressListener.h>
#include <cppunit/TestListener.h>

using namespace CppUnit;

class CppUnitTestWrapper {
public:
	CppUnitTestWrapper();
	~CppUnitTestWrapper();

	void setResultsPath(std::string path) { this->path = path; }
	void setListener(TestListener *listener) { this->pListener = listener; }

	Test *makeTest(std::string name);
	Test *getChildIndex(Test *test, int idx);
	int getTestCount(Test *test);
	std::string getTestName(Test *test);

	bool runTests(Test *test);

private:
	TestListener *pListener;

	CppUnit::TestResult controller;

	std::string path;
};

extern std::vector<std::string> g_cppUnitRegistries;

class SciAutoTestRegister {
public:
	SciAutoTestRegister(std::string name) {
		g_cppUnitRegistries.push_back(name);
	}

private:
	std::string name;
};

#define ADD_TO_NAMED_REGISTRIES(name) SciAutoTestRegister cppUnitReg_##name(#name)


#endif
