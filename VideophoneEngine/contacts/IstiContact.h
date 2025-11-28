/*!
*  \file IstiContact.h
*  \brief The Contact List Item Interface Class
*
*  Class declaration for the Contact List Item.  Store information about
*  an individual contact.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*/

#ifndef ISTICONTACT_H
#define ISTICONTACT_H


#ifdef stiCONTACT_LIST_MULTI_PHONE
#include "CstiContactListItem.h"
#endif


/*!
 * \ingroup VideophoneEngineLayer
 * \brief Contact List Item Interface Class
*/
class IstiContact
{
public:

	/*!
	 * \brief Constructor
	 */
	IstiContact () = default;

	/*!
	 * \brief Virtual destructor
	 */
	virtual ~IstiContact () = default;

	/*!
	 * \brief Retrieves the contact's Name
	 *
	 * \return The (allocated) name string
	 */
	virtual std::string NameGet() const = 0;

	/*!
	 * \brief Retrieves the contact's Company Name
	 *
	 * \return The (allocated) company string
	 */
	virtual std::string CompanyNameGet () const = 0;

	/*!
	 * \brief Retrieves the contact's unique public identifier.
	 *
	 * \return The (allocated) public identifier
	 */
	virtual std::string PublicIDGet() const = 0;

	/*!
	 * \brief Retrieves the contact's item identifier.
	 *
	 * \return The contact's item identifier
	 */
	virtual CstiItemId PublicIdGet() const = 0;

#ifdef stiCONTACT_LIST_MULTI_PHONE

	/*!
	 * \brief Retrieves the ID of a particular phone number
	 *
	 * \deprecated
	 *
	 * \param type Which phone number are you looking for?
	 * \param pID Outputs the ID of the phone number
	 *
	 * \return Success or Not Found
	 */
	virtual int PhoneNumberIDGet(
		CstiContactListItem::EPhoneNumberType type,
		std::string *id) const = 0;

	/*!
	 * \brief Retrieves the ID of a particular phone number
	 *
	 * \param type Which phone number are you looking for?
	 *
	 * \return The ID of the phone number
	 */
	virtual CstiItemId PhoneNumberIdGet(
		CstiContactListItem::EPhoneNumberType type) const = 0;

	/*!
	 * \brief Retrieves the Public ID of a particular phone number
	 *
	 * \param type Which phone number are you looking for?
	 *
	 * \return The Public ID of the phone number
	 */
	virtual CstiItemId PhoneNumberPublicIdGet(
		CstiContactListItem::EPhoneNumberType type) const = 0;

	/*!
	 * \brief Retrieves the dial string of a particular phone number type
	 *
	 * \param type Which phone number are you looking for?
	 * \param pDialString Outputs the dial string in that phone number type
	 *
	 * \return Success or Not Found
	 */
	virtual int DialStringGet(
		CstiContactListItem::EPhoneNumberType type,
		std::string *dialString) const = 0;

	/*!
	 * \brief Retrieves the dial string with the specified phone number ID
	 *
	 * \param NumberId Id of the phone number are you looking for
	 * \param pDialString Outputs the dial string in that phone number type
	 *
	 * \return Success or Not Found
	 */
	virtual int DialStringGetById(
		const CstiItemId &NumberId,
		std::string *pDialString) = 0;

	/*!
	 * \brief Retrieves the phone number type for a specified dial string
	 *
	 * \param pDialString The phone number to look up
	 * \param pType Outputs the type of phone number
	 *
	 * \return Success or Not Found
	 */
	virtual int PhoneNumberTypeGet(
		const std::string &dialString,
		CstiContactListItem::EPhoneNumberType *pType) = 0;

#else

	/*!
	 * \brief Retrieves the Home phone number
	 *
	 * \param pNumber Outputs the Home phone number
	 *
	 * \return Success or Not Found
	 */
	virtual int HomeNumberGet(
		std::string *number) const = 0;

	/*!
	 * \brief Retrieves the Cell phone number
	 *
	 * \param pNumber Outputs the Cell phone number
	 *
	 * \return Success or Not Found
	 */
	virtual int CellNumberGet(
		std::string *number) const = 0;

	/*!
	 * \brief Retrieves the Work phone number
	 *
	 * \param pNumber Outputs the Work phone number
	 *
	 * \return Success or Not Found
	 */
	virtual int WorkNumberGet(
		std::string *number) const = 0;
#endif

	/*!
	 * \brief Retrieves the ring pattern
	 *
	 * \param pPattern Outputs the ring pattern
	 *
	 * \return Success or Not Found
	 */
	virtual int RingPatternGet(
		int *pPattern) const = 0;

	/*!
	 * \brief Retrieves the LightRing color
	 *
	 * \param pulColor Outputs the 24-bit RGB color value
	 *
	 * \return Success or Not Found
	 */
	 virtual int RingPatternColorGet(
        int *pnColor) const = 0;

	/*!
	 * \brief Retrieves the language name
	 *
	 * \param pLanguage Outputs the language name string
	 *
	 * \return Success or Not Found
	 */
	virtual int LanguageGet(
		std::string *language) const = 0;

	/*!
	 * \brief Retrieves whether or not VCO is enabled for this contact
	 *
	 * \param pVCO Outputs whether or not VCO is enabled
	 *
	 * \return Success or Not Found
	 */
	virtual int VCOGet(
		bool *pVCO) const = 0;

	/*!
	 * \brief Retrieves the photo ID
	 *
	 * \param pPhotoID Outputs the photo ID
	 *
	 * \return Success or Not Found
	 */
	virtual int PhotoGet(
		std::string *photoID) const = 0;

