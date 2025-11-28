// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
#include "CstiFileIO.h"
#include <fstream>

stiHResult WriteToFile(const std::string &path, const std::string &data, bool append)
{
	std::ofstream outputFile;
	try
	{

		if(append)
		{
			outputFile.open(path, std::ofstream::app);
		}
		else
		{
			outputFile.open(path);
		}
		outputFile << data;
		outputFile.close();
	}
	catch(std::exception &e)
	{
		return(stiRESULT_LAST);
	}
	return stiRESULT_SUCCESS;
}



