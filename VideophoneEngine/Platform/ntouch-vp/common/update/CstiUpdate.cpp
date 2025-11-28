///
/// \file CstiUpdate.cpp
/// \brief Definition of the Update class for ntouch
///
///
/// Sorenson Communications Inc. Confidential. --  Do not distribute
/// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
///

//
// Includes
//

#include <cctype>  // standard type definitions.
#include <sys/stat.h>

#include "stiOS.h"          // OS Abstraction
#include "CstiUpdate.h"
#include "CstiUpdateWriter.h"
#include "stiDebugTools.h"
#include "stiTools.h"


#include "ccrc.h"   // crc calculation library
#include "CstiUpdateConnection.h"

#define MIN_DOWNLOAD_AND_WRITE_BUFFER_LENGTH (16 * 1024)
#define MAX_DOWNLOAD_AND_WRITE_BUFFER_LENGTH (128 * 1024)

// The following percentages should total 100!
#define DOWNLOAD_PERCENT 100

//
// Constants
//

#if DEVICE == DEV_X86
const char szLOCAL_FILE[] = "/tmp/flash_image.bin";
const char szOLD_FILE[] = "/tmp/old_flash_image.bin";
#else
const char szLOCAL_FILE[] = "/data/ntouchvp2/ntouchvp2_update.image";
#endif

const char szIMAGE1[] = "/dev/mmcblk0p1";
const char szIMAGE2[] = "/dev/mmcblk0p2";


// COMBO_TYPE_NAME is used to verify that the downloaded combo image is the correct type
#define COMBO_TYPE_NAME "ntouchvp2"

/* The following define and the structure that follows are used for the
 * CRC validation process.
 */
#define IH_NMLEN		32	    /* Image Name Length		*/

/*
 * all data in network byte order (aka natural aka bigendian)
 */

//
// Structs
//

typedef struct image_header {
	uint32_t    ih_magic;   /* Image Header Magic Number */
	uint32_t    ih_hcrc;    /* Image Header CRC Checksum */
	uint32_t    ih_time;    /* Image Creation Timestamp */
	uint32_t    ih_size;    /* Image Data Size */
	uint32_t    ih_load;    /* Data	 Load  Address */
	uint32_t    ih_ep;      /* Entry Point Address */
	uint32_t    ih_dcrc;    /* Image Data CRC Checksum */
	uint8_t     ih_os;      /* Operating System */
	uint8_t     ih_arch;    /* CPU architecture */
	uint8_t     ih_type;    /* Image Type */
	uint8_t     ih_comp;    /* Compression Type */
	uint8_t     ih_name[IH_NMLEN]; /* Image Name */
} image_header_t;

//
// Globals
//

//
// Function Definitions
//

//
// Function Declarations
//

///
/// \brief Constructor
///
CstiUpdate::CstiUpdate ()
	:
	CstiEventQueue("CstiUpdate")
{
} // end CstiUpdate::CstiUpdate


///
/// \brief Destructor
///
CstiUpdate::~CstiUpdate ()
{
	CstiEventQueue::StopEventLoop ();

	if (m_pUpdateConnection)
	{
		delete m_pUpdateConnection;
		m_pUpdateConnection = nullptr;
	}

	m_updateWriter = nullptr;
	
} // end CstiUpdate::~CstiUpdate


///
/// \brief Initialization method for the class
///
stiHResult CstiUpdate::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stateSet (eIDLE);
	
	return (hResult);

} // end CstiUpdate::Initialize

///
/// \brief Starts up the task
///
///	This method will call the method that spawns the task
///	and also will create the message queue that is used to receive messages
///	sent to this task by other tasks.
///
stiHResult CstiUpdate::Startup ()
{
	StartEventLoop();
	return stiRESULT_SUCCESS;

}//end CstiUpdate::Startup


///
/// \brief Connect to the server and get the updated firmware
///
///	This method connects to an update server and downloads the firmware image.
///  Upon successfully downloading the file, it writes it to flash.
///
/// \Return estiOK when successful, else estiERROR
///
void CstiUpdate::update (const std::string &url)
{
	PostEvent ([this, url]{ eventUpdate (url); });
} // end CstiUpdate::Update ()


void CstiUpdate::updateDownload ()
{
	errorSignal.Emit ();
	stiASSERTMSG (false, "updateDownload: This function is not supported by this platform");
}

void CstiUpdate::updateInstall ()
{
	errorSignal.Emit ();
	stiASSERTMSG (false, "updateInstall: This function is not supported by this platform");
}

/*!
 * \brief Pauses the download of a firmware update if one is in progress
 *
 * \return stiHResult
 */
void CstiUpdate::pause ()
{
	m_nPauseCount++;

	if (eDOWNLOADING == stateGet())
	{
		stiDEBUG_TOOL (g_stiUpdateDebug,
			stiTrace("\n   >>>>>>>> UPDATE IS PAUSED...\n\n");
		); // stiDEBUG_TOOL

		stateSet(eDOWNLOAD_PAUSED);
	}
	else if (eCONNECTING == stateGet ())
	{

		stiDEBUG_TOOL (g_stiUpdateDebug,
			stiTrace("\n   >>>>>>>> UPDATE IS PAUSED...\n\n");
		); // stiDEBUG_TOOL

		stateSet (eCONNECTING_PAUSED);
	}

	return;
}