	/*!
	 * \brief Retrieves the photo timestamp
	 *
	 * \param pPhotoTimestamp Outputs the photo timestamp
	 *
	 * \return Success or Not Found
	 */
	virtual int PhotoTimestampGet(std::string *photoTimestamp) const = 0;

	/*!
	 * \brief Sets the contact name
	 *
	 * \param pName The name to set
	 */
	virtual void NameSet(
		const std::string &name) = 0;

	/*!
	 * \brief Sets the company name
	 *
	 * \param pCompany The name to set
	 */
	virtual void CompanyNameSet(
		const std::string &company) = 0;


#ifdef stiCONTACT_LIST_MULTI_PHONE

	/*!
	 * \brief Compares a dial string to see if it matches one
	 *
	 * \param pDialString The dial string to compare
	 *
	 * \return True if it matches, False otherwise
	 */
	virtual bool DialStringMatch(
		const std::string &dialString) = 0;

	/*!
	 * \brief Searches each phone number to find a matching dial string
	 *
	 * \param pDialString The dial string to compare
	 *
	 * \return The phone number type where the dial string was found
	 */
	virtual CstiContactListItem::EPhoneNumberType DialStringFind(
		const std::string &dialString) = 0;

	/*!
	 * \brief Searches each phone number to find a matching public ID.
	 *
	 * \param PublicId The public ID to compare
	 *
	 * \return The phone number type where the ID was found
	 */
	virtual CstiContactListItem::EPhoneNumberType PhoneNumberPublicIdFind(
		CstiItemId &PublicId) = 0;

	/*!
	 * \brief Sets the public ID on a phone number.
	 *
	 * \param type The phone number type to set the ID to
	 * \param PublicId The public ID to set
	 *
	 * \return The phone number type where the ID was found
	 */
	virtual void PhoneNumberPublicIdSet(
		CstiContactListItem::EPhoneNumberType type,
		const CstiItemId &PublicId) = 0;

	/*!
	 * \brief Erases all phone numbers
	 */
	virtual void ClearPhoneNumbers() = 0;

	/*!
	 * \brief Adds a phone number to the contact
	 *
	 * \param type The type of phone number being added
	 * \param pDialString The phone number being added
	 */
	virtual void PhoneNumberAdd(
		CstiContactListItem::EPhoneNumberType type,
		const std::string &dialString) = 0;

	/*!
	 * \brief Adds a phone number to or modifies a phone number in the contact
	 *
	 * \param type The type of phone number being added/edited
	 * \param pDialString The phone number being added/edited
	 */
	virtual void PhoneNumberSet(
		CstiContactListItem::EPhoneNumberType type,
		const std::string &dialString) = 0;

#else
	/*!
	 * \brief Sets the Home phone number
	 *
	 * \param pNumber The new Home phone number
	 */
	virtual void HomeNumberSet(
		const std::string &number) = 0;
		
	/*!
	 * \brief Sets the Cell phone number
	 *
	 * \param pNumber The new Cell phone number
	 */
	virtual void CellNumberSet(
		const std::string &number) = 0;
		
	/*!
	 * \brief Sets the Work phone number
	 *
	 * \param pNumber The new Work phone number
	 */
	virtual void WorkNumberSet(
		const std::string &number) = 0;
#endif

	/*!
	 * \brief Sets the ring pattern
	 *
	 * \param Pattern The new ring pattern
	 */
	virtual void RingPatternSet(
		int Pattern) = 0;
		
	/*!
	 * \brief Sets the LightRing color
	 *
     * \param nColor An integer -1 to 7 representing desired color
	 */
	 virtual void RingPatternColorSet(
        int nColor) = 0;

	/*!
	 * \brief Sets the language
	 *
	 * \param pLanguage The new language
	 */
	virtual void LanguageSet(
		const std::string &language) = 0;
		
	/*!
	 * \brief Sets VCO
	 *
	 * \param bVCO VCO enabled/disabled
	 */
	virtual void VCOSet(
		bool bVCO) = 0;
		
	/*!
	 * \brief Sets the photo ID
	 *
	 * \param pPhotoID The new photo ID
	 */
	virtual void PhotoSet(
		const std::string &photoID) = 0;

	/*!
	 * \brief Sets the photo timestamp
	 *
	 * \param pPhotoTimestamp The new photo timestamp
	 */
	virtual void PhotoTimestampSet(const std::string &photoTimestamp) = 0;

	/*!
	 * \brief Sets the ItemID
	 *
	 * \deprecated
	 *
	 * \param pID The new ItemId
	 */
	virtual void IDSet(
		const std::string &id) = 0;

	/*!
	 * \brief Sets the ItemID
	 *
	 * \param Id The new ItemId
	 */
	virtual void ItemIdSet(
		const CstiItemId &Id) = 0;

	/*!
	 * \brief Retrieves the ItemId
	 *
	 * \return The ItemId
	 */
	virtual std::string IDGet() const = 0;

	/*!
	 * \brief Retrieves the ItemId
	 *
	 * \return The ItemId
	 */
	virtual CstiItemId ItemIdGet() const = 0;

	virtual void HomeFavoriteOnSaveSet(bool homeFavorite) = 0;
	virtual void WorkFavoriteOnSaveSet(bool workFavorite) = 0;
	virtual void MobileFavoriteOnSaveSet(bool mobileFavorite) = 0;

	virtual CstiContactListItemSharedPtr GetCstiContactListItem() = 0;
};

using IstiContactSharedPtr = std::shared_ptr<IstiContact>;

#endif
