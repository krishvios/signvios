/*!
*  \file IstiFileDownloadHandler.h
*  \brief The File Download Interface
*
*  Class declaration for the File Download Interface Class.  
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2016 Sorenson Communications, Inc. -- All rights reserved
*
*/
#if !defined(ISTIFILEDOWNLOADHANDLER_H)
#define ISTIFILEDOWNLOADHANDLER_H

#include "CstiSignal.h"
#include "stiSVX.h"

//
// Forward Declarations
//
class IstiFileDownloadHandler;

//
// External Declarations
//
stiHResult CstiFileDownloadHandlerInitialize ();

stiHResult CstiFileDownloadHandlerUninitialize ();


/*!
 * \brief This class is used to view or record video messages.
 */
class IstiFileDownloadHandler
{
public:
	/*!
	 * \brief Get instance
	 * 
	 * \return IstiFileDownloadHandler* 
	 */
	static IstiFileDownloadHandler *InstanceGet ();

	//
	// State identifiers.
	// Each value must be a power of two.
	//
	/*! 
	*  \enum EState
	*  
	*  \brief EState lists the valid states of the Message Viewer.
	* 
	*/ 
	enum EState
	{
		eDOWNLOADING,         
		eDOWNLOAD_PAUSED,
		eDOWNLOAD_RESUME,
		eDOWNLOAD_COMPLETE,
		eDOWNLOAD_ERROR,
		eDOWNLOAD_IDLE,
		eDOWNLOAD_INITIALIZED,
		eDOWNLOAD_STOP,
	};
		
	/*! 
	* \enum EError
	*  
	*  \brief EError lists the error states of the File Download Handler.
	*/ 
	enum EError
	{
		eERROR_NONE,                          
		eERROR_DOWNLOADING,                          
		eERROR_INITIALIZING_DOWNLOAD,                          
	};
 
	/*!
	 * \brief Places an event on the FileDownloadHandler's task queue to pause the download.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult Pause () = 0;
	
	/*!
	 * \brief Places an event on the FileDownloadHandler's task queue to resume the download.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult Resume () = 0;
	
	/*!
	 * \brief Stores the file name and url in a structure and places an event on the FileDownloadHandler's 
	 *  	  task queue to start the download
	 *  
	 * \param pFileName is the name and location of the saved file.
	 * \param pUrl is the name and download location of the file to download.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult Start (
		const std::string &fileName,
		const std::string &url) = 0;

	/*!
	 * \brief Starts the task.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult Startup () = 0;

	/*!
	 * \brief Stops the download and removes the file.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult Stop () = 0;

	/*!
	 * \brief Enables or disables HTTPS.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult UseHTTPS (EstiBool useHTTPS) = 0;

	/*!
	 * \brief Returns the current state of the File Download Handler.
	 * 
	 * \return One of the states defined in the EState enum.
	 */
	virtual EState StateGet () const = 0;
	
	/*!
	 * \brief Returns the current error state of the File Download Handler.
	 * 
	 * \return One of the errors defined in the EError enum.
	 */
	virtual EError ErrorGet () const = 0;

	virtual ISignal <int>& fileDownloadProgressSignalGet () = 0;
	virtual ISignal <EState>& fileDownloadStateSetSignalGet () = 0;
};

#endif // ISTIFILEDOWNLOADHANDLER_H
