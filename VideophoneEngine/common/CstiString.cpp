/*!
* \file CstiString.cpp
* \brief Like std::string except it allows us to differentiate between an empty
*        string and one that has not been set.  Most of the time you should just
*        use std::string.  But in cases where you need a third state, this
*        class can be useful.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/

#include "CstiString.h"


/*!
 * \brief Constructor, initialize by std::string
 */
CstiString::CstiString (
    const std::string &value)
{
	m_value = value;
	m_hasValue = true;
}


/*!
 * \brief Allows assignments from other CstiStrings
 */
CstiString &CstiString::operator= (
	const CstiString &other)
{
	m_hasValue = !other.isNull();
	m_value = other.toStdString();

	return *this;
}


/*!
 * \brief Allows assignments to const char *
 */
CstiString &CstiString::operator= (
    const char *value)
{
	m_value.clear ();
	m_hasValue = true;

	if (value)
	{
		m_value = value;
	}

	return *this;
}


/*!
 * \brief Allows assignments to std::string.  Note, we can't use this for
 *        const char * assignment, because a char * can be nullptr, which would
 *        result in a SIGABRT.
 */
CstiString &CstiString::operator= (
    const std::string &value)
{
	m_value = value;
	m_hasValue = true;

	return *this;
}


/*!
 * \brief Returns true if the string hasn't been set or is empty
 */
bool CstiString::IsNullOrEmpty () const
{
	return (isNull() || isEmpty());
}


/*!
 * \brief Returns true if the string hasn't been set
 */
bool CstiString::isNull () const
{
	return !m_hasValue;
}


/*!
 * \brief Returns true if the string is empty, but set
 */
bool CstiString::isEmpty () const
{
	return !m_hasValue || m_value.empty();
}


/*!
 * \brief Returns the length of the string
 */
int CstiString::length () const
{
	return m_value.length ();
}


std::string CstiString::toStdString () const
{
	return m_value;
}
