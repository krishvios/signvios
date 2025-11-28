
#include "OptString.h"

OptString::OptString (const char *cstr)
{
	if (cstr)
	{
		this->assign(std::string(cstr));
	}
	else
	{
		this->assign(boost::none);
	}
}


OptString::OptString (const std::string &str)
{
	this->assign(str);
}


OptString & OptString::operator= (const std::string &str)
{
	this->assign (str);
	return *this;
}

/* legacy interfaces */
OptString & OptString::operator= (const char *cstr)
{
	if (cstr)
	{
		this->assign(std::string(cstr));
	}
	else
	{
		this->assign(boost::none);
	}

	return *this;
}


bool operator== (const OptString &optString, const char *cstr)
{
	if (optString)
	{
		return optString.value () == cstr;
	}

	return cstr == nullptr;
}

bool operator== (const char *cstr, const OptString &optString)
{
	return operator== (optString, cstr);
}


bool operator!= (const OptString &optString, const char *cstr)
{
	return !operator== (optString, cstr);
}


bool operator!= (const char *cstr, const OptString &optString)
{
	return !operator== (optString, cstr);
}
