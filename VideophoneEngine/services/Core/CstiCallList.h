////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiCallList
//
//	File Name:	CstiCallList.h
//
//	Owner:		Scot L. Brooksby
//
//	Abstract:
//		See CstiCallList.cpp
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTI_CALL_LIST_H
#define CSTI_CALL_LIST_H

//
// Includes
//
#include "stiDefs.h"
#include "CstiList.h"
#include "CstiCallListItem.h"

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
// Class Declaration
//
class CstiCallList : public CstiList
{
public:

	//
	// Public Enumerations
	//
	/*!
	 * \brief Enumerates the types of call lists
	 */
	enum EType
	{
		eTYPE_UNKNOWN = 0,		//!< Unknown list type
		eANSWERED = 3,		//!< Answered (Received) calls
		eMISSED = 4,			//!< Missed calls
		eDIALED = 5,			//!< Dialed calls
		eCONTACT = 6,			//!< Contact list
		eBLOCKED = 7,			//!< Blocked callers list
		eRECENT = 8,            //!< Combined Dialed/Answered/Missed list

		// Add any new entries above this line
		eTYPE_LAST		//!< A count of the number of call lists possible

	}; // end EType

	/*!
	 * \brief Enumerates the fields by which to sort the list
	 */
	enum ESortField
	{
		eNAME,				//!< Sort by Name
		eTIME,				//!< Sort by Time
		eDIAL_TYPE,			//!< Sort by Dial type
		eDIAL_STRING,		//!< Sort by Dial string
	}; // end ESortField

	//
	// Public Methods
	//
	CstiCallList () = default;
	~CstiCallList () override = default;

	stiHResult ItemAdd (const CstiCallListItemSharedPtr &callListItem);
	CstiCallListItemSharedPtr ItemGet (unsigned int nItem) const;

	EType TypeGet () const;
	stiHResult TypeSet (EType eType);
	ESortField SortFieldGet () const;
	stiHResult SortFieldSet (ESortField eType);
	unsigned int BaseIndexGet () const;
	stiHResult BaseIndexSet (unsigned int unBaseIndex);
	int ThresholdGet () const;
	stiHResult ThresholdSet (int nThreshold);

private:

	using CstiList::ItemAdd;

	EType m_eType{eTYPE_UNKNOWN};
	ESortField m_eSortField{eNAME};
	unsigned int m_unBaseIndex{0};
	int m_nThreshold{-1};

}; // end class CstiCallList


#endif // CSTI_CALL_LIST_H

// end file CstiCallList.h
