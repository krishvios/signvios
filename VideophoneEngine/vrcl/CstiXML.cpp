////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: CstiXML
//
//  File Name:  CstiXML.cpp
//
//  Owner:	  Justin L. Ball
//
//  Abstract:
//	This class handles basic XML data.  It is in no way a complete, or even
//	standards compliant XML parser.
//
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
#include <cstdio>
#include "CstiXML.h"
#include "stiTools.h"
#include <cstdlib>
#include "stiTrace.h"

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

//
// Locals
//

//
// Class Definitions
//


///////////////////////////////////////////////////////////////////////////////
//; CstiXML::GenerateIntegerXMLPackage
//
// Description: packages an integer value into XML format.
//
// Abstract:
//		This is a helper function that will allow any integer value
//		to be packaged between two xml tags.
//
// Returns:
//
//	estiOK (Success) or estiERROR (Failure)
//
EstiResult CstiXML::GenerateIntegerXMLPackage(
		OUT char * szBuffer,		// Output buffer for package
		IN const char * szOpen,		// Opening XML Tag
		IN const char * szClose,	// Closing XML Tag
		IN int nValue)				// Integer value to enclose in the tag
{
	EstiResult eResult = estiOK;

	sprintf(szBuffer, "%s%d%s", szOpen, nValue, szClose);

	return eResult;

} // end CstiXML::GenerateIntegerXMLPackage


////////////////////////////////////////////////////////////////////////////////
//; CstiXML::GetXMLInt
//
// Description: Gets an integer value from an xml tag
//
// Abstract:
//		Control messages are in xml format.
//		This function requires and opening and a closing tag.  From these values
//		and the input buffer containing the tags and the value, we can determine
//		the integer value.
//
// Returns:
//		estiOK on success
//		estiERROR on failure
//
EstiResult CstiXML::GetXMLInt(
	IN const char * szXMLString,  	// Original xml string
	IN const char * szOpenTag,		// Opening xml tag
	IN const char * szCloseTag,	// Closing xml tag
	OUT int & nValue)						// Value retreived from string
{
	stiLOG_ENTRY_NAME(CstiXML::GetXMLInt);
	EstiResult	eResult = estiOK;

	char achValue[nMAX_XML_VALUE];

	if(estiOK == GetXMLString(szXMLString, szOpenTag, szCloseTag, achValue))
	{
		nValue = atoi(achValue);
	} // end if

	return eResult;

} // end CstiXML::GetXMLInt


////////////////////////////////////////////////////////////////////////////////
//; CstiXML::GetXMLString
//
// Description: Gets an string value from an xml tag
//
// Abstract:
//		Control messages are in xml format.
//		This function requires and opening and a closing tag.  From these values
//		and the input buffer containing the tags and the value.
//
// Returns:
//		estiOK on success
//		estiERROR on failure
//
EstiResult CstiXML::GetXMLString(
	IN const char * szXMLString,  	// original xml string
	IN const char * szOpenTag,		// opening xml tag
	IN const char * szCloseTag,	// closing xml tag
	OUT char * szValue)			// Value retreived from string
{
	stiLOG_ENTRY_NAME(CstiXML::GetXMLString);
	EstiResult	eResult = estiOK;

	unsigned int nBegin = 0;
	unsigned int nEnd = 0;
	unsigned int nStrLen = 0;

	// Make sure that szOpenTag tag is the first tag in the string,
	// and that szCloseTag is the last tag in the string
	if(0 == strncmp(szXMLString, szOpenTag, strlen(szOpenTag)) &&
		0 == strncmp(
			(szXMLString + (strlen(szXMLString) - strlen(szCloseTag))),
			szCloseTag,
			strlen(szCloseTag)))
	{
		nBegin = strlen(szOpenTag);
		nEnd = (strlen(szXMLString) - strlen(szCloseTag));

		nStrLen = nEnd - nBegin;

		const char * pchBeginValue = (char *)(szXMLString + strlen(szOpenTag));

		if(nStrLen < nMAX_XML_VALUE)
		{
			// copy string into buffer
			if(nullptr == strncpy(szValue, pchBeginValue, nStrLen))
			{
				stiDEBUG_TOOL (g_stiXMLOutput,
					stiTrace ("CstiXML::GetXMLString "
						"An error occured copying xml string into buffer.\n");
				); // stiDEBUG_TOOL

				// There was an error copying the string
				eResult = estiERROR;


			} // end if

			// null terminate the buffer
			*(szValue + nStrLen) = '\0';
		} // end if
		else
		{

			stiDEBUG_TOOL (g_stiXMLOutput,
				stiTrace ("CstiXML::GetXMLString "
					"xml string was to long for buffer.\n");
			); // stiDEBUG_TOOL

			eResult = estiERROR;
		} // end else

	}//end if
	else
	{
		stiDEBUG_TOOL (g_stiXMLOutput,
			stiTrace ("CstiXML::GetXMLString "
				"Closing tag did not match opening tag.\n");
		); // stiDEBUG_TOOL

		eResult = estiERROR;
	}//end else


	if (estiERROR == eResult)
	{
		szValue = nullptr;
	} // end if
	return eResult;

} // end CstiXML::GetXMLString


