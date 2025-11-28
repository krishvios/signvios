/*!
 * \file stiAssert.h
 * \brief See stiAssert.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef STIASSERT_H
#define STIASSERT_H

/*
 * Includes
 */
#include <cassert>
#include <cstdlib> /* For size_t */
#include <string>
/*
 * Constants
 */

/*
 * Typedefs
 */

typedef void (*pRemoteAssertSend)(size_t *, const std::string &, const std::string &);

void stiAssertRemoteLoggingSet(size_t* poRemoteLogger, pRemoteAssertSend pmRemoteAssertSend);

int stiAssert (const void *pExp, const void *pFileName, int nLineNumber);

int stiAssertMsg (
	const void *pExp,       //!< Expression that was tested.
	const void *pFileName,  //!< Name of file where assertion failed.
	int nLineNumber,        //!< Line number where assertion failed.
	const char *pszFormat,
	...);

	/*! \brief Use stiASSERT to test expectations.*/
	#define stiASSERT(exp) (void)((exp) || \
									stiAssert (#exp, __FILE__, __LINE__))
	#define stiASSERTMSG(exp,format,...) (void)((exp) || \
									stiAssertMsg (#exp, __FILE__, __LINE__, format, ##__VA_ARGS__))

#ifndef stiASSERT_VALID
#define stiASSERT_VALID(pOb)  /*(pOb->AssertValid())*/
#endif

/*
 * Forward Declarations
 */

/*
 * Globals
 */

/*
 * Function Declarations
 */

#endif /* STIASSERT_H */
/* end file stiAssert.h  */
