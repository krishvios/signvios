/*!
 * \file SecretCodeParser.h
 * \brief Declaration of the secret code parser.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#ifndef SECRETCODEPARSER_H
#define SECRETCODEPARSER_H


#include <time.h>

namespace Vp200Ui
{

const unsigned int MAX_COMMAND_LENGTH = 4;       //!< The maximum secret command length.


typedef void (*SecretCodeFuncPtr)(void *pParam);


typedef struct
{
	char szSecretCode[MAX_COMMAND_LENGTH + 1];
	SecretCodeFuncPtr fpSecretCodeFunction;
} SecretCode;


class CSecretCodeParser
{
public:
	CSecretCodeParser(
		SecretCode *pCodes,
		unsigned int unNumCodes,
		void *pParam);

	bool CheckCodes(
		char cCharacter);

private:

	unsigned int m_unSecretIndex;
	SecretCode *m_pCodes;
	unsigned int m_unNumCodes;
	void *m_pParam;
	time_t m_tLastSecretKey;
	char m_szSecret[MAX_COMMAND_LENGTH + 1];
};

}
#endif // SECRETCODEPARSER_H