////////////////////////////////////////////////////////////////////////////////
//; CstiXML::GetXMLIntEx
//
// Description: Gets an integer value from an xml tag
//
// Abstract:
//		Control messages are in xml format.
//		This function does not require an opening and a closing tag.
//
// Returns:
//		estiOK on success
//		estiERROR on failure
//
EstiResult CstiXML::GetXMLIntEx(
	IN const char * szXMLString,  	// Original xml string
	OUT int & nValue,				// Value retreived from string
	OUT const char ** szNewXMLString)		// pointer to new end of XML String (unprocessed Data)
{
	stiLOG_ENTRY_NAME(CstiXML::GetXMLIntEx);
	EstiResult	eResult = estiOK;

	char achValue[nMAX_XML_VALUE];

	if(estiOK == GetXMLStringEx(szXMLString, achValue, szNewXMLString))
	{
		nValue = atoi(achValue);
	} // end if
	else
	{
		nValue = 0;
		eResult = estiERROR;
		*szNewXMLString = nullptr;
	} // end if

	return eResult;

} // end CstiXML::GetXMLIntEx


////////////////////////////////////////////////////////////////////////////////
//; CstiXML::GetXMLStringEx
//
// Description: Gets an string value from an xml tag
//
// Abstract:
//		Control messages are in xml format.
//		This function does not require an opening and a closing tag.
//
// Returns:
//		estiOK on success
//		estiERROR on failure
//
EstiResult CstiXML::GetXMLStringEx(
	IN const char * szXMLString,  	// original xml string
	OUT char * szValue,				// Value retreived from string
	OUT const char ** szNewXMLString)		// Pointer to new end of XML string (unprocessed data)
{
	stiLOG_ENTRY_NAME(CstiXML::GetXMLStringEx);
	EstiResult	eResult = estiOK;

	// Get the first and last tag info
//	char * szFirstTagEnd = strpbrk(szXMLString, ">");
//	char * szLastTagBegin = strrchr(szXMLString, '<');
//	char * szLastTagEnd = szLastTagBegin + strlen(szLastTagBegin);
	*szNewXMLString = nullptr;

	if (nullptr == szXMLString)
	{
		return estiERROR;
	} // end if

	const char * szOpenTagBegin	= strpbrk(szXMLString, "<");
	if (nullptr == szOpenTagBegin)
	{
		return estiERROR;
	} // end if

	const char * szOpenTagEnd		= strpbrk(szOpenTagBegin+1, ">");
	if (nullptr == szOpenTagEnd)
	{
		return estiERROR;
	} // end if

	const char * szCloseTagBegin	= strpbrk(szOpenTagEnd+1, "<");
	if (nullptr == szCloseTagBegin)
	{
		return estiERROR;
	} // end if

	const char * szCloseTagEnd	= strpbrk(szCloseTagBegin+1, ">");
	if (nullptr == szCloseTagEnd)
	{
		return estiERROR;
	} // end if

	//
	// Check to make sure that the tags match, and that they are properly
	// formated.
	//

	// Check opening tag start bracket
	if(*szOpenTagBegin != '<')
	{
		// bracket is not in the proper location
		eResult = estiERROR;
	} // end if

	// Check opening tag end bracket
	if(*szOpenTagEnd != '>')
	{
		// bracket is not in the proper location
		eResult = estiERROR;
	} // end if

	// Check closing tag start bracket
	if(*szCloseTagBegin != '<')
	{
		// bracket is not in the proper location
		eResult = estiERROR;
	} // end if

	// Check for / after closing tag start bracket
	if(*(szCloseTagBegin+1) != '/')
	{
		// bracket is not in the proper location
		eResult = estiERROR;
	} // end if

	// Check closing tag end bracket
	if(*szCloseTagEnd != '>')
	{
		// bracket is not in the proper location
		eResult = estiERROR;
	} // end if

	// compare the tag strings i.e. <xxxx> to </xxxx>
	if(0 != strncmp((szOpenTagBegin + 1), (szCloseTagBegin + 2),
			szOpenTagEnd - (szOpenTagBegin+1)))
	{
		// the tags are not the same
		eResult = estiERROR;
	} // end if

	if(estiOK == eResult)
	{
		unsigned int unStrLen = szCloseTagBegin - (szOpenTagEnd + 1);
		if(unStrLen < nMAX_XML_VALUE)
		{
			// The xml string is valid.  Retrieve the value from the tags
			if(nullptr == strncpy(szValue, szOpenTagEnd+1, unStrLen))
			{
				// There was an error copying the string
				eResult = estiERROR;
			} // end if

			// null terminate the buffer
			*(szValue + unStrLen) = '\0';

			if (strlen(szCloseTagEnd + 1) > 0)
			{
			   *szNewXMLString = szCloseTagEnd + 1;
			} // end if
			else
			{
			   *szNewXMLString = nullptr;
			} // end else

		} // end if
		else
		{
			eResult = estiERROR;
		} // end else

	} // end if

	return eResult;

} // end CstiXML::GetXMLStringEx



