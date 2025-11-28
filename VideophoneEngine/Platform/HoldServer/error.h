#if 0
/*!
 * \file error.h
 * \brief Constants and macros for error and status returns from methods
 *
 *  HRESULTs are 32 bit values layed out as follows:<br><br>
 * 
 <pre>
   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
  +-+-+-+-+-+---------------------+-------------------------------+
  |S|R|C|N|r|    Facility         |               Code            |
  +-+-+-+-+-+---------------------+-------------------------------+
    </pre><p>
 * where
 *
 * - \b S - Severity - indicates success/fail
 *  - \b 0 - Success
 *  - \b 1 - Fail (COERROR)
 * - \b R - reserved portion of the facility code,
 * - \b C - reserved portion of the facility code,
 * - \b N - reserved portion of the facility code.
 * - \b r - reserved portion of the facility code.
 * - \b Facility - is the facility code
 * - \b Code - is the facility's status code
 *
 *
 * \date Tue Dec 17 2003
 *
 * Copyright (C) 2003-2004 by Sorenson Media, Inc.  All Rights Reserved
 */

#ifndef _ERROR_H_
#define _ERROR_H_

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef unsigned long HRESULT;  //!< The generic 32-bit value used as return results from many functions.
#define _HRESULT_TYPEDEF_(_sc) ((HRESULT)_sc)
#endif

//
// Error Code Definitions
//

#ifdef WIN32
	#include <WinError.h>
#else

const HRESULT SEVERITY_SUCCESS  =  0;   
const HRESULT SEVERITY_ERROR    =  1;


/*!
 * \brief Generic test for success on any status value (non-negative numbers
 * indicate success).
 */
//#define SUCCEEDED(Status) ((HRESULT)(Status & 0x80000000) == 0)      

/*!
 * \brief The inverse of the generic test for success
 */
//#define FAILED(Status) ((HRESULT)(Status & 0x80000000) != 0)


/*!
 * \brief Generic test for error on any status value.
 */
//#define IS_ERROR(Status) (PEGULONG(Status) >> 31 == SEVERITY_ERROR)

/*!
 * \brief Return the code portion of the HRESULT.
 */
#define HRESULT_CODE(hr)    ((hr) & 0xFFFF)


/*!
 *  \brief Return the facility portion of the HRESULT.
 */
#define HRESULT_FACILITY(hr)  (((hr) >> 16) & 0x1fff)


/*!
 *  \brief Return the severity portion of the HRESULT.
 */
#define HRESULT_SEVERITY(hr)  (((hr) >> 31) & 0x1)


/*!
 * \brief Create an HRESULT value from component pieces.
 */
#define MAKE_HRESULT(sev,fac,code) \
    ((HRESULT) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )

/*!
 * \brief Facility codes
 */
const HRESULT FACILITY_UI =   0x01;
const HRESULT FACILITY_CALL = 0x01;

//
// Success codes
//
const HRESULT S_OK          = ((HRESULT)0x00000000L);          //!< Success.
const HRESULT S_FALSE       = ((HRESULT)0x00000001L);          //!< Success but...
const HRESULT NOERROR       = 0L;                               //!< Success.
const HRESULT ERROR_SUCCESS = 0L;                              //!< Success.
const HRESULT NO_ERROR      = 0L;                              //!< Success.
const HRESULT E_UNEXPECTED  = _HRESULT_TYPEDEF_(0x8000FFFFL);  //!< Method called out of sequence.
const HRESULT E_NOTIMPL     = _HRESULT_TYPEDEF_(0x80000001L);  //!< Method has not been implemented (yet).
const HRESULT E_OUTOFMEMORY = _HRESULT_TYPEDEF_(0x80000002L);  //!< Insufficient memory to complete the operation.
const HRESULT E_INVALIDARG  = _HRESULT_TYPEDEF_(0x80000003L);  //!< An argument passed to the method is not valid.
const HRESULT E_ABORT       = _HRESULT_TYPEDEF_(0x80000007L);  //!< The operation has aborted unexpectedly.
const HRESULT E_FAIL        = _HRESULT_TYPEDEF_(0x80000008L);  //!< The operation failed (Generic failure).
const HRESULT E_INDEX_OUT_OF_BOUNDS = _HRESULT_TYPEDEF_(0x80000009L); //!< The index parameter was out of bounds.    

#endif //WIN32

const HRESULT E_UI_INIT_FAIL    = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_UI, 1);     //!< The UI has failed to initialize.
const HRESULT E_STATE_TRANSITION = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_UI, 2);    //!< A request was made to change to a state but it is not possible from the current state.

#define SCERR_PROPERTY_MANAGER_FAILED			1
#define SCERR_CANNOT_DIAL_SELF					2
#define SCERR_MEMORY_ALLOCATION_FAILED			3
#define SCERR_DIAL_FAILED						4
#define SCERR_INVALID_CALL_LIST_ITEM			5
#define SCERR_INVALID_MODE_FOR_OUTGOING_CALLS	6
#define SCERR_ALREADY_ACTIVE_CALL				7
#define SCERR_NULL_DIALED_CALL_LIST				8
#define SCERR_COULD_NOT_FIND_CALL				9
#define SCERR_COULD_NOT_ADD_CALL				10
#define SCERR_CORE_REQUEST_CREATION_FAILED		11
#define SCERR_NO_CURRENT_USER					12
#define SCERR_LOCAL_NAME_SET_FAILED				13
#define	SCERR_CCI_INTERFACE_ERROR				14
#define SCERR_NO_ROUTER_OBJECT					15
#define SCERR_NO_VIDEOPHONE_OBJECT				16


#define SCI_TESTCOND(cond, errorcode) if (!(cond)) { hResult = MAKE_HRESULT(SEVERITY_ERROR, 0, errorcode); goto SCI_BAIL; }
#define SCI_TESTRESULT() if (IS_ERROR(hResult)) { goto SCI_BAIL; }

#endif

#endif

