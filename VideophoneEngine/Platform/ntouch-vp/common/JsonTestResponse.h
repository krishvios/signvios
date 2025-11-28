// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include <Poco/JSON/Object.h>


class JsonTestResponse : public Poco::JSON::Object
{
public:

	JsonTestResponse (const Poco::JSON::Object &command);
	JsonTestResponse (const std::string &command);

	~JsonTestResponse () = default;

	void successfulSet (bool successful);
	bool successfulIsEmpty ();
	void errorMessageSet (const std::string &message);
	bool errorMessageIsEmpty ();
	void invalidParametersSet ();
};
