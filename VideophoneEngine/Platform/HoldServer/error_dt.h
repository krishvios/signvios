/*!
 * \file error_dt.h
 * \brief Constants and macros for error used by data tasks. 
 *
 * See error.h for detail
 *
 * \author Ting-Yu Yang
 *
 * Copyright (C) 2003-2004 by Sorenson Media, Inc.  All Rights Reserved
 */

#ifndef ERROR_DT_H
#define ERROR_DT_H

#include "error.h"

const HRESULT FACILITY_DT =   0x02;

const HRESULT E_DT_INVALIDEVICE		= MAKE_HRESULT(SEVERITY_ERROR, FACILITY_DT, 1);
const HRESULT E_DT_DEVICEOPEN_FAIL	= MAKE_HRESULT(SEVERITY_ERROR, FACILITY_DT, 2);
const HRESULT E_DT_DEVICCLOSE_FAIL	= MAKE_HRESULT(SEVERITY_ERROR, FACILITY_DT, 3);
const HRESULT E_DT_DEVICESET_FAIL	= MAKE_HRESULT(SEVERITY_ERROR, FACILITY_DT, 4);
const HRESULT E_DT_DEVICEGET_FAIL	= MAKE_HRESULT(SEVERITY_ERROR, FACILITY_DT, 5);
const HRESULT E_DT_DEVICEWRITE_FAIL	= MAKE_HRESULT(SEVERITY_ERROR, FACILITY_DT, 6);

#endif