/*!
 * \brief Resume the download of a firmware update if one is in progress
 *
 * \return stiHResult
 */
void CstiUpdate::resume ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_nPauseCount > 0)
	{
		m_nPauseCount--;

		if (m_nPauseCount == 0)
		{
			if (eDOWNLOAD_PAUSED == stateGet())
			{
				stiDEBUG_TOOL (g_stiUpdateDebug,
					stiTrace("\n   >>>>>>>> UPDATE IS Resumed...\n\n");
				); // stiDEBUG_TOOL

				stateSet (eDOWNLOADING);

				hResult = nextBufferGet ();
				stiTESTRESULT ();
			}
			else if (eCONNECTING_PAUSED == stateGet ())
			{
				stiDEBUG_TOOL (g_stiUpdateDebug,
					stiTrace("\n   >>>>>>>> UPDATE IS Resumed...\n\n");
				); // stiDEBUG_TOOL

				stateSet (eCONNECTING);

				hResult = downloadProcessStart ();
				stiTESTRESULT ();
			}
		}
	}

STI_BAIL:

	return;
}

///
/// \brief Starts the update process for the passed-in UpdateSource object.
/// 
/// \param tUpdateSource
/// 
/// \return
/// 
stiHResult CstiUpdate::downloadProcessStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	bool bOpenForAppend = false;	// By default, erase the downloaded file and open for writing
	int nStartByte = -1;			// By default, set Start and End bytes to -1; changed later
	char *pBuffer1 = nullptr;
	char *pBuffer2 = nullptr;

	m_nMaxDownloadLength = MIN_DOWNLOAD_AND_WRITE_BUFFER_LENGTH;

	// If Update started previously and was paused, resume it
	if (m_bResume)
	{
		bOpenForAppend = true;				// open the partially downloaded file for append
		nStartByte = m_nBytesWritten;		// set start and end bytes for the range request
		m_nBytesDownloaded = m_nBytesWritten; // Set the number of bytes downloaded to the number of bytes written.
	}
	else
	{
		// Initialize the member vars that keep track of progress
		m_nTotalDataLength = 0;
		m_nBytesDownloaded = 0;
		m_nBytesWritten = 0;
		nStartByte = 0;

		pBuffer1 = new char [MAX_DOWNLOAD_AND_WRITE_BUFFER_LENGTH + 1];
		stiTESTCOND (pBuffer1 != nullptr, stiRESULT_ERROR);

		pBuffer2 = new char [MAX_DOWNLOAD_AND_WRITE_BUFFER_LENGTH + 1];
		stiTESTCOND (pBuffer2 != nullptr, stiRESULT_ERROR);

		m_EmptyList.push_back (pBuffer1);
		m_EmptyList.push_back (pBuffer2);
		m_FullList.clear ();
	}

	//
	// Setup the download and get the socket
	//
	m_pUpdateConnection = new CstiUpdateConnection;
	stiTESTCOND (m_pUpdateConnection, stiRESULT_ERROR);

	hResult = m_pUpdateConnection->Open (m_updateUrl,
				nStartByte,
				m_nMaxDownloadLength);

	if (stiIS_ERROR (hResult))
	{
		stiDEBUG_TOOL (g_stiUpdateDebug,
			stiTrace ("CstiUpdate::downloadProcessStart: Error: m_pUpdateConnection->Open failed\n");
		); // stiDEBUG_TOOL

		stiTHROW (stiRESULT_ERROR);
	}
	else
	{
		stiDEBUG_TOOL (g_stiUpdateDebug,
			vpe::trace ("CstiUpdate::downloadProcessStart: Beginning Download from ", m_updateUrl, ".\n");
		); // stiDEBUG_TOOL
	}

	//
	// Make sure the update writer is shutdown before we start it up again
	// with a new file to download.
	//
	m_updateWriter = nullptr;
	
	
	m_updateWriter = sci::make_unique<CstiUpdateWriter>();
	stiTESTCOND (m_updateWriter != nullptr, stiRESULT_ERROR);

	//
	// Point the Update Writer to download to the RAM image
	//
	hResult = m_updateWriter->Initialize (szLOCAL_FILE);
	stiTESTRESULT ();
	
	
	m_signalConnections.push_back(m_updateWriter->bufferWrittenSignal.Connect (
			[this](char * buffer, int bytesWritten)
			{
				PostEvent ([this, buffer, bytesWritten]{ eventBufferWritten (buffer, bytesWritten);});
			}
		));
	
	m_signalConnections.push_back(m_updateWriter->errorSignal.Connect (
			[this](const char * buffer)
			{
				// If the returned buffer isn't NULL, free it.
				if (buffer)
				{
					delete [] buffer;
					buffer = nullptr;
				}
				
				// Now, inform the application of the error.
				this->errorSignal.Emit ();

				// Clean up.
				this->cleanup ();
			}
		));
	
	
	hResult = m_updateWriter->Startup ();
	stiTESTRESULT ();
	
	// Tell the writer to initialize the flash image (either erase or append and open for writing)
	hResult = m_updateWriter->imageInitialize (bOpenForAppend);
	stiTESTRESULT ();
	
	//
	// Alert the application that we are now in the downloading state.
	//
	stateSet (eDOWNLOADING);

	m_nTotalDataLength = m_pUpdateConnection->FileSizeGet ();

	if (m_nTotalDataLength < 0)
	{
		stiTrace ("CstiUpdate::downloadProcessStart: file size is less than 0\n");
		stiTHROW (stiRESULT_ERROR);
	}

	hResult = nextBufferGet ();
	stiTESTRESULT ();

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		// Something failed.  The application should have been notified at the location of the error.
		// Free the buffers and close the session.
		if (pBuffer1)
		{
			delete []  pBuffer1;
			pBuffer1 = nullptr;
		}

		if (pBuffer2)
		{
			delete [] pBuffer2;
			pBuffer2 = nullptr;
		}

		cleanup();

		// Transition back to an idle state.
		stateSet (eIDLE);

		errorSignal.Emit ();
	}

	return (hResult);
}


