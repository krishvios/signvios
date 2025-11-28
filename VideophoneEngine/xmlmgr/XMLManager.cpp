/*!
*  \file XMLManager.cpp
*  \brief Implementation of the XML Manager class
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*/

#include <cwchar>
#include "XMLManager.h"
#include "stiTrace.h"
#include "stiTools.h"
#include "XMLList.h"

#ifdef stiENCRYPT_XML_FILES
#include <openssl/evp.h>
#endif

#define MAX_FIELDS_TO_SEND 30
#define MAX_TIME_TO_WAIT (5 * 1000) // 5 seconds, time is in milliseconds
#define MAX_ERROR_TIME_TO_WAIT (5 * 60 * 1000) // 5 minutes, time is in milliseconds

#ifdef stiENCRYPT_XML_FILES
const int MAX_FILE_NAME = 255;
#endif


using namespace std;
using namespace WillowPM;


// Number of Minutes after last change before save to flash
const unsigned int unFLASH_SAVE_DELAY = 250;  // Delay time in milliseconds


#ifdef stiENCRYPT_XML_FILES
// Encrytption keys
const unsigned char SSL_KEY[] = {8, 0, 1, 2, 8, 7, 9, 4, 0, 0, 4, 1, 9, 2, 4, 1};
const unsigned char SSL_IV[] = {4, 1, 9, 2, 4, 1, 9, 2};

// Using key and IV above and blowfish encrytption can decrypt with:
// openssl bf -K 08000102080709040000040109020401 -iv 0401090204010902 -in PropertyTable.xml.bin -d -out PropertyTable.xml
// Encrypt:
// openssl bf -K 08000102080709040000040109020401 -iv 0401090204010902 -in PropertyTable.xml -e -out PropertyTable.xml.bin

/**
* \brief Function to encrypt or decrypt file
*
* Encrypts (or decrypts) file stream.
* 
* \param in input stream
* \param out output stream
* \do_encrypt out output stream
*/
stiHResult do_crypt_file(FILE *in, FILE *out, int do_encrypt)
{
	/* Allow enough space in output buffer for additional block */
	unsigned char inbuf[8096], outbuf[8096 + EVP_MAX_BLOCK_LENGTH];
	int inlen, outlen;
	auto hResult = stiRESULT_SUCCESS;

	auto ctx = EVP_CIPHER_CTX_new();
	EVP_CipherInit(ctx, EVP_bf_cbc(), SSL_KEY, SSL_IV, do_encrypt);
	for(;;)
	{
		inlen = fread(inbuf, 1, 8096, in);
		if(inlen <= 0) break;
		if(!EVP_CipherUpdate(ctx, outbuf, &outlen, inbuf, inlen))
		{
			/* Error */
			stiTHROW (stiRESULT_ERROR);
		}
		fwrite(outbuf, 1, outlen, out);
	}
	if(!EVP_CipherFinal_ex(ctx, outbuf, &outlen))
	{
		/* Error */
		stiTHROW (stiRESULT_ERROR);
	}
	fwrite(outbuf, 1, outlen, out);

STI_BAIL:
	EVP_CIPHER_CTX_free (ctx);
	return hResult;
}
#endif //stiENCRYPT_XML_FILES

/**
* \brief Callback function for the SaveData WatchDog.  Called when timer "goes off"
*
* Simply calls the property manager's saveToPersistentStorage() function, and then
* resets the timer to false.
* \param nParam Not used
*/
int wdSaveDataCallback(size_t nParam)
{
	stiDEBUG_TOOL(g_stiPMDebug,
		stiTrace("wdSaveDataCallback - In wdSaveDataCallback\n");
	);

	// call the save data
	auto xm = (XMLManager *)nParam;
	xm->saveToPersistentStorage();
	xm->SaveDataTimerSet(false);

	return 0;
}


// private constructor
XMLManager::XMLManager()
{
	stiDEBUG_TOOL (g_stiPMDebug,
		stiTrace ("XMLManager::XMLManager:%s\n", "Constructor called");
	);

	m_mutex = stiOSMutexCreate ();
}

