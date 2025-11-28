/*!
 * \file OptString.h
 * \brief typedef for optional string
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
 */
#pragma once

#include <string>
#include <boost/optional.hpp>

/*!
 * \brief Class to model a string that needs to distinguish between unset and empty
 *
 * NOTE: do not add additional non-c-string interface methods to this
 *
 */
class OptString : public boost::optional<std::string>
{
public:
	OptString () = default;
	OptString (const char *);
	OptString (const std::string &str);

	// NOTE: weird that this had to be reimplemented?
	// wouldn't this be inherited?
	OptString & operator= (const std::string &str);

	/* legacy c-string interfaces */
	OptString & operator= (const char *cstr);
};


bool operator== (const OptString &optString, const char *cstr);
bool operator== (const char *cstr, const OptString &optString);
bool operator!= (const OptString &optString, const char *cstr);
bool operator!= (const char *cstr, const OptString &optString);