stiHResult comboTypeVerify (
	const char *pszImageHeaderName)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string strImageHeaderName(pszImageHeaderName);
	size_t nFoundPos;

	nFoundPos = strImageHeaderName.find (COMBO_TYPE_NAME);
	stiTESTCOND (nFoundPos != std::string::npos, stiRESULT_ERROR);

STI_BAIL:

	return hResult;
}


///
/// \brief Verifies both the header and data checksum values of the image
///
/// \NOTE: the image header is stored in bigendian, so the code uses
/// ntohl() to convert values to little endian.
///
stiHResult checksumVerify (
	FILE* fp) // Pointer to the open file to be verified
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CCRC ccrc;
	char* data;
	int len;
	long crc, checksum;
	image_header_t header;
	image_header_t *hdr = &header;
	int nRead; stiUNUSED_ARG (nRead);

	/* make a local copy of the header */
	nRead = fread (&header, sizeof (image_header_t), 1, fp);

	data = (char*)&header;
	len  = sizeof(image_header_t);

	/* Check the header checksum first */
	checksum = ntohl(hdr->ih_hcrc);
	hdr->ih_hcrc = 0;

	crc = ccrc.getStringCRC(data, len);
	stiTESTCOND (crc == checksum, stiRESULT_ERROR);

	stiDEBUG_TOOL (g_stiUpdateDebug,
		stiTrace ("<CstiUpdate> Header Checksum of 0x%x proves valid\n", crc);
	); // stiDEBUG_TOOL

	/* Now check the data checksum */
	crc = ccrc.getFileCRC(fp, (unsigned long)ntohl(hdr->ih_size));
	stiTESTCOND (crc == (long)ntohl(hdr->ih_dcrc), stiRESULT_ERROR);

	stiDEBUG_TOOL (g_stiUpdateDebug,
		stiTrace ("<CstiUpdate> Data Checksum of 0x%x proves valid\n", crc);
	); // stiDEBUG_TOOL

	// Verify Combo Image Type
	hResult = comboTypeVerify ((const char *)hdr->ih_name);
	stiTESTRESULT ();

	stiDEBUG_TOOL (g_stiUpdateDebug,
		stiTrace ("<CstiUpdate> Combo Image type is correct\n");
	); // stiDEBUG_TOOL

STI_BAIL:

	return (hResult);

} // end checksumVerify


///
/// \brief Handles next step when informed that the buffer has been written successfully.
///
void CstiUpdate::eventBufferWritten (
	char * buffer,
	int bytesWritten)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	FILE *pChecksumTestFile = nullptr;

	m_nBytesWritten += bytesWritten;

	//
	// See if the the buffer is in the full list.  If it is not then it must be
	// a buffer from a previous download attempt. We can safely delete the buffer
	// in this case.
	//
	std::list<char *>::iterator i;
	bool bFound = false;

	for (i = m_FullList.begin (); i != m_FullList.end (); ++i)
	{
		if ((*i) == buffer)
		{
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		//
		// Did not find it in the full list. It must be an old
		// buffer we no longer need so delete it.
		//
		delete [] buffer;
		buffer = nullptr;
	}
	else
	{
		//
		// Found the buffer in the full list.  Remove it and add
		// it back to the empty list.
		//
		m_FullList.remove (buffer);
		m_EmptyList.push_back (buffer);

		// Is there more data to download?
		if (m_nBytesDownloaded < m_nTotalDataLength)
		{
			if (eDOWNLOAD_PAUSED != stateGet () && eIDLE != stateGet ())
			{
				hResult = nextBufferGet ();
				stiTESTRESULT ();
			}
		}
		// Have we DOWNLOADED the entire file?
		else if (m_nBytesWritten >= m_nTotalDataLength)
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				stiTrace ("CstiUpdate::eventBufferWritten: Download Completed.  Wrote %d bytes.\n", m_nBytesWritten);
			); // stiDEBUG_TOOL
			// Since the download is complete, clean up.
			cleanup ();

			// Verify the checksum of the copied file to determine the success of the update.
			// NOTE: open the file read/write because we zero out the checksum bytes in the file.
			pChecksumTestFile = fopen (szLOCAL_FILE, "r+");

			if (pChecksumTestFile == nullptr)
			{
				//
				// There was a problem opening the file to do a checksum. We'll have to start
				// over. Clear the update source list so we will download again
				// at the next update check.
				//
				m_updateUrl.clear ();
				stiTHROW (stiRESULT_ERROR);
			}

			hResult = checksumVerify (pChecksumTestFile);

			if (stiIS_ERROR (hResult))
			{
				//
				// There was a problem with the checksums. We'll have to start
				// over. Clear the update source list so we will download again
				// at the next update check.
				//
				m_updateUrl.clear ();
				stiTESTRESULT ();
			}

			// Close the file now; we're done.
			fclose (pChecksumTestFile);
			pChecksumTestFile = nullptr;

			// Finished downloading... Return to idle state
			m_bUpdateDownloaded = true;
			downloadSuccessfulSignal.Emit ();

			hResult = writeImageToFlash ();
			stiTESTRESULT ();
		}
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		if (nullptr != pChecksumTestFile)
		{
			fclose (pChecksumTestFile);
			pChecksumTestFile = nullptr;
		}

		cleanup();

		// Return to idle state
		stateSet (eIDLE);

		errorSignal.Emit ();
	}

	return;
} // end CstiUpdate::eventBufferWritten


