// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved

#include "JsonTestResponse.h"

const static std::string COMMAND {"command"};
const static std::string SUCCESSFUL {"successful"};
const static std::string ERROR_MESSAGE {"errorMessage"};


JsonTestResponse::JsonTestResponse (const Poco::JSON::Object &command)
{
	Poco::JSON::Object::set (COMMAND, command);
}

JsonTestResponse::JsonTestResponse (const std::string &command)
{
	Poco::JSON::Object::set (COMMAND, command);
}

void JsonTestResponse::successfulSet (bool successful)
{
	Poco::JSON::Object::set (SUCCESSFUL, successful);
}

bool JsonTestResponse::successfulIsEmpty ()
{
	return Poco::JSON::Object::get (SUCCESSFUL).isEmpty ();
}

void JsonTestResponse::errorMessageSet (const std::string &message)
{
	Poco::JSON::Object::set (ERROR_MESSAGE, message);
}

bool JsonTestResponse::errorMessageIsEmpty ()
{
	return Poco::JSON::Object::get (ERROR_MESSAGE).isEmpty();
}

void JsonTestResponse::invalidParametersSet ()
{
	successfulSet (false);
	errorMessageSet ("Invalid Parameters");
}
