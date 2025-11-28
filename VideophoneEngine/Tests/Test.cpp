// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/CompilerOutputter.h>

#include <list>
#include <regex>
#include <iostream>

std::list<CPPUNIT_NS::Test *> collectTests(const std::regex &testSelectionRegex, CPPUNIT_NS::Test * test)
{
	std::list<CPPUNIT_NS::Test *> result;

	// It looks like if any collected nodes are in the same hierarchy, double deletes happen on program shutdown
	// For now, only collect leaf nodes
	if(0 == test->getChildTestCount())
	{
		if(std::regex_match(test->getName(), testSelectionRegex))
		{
			// debug
			std::cout << "Adding test: " << test->getName() << "\n";
			result.push_back(test);
		}
		else
		{
			std::cout << "Not adding test: " << test->getName() << "\n";
		}
	}
	else
	{
		//std::cout << "Not adding test with children: " << test->getName() << '\n';
	}

	int testCount = test->getChildTestCount();

	for(int i = 0; i < testCount; ++i)
	{
		auto childResult = collectTests(testSelectionRegex, test->getChildTestAt(i));

		// Add to the result
		result.splice(result.end(), childResult);
	}

	return result;
}

int main(
	int argc,
	char* argv[])
{
	const std::string xmlFile ="VideophoneEngineTests.xml";

	// Get the top level suite from the registry
	CPPUNIT_NS::Test *pSuite = CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest();

	// Adds the test to the list of test to run
	CPPUNIT_NS::TextTestRunner Runner;

	// Check for environment variable to see if a subset of tests should be run
	// TODO: take command line arg instead?
	char * testSelection = std::getenv("CPPUNIT_RUN_FILTERS");

	if(nullptr != testSelection)
	{
		// TODO: catch invalid regex exception?
		std::regex testSelectionRegex(testSelection);

		std::list<CPPUNIT_NS::Test *> tests = collectTests(testSelectionRegex, pSuite);
		for(auto test : tests)
		{
			Runner.addTest(test);
		}
	}
	else
	{
		Runner.addTest( pSuite );
	}

	// Make nice output that editors can jump to (much like compiler errors)
	Runner.setOutputter(new CPPUNIT_NS::CompilerOutputter(&Runner.result(), std::cout));

	// Run the test.
	bool wasSucessful = Runner.run();

	// Store results in xml file
	CPPUNIT_NS::OFileStream fs (xmlFile.c_str ());
	CPPUNIT_NS::XmlOutputter outputter (&Runner.result (), fs);
	outputter.write ();

	// Return error code 1 if the one of test failed.
	return wasSucessful ? 0 : 1;
}