/*!
* \brief Class destructor
*/
XMLManager::~XMLManager()
{
	uninit ();

	if (m_bSaveDataTimerSet)
	{
		stiDEBUG_TOOL (g_stiPMDebug,
			stiTrace ("XMLManager::~XMLManager:%s\n",
				"Calling wdSaveDataCallback from destructor:");
		);
		wdSaveDataCallback(size_t(this));
	}
	m_bSaveDataTimerSet = false;

	// free up dom tables
	delete m_pdoc;
	m_pdoc = nullptr;

	stiDEBUG_TOOL (g_stiPMDebug,
		stiTrace ("XMLManager::~XMLManager:%s\n", "Tables Freed");
	);

	if (m_mutex)
	{
		stiOSMutexDestroy (m_mutex);
		m_mutex = nullptr;
	}
}

void XMLManager::uninit ()
{
	if (nullptr != m_wdSaveData)
	{
		stiHResult hResult = stiOSWdDelete (m_wdSaveData);

		if (stiIS_ERROR (hResult))
		{
			stiDEBUG_TOOL (g_stiPMDebug,
				stiTrace ("XMLManager::~XMLManager stiOSWdDelete returned estiERROR");
			);
		}

		m_wdSaveData = nullptr;
	}
}


// loads the persistent data table
// returns stiRESULT_ERROR, stiRESULT_UNABLE_TO_OPEN_FILE, or stiRESULT_SUCCESS
stiHResult XMLManager::loadPersistentTable()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// load persistent data table
	// open up the xml file, and parse it
	IXML_Document *pXML = nullptr;

#ifdef stiENCRYPT_XML_FILES
	int ret = 0;
	char szEncryptedFileName[MAX_FILE_NAME];
	sprintf(szEncryptedFileName, "%s.bin", m_filename.c_str());
	FILE *pFile = fopen(szEncryptedFileName, "rb");
	if (pFile)
	{
		char szDecryptedFileName[MAX_FILE_NAME];
		sprintf(szDecryptedFileName, "%s.tmp", m_filename.c_str());
		FILE *pDecryptedFile = fopen(szDecryptedFileName, "w");
		if (pDecryptedFile)
		{
			do_crypt_file(pFile, pDecryptedFile, 0);
			fclose(pDecryptedFile);
		}
		fclose(pFile);
		ret = ixmlLoadDocumentEx(szDecryptedFileName, &pXML);
		int result = remove(szDecryptedFileName);
	}
	else
	{
		// If no encrypted file was present attempt to load a decrypted file and remove it
		ret = ixmlLoadDocumentEx(m_filename.c_str (), &pXML);
		remove(m_filename.c_str ());
	}
	
#else
	// load persistent data table
	// open up the xml file, and parse it
	int ret = ixmlLoadDocumentEx(m_filename.c_str (), &pXML);