stiHResult CstiUpdate::nextBufferGet ()
{
	PostEvent ([this]{ eventNextBufferGet (); });

	return stiRESULT_SUCCESS;
}


///
/// \brief Gets the next buffer full from the remote system
///
void CstiUpdate::eventNextBufferGet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nCurrentReadLength = 0;
	char *pBuffer = nullptr;
	char *pBufferLocation = nullptr;
	int nBytesRead = 0;
	int nNoBytesReadCount = 0;	// Counter of how often the read returns 0 bytes read
	int nBytesLeft = std::min (m_nTotalDataLength - m_nBytesDownloaded, m_nMaxDownloadLength);
	int nPrevBytesDownloaded = m_nBytesDownloaded;

	if (!m_EmptyList.empty () && stateGet () == eDOWNLOADING)
	{
		// Check to see if we have a valid handle
		stiTESTCOND (nullptr != m_pUpdateConnection, stiRESULT_ERROR);

		pBuffer = m_EmptyList.front ();
		m_EmptyList.pop_front ();

		m_FullList.push_back (pBuffer);

		pBufferLocation = pBuffer;

		//
		// Monitor the download rate and adjust the buffer size up if allowed.
		//
		timespec Time1{};

		clock_gettime (CLOCK_MONOTONIC, &Time1);

		do
		{
			hResult =  m_pUpdateConnection->Read (
				pBufferLocation,  				  // pointer to the read buffer
				nBytesLeft + 1, // size of buffer
				&nCurrentReadLength);

			if (stiIS_ERROR (hResult))
			{
				stiDEBUG_TOOL (g_stiUpdateDebug,
					stiTrace ("CstiUpdate::eventNextBufferGet: UpdateRead failed, hResult = %x, nCurrentReadLength = %d, nBytesLeft = %d\n",
						hResult, nCurrentReadLength, nBytesLeft);
				); // stiDEBUG_TOOL

				stiTESTRESULT ();
			}

			if (nCurrentReadLength > 0)
			{
				pBufferLocation += nCurrentReadLength;
				m_nBytesDownloaded += nCurrentReadLength;
				nBytesRead += nCurrentReadLength;
				nBytesLeft -= nCurrentReadLength;

				if (nBytesLeft == 0)
				{
					break;
				}

				nNoBytesReadCount = 0;	// reset the "zero bytes read" counter
			}
			else
			{
				// If our attempts to read keep yielding zero bytes,
				// eventually stop trying and consider it an error
				stiTESTCOND(nNoBytesReadCount < 99, stiRESULT_ERROR);
				nNoBytesReadCount++;
			}

		} while (nBytesLeft > 0);

		if (nBytesRead > 0)
		{
			//
			// Compute the download rate.  We are trying to achieve around
			// one second per download buffer so that the download process
			// can be put on hold fairly quickly.
			//
			timespec Time2{};
			timeval Time3{};
			timeval Time4{};
			timeval TimeDiff{};

			clock_gettime (CLOCK_MONOTONIC, &Time2);

			Time3.tv_sec = Time1.tv_sec;
			Time3.tv_usec = Time1.tv_nsec / 1000;

			Time4.tv_sec = Time2.tv_sec;
			Time4.tv_usec = Time2.tv_nsec / 1000;

			timersub (&Time4, &Time3, &TimeDiff);

			float fDataRate = (nBytesRead * 8 / (TimeDiff.tv_sec + TimeDiff.tv_usec / 1000000.0F));

			stiDEBUG_TOOL (g_stiUpdateDebug,
					stiTrace ("Data Rate = %f kbps\n", fDataRate / 1024);
			);

			if (fDataRate > m_nMaxDownloadLength && m_nMaxDownloadLength < MAX_DOWNLOAD_AND_WRITE_BUFFER_LENGTH)
			{
				m_nMaxDownloadLength += 64 * 1024;

				if (m_nMaxDownloadLength > MAX_DOWNLOAD_AND_WRITE_BUFFER_LENGTH)
				{
					m_nMaxDownloadLength = MAX_DOWNLOAD_AND_WRITE_BUFFER_LENGTH;
				}
			}

			stiDEBUG_TOOL (g_stiUpdateDebug,
					stiTrace ("CstiUpdate::eventNextBufferGet: Bytes downloaded: %d, Total Length: %d\n", m_nBytesDownloaded, m_nTotalDataLength);
			);

			//
			// Calculate download percent and, if it has changed, send the updated percentage in a callback
			//
			float fPercent = ((float)m_nBytesDownloaded / (float)m_nTotalDataLength);

			int nProgressPercent = m_nProgress + (DOWNLOAD_PERCENT * fPercent);

			if (nProgressPercent != m_nProgressPercent)
			{
				downloadingSignal.Emit(nProgressPercent);

				m_nProgressPercent = nProgressPercent;
			}

			// Tell the UpdateWriter to write this buffer
			stiTESTCOND (m_updateWriter, stiRESULT_ERROR);
			hResult = m_updateWriter->bufferWrite (pBuffer, nBytesRead);
			stiTESTRESULT ();
			
			pBuffer = nullptr;
		}
	}

