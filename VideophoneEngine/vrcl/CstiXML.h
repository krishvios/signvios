////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	stiXML
//
//  File Name:	CSTIXML_H
//
//  Owner:		Justin L Ball
//
//	Abstract:
//      This class encapsulates the control protocol for the client server 
//		communication.

//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTIXML_H
#define CSTIXML_H

//
// Includes
//
#include "stiDefs.h"
#include <cstring>

//
// Constants
//
	
//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//
#define nMAX_XML_VALUE 100

//
// Class Declaration
//

class CstiXML
{
public:

	EstiBool CompareTags(IN char *szOriginalTag, 
		IN char *szTestTag, 
		IN int nTestTagLen, 
		OUT char ** szNewXMLString);

	CstiXML () = default;
	virtual ~CstiXML () = default;

	EstiResult GetXMLString(
		IN const char * szXMLString,  
		IN const char * szOpenTag, 
		IN const char * szCloseTag, 
		OUT char * szValue);

	EstiResult GetXMLInt(
		IN const char * szXMLString, 
		IN const char * szOpenTag, 
		IN const char * szCloseTag, 
		OUT int & nValue);

	EstiResult GetXMLStringEx(
		IN const char * szXMLString,
		OUT char * szValue,
		OUT const char **szNewXMLString = nullptr);

	EstiResult GetXMLIntEx(
		IN const char * szXMLString,
		OUT int & nValue,
		OUT const char **szNewXMLString = nullptr);

	EstiResult IsSingleXMLTag(
		IN const char * szXMLString, 
		OUT EstiBool &bIsSingleTag);

	EstiResult GenerateIntegerXMLPackage(
		OUT char * szBuffer, IN const char * szOpen, 
		IN const char * szClose, IN int nValue);

	EstiResult RemoveNextTag(
		IN const char * szXMLString,
		OUT const char ** szNewXMLString);

private:


};

#endif // CSTIXML_H
// end file CSTIXML_H
