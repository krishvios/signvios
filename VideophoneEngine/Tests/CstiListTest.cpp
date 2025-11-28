
#include "CstiListTest.h"

#include "CstiList.h"

class TestList : public CstiList
{
public:

	virtual ~TestList() = default;
};

/*!
 * \brief check some interesting behavior of allowing count to be set
 * even though list contains no items
 */
void CstiListTests::SizeWithoutListTest ()
{
	TestList testList;

	testList.CountSet (100);

	CPPUNIT_ASSERT_EQUAL(100U, testList.CountGet ());

	testList.ItemAdd(std::make_shared<CstiListItem> ());

	CPPUNIT_ASSERT_EQUAL(1U, testList.CountGet ());

}