STI_BAIL:

	if (pBuffer)
	{
		//
		// If we have a pointer to a buffer we need to make sure it is removed from
		// the full list and add it back to the empty list as it must not have been
		// passed to the writer.
		//
		m_FullList.remove (pBuffer);
		m_EmptyList.push_back (pBuffer);

		//
		// Since we failed we need to set the number of bytes downloaded
		// back to the value that it was when we entered this method.
		//
		m_nBytesDownloaded = nPrevBytesDownloaded;

		cleanup();

		//
		// If the error code indicates that our range request was
		// out of range then assume the file changed on the back end
		// and clear all knowledge of the current update request. We will
		// try again on the next update requst.
		//
		if (stiRESULT_CODE(hResult) == stiRESULT_INVALID_RANGE_REQUEST)
		{
			m_updateUrl.clear ();
		}

		stateSet (eIDLE);

		errorSignal.Emit ();
	}

	return;
} // end CstiUpdate::eventNextBufferGet


//:-----------------------------------------------------------------------------
//: Public methods
//:-----------------------------------------------------------------------------

///
/// \brief Returns the state of the Update service
///
///	\return CstiUpdate::EState
///
CstiUpdate::EState CstiUpdate::stateGet ()
{
	return (m_eState);

} // end CstiUpdate::stateGet

///
/// \brief Returns the size of the image being downloaded.
///
int CstiUpdate::imageSizeGet ()
{
	return m_nTotalDataLength;
}

///
/// \brief Returns the the number of bytes downloaded at this point
///
int CstiUpdate::bytesDownloadedGet ()
{
	return m_nBytesDownloaded;
}

///
/// \brief Returns the number of bytes written to flash at this point.
///
int CstiUpdate::bytesWrittenGet ()
{
	return m_nBytesWritten;
}

///
/// \brief Handles initial connection to the server
///
void CstiUpdate::eventUpdate (
		const std::string &url)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	bool bStartUpdate = false;

	stiDEBUG_TOOL (g_stiUpdateDebug,
		stiTrace ("CstiUpdate::eventUpdate\n");
	); // stiDEBUG_TOOL

	updateNeededSignal.Emit (true);

	if (url != m_updateUrl)
	{
		//
		// If we are currently updating we need to stop the update and
		// start a new one.
		//

		m_updateUrl = url;

		m_nProgress = 0;

		m_bResume = false;

		bStartUpdate = true;
	}
	else
	{
		// Check the update state: if an image is downloaded, write to flash
		if (updateDownloaded())
		{
			hResult = writeImageToFlash();
			stiTESTRESULT ();
		}
		else if (eIDLE == stateGet ())
		{
			m_bResume = true;

			bStartUpdate = true;
		}
	}

	if (bStartUpdate)
	{
		//
		// If we have not been told to pause then proceed with connecting
		// and downloading the update. Otherwise, enter into the "connecting paused" state.
		//
		if (m_nPauseCount == 0)
		{
			stateSet (eCONNECTING);

			// Call downloadProcessStart function to kick off the update process
			hResult = downloadProcessStart ();
			stiTESTRESULT ();
		}
		else
		{
			stateSet (eCONNECTING_PAUSED);
		}
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		cleanup();

		// Transition back to an idle state.
		stateSet (eIDLE);

		errorSignal.Emit ();
	}

	return;
} // end CstiUpdate::eventUpdate


///
/// \brief Boolean check if a firmware update image has been downloaded
///
/// \Return true if a firmware update is downloaded, otherwise false
///
bool CstiUpdate::updateDownloaded ()
{
	std::lock_guard<std::recursive_mutex> lockExec(m_execMutex);
	return m_bUpdateDownloaded;
}


//:-----------------------------------------------------------------------------
//: Private methods
//:-----------------------------------------------------------------------------