////////////////////////////////////////////////////////////////////////////////
//; CstiXML::RemoveNextTag
//
// Description: Determines if the string is a single tag
//
// Abstract:
//
//
// Returns:
//		estiOK on success
//		estiERROR on failure
//
EstiResult CstiXML::RemoveNextTag(
	IN const char * szXMLString, // String to check
	OUT const char ** szNewXMLString)	 // Pointer to next XML tag
{
	EstiResult eResult = estiOK;


	*szNewXMLString = nullptr;

	if (nullptr == szXMLString)
	{
		return estiERROR;
	} // end if


	// Look for the first tag in the string
	const char * szOpenTagBegin = strpbrk(szXMLString, "<");
	if (nullptr == szOpenTagBegin)
	{
		return estiERROR;
	} // end if

	const char * szOpenTagEnd	= strpbrk(szOpenTagBegin+1, ">");
	if (nullptr == szOpenTagEnd)
	{
		return estiERROR;
	} // end if


	// Now check to see if there is a / at the end of the tag
	// This test will tell us if the tag is of the form <xxxx/> or of the form <xxxx></xxxx>
	if (*(szOpenTagEnd - 1) == '/')
	{
		// Tag is of the form <xxxx/>
		// Just point the output pointer past the last >
		*szNewXMLString = szOpenTagEnd + 1;
	} // end if
	else
	{
		// tag is of the form <xxxx></xxxx>
		// use get string to remove the tag
		char achValue[nMAX_XML_VALUE];
		eResult = GetXMLStringEx(szXMLString, (char *)(&achValue), szNewXMLString);

	} // end else


	return eResult;
} // end CstiXML::RemoveNextTag




////////////////////////////////////////////////////////////////////////////////
//; CstiXML::IsSingleXMLTag
//
// Description: Determines if the string is a single tag
//
// Abstract:
//
//
// Returns:
//		estiOK on success
//		estiERROR on failure
//
EstiResult CstiXML::IsSingleXMLTag(
	IN const char * szXMLString, // String to check
	OUT EstiBool &bIsSingleTag)	 // Boolean indicating if this is a single tag
{
	stiLOG_ENTRY_NAME(CstiXML::IsSingleXMLTag);

	EstiResult	eResult = estiOK;
	EstiBool 	bContinue = estiTRUE;
	char * 		szTagTest = nullptr;
	int 		nTagCounter = 0;
	int 		nOriginalStrLen = 0;
	int 		nCurrentStrLen = 0;

	// Get the first and last tag info
	szTagTest = (char *)szXMLString;
	while(estiTRUE == bContinue)
	{
		// Get the pointer to the first occurence of ","
		szTagTest = strpbrk(szTagTest, "<");

		if(nullptr == szTagTest)
		{
			bContinue = estiFALSE;
		} // end if
		else
		{
			// We have found an open tag ("<")
			nTagCounter++;

			// Increment the pointer so that we don't keep finding the
			// same "<"
			szTagTest++;

			// Make sure that we have not gone past the end of the string
			nOriginalStrLen = strlen(szXMLString);
			nCurrentStrLen = szTagTest-szXMLString;

			if(nCurrentStrLen > nOriginalStrLen)
			{
				bContinue = estiFALSE;
			} // end if

		} // end else

	} // end while

	if(nTagCounter > 1)
	{
		bIsSingleTag = estiFALSE;
	} // end if
	else
	{
		bIsSingleTag = estiTRUE;
	} // end else

	return eResult;

} // end CstiXML::GetXMLStringEx

// end file CstiXML.cpp



////////////////////////////////////////////////////////////////////////////////
//; CstiXML::CompareTags
//
// Description: Compares two tags to see if they are the same and gives a pointer
//				to the position in the buffer after the test tag
//
// Abstract:
//
//
// Returns:
//		estiTRUE if the tags are the same
//		estiFALSE otherwise
//
EstiBool CstiXML::CompareTags(
	IN char *szOriginalTag,	// tag to compare (this tag will be in a buffer with other tags)
	IN char *szTestTag,		// tag to be tested against.
	IN int nTestTagLen,
	OUT char ** szNewXMLString) // pointer to beginning of new tag in string or null
								// if there are no more tags
{

	EstiBool bRet = estiFALSE;
	*szNewXMLString = szOriginalTag;

	// test string lengths
	if (strlen(szOriginalTag) == strlen(szTestTag))
	{
		if (0 == strncmp(szOriginalTag, szTestTag, nTestTagLen))
		{
			bRet = estiTRUE;
			*szNewXMLString = szOriginalTag + nTestTagLen + 1;
		} // end if

	} // end if

	return bRet;

} // end CstiXML::CompareTags
