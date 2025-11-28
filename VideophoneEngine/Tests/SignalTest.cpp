
#include "SignalTest.h"

#include "CstiSignal.h"

namespace
{

class Foo
{
public:

	void trigger(int /*value*/)
	{
		signalTriggered = true;
	}

	void trigger2(int /*value*/)
	{
		signal2Triggered = true;
	}

	void reset()
	{
		signalTriggered = false;
		signal2Triggered = false;
	}

	bool signal2Triggered = false;
	bool signalTriggered = false;
};

}

void SignalTests::ConnectionTest()
{
	CstiSignal<int> mySignal;

	Foo foo;

	{
		// NOTE: be sure to bind reference to foo so a copy isn't made
		auto connection = mySignal.Connect(std::bind(&Foo::trigger, std::ref(foo), std::placeholders::_1));
		// Just to avoid "not used" warning
		(void)connection;

		mySignal.Emit(42);
		CPPUNIT_ASSERT_EQUAL(foo.signalTriggered, true);

		foo.reset();
	}

	mySignal.Emit(43);
	CPPUNIT_ASSERT_EQUAL(foo.signalTriggered, false);
}


void SignalTests::ConnectionCollectionTest()
{
	CstiSignal<int> mySignal;

	Foo foo;

	{
		std::vector<CstiSignalConnection> connections;

		// NOTE: be sure to bind reference to foo so a copy isn't made
		connections.push_back(
			mySignal.Connect(std::bind(&Foo::trigger, std::ref(foo), std::placeholders::_1))
		);
		// Just to avoid "not used" warning
		(void)connections;

		mySignal.Emit(42);
		CPPUNIT_ASSERT_EQUAL(foo.signalTriggered, true);

		foo.reset();
	}

	mySignal.Emit(43);
	CPPUNIT_ASSERT_EQUAL(foo.signalTriggered, false);
}

void SignalTests::DisconnectTest()
{
	CstiSignal<int> mySignal;

	Foo foo;

	// NOTE: be sure to bind reference to foo so a copy isn't made
	auto connection = mySignal.Connect(std::bind(&Foo::trigger, std::ref(foo), std::placeholders::_1));

	connection.Disconnect();

	mySignal.Emit(42);
	CPPUNIT_ASSERT_EQUAL(foo.signalTriggered, false);
}

void SignalTests::ConnectionAssignmentTest()
{
	CstiSignal<int> mySignal;

	Foo foo;

	CstiSignalConnection connection;

	// NOTE: be sure to bind reference to foo so a copy isn't made
	connection = mySignal.Connect(std::bind(&Foo::trigger, std::ref(foo), std::placeholders::_1));

	connection.Disconnect();

	mySignal.Emit(42);
	CPPUNIT_ASSERT_EQUAL(foo.signalTriggered, false);
}

void SignalTests::ConnectionReassignmentTest()
{
	CstiSignal<int> mySignal;

	Foo foo;

	CstiSignalConnection connection = mySignal.Connect(std::bind(&Foo::trigger, std::ref(foo), std::placeholders::_1));


	// Reporpose connection, should disconnect first signal
	connection = mySignal.Connect(std::bind(&Foo::trigger2, std::ref(foo), std::placeholders::_1));

	mySignal.Emit(42);
	CPPUNIT_ASSERT_EQUAL(foo.signalTriggered, false);
	CPPUNIT_ASSERT_EQUAL(foo.signal2Triggered, true);
}


void SignalTests::ConnectionReassignmentTest2()
{
	CstiSignal<int> mySignal;

	Foo foo;

	CstiSignalConnection connection1 = mySignal.Connect(std::bind(&Foo::trigger, std::ref(foo), std::placeholders::_1));

	CstiSignalConnection connection2 = mySignal.Connect(std::bind(&Foo::trigger2, std::ref(foo), std::placeholders::_1));

	// Old Connection 1 should disconnect
	connection1 = std::move(connection2);


	mySignal.Emit(42);
	CPPUNIT_ASSERT_EQUAL(foo.signalTriggered, false);
	CPPUNIT_ASSERT_EQUAL(foo.signal2Triggered, true);
}
