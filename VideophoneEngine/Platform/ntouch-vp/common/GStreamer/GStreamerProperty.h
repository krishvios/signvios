#pragma once

#include <gst/gst.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Array.h>


Poco::DynamicStruct getPropertyAsJson (const GParamSpec &paramSpec, const GValue &propertyValue);

std::tuple<std::string, std::string> getTypeAndValueStrings (
	const GValue &propertyValue);
