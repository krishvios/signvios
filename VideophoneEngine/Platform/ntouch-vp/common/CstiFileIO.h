/*!
 * \file CstioFileIO.h
 * \brief Safe File IO 
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2016 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "stiError.h"



stiHResult WriteToFile(const std::string &path, const std::string &data, bool append=false);

