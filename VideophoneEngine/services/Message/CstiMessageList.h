////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiMessageList
//
//	File Name:	CstiMessageList.h
//
//	Owner:		Scot L. Brooksby
//
//	Abstract:
//		See CstiMessageList.cpp
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTI_MESSAGE_LIST_H
#define CSTI_MESSAGE_LIST_H

//
// Includes
//
#include "stiDefs.h"
#include "CstiList.h"
#include "CstiMessageListItem.h"

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
class CstiMessageList : public CstiList
{
public:
	
	//
	// Public Enumerations
	//
	/*!
	 * \brief Enumerates the fields by which to sort the list
	 */
	enum ESortField
	{
		eNAME,				//!< Sort by name
		eRECIEVED_TIME,		//!< Sort by Recieved time
		eLAST_VIEWED_TIME,	//!< Sort by last viewed time
		eDIAL_STRING,		//!< Sort by dial string
		eDIAL_TYPE			//!< Sort by dial type

	}; // end ESortField
	
	//
	// Public Methods
	//
	CstiMessageList () = default;
	~CstiMessageList () override = default;
	
	CstiMessageList (const CstiMessageList &) = delete;
	CstiMessageList (CstiMessageList &&) = delete;
	CstiMessageList &operator= (const CstiMessageList &) = delete;
	CstiMessageList &operator= (CstiMessageList &&) = delete;

	stiHResult ItemAdd (const CstiMessageListItemSharedPtr &messageListItem);
	CstiMessageListItemSharedPtr ItemGet (unsigned int nItem) const;
	
	unsigned int SignMailMsgLimitGet()
	{
		return m_unSignMailMsgLimit;
	}
	void SignMailMsgLimitSet(unsigned int unMsgLimit)
	{
		m_unSignMailMsgLimit = unMsgLimit;
	}
	unsigned int SignMailMsgCountGet()
	{
		return m_unSignMailMsgCount;
	}
	void SignMailMsgCountSet(unsigned int unMsgCount)
	{
		m_unSignMailMsgCount = unMsgCount;
	}
	ESortField SortFieldGet () const;
	stiHResult SortFieldSet (ESortField eType);
	unsigned int BaseIndexGet () const;
	stiHResult BaseIndexSet (unsigned int unBaseIndex);
	int ThresholdGet () const;
	stiHResult ThresholdSet (int nThreshold);
	
private:
		
	using CstiList::ItemAdd;

	ESortField m_eSortField{eRECIEVED_TIME};
	unsigned int m_unBaseIndex{0};
	int m_nThreshold{-1};

	unsigned int m_unSignMailMsgCount{0};
	unsigned int m_unSignMailMsgLimit{0};
}; // end class CstiMessageList


#endif // CSTI_MESSAGE_LIST_H

// end file CstiMessageList.h
