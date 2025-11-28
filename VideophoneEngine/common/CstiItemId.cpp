/*!
*  \file CstiItemId.cpp
*  \brief The Item ID class
*
*  Class implmentation for the Item ID class.
*  CstiItemId allows the appliation to store and compare an ID without worring about the ID type.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*/
#include <cctype>
#include <cstring>

#include "stiError.h"
#include "stiDefs.h"
#include "stiTrace.h"
#include "CstiItemId.h"



CstiItemId::CstiItemId (const char *pszItemId) :
	m_nItemIdType (estiITEM_ID_TYPE_CHAR)
{
	if (pszItemId)
	{
		m_pszItemId = new char[strlen(pszItemId) + 1];
		if (m_pszItemId)
		{
			strcpy(m_pszItemId, pszItemId);
		}
		else
		{
			Clear();
		}
	}
}

CstiItemId::CstiItemId (uint32_t un32ItemId) :
	m_nItemIdType (estiITEM_ID_TYPE_INT),
	m_un32ItemId (un32ItemId)
{

}

CstiItemId::CstiItemId (const CstiItemId& itemId) 
{
	if (itemId.m_nItemIdType == estiITEM_ID_TYPE_CHAR)
	{
		m_nItemIdType = estiITEM_ID_TYPE_CHAR;
		if (itemId.m_pszItemId != nullptr)
		{	
			m_pszItemId = new char[strlen(itemId.m_pszItemId) + 1];
			strcpy(m_pszItemId, itemId.m_pszItemId);
		}
	}
	else if (itemId.m_nItemIdType == estiITEM_ID_TYPE_INT)
	{
		m_nItemIdType = estiITEM_ID_TYPE_INT;
		m_un32ItemId = itemId.m_un32ItemId;
	}
}

CstiItemId& CstiItemId::operator = (const CstiItemId& itemId)
{
	if (this != &itemId)
	{
		if (m_pszItemId != nullptr)
		{
			delete [] m_pszItemId;
			m_pszItemId = nullptr;
		}

		m_nItemIdType = itemId.m_nItemIdType;
		m_un32ItemId = itemId.m_un32ItemId;
		
		if (itemId.m_pszItemId != nullptr)
		{	
			m_pszItemId = new char[strlen(itemId.m_pszItemId) + 1];
			strcpy(m_pszItemId, itemId.m_pszItemId);
		}		
	}
	return *this;
}

CstiItemId::~CstiItemId ()
{
	Clear();
}

/*!
 * \brief Returns an Item ID of type string.
 * 
 * \param char pszItemIdBuf 
 * \param int nItemIdBufSize
 * 
 * \return stiRESULT_SUCCESS if the Item ID is a string and copied successfully, 
 *  	   stiRESULT_INVALID_PARAMETER if Item ID is not a char string,
 * 		   stiRESULT_BUFFER_TOO_SMALL if pszItemIdBuf is null or nItemIdBufSize is less than the Item ID length. 
 *  
 */
stiHResult CstiItemId::ItemIdGet(
		char *pszItemIdBuf,
		unsigned int unItemIdBufSize) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Make sure we were given a buffer
	stiTESTCOND (pszItemIdBuf, stiRESULT_INVALID_PARAMETER);

	// Zero out the buffer, just in case
	pszItemIdBuf[0] = 0;

	// Make sure the itemId is a char if not error out.
	if (m_nItemIdType == estiITEM_ID_TYPE_CHAR && m_pszItemId)
	{
		// Make sure the buffer is large enough.
		stiTESTCOND (unItemIdBufSize > strlen(m_pszItemId), stiRESULT_BUFFER_TOO_SMALL);

		// Copy the Item ID.
		strcpy(pszItemIdBuf, m_pszItemId);
	}
	
STI_BAIL:

	return hResult;
}

/*!
 * \brief Returns an Item ID of type string.
 *
 * \param char pszItemIdBuf
 *
 * \return stiRESULT_SUCCESS if the Item ID is a string and copied successfully,
 *  	   stiRESULT_INVALID_PARAMETER if Item ID is not a char string,
 */
stiHResult CstiItemId::ItemIdGet(
		std::string *pszItemIdBuf) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Make sure we were given a buffer
	stiTESTCOND (pszItemIdBuf, stiRESULT_INVALID_PARAMETER);

	// Zero out the buffer, just in case
	pszItemIdBuf->clear();

	// Make sure the itemId is a char if not error out.
	stiTESTCOND (m_nItemIdType == estiITEM_ID_TYPE_CHAR && m_pszItemId, stiRESULT_INVALID_PARAMETER);

	// Copy the Item ID.
	*pszItemIdBuf = m_pszItemId;

STI_BAIL:

	return hResult;
}

/*!
 * \brief Sets an Item ID of type string.
 * 
 * \param char pszItemId 
 * 
 * \return stiRESULT_SUCCESS if the Item ID is stored successfully, 
	 *  	   stiRESULT_ERROR if pszItemId is null or the current stored ItemId is of a different type,
	 *  	   stiRESULT_MEMORY_ALLOC_ERROR if a storage buffer could not be allocated.
 */