#endif //stiENCRYPT_XML_FILES

	if ((nullptr != pXML) && (0 == ret))
	{
		stiDEBUG_TOOL (g_stiPMDebug,
			stiTrace ("XMLManager::loadPersistentTable - Found and loaded %s.\n",
					m_filename.c_str ());
		);
	}
	else
	{
		if (!m_backup1Filename.empty ())
		{
			// Error loading persistent table; may be corrupted, so try backup table
			int ret = ixmlLoadDocumentEx(m_backup1Filename.c_str (), &pXML);

			if ((nullptr != pXML) && (0 == ret))
			{
				stiDEBUG_TOOL (g_stiPMDebug,
					stiTrace ("XMLManager::loadPersistentTable - %s Not Found.  Loaded %s.\n",
							m_filename.c_str (), m_backup1Filename.c_str ());
				);

				// Copy the backup file to the main file
				hResult = stiCopyFile(m_backup1Filename, m_filename);
				stiTESTRESULT()
			}
		}

		if (nullptr == pXML && !m_defaultFilename.empty ())
		{
			// Error loading persistent table and backup; probably doesn't exist so load defaults table
			int ret = ixmlLoadDocumentEx(m_defaultFilename.c_str (), &pXML);

			if ((nullptr != pXML) && (0 == ret))
			{
				stiDEBUG_TOOL (g_stiPMDebug,
					stiTrace ("XMLManager::loadPersistentTable - %s Not Found.  Loaded %s.\n",
							m_filename.c_str (), m_defaultFilename.c_str ());
				);
			}
		}
	}

	// Now was file loaded?
	if (nullptr == pXML)
	{
		// NOTE: ENGINE WILL TO FAIL TO START, SINCE NECESSARY DEFAULTS AREN'T FOUND
		stiDEBUG_TOOL (g_stiPMDebug,
			stiTrace ("XMLManager::loadPersistentTable - %s Not found.  Creating new table.\n",
					m_defaultFilename.c_str ());
		);

		stiTHROW_NOLOG (stiRESULT_UNABLE_TO_OPEN_FILE);
	}
	else
	{
		IXML_NodeList *pNodeList = ixmlNode_getChildNodes(ixmlNode_getFirstChild((IXML_Node *)pXML));
		IXML_Node *pNode = nullptr;
		int i = 0;

		while ((pNode = ixmlNodeList_item(pNodeList, i++)))
		{
			XMLListItemSharedPtr item = NodeToItemConvert(pNode);
			m_pdoc->ItemAdd(item);
		}

		ixmlNodeList_free(pNodeList);
		ixmlDocument_free(pXML);

	}

STI_BAIL:

	return hResult;
}


/// DEPRECATED
/// Load a previous version of the persistent table
void XMLManager::loadPreviousPersistentTable()
{
	stiDEBUG_TOOL(g_stiPMDebug,
		stiTrace("XMLManager::loadPreviousPersistentTable - loading previous version's table\n");
	);

	// Now close the current persistent table
	m_pdoc->ListFree();

	// Rename the previous file to that expected by the application.
	// If the file doesn't exist, that is fine.  The system will act as though
	// it is run for the first time and load values from the Defaults table.
	rename(m_backup1Filename.c_str (), m_filename.c_str ());

	// Re-open the Persistent table now
	loadPersistentTable();
}

/*!
 * \brief Backup the persistent xml table in flash
 * \retval An integer error code, 0 == SUCCESS
 */
int XMLManager::backup ()
{
	int nRetVal = XM_RESULT_SUCCESS;
	int nSysStatus;  stiUNUSED_ARG (nSysStatus);

	stiDEBUG_TOOL(g_stiPMDebug,
		stiTrace("XMLManager::PropertyTableBackup - entry\n");
	);

	// If there is a pending write to flash, make it happen now.
	if (m_bSaveDataTimerSet)
	{
		saveToPersistentStorage();
	}

	// lock the XMLManager during the backup
	lock();

	// Load persistent data file to verify validity
	IXML_Document *pXML = nullptr;
	int ret = ixmlLoadDocumentEx(m_filename.c_str (), &pXML);

	if ((nullptr != pXML) && (0 == ret))
	{
		// File is valid... copy to the backup file
		stiCopyFile(m_filename, m_backup1Filename);

		ixmlDocument_free(pXML);
	}
	
	// unlock the XMLManager
	unlock();

	return (nRetVal);
}

/**
* \brief Initializes the XMLManager object by loading XML documents
* \return stiRESULT_ERROR, stiRESULT_UNABLE_TO_OPEN_FILE, or stiRESULT_SUCCESS
*/
stiHResult XMLManager::init()
{
	stiHResult hResult = stiMAKE_ERROR(stiRESULT_ERROR);

	// lock the XMLManager during the initialize
	lock();

	if (m_wdSaveData == nullptr)
	{
		// create the watchdog timer
		if (nullptr == (m_wdSaveData = stiOSWdCreate ()))
		{
			// LW: log some error
		}
	}
	m_bSaveDataTimerSet = false;

	if (m_pdoc)
	{
		delete m_pdoc;
		m_pdoc = nullptr;
	}

	m_pdoc = new XMLList(9999);

	// Load the persistent table
	hResult = loadPersistentTable ();

	//debug: print out tables
	#if 0
	stiDEBUG_TOOL (g_stiPMDebug,
		stiTrace ("XMLManager::init - PropertyTable:%s\n",
			ixmlPrintDocument(m_pdoc));
	);
	stiDEBUG_TOOL (g_stiPMDebug,
		stiTrace ("XMLManager::init - Temp Table:%s\n",
			ixmlPrintDocument(m_tdoc));
	);
	#endif

	// unlock the PM
	unlock();

	return hResult;
}

