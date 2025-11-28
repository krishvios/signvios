/*!
 * \file SecretCodeParser.cpp
 * \brief Definition of the secret code parser.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

#include <stdio.h>
#include <string.h>
#include "SecretCodeParser.h"

using namespace Vp200Ui;

const int MAX_KEY_DELAY = 3;            //!< Maximum amount of time to press a key to have it count toward a secret code.


//!
// \brief Constructor for the class.
//
// \param pCodes A pointer to a list of codes and callback functions
// \param unNumCodes The number of entries in pCodes
// \param pParam A parameter that is passed to the callback function
//
CSecretCodeParser::CSecretCodeParser(
	SecretCode *pCodes,
	unsigned int unNumCodes,
	void *pParam)
{
	m_unSecretIndex = ~0;
	m_pCodes = pCodes;
	m_unNumCodes = unNumCodes;
	m_pParam = pParam;
}


//!
// \brief This function check to see if any of the codes have been matched
//
// \param cCharacter The next input character
// \return true if code was matched
//
bool CSecretCodeParser::CheckCodes(
	char cCharacter)
{
	bool bRet = false;

	time_t now;
	unsigned int i;

	time(&now);

	if (cCharacter == '*')
	{
		m_unSecretIndex = 0;
		m_tLastSecretKey = now;
	}
	else if (m_unSecretIndex < MAX_COMMAND_LENGTH)
	{
		if (difftime(now, m_tLastSecretKey) < MAX_KEY_DELAY)
		{
			//
			// Add the character to the code already entered
			// and null terminate it
			//
			m_szSecret[m_unSecretIndex++] = cCharacter;
			m_szSecret[m_unSecretIndex] = '\0';

			for (i = 0; i < m_unNumCodes; i++)
			{
				if (strcmp(m_szSecret, m_pCodes[i].szSecretCode) == 0)
				{
					m_unSecretIndex = 0;

					m_pCodes[i].fpSecretCodeFunction(m_pParam);

					bRet = true;

					break;
				}
			}
		}
		else
		{
			m_unSecretIndex = ~0;
		}
	}
	return bRet;
}


