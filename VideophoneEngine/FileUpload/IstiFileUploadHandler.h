/*!
*  \file IstiFileUploadHandler.h
*  \brief The File Upload Interface
*
*  Class declaration for the File Upload Interface Class.  
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2018 Sorenson Communications, Inc. -- All rights reserved
*
*/
#if !defined(ISTIFILEUPLOADHANDLER_H)
#define ISTIFILEUPLOADHANDLER_H

#include "CstiSignal.h"
#include "stiSVX.h"

//
// Forward Declarations
//
class IstiFileUploadHandler;

//
// External Declarations
//
stiHResult CstiFileUploadHandlerInitialize (
	PstiObjectCallback pfnAppCallback,
	size_t CallbackParam);

stiHResult CstiFileUploadHandlerUninitialize ();


/*!
 * \brief This class is used to view or record video messages.
 */
class IstiFileUploadHandler
{
public:
	/*!
	 * \brief Get instance
	 * 
	 * \return IstiUpdate* 
	 */
	static IstiFileUploadHandler *InstanceGet ();

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
		eUPLOADING,         
		eUPLOAD_PAUSED,
		eUPLOAD_RESUME,
		eUPLOAD_COMPLETE,
		eUPLOAD_ERROR,
		eUPLOAD_IDLE,
		eUPLOAD_INITIALIZED,
		eUPLOAD_STOP,
	};
		
	/*! 
	* \enum EError
	*  
	*  \brief EError lists the error states of the File Upload Handler.
	*/ 
	enum EError
	{
		eERROR_NONE,                          
		eERROR_UPLOADING,                          
		eERROR_INITIALIZING_UPLOAD,                          
	};
 
	/*!
	 * \brief Initialize the class.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult Initialize () = 0;

	
	/*!
	 * \brief Stores the file name and url in a structure and places an event on the FileUploadHandler's 
	 *  	  task queue to start the Upload
	 *  
	 * \param server ip of the upload server.
	 * \param fileName is the name of the saved file.
	 * \param filePath is the location of the saved file.
	 * \param fileUploadInfo contains info about the upload file.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult start (
		std::string server,
		std::string fileName,
		std::string filePath,
		std::string fileUploadInfo) = 0;

	/*!
	 * \brief Starts the task.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult startup () = 0;

	/*!
	 * \brief Stops the Upload and removes the file.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult stop () = 0;

	/*!
	 * \brief Enables or disables HTTPS.
	 * 
	 * \return stiRESULT_SUCCESS if successful 
	 *  	   stiRESULT_ERROR if an error occurs. 
	 */
	virtual stiHResult useHTTPS (EstiBool useHTTPS) = 0;

	/*!
	 * \brief Returns the current state of the File Upload Handler.
	 * 
	 * \return One of the states defined in the EState enum.
	 */
	virtual EState stateGet () const = 0;
	
	/*!
	 * \brief Returns the current error state of the File Upload Handler.
	 * 
	 * \return One of the errors defined in the EError enum.
	 */
	virtual EError errorGet () const = 0;

	virtual ISignal <>& fileUploadErrorSignalGet () = 0;
	virtual ISignal <std::string>& fileUploadCompleteSignalGet () = 0;
	virtual ISignal <unsigned int>& fileUploadProgressSignalGet () = 0;
};

#endif // ISTIFILEDOWNLOADHANDLER_H