/*!
* \brief Forces a save of the DOM table out to XML storage
*/
stiHResult XMLManager::saveToPersistentStorage()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// lock the XMLManager during the initialize
	lock();

	// Print the number of persistent table writes
	stiDEBUG_TOOL (g_stiPMDebug,
		stiTrace ("#####\nXMLManager::saveToPersistentStorage - Write #: %d\n",
			++m_nWriteCount);
	);

	// if a the data save timer is running, cancel it
	if (m_bSaveDataTimerSet)
	{
		stiOSWdCancel(m_wdSaveData);
		m_bSaveDataTimerSet = false;
	}

	if (m_bPersistentSave)
	{
		stiTESTCOND (m_pdoc, stiRESULT_ERROR); // rare error: document never loaded, return error

		// TODO: REMOVE THIS CODE ?? Move it elsewhere ???
// 		if (!m_backup1Filename.empty ())
// 		{
// 			writeToFile(m_backup1Filename.c_str ());
// 		}

		writeToFile(m_filename);
	}

STI_BAIL:
	// unlock the PM
	unlock();

	return hResult;
}


///
/// \brief Write the file out to persistent memory
///
void XMLManager::writeToFile(const std::string &file)
{
	FILE *pFile = fopen(file.c_str (), "w");
	if (pFile)
	{
		fprintf(pFile, "<?xml version=\"1.0\"?>\n");
		fprintf(pFile, "<root>\n");

		for (unsigned int i = 0; i < m_pdoc->CountGet(); i++)
		{
			m_pdoc->ItemGet(i)->Write(pFile);
		}

		fprintf(pFile, "</root>\n");

		fclose(pFile);
	}
#ifdef stiENCRYPT_XML_FILES
	pFile = fopen(file.c_str (), "r");
	if (pFile)
	{
		char szEncryptedFileName[MAX_FILE_NAME];
		sprintf(szEncryptedFileName, "%s.bin", file.c_str ());
		FILE *pEncryptedFile = fopen(szEncryptedFileName, "wb");
		if (pEncryptedFile)
		{
			do_crypt_file(pFile, pEncryptedFile, 1);
			fclose(pEncryptedFile);
			fclose(pFile);
		}
#ifndef _DEBUG
		remove(file.c_str ()); // Delete the unencrypted file
#endif
	}
#endif //#if APPLICATION == APP_NTOUCH_PC}
}

/*!
* \brief For Debug: print out the persistent and temporary tables
*/
int XMLManager::printTables()
{
	// lock the XMLManager during the initialize
	lock();

	int errCode = XM_RESULT_SUCCESS;

	if (nullptr == m_pdoc)
	{
		unlock();
		return XM_RESULT_IXML_ERROR;     // rare error: document never loaded, just return
	}
	/*
	// debug: print tables
	stiDEBUG_TOOL (g_stiPMDebug,
		stiTrace ("XMLManager::printTables - PropertyTable:\n%s\n",
			ixmlPrintDocument(m_pdoc));
	);
	stiDEBUG_TOOL (g_stiPMDebug,
		stiTrace ("XMLManager::printTables - Temp Table:\n%s\n",
			ixmlPrintDocument(m_tdoc));
	);
*/
	// unlock the PM
	unlock();

	return errCode;
}


/**
 * \brief Resets to factory defaults erasing any user settings
 */
void XMLManager::resetToDefaults()
{
	// TODO:  Do this
	// Flush anything waiting to be written (just throw it away)
	// Delete and recreate or remove all keys from the LOCAL_PERSIST_XML file
}