///
/// \brief Sets the current state
///
void CstiUpdate::stateSet (
	EState eState)
{
	m_eState = eState;

	// Switch on the new state...
	switch (eState)
	{
		case eUNINITIALIZED:
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				stiTrace ("CstiUpdate::stateSet - Changing state to eUNINITIALIZED\n");
			); // stiDEBUG_TOOL
			break;
		} // end case eUNITIALIZED

		case eIDLE:
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				stiTrace ("CstiUpdate::stateSet - Changing state to eIDLE\n");
			); // stiDEBUG_TOOL

			//idleSignal.Emit ();
			break;
		} // end case eIDLE

		case eCONNECTING:
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				stiTrace ("CstiUpdate::stateSet - Changing state to eCONNECTING\n");
			); // stiDEBUG_TOOL

			//connectingSignal.Emit ();
			break;
		} // end case eCONNECTING

		case eCONNECTING_PAUSED:
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				stiTrace ("CstiUpdate::stateSet - Changing state to eCONNECTING_PAUSED\n");
			); // stiDEBUG_TOOL

			break;
		} // end case eCONNECTING

		case eDOWNLOADING:
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				stiTrace ("CstiUpdate::stateSet - Changing state to eDOWNLOADING\n");
			); // stiDEBUG_TOOL

			downloadingSignal.Emit (0);

			break;
		} // end case eDOWNLOADING

		case ePROGRAMMING:
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				stiTrace ("CstiUpdate::stateSet - Changing state to ePROGRAMMING\n");
			); // stiDEBUG_TOOL

			programmingSignal.Emit (0);

			break;
		} // end case ePROGRAMMING

		case eDOWNLOAD_PAUSED:
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				stiTrace ("CstiUpdate::stateSet - Changing state to eDOWNLOAD_PAUSED\n");
			); // stiDEBUG_TOOL

			// NOTE: this is happening already, no need to callback again
			// idleSignal.Emit ();

			break;
		} // end case eDOWNLOAD_PAUSED

		case eCONNECTED:
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				stiTrace ("CstiUpdate::stateSet - Changing state to eCONNECTED\n");
			); // stiDEBUG_TOOL

			break;
		} // end case eCONNECTED
		
		case eUPDATE_COMPLETE:
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				stiTrace ("CstiUpdate::stateSet - Changing state to eUPDATE_COMPLETE\n");
			); // stiDEBUG_TOOL

			successfulSignal.Emit ();
			
			break;
		} // end case eNETWORK_ERROR
			
		
		case eNETWORK_ERROR:
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				stiTrace ("CstiUpdate::stateSet - Changing state to eNETWORK_ERROR\n");
			); // stiDEBUG_TOOL

			//errorSignal.Emit ();
			
			break;
		} // end case eNETWORK_ERROR
		
		case eCRC_ERROR:
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				stiTrace ("CstiUpdate::stateSet - Changing state to eCRC_ERROR\n");
			); // stiDEBUG_TOOL

			//errorSignal.Emit ();
			
			break;
		} // end case eCRC_ERROR
		
		case eFLASH_ERROR:
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				stiTrace ("CstiUpdate::stateSet - Changing state to eFLASH_ERROR\n");
			); // stiDEBUG_TOOL

			//errorSignal.Emit ();
			
			break;
		} // end case eFLASH_ERROR
			

	} // end switch

} // end CstiUpdate::stateSet

///
/// \brief Performs cleanup tasks.
///
stiHResult CstiUpdate::cleanup ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_pUpdateConnection)
	{
		m_pUpdateConnection->Close ();

		delete m_pUpdateConnection;
		m_pUpdateConnection = nullptr;
	}

	m_updateWriter = NULL;

	return hResult;
	
} // end CstiUpdate::cleanup

///
/// \brief Writes the previously downloaded image to flash
///
/// \Return estiOK when successful, else estiERROR
///
stiHResult CstiUpdate::writeImageToFlash ()
{
	PostEvent ([this]{ eventWriteToFlash (); });

	return stiRESULT_SUCCESS;
	
} // end CstiUpdate::writeImageToFlash ()


//:-----------------------------------------------------------------------------
//: Private functions
//:-----------------------------------------------------------------------------

///
/// \brief Writes the previously downloaded image to flash
///
/// \Return estiOK when successful, else estiERROR
///
void CstiUpdate::eventWriteToFlash ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Make sure we have an image downloaded
	stiTESTCOND (m_bUpdateDownloaded, stiRESULT_ERROR);
	
	// Alert the application that we are now in the programming state.
	stateSet (ePROGRAMMING);

	// Replace old image with newly downloaded one
	hResult = oldImageReplace (szLOCAL_FILE);
	stiTESTRESULT ();
	
	// Finished with all available downloads... Return to idle state
	stateSet (eUPDATE_COMPLETE);

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		cleanup();

		// Return to idle state
		stateSet (eIDLE);

		errorSignal.Emit ();
	}
	
	return;
} // end CstiUpdate::eventWriteToFlash

///
/// \brief Copies the successfully downloaded combo image to flash
///
stiHResult CstiUpdate::oldImageReplace (
	const char * pszNewImagePath)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nReturn;
	
	const char *pszOldImagePath = nullptr;
	long nHighestTimeStamp = 0;
	
	stiDEBUG_TOOL (g_stiUpdateDebug,
				stiTrace ("<CstiUpdate::OldImageReplace> pszNewImagePath = %s\n", pszNewImagePath);
			); // stiDEBUG_TOOL
	
	m_nProgress = 0;
	programmingSignal.Emit (0);
	
#if DEVICE == DEV_NTOUCH_VP2
	// Find the image to replace
	// this is done by checking timestamps and
	// checking to see that the checksum is valid
	hResult = oldImageGet (&pszOldImagePath, &nHighestTimeStamp);
	stiTESTRESULT();
	
	stiDEBUG_TOOL (g_stiUpdateDebug,
				stiTrace ("<CstiUpdate::OldImageReplace> pszOldImagePath = %s, nHighestTimeStamp = %d\n", pszOldImagePath, nHighestTimeStamp);
	); // stiDEBUG_TOOL
