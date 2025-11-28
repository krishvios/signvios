/*!
 * \file stiMem.h
 * \brief This file contains defines for simple allocation and freeing of memory.
 *
 * Use of the macros defined herein allows the developer to easily switch to a
 * custom memory allocation set of procedures.
 *
 * - stiHEAP_ALLOC(x) Allocates "x" bytes of memory and returns it as a void 
 *   pointer.  See ANSI function "malloc" for more details; it follows the same 
 *   syntax.
 * - stiHEAP_CALLOC(x, y) Allocates an array of "x" elements; each "y" bytes in 
 *   size.  It returns a void pointer to the array.  See ANSI function "calloc"
 *   for more details; it follows the same syntax.
 * - stiHEAP_FREE(x) Frees memory pointed to by "x" and returns it to the heap.
 *   See ANSI function "free" for more details; it follows the same syntax.
 * - stiHEAP_REALLOC(x, y) Reallocates/resizes the array pointed to by "x" to the
 *   size specified by "y".  See ANSI function "realloc" for more details; it 
 *   follows the same syntax.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef STIMEM_H
#define STIMEM_H

/*
 * Includes
 */
#include <cstdlib>		/* Contains the definition for malloc and free */

/*
 * Constants
 */

/*
 * Typedefs
 */
#define stiHEAP_ALLOC(x) (void *)new char[(x)]
#define stiHEAP_FREE(x)	delete [] (x)

/*
 * Forward Declarations
 */

/*
 * Globals
 */

/*
 * Function Declarations
 */

#endif /* STIMEM_H */
/* end file stiMem.h */