/**
* \brief Starts the SaveData timer to enable the automatic data save process
*
*/
void XMLManager::startSaveDataTimer()
{
	if (m_bSaveDataTimerSet)
	{
		// already running, cancel it before we start it again
		stiOSWdCancel(m_wdSaveData);
		m_bSaveDataTimerSet = false;
		stiDEBUG_TOOL(g_stiPMDebug,
			stiTrace("XMLManager::startSaveDataTimer - Previously running timer cancelled\n");
		);
	}

	stiHResult hResult = stiOSWdStart(m_wdSaveData, unFLASH_SAVE_DELAY,
			   (stiFUNC_PTR) &wdSaveDataCallback, (size_t)this);

	if (stiIS_ERROR (hResult))
	{
	//    stiASSERT (estiFALSE);
	}
	else
	{
		stiDEBUG_TOOL(g_stiPMDebug,
			stiTrace("XMLManager::startSaveDataTimer - WatchDog Timer started\n");
		);
	}

	// Always set this here so even if timer didn't start, data can be saved at exit
	m_bSaveDataTimerSet = true;
}

int XMLManager::itemGet(const std::string &name, XMLListItemSharedPtr *item) const
{
	int errCode = XM_RESULT_SUCCESS;

	if (m_pdoc)
	{
		int index = m_pdoc->ItemFind(name);
		if (index != -1)
		{
			*item = m_pdoc->ItemGet(index);
		}
		else
		{
			errCode = XM_RESULT_NOT_FOUND;
		}
	}
	else
	{
		errCode = XM_RESULT_NOT_FOUND;
	}

	return errCode;
}

int XMLManager::intGet(
	const std::string &itemName,
	int64_t *pIntVal,
	int Field,
	int64_t defaultVal) const
{
	lock();

	auto errCode = intGet (itemName, pIntVal, Field);

	if (errCode == XM_RESULT_NOT_FOUND)
	{
		*pIntVal = defaultVal;
	}

	unlock();

	return errCode;
}

int XMLManager::intGet(
	const std::string &itemName,
	int64_t *pIntVal,
	int Field) const
{
	lock();

	XMLListItemSharedPtr item = nullptr;

	int errCode = itemGet(itemName, &item);

	if (errCode == XM_RESULT_SUCCESS)
	{
		errCode = item->IntGet(*pIntVal, Field);
	}
	else
	{
		*pIntVal = g_IntegerDefault;
	}

	unlock();

	return errCode;
}


int XMLManager::stringGet(
	const std::string &itemName,
	std::string &value,
	int Field,
	const std::string &defaultVal) const
{
	lock();

	auto errCode = stringGet (itemName, value, Field);

	if (errCode == XM_RESULT_NOT_FOUND)
	{
		value = defaultVal;
	}

	unlock();

	return errCode;
}


int XMLManager::stringGet(
	const std::string &itemName,
	std::string &value,
	int Field) const
{
	lock();

	XMLListItemSharedPtr item;

	int errCode = itemGet(itemName, &item);

	if (errCode == XM_RESULT_SUCCESS)
	{
		errCode = item->StringGet(value, Field);
	}
	else
	{
		value = {};
	}

	unlock();

	return errCode;
}


int XMLManager::valueGet(
	const std::string &name,
	std::string &value,
	std::string &typeString,
	int Field) const
{
	// lock the XMLManager during the get
	lock();

	XMLListItemSharedPtr item;
	int errCode = itemGet(name, &item);

	if (errCode == XM_RESULT_SUCCESS)
	{
		item->StringGet(value, Field);
	}

	// unlock the PM
	unlock();

	return errCode;
}

int XMLManager::valueSet(const std::string &name, const std::string &value, int Field)
{
	lock();

	XMLListItemSharedPtr item = nullptr;
	int errCode = itemGet(name, &item);

	if (errCode == XM_RESULT_SUCCESS)
	{
		bool bMatch = false;
		item->ValueCompare(value, Field, bMatch);

		if (!bMatch)
		{
			item->StringSet(value, Field);
			errCode = XM_RESULT_VALUE_CHANGED;
		}
	}

	unlock();

	return errCode;
}