#elif DEVICE == DEV_NTOUCH_VP3
	pszOldImagePath = szOLD_FILE;
#endif
	
	// Get header of image to write
	hResult = imageTimeStampSet (pszNewImagePath, nHighestTimeStamp + 1);
	stiTESTRESULT();
	
	// Copy image from one location to another
	// There are many ways to do this
	// ImageCopy should choose the fastest method
	hResult = imageCopy (pszNewImagePath, pszOldImagePath);
	stiTESTRESULT();

STI_BAIL:
	
	//We want to erase the file if there was a problem
	//This will cause a new download
	nReturn = remove (pszNewImagePath);
	if (nReturn != 0)
	{
		stiTrace ("<CstiUpdate::OldImageReplace> Error: Removing file pszNewImagePath = %s\n", pszNewImagePath);
	}
	
	if (!stiIS_ERROR(hResult))
	{
		// We're 100% complete!!
		m_nProgress = 100;
		programmingSignal.Emit(m_nProgress);
	}
		
	
	return hResult;
}

///
/// \brief Returns the image to replace and the highest timestamp
///
stiHResult CstiUpdate::oldImageGet (
	const char **ppszOldImagePath,
	long * pnHighestTimeStamp)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	int nRead = 0;
	long nHighestTimeStamp = 0;
	long nTimeStampOfImage1 = 0;
	long nTimeStampOfImage2 = 0;
	image_header_t header1;
	image_header_t header2;
	FILE* fpImage1;
	FILE* fpImage2;
	
	// Open the file in /dev/mmcblk0p1 and /dev/mmcblk0p2
	// which correspond to the first and
	// second image locations respectively.
	fpImage1 = fopen (szIMAGE1, "r");
	stiTESTCOND (fpImage1, stiRESULT_ERROR);
	
	fpImage2 = fopen (szIMAGE2, "r");
	stiTESTCOND (fpImage2, stiRESULT_ERROR);
	
	// Find out which image is being run currently
	// to do this we must look at the time-stamp value
	// in the header

	// Read both headers
	nRead = fread (&header1, 1, sizeof (image_header_t), fpImage1);
	if (nRead != sizeof (image_header_t))
	{
		stiTrace ("<CstiUpdate::OldImageGet> Error: nRead = %d, sizeof (image_header_t) = %d\n", nRead, sizeof (image_header_t) );
		stiTHROW (stiRESULT_ERROR);
	}
	
	nRead = fread (&header2, 1, sizeof (image_header_t), fpImage2);
	if (nRead != sizeof (image_header_t))
	{
		stiTrace ("<CstiUpdate::OldImageGet> Error: nRead = %d, sizeof (image_header_t) = %d\n", nRead, sizeof (image_header_t) );
		stiTHROW (stiRESULT_ERROR);
	}
	
	// rewind files because we use them later
	rewind (fpImage1);
	rewind (fpImage2);
	
	nTimeStampOfImage1 = ntohl (header1.ih_time);
	nTimeStampOfImage2 = ntohl (header2.ih_time);

	stiDEBUG_TOOL (g_stiUpdateDebug,
			stiTrace ("<CstiUpdate::OldImageGet> nTimeStampOfImage1 = %d\n nTimeStampOfImage2 time-stamp = %d\n", nTimeStampOfImage1, nTimeStampOfImage2);
		); // stiDEBUG_TOOL

	// Which image has higher time-stamp value?
	if (nTimeStampOfImage1 > nTimeStampOfImage2)
	{
		nHighestTimeStamp = nTimeStampOfImage1;

		// Image1 is current.  Does it have a valid CRC?
		stiHResult hResult = checksumVerify (fpImage1);

		if (!stiIS_ERROR (hResult))
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				stiTrace ("<CstiUpdate::OldImageGet> Image1 is current, and has a valid crc.\nReplace Image2.\n");
			); // stiDEBUG_TOOL

			// The image to update is szIMAGE4
			*ppszOldImagePath = szIMAGE2;
		}
		else
		{
			// This could only happen if the crc was valid in U-boot
			stiDEBUG_TOOL (g_stiUpdateDebug,
				stiTrace ("<CstiUpdate::OldImageGet> Warning: Image1 is current, but has a bad CRC.\nReplace Image1.\n");
			); // stiDEBUG_TOOL

			// The image to update is szIMAGE1
			*ppszOldImagePath = szIMAGE1;
		}

	} // end if
	else
	{
		nHighestTimeStamp = nTimeStampOfImage2;

		// Image2 is current.  Does it have a valid CRC?
		stiHResult hResult = checksumVerify (fpImage2);

		if (!stiIS_ERROR (hResult))
		{
			stiDEBUG_TOOL (g_stiUpdateDebug,
				stiTrace ("<CstiUpdate::OldImageGet> Image2 is current, and has a valid crc.\nReplace Image1.\n");
			); // stiDEBUG_TOOL

			// The image to update is szIMAGE1
			*ppszOldImagePath = szIMAGE1;
		}
		else
		{
			// This could only happen if the crc was valid in U-boot
			stiDEBUG_TOOL (g_stiUpdateDebug,
				stiTrace ("<CstiUpdate::OldImageGet> Warning: Image2 is current, but has a bad CRC.\nReplace Image2.\n");
			); // stiDEBUG_TOOL

			// The image to update is szIMAGE2
			*ppszOldImagePath = szIMAGE2;
		}

	}

	// Close the files, we no longer need them open.
	fclose (fpImage1);
	fclose (fpImage2);

	*pnHighestTimeStamp = nHighestTimeStamp;

