/*!
*  \file CstiItemId.h
*  \brief The Item ID class
*
*  Class declaration for the Item ID class.
*  CstiItemId allows the appliation to store and compare an ID without worring about the ID type.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*/

#ifndef CSTIITEMID_H
#define CSTIITEMID_H

#include <string>

enum EItemIdType
{
	estiITEM_ID_TYPE_NONE,                   //!
	estiITEM_ID_TYPE_CHAR,                   //!
	estiITEM_ID_TYPE_INT                     //!
};


/*!
* \ingroup VideophoneEngineLayer 
 * \brief Item ID Class
*/
class CstiItemId
{
public:

	/*!
	 * \brief Constructor
	 */
	CstiItemId () = default;

	/*!
	 * \brief Constructor
	 */
	CstiItemId (const char *pszItemId);

	/*!
	 * \brief Constructor
	 */
	CstiItemId (uint32_t un32ItemId);

	/*!
	 * \brief Copy Constructor
	 */
	CstiItemId (const CstiItemId& itemId);
	
	/*!
	 * \brief = operator used as a copy constructor.
	 */
	CstiItemId& operator =(const CstiItemId& pItemId);
	
	/*!
	 * \brief Destructor
	 */
	virtual ~CstiItemId ();
	
	/*!
	 * \brief Returns the Item ID as a char string. 
	 *  
	 * \param pszItemIdBuf A buffer that will be filled with the Item ID.
	 * \param unItemIdBufSize The size of pszItemIdBuf.
	 *  
	 * \return stiRESULT_SUCCESS if the Item ID is a string and copied successfully, 
	 *  	   stiRESULT_INVALID_PARAMETER if Item ID is not a char string,
	 * 		   stiRESULT_BUFFER_TOO_SMALL if pszItemIdBuf is null or unItemIdBufSize is less than the Item ID length.
	 */
	stiHResult ItemIdGet(
		char *pszItemIdBuf,
		unsigned int unItemIdBufSize) const;

	/*!
	 * \brief Returns the Item ID as a std::string.
	 *
	 * \param pszItemIdBuf A string that will be filled with the Item ID.
	 *
	 * \return stiRESULT_SUCCESS if the Item ID is a string and copied successfully,
	 *  	   stiRESULT_INVALID_PARAMETER if Item ID is not a char string,
	 */
	stiHResult ItemIdGet(
		std::string *pszItemIdBuf) const;

	/*!
	 * \brief Sets the Item ID as a string. 
	 *  
	 * \param pszItemId is a pointer to a buffer that that contains the Item ID. 
	 *  
	 * \return stiRESULT_SUCCESS if the Item ID string buffer is allocated and copied successfully, 
	 *  	   stiRESULT_ERROR if pszItemId is null or the current stored ItemId is of a different type,
	 *  	   stiRESULT_MEMORY_ALLOC_ERROR if a storage buffer could not be allocated.
	 */
	stiHResult ItemIdSet(
		const char *pszItemId);

	/*!
	 * \brief Returns the Item ID as an integer. 
	 *  
	 * \param pun32ItemId is an integer that the Item ID will be returned in. 
	 *  
	 * \return stiRESULT_SUCCESS if the Item ID is an interger and copied into the pnItemId successfully, 
	 *         stiRESULT_ERROR if pnItemId is NULL,
	 *         stiRESULT_INVALID_PARAMETER if the internal Item ID is an integer. 
	 */
	stiHResult ItemIdGet(
		uint32_t *pun32ItemId) const;

	/*!
	 * \brief Sets the Item ID as an integer. 
	 *  
	 * \param un32ItemId is an integer that contains the Item ID. 
	 *  
	 * \return stiRESULT_SUCCESS if the Item ID integer is stored successfully, 
	 *  	   stiRESULT_ERROR if the internal Item ID is not an integer.
	 */
	stiHResult ItemIdSet(
		uint32_t un32ItemId);

	/*!
	 * \brief Compares the Item IDs contained in two CstiItemIds to see if they are equal. 
	 *  
	 * \return true if they are equal, otherwise false. 
	 */
	bool operator==(const CstiItemId &itemId) const;

	/*!
	 * \brief Compares the Item IDs contained in two CstiItemIds to see if they are not equal. 
	 *  
	 * \return true if they are not equal, otherwise false. 
	 */
	bool operator!=(const CstiItemId &itemId) const;

	/*!
	 * \brief Determines if the ItemId has been set or not
	 *
	 * \return true if the ItemID has been set, otherwise false.
	 */
	bool IsValid() const;

	/*!
	 * \brief Empties the contents of the ItemId
	 */
	void Clear();
	
	/*!
	 * \brief Type of the ItemID
	 *
	 * \return the type of the ItemID.
	 */
	int ItemIdTypeGet();

private:


	int m_nItemIdType{estiITEM_ID_TYPE_NONE};
	uint32_t m_un32ItemId{0};
	char *m_pszItemId{nullptr};
};

#endif // ISTIMESSAGECATEGORY_H
