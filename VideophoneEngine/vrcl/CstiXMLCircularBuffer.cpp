////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: CstiXMLCircularBuffer
//
//  File Name:  stiXMLCircularBuffer.cpp
//
//  Owner:	  Justin L. Ball
//
//  Abstract:
//	This class provides a circular buffer with XML capabilities.
//	
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
#include "CstiXMLCircularBuffer.h"
#include "stiTools.h" 
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




////////////////////////////////////////////////////////////////////////////////
//; CstiXMLCircularBuffer::GetNextXMLTag
//
// Description: Gets the next XML tag from the buffer
//
// Abstract:
//				All incoming TCP data should be in an XML format.  This function
//				parses that data and finds an XML tag and then returns the
//				tag along with it's matching data.
//
// Returns:
//				estiOK if success or estiERROR if failure
//
stiHResult CstiXMLCircularBuffer::GetNextXMLTag(
	char * szTag,				// Pointer to a buffer where the tag and data will be written
	uint32_t un32TagBufferSize, // Size of input buffer
	uint32_t & un32TagSize,		// Size of XML tag including data
	EstiBool & bTagFound,		// Indicator of whether or not a tag was found
	char ** pchData,			// Pointer to the data inside the tag buffer (Data will reside in the szTag buffer
	uint32_t & un32DataSize,	// Length of the data in the tag buffer (without the tags)
	EstiBool & bInvalidTag)		// Flag indicating if the tag found was valid
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	// Pointers to XML tag positions
	char * pchOpenTagBegin = nullptr;
	char * pchOpenTagEnd = nullptr;
	char * pchCloseTagBegin = nullptr;
	char * pchCloseTagEnd = nullptr;

	EstiBool bFoundSingleTags = estiFALSE;
	EstiBool bFoundMatchingTags = estiFALSE;

	uint32_t un32BytesRead = 0;
	uint32_t un32NumberOfBytesToSeek = 0;

	bInvalidTag = estiFALSE;
	bTagFound = estiFALSE;

	// Copy data from the circular buffer into the buffer that was passed in
	if(estiOK == Peek (szTag, un32TagBufferSize-1, un32BytesRead))
	{
		// Null terminate the buffer
		szTag[un32BytesRead] = '\0';
		
		stiDEBUG_TOOL (g_stiXMLCircularBufferOutput,
			stiTrace("CstiXMLCircularBuffer::GetNextXMLTag:\n"
				"\tAttempted to read %ld bytes from the circular buffer.\n"
				"\tRead %ld bytes from the circular buffer.\n"
				"\tString read from the circular buffer:  %s.\n", 
				un32TagBufferSize-1,
				un32BytesRead,
				szTag);
		); // end stiDEBUG_TOOL
		
		//
		// Look for an open tag
		//

		// find opening bracket
		pchOpenTagBegin = strpbrk(szTag, "<");
		if (nullptr == pchOpenTagBegin)
		{
			stiDEBUG_TOOL (g_stiXMLCircularBufferOutput,
				stiTrace("CstiXMLCircularBuffer::GetNextXMLTag: "
					"Opening bracket not found.\n");
			); // end stiDEBUG_TOOL

			bInvalidTag = estiTRUE;
		} // end if
		else
		{
			// find closing bracket
			pchOpenTagEnd = strpbrk(pchOpenTagBegin+1, ">");
			if (nullptr == pchOpenTagEnd)
			{
				stiDEBUG_TOOL (g_stiXMLCircularBufferOutput,
					stiTrace("CstiXMLCircularBuffer::GetNextXMLTag: "
						"Closing bracket not found.\n");
				); // end stiDEBUG_TOOL

				bInvalidTag = estiTRUE;
			} // end if
			else
			{
				// Now check to see if there is a / at the end of the tag
				// This test will tell us if the tag is of the form <xxxx/> or 
				// of the form <xxxx></xxxx>
				if (*(pchOpenTagEnd - 1) == '/')
				{
					// Tag is of the form <xxxx/>
					
					stiDEBUG_TOOL (g_stiXMLCircularBufferOutput,
						stiTrace("CstiXMLCircularBuffer::GetNextXMLTag: "
							"Single Tag Found.\n");
					); // end stiDEBUG_TOOL

					// Just point the output pointer past the last >
					bFoundSingleTags = estiTRUE;
				
				} // end if
				else
				{
					// tag is of the form <xxxx></xxxx>

					//
					// Look for closing tag
					//

					pchCloseTagBegin = strpbrk(pchOpenTagEnd+1, "<");
					if (nullptr == pchCloseTagBegin)
					{
						stiDEBUG_TOOL (g_stiXMLCircularBufferOutput,
							stiTrace("CstiXMLCircularBuffer::GetNextXMLTag: "
								"Opening bracket of end tag not found.\n");
						); // end stiDEBUG_TOOL

						bInvalidTag = estiTRUE;
					} // end if
					else
					{
						pchCloseTagEnd = strpbrk(pchCloseTagBegin+1, ">");
						if (nullptr == pchCloseTagEnd)
						{
							stiDEBUG_TOOL (g_stiXMLCircularBufferOutput,
								stiTrace("CstiXMLCircularBuffer::GetNextXMLTag: "
									"Closing bracket of end tag not found.\n");
							); // end stiDEBUG_TOOL

							bInvalidTag = estiTRUE;
						} // end if
						else
						{
							if (*(pchCloseTagBegin + 1) == '/')
							{
								// Tag is of the form <xxxx></xxxx>
								bFoundMatchingTags = estiTRUE;
							} // end if
							else
							{
								// Get the full tag, we need to search for it further down the buffer
								char *pchOpenTagNameEnd = strpbrk (pchOpenTagBegin, " >");
								if (pchOpenTagNameEnd)
								{
									// Tag is of the form <xxxx><subtag /></xxxx>
									// or <xxxx><subtag>data</subtag></xxxx>
									// or multiple subtags withing the tag begin and end
									int nTagNameLen = pchOpenTagNameEnd - pchOpenTagBegin - 1;
									auto pszTagName = (char*)malloc (sizeof (char) * (nTagNameLen + 3));
									stiTESTCOND (pszTagName != nullptr, stiRESULT_ERROR);
									
									strcpy (pszTagName, "</");
									memcpy (pszTagName + 2, pchOpenTagBegin + 1, nTagNameLen);
									pszTagName[nTagNameLen + 2] = 0;
									pchCloseTagBegin = strstr (pchOpenTagEnd + 1, pszTagName);
									free (pszTagName);
									if (pchCloseTagBegin)
									{
										pchCloseTagEnd = strpbrk(pchCloseTagBegin+1, ">");
										if (nullptr == pchCloseTagEnd)
										{
											stiDEBUG_TOOL (g_stiXMLCircularBufferOutput,
												stiTrace("CstiXMLCircularBuffer::GetNextXMLTag: "
													"Closing bracket of end tag not found.\n");
											); // end stiDEBUG_TOOL
											bInvalidTag = estiTRUE;
										}
										else
										{
											bFoundMatchingTags = estiTRUE;
										}
									}
									else
									{
								stiDEBUG_TOOL (g_stiXMLCircularBufferOutput,
									stiTrace("CstiXMLCircularBuffer::GetNextXMLTag: "
										"Slash of end tag not found.\n");
								); // end stiDEBUG_TOOL
									}
								}
							} // end else
						
						} // end else
						
					} // end else
					
				} // end else
			
			} // end else

		} // end else
	} // end if


	if (estiTRUE == bFoundMatchingTags)
	{
		if (pchCloseTagEnd > pchOpenTagBegin)
		{
			un32NumberOfBytesToSeek = pchCloseTagEnd - szTag;

			//
			// Get the value for the tags
			// 
			// compare the tag strings i.e. <xxxx> to </xxxx>
			if(0 != strncmp((const char *)(pchOpenTagBegin + 1), 
				(const char *)(pchCloseTagBegin + 2), 
//SLB				pchOpenTagEnd - (pchOpenTagBegin+1))) // Changed 7/27/04 to support attributes
				pchCloseTagEnd - (pchCloseTagBegin+2)))
			{
				stiDEBUG_TOOL (g_stiXMLCircularBufferOutput,
					stiTrace("CstiXMLCircularBuffer::GetNextXMLTag: "
						"Tags don't match!.\n");
				); // end stiDEBUG_TOOL

				// the tags are not the same
				bInvalidTag = estiTRUE;
			} // end if
			else
			{
				// The xml string is valid.  Retrieve the value from the tags
				* pchData = pchOpenTagEnd+1;
				un32DataSize = pchCloseTagBegin - (pchOpenTagEnd + 1);
				bTagFound = estiTRUE;
			} // end else
		
		} // end if

	} // end if


	if (estiTRUE == bFoundSingleTags)
	{
		un32NumberOfBytesToSeek = (pchOpenTagEnd - szTag);
		bTagFound = estiTRUE;
	} // end if


	if (bTagFound)
	{
		// Add to the bytes to seek so that we are looking at the next
		// tag and not the last char of the current tag
		un32NumberOfBytesToSeek++;

		hResult = Seek (un32NumberOfBytesToSeek);
		
		un32TagSize = un32NumberOfBytesToSeek;
		
		stiTESTRESULT ();
		
	} // end if
	else
	{
//SLB		// We have not found a tag.  At this point we need to advance the
//SLB		// buffer so that the next time we look for a tag we start looking in
//SLB		// a different spot.
//SLB		un32NumberOfBytesToSeek = 1;
//SLB
//SLB		if ( estiERROR == Seek (un32NumberOfBytesToSeek))
//SLB		{
//SLB			eResult = estiERROR;
//SLB		} // end if
		un32TagSize = 0;
	} // end else

STI_BAIL:

	// Null terminate the buffer
	szTag[un32TagSize] = '\0';

	return hResult;
} // end CstiXMLCircularBuffer::GetNextXMLTag

// End File CstiXMLCircularBuffer.cpp 