STI_BAIL:
	
	return hResult;

} // end CstiUpdate::OldImageGet

// Get header of image to write
stiHResult CstiUpdate::imageTimeStampSet (
	const char * pszNewImagePath,
	long nTimeStamp)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nRead;
	int nWritten;
	image_header_t Imageheader;
	CCRC ccrc;
	long nHeaderCRC;
	
	stiDEBUG_TOOL (g_stiUpdateDebug,
		stiTrace ("<CstiUpdate::ImageTimeStampSet> pszNewImagePath = %s, nTimeStamp = %d\n", pszNewImagePath, nTimeStamp);
	); // stiDEBUG_TOOL
	
	FILE* fp = fopen (pszNewImagePath, "rb+");
	if (!fp)
	{
		stiTrace ("<CstiUpdate::ImageTimeStampSet> Error: opening file = %s\n", pszNewImagePath);
		stiTHROW (stiRESULT_ERROR);
	}
	
	nRead = fread (&Imageheader, 1, sizeof (image_header_t), fp);
	if (nRead != sizeof (image_header_t))
	{
		stiTrace ("<CstiUpdate::ImageTimeStampSet> Error: nRead = %d, sizeof (image_header_t) = %d\n", nRead, sizeof (image_header_t) );
		stiTHROW (stiRESULT_ERROR);
	}

	// Manipulate header
	// Set the time-stamp
	Imageheader.ih_time = ntohl(nTimeStamp);

	// Set header crc to 0 so it will not be included in calculation of crc
	Imageheader.ih_hcrc = 0;
	
	// Get the new crc value (since we've changed the timestamp).
	nHeaderCRC = ccrc.getStringCRC ((const char*)&Imageheader, sizeof (image_header_t));

	stiDEBUG_TOOL (g_stiUpdateDebug,
		stiTrace ("<CstiUpdate::ImageTimeStampSet> New CRC nHeaderCRC = 0x%x.\n", nHeaderCRC);
	); // stiDEBUG_TOOL

	// Set the new header crc value
	Imageheader.ih_hcrc = ntohl(nHeaderCRC);
	
	// Write the new header to the file
	rewind (fp);
	
	nWritten = fwrite (&Imageheader, 1, sizeof (image_header_t), fp);
	if (nWritten != sizeof (image_header_t))
	{
		stiTrace ("<CstiUpdate::ImageTimeStampSet> Error: nWritten = %d, sizeof (image_header_t) = %d\n", nWritten, sizeof (image_header_t) );
		stiTHROW (stiRESULT_ERROR);
	}
	
	fclose (fp);
	
STI_BAIL:
	
	return hResult;
}

stiHResult CstiUpdate::imageCopy (
	const char * pszSrcImagePath,
	const char * pszDstImagePath)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	char pBuffer[BUFSIZ];
	size_t nRead = 0;
	size_t nWritten = 0;
	unsigned long long nTotalSize = 0;
	unsigned long long nCurrentSize = 0;
	size_t nCurrentProgress = 0;
	FILE* fpSrc = nullptr;
	FILE* fpDst = nullptr;
	
	fpSrc = fopen (pszSrcImagePath, "rb");
	stiTESTCOND (fpSrc, stiRESULT_ERROR);
	
	fpDst = fopen (pszDstImagePath, "wb");
	stiTESTCOND (fpDst, stiRESULT_ERROR);
	
	fseek (fpSrc, 0, SEEK_END);
	nTotalSize = ftell (fpSrc);
	fseek (fpSrc, 0, SEEK_SET);
	
	stiTrace ("<CstiUpdate::ImageCopy> copying image, nTotalSize = %llu\n", nTotalSize);

	while ((nRead = fread(pBuffer, sizeof(char), sizeof(pBuffer), fpSrc)) > 0)
	{
		nWritten = fwrite (pBuffer, sizeof(char), nRead, fpDst);
		
		if (nWritten != nRead)
		{
			stiTrace ("<CstiUpdate::ImageCopy> Error copying image, nRead = %d, nWritten = %d\n", nRead, nWritten);
			stiTHROW (stiRESULT_ERROR);
		}
		
		nCurrentSize = nCurrentSize + nWritten;
		
		nCurrentProgress = (unsigned int)((nCurrentSize * 100) / nTotalSize);
		
		//stiTrace ("<CstiUpdate::ImageCopy> copying image, nCurrentProgress = %d\n", nCurrentProgress);
		
		// Only callback around 50 times
		if (nCurrentProgress > m_nProgress + 2)
		{
			m_nProgress = nCurrentProgress;
			// Figure out progress
			programmingSignal.Emit (m_nProgress);
			stiTrace ("<CstiUpdate::ImageCopy> copying image, m_nProgress = %d\n", m_nProgress);
		}
	}
		
STI_BAIL:

	if (fpSrc)
	{
		fclose (fpSrc);
	}
	
	if (fpDst)
	{
		fclose (fpDst);
	}
	
	return hResult;
}