stiHResult CstiItemId::ItemIdSet(
		const char *pszItemId)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Make sure the pszItemId is not null.
	stiTESTCOND (pszItemId, stiRESULT_ERROR);

	// Make sure the item ID is the correct type.
	stiTESTCOND (m_nItemIdType == estiITEM_ID_TYPE_NONE || m_nItemIdType == estiITEM_ID_TYPE_CHAR, stiRESULT_ERROR);

	if (m_pszItemId)
	{
		delete []m_pszItemId;
		m_pszItemId = nullptr;
	}

	m_pszItemId = new char[strlen(pszItemId) + 1];

	// Make sure we were given a buffer and that it is large enough.
	stiTESTCOND (m_pszItemId, stiRESULT_MEMORY_ALLOC_ERROR);

	// Copy the Item ID.
	strcpy(m_pszItemId, pszItemId);

	m_nItemIdType = estiITEM_ID_TYPE_CHAR;

STI_BAIL:

	return hResult;
}

/*!
 * \brief Returns the Item ID as an integer. 
 *  
 * \param pun32ItemId is an integer that the Item ID will be returned in. 
 *  
 * \return stiRESULT_SUCCESS if the Item ID is an interger and copied into the pnItemId successfully, 
 *         stiRESULT_ERROR if pnItemId is NULL,
 *         stiRESULT_INVALID_PARAMETER if the internal Item ID is an integer. 
 */
stiHResult CstiItemId::ItemIdGet(
	uint32_t *pun32ItemId) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Make sure the itemId is a int if not error out.
	stiTESTCOND (m_nItemIdType == estiITEM_ID_TYPE_INT, stiRESULT_INVALID_PARAMETER);

	// Make sure we were given a pointer to an integer.
	stiTESTCOND (pun32ItemId, stiRESULT_ERROR);

	// Copy the Item ID.
	*pun32ItemId = m_un32ItemId;

STI_BAIL:

	return hResult;
}

/*!
 * \brief Sets the Item ID as an integer. 
 *  
 * \param un32ItemId is an integer that contains the Item ID. 
 *  
 * \return stiRESULT_SUCCESS if the Item ID integer is stored successfully, 
 *  	   stiRESULT_ERROR if the internal Item ID is not an integer.
 */
stiHResult CstiItemId::ItemIdSet(
	uint32_t un32ItemId)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Make sure the item ID is the correct type.
	stiTESTCOND (m_nItemIdType == estiITEM_ID_TYPE_NONE || m_nItemIdType == estiITEM_ID_TYPE_INT, stiRESULT_ERROR);

	m_un32ItemId = un32ItemId;

	m_nItemIdType = estiITEM_ID_TYPE_INT;

STI_BAIL:

	return hResult;
}

/*!
 * \brief Compares the Item IDs contained in two CstiItemIds to see if they are equal. 
 *  
 * \return true if they are equal, otherwise false. 
 */
bool CstiItemId::operator==(const CstiItemId &itemId) const
{
	bool bSameId = false;

	// If the Item IDs are the same type then check to see if they are equal.
	if (m_nItemIdType == itemId.m_nItemIdType)
	{
		if (m_nItemIdType == estiITEM_ID_TYPE_CHAR)
		{
			if (IsValid() && itemId.IsValid() && // Make sure both have valid item IDs to prevent crash on strcasecmp
				0 == strcasecmp(m_pszItemId, itemId.m_pszItemId))
			{
				bSameId = true;
			}
		}
		else if (m_nItemIdType == estiITEM_ID_TYPE_INT)
		{
			if (m_un32ItemId == itemId.m_un32ItemId)
			{
				bSameId = true;
			}
		}
	}
	return bSameId;
}

/*!
 * \brief Compares the Item IDs contained in two CstiItemIds to see if they are not equal. 
 *  
 * \return true if they are not equal, otherwise false. 
 */
bool CstiItemId::operator!=(const CstiItemId &itemId) const
{
	bool bDifferentId = true;

	// If the Item IDs are the same type then check to see if they are equal.
	if (m_nItemIdType == itemId.m_nItemIdType)
	{
		if (m_nItemIdType == estiITEM_ID_TYPE_CHAR)
		{
			if (IsValid() && itemId.IsValid() && // Make sure both have valid item IDs to prevent crash on strcasecmp
				0 == strcasecmp(m_pszItemId, itemId.m_pszItemId))
			{
				bDifferentId = false;
			}
		}
		else if (m_nItemIdType == estiITEM_ID_TYPE_INT)
		{
			if (m_un32ItemId == itemId.m_un32ItemId)
			{
				bDifferentId = false;
			}
		}
	}
	return bDifferentId;
}

/*!
 * \brief Determines if the ItemId has been set or not
 *
 * \return true if the ItemID has been set, otherwise false.
 */
bool CstiItemId::IsValid() const
{
	bool bValid = false;

	switch (m_nItemIdType)
	{
	case estiITEM_ID_TYPE_CHAR:
		bValid = (m_pszItemId != nullptr);
		break;

	case estiITEM_ID_TYPE_INT:
		bValid = (m_un32ItemId != 0);
		break;

	case estiITEM_ID_TYPE_NONE:
	default:
		bValid = false;
		break;
	}

	return bValid;
}

/*!
 * \brief Empties the ItemID
 */
void CstiItemId::Clear()
{
	m_nItemIdType = estiITEM_ID_TYPE_NONE;

	m_un32ItemId = 0;

	if (m_pszItemId)
	{
		delete [] m_pszItemId;
		m_pszItemId = nullptr;
	}
}

/*!
 * \brief Type of the ItemID
 *
 * \return the type of the ItemID.
 */
int CstiItemId::ItemIdTypeGet()
{
	return m_nItemIdType;
}


