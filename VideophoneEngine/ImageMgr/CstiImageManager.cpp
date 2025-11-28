/*!
*  \file CstiImageManager.cpp
*  \brief The purpose of the ImageManager task is to manage the loading and
*         storing of images from and to the filesystem.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*
*/
//
// Includes
//
#ifdef stiIMAGE_MANAGER
#include "CstiImageManager.h"

#include "CstiCoreRequest.h"
#include "CstiCoreService.h"
#include "CstiImageDownloadService.h"
#include "CstiNvmImageCache.h"
#include "PropertyManager.h"
#include "cmPropertyNameDef.h"
#include "CstiExtendedEvent.h"

#include "stiTrace.h"

#include <algorithm>
#include <iterator>
#include <fstream>
#include <set>

#include <dirent.h>
#include <csetjmp>
#include <cstdio>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

using namespace WillowPM;
using namespace boost::filesystem;

#if (   ((DEVICE == DEV_X86) || (DEVICE == DEV_NTOUCH_VP2) || (DEVICE == DEV_NTOUCH_VP3) || (DEVICE == DEV_NTOUCH_VP4) || defined(stiLIBJPEG_DECODE_ON_DEV_NTOUCH))\
	 && (APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3|| APPLICATION == APP_NTOUCH_VP4 || APPLICATION == APP_NTOUCH_PC))
	#define USE_LIBJPEG_TO_DECODE
#endif

#ifdef USE_LIBJPEG_TO_DECODE
extern "C"
{
	#include "jpeglib.h"
}
#endif

//
// Constant
//
#define stiIMAGE_DISABLED_ID "-3"
#define stiIMAGE_RAM_CACHE_SIZE    40

#define stiIMAGE_MAX_MESSAGES_IN_QUEUE 40
#define stiIMAGE_MAX_MSG_SIZE 512
#define stiIMAGE_TASK_NAME "CstiImageManager"
#define stiIMAGE_TASK_PRIORITY 151
#define stiIMAGE_STACK_SIZE 4096

#define stiIMAGE_INPUT_SIZE (1280 * 720)
#define stiIMAGE_OUTPUT_SIZE (1280 * 720)

namespace
{
	const char* NVM_CACHE_PATH_SUFFIX = "contacts/photo_cache";

	const unsigned int NVM_FULLSIZE_CACHE_CAPACITY = 100;

	const unsigned int MAX_DOWNLOAD_QUEUE_SIZE = 20;

	const uint32_t NVM_MANIFEST_WRITE_MSEC = 1000 * 60 * 5;
}

stiEVENT_MAP_BEGIN (CstiImageManager)
stiEVENT_DEFINE (estiEVENT_SHUTDOWN, CstiImageManager::ShutdownHandle)
stiEVENT_DEFINE (estiIMAGE_REQUESTED, CstiImageManager::ImageLoad)
stiEVENT_DEFINE (estiIMAGE_ENCODE, CstiImageManager::ImageEncodeAndCache)
stiEVENT_DEFINE (estiIMAGE_DOWNLOAD_COMPLETED, CstiImageManager::LoadDownloadedImage)
stiEVENT_DEFINE (estiIMAGE_DOWNLOAD_FAILED, CstiImageManager::CancelImageDownload)
stiEVENT_DEFINE (estiIMAGE_ADD, CstiImageManager::EventImageAdd)
stiEVENT_MAP_END (CstiImageManager)
stiEVENT_DO_NOW (CstiImageManager)


//
// Globals
//
/*!
 * \brief Determines whether or not core can be expected to provide the
 * specified image if asked.
 *
 * \param imageId image GUID to test
 *
 * \return true when core is supposed to have the specified image, false
 *         otherwise
 */
bool CoreHasImage(const std::string& imageId)
{
	static const std::vector<std::string> reserved =
	{
		"00000000-0000-0000-0000-000000001000",
		"00000000-0000-0000-0000-000000001001",
		"00000000-0000-0000-0000-000000001002"
	};

	// Do not try to ask Core for one of our reserved image IDs.
	if (std::find(reserved.begin(), reserved.end(), imageId) != reserved.end())
	{
		return false;
	}

	// Check for avatar IDs.
	return !(   (imageId.length() == IMAGE_GUID_LENGTH)
			 && !strncmp(imageId.c_str(), AvatarGUIDPrefix, strlen(AvatarGUIDPrefix))
			 && isdigit(imageId[32])
			 && isdigit(imageId[33])
			 && isdigit(imageId[34])
			 && isdigit(imageId[35]));
}


/*! \brief Constructor
 *
 * This is the default constructor for the CstiImageManager class.
 */
CstiImageManager::CstiImageManager(
	CstiCoreService *pCoreService,
	PstiObjectCallback pfnAppCallback,
	size_t CallbackParam)
:
	CstiOsTaskMQ(
		pfnAppCallback,
		CallbackParam,
		(size_t)this,
		stiIMAGE_MAX_MESSAGES_IN_QUEUE,
		stiIMAGE_MAX_MSG_SIZE,
		stiIMAGE_TASK_NAME,
		stiIMAGE_TASK_PRIORITY,
		stiIMAGE_STACK_SIZE),
	m_bInitialized(estiFALSE),
	m_nNumImages(0),
	m_unRequestId(0),
	m_pCoreService(pCoreService),
	m_pFullsizeNvmCache(nullptr),
	m_pDownloadService(nullptr),
	m_DownloadState(DL_IDLE),
	m_LastNvmManifestWriteMsec(0),
	m_pContactManager(nullptr),
	m_bContactImagesEnabled(estiFALSE)
{
	stiLOG_ENTRY_NAME("CstiImageManager::CstiImageManager\n");

	std::string StaticDataFolder;

	stiOSStaticDataFolderGet (&StaticDataFolder);

	m_ImageDirectory << StaticDataFolder << "images/";
}

/*! \brief Destructor
 *
 * This is the default destructor for the CstiImageManager class.
 *
 * \return None
 */
CstiImageManager::~CstiImageManager ()
{
	stiLOG_ENTRY_NAME(CstiImageManager::~CstiImageManager);

	Uninitialize();
}

/*! \brief Initialize the CstiImageManager task
 *
 * \return stiHResult 
 * \retval stiRESULT_SUCCESS if success
 */
stiHResult CstiImageManager::Initialize (
	EstiBool bContactImagesEnabled)
{
	stiLOG_ENTRY_NAME(CstiImageManager::Initialize);

	stiHResult hResult = stiRESULT_SUCCESS;
	std::string basePath;
	std::stringstream ss;

	ContactImagesEnabledSet(bContactImagesEnabled);

	m_Mutex = stiOSMutexCreate();

	m_pDownloadService =
		new CstiImageDownloadService(HandleImageDownloadCompleted, (size_t)this);
	stiTESTCOND (m_pDownloadService, stiRESULT_ERROR);

	m_pDownloadService->Initialize();

	StaticManifestBuild();

	stiOSDynamicDataFolderGet(&basePath);

	ss << basePath
	   << NVM_CACHE_PATH_SUFFIX;

	m_pFullsizeNvmCache =
		new CstiNvmImageCache(ss.str(), NVM_FULLSIZE_CACHE_CAPACITY);
	stiTESTCOND (m_pFullsizeNvmCache, stiRESULT_ERROR);

	m_pFullsizeNvmCache->Initialize();

	m_bInitialized = estiTRUE;

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		Uninitialize ();
	}

	return (hResult);

}//end CstiImageManager::Initialize ()

/*! \brief Final cleanup of the CstiImageManager task
 *
 *  \retval None
 */
void CstiImageManager::Uninitialize ()
{
	stiLOG_ENTRY_NAME (CstiImageManager::Uninitialize);

    CacheClear();

	if (m_Mutex)
	{
		stiOSMutexDestroy(m_Mutex);
		m_Mutex = nullptr;
	}

	if (m_pDownloadService)
	{
		delete m_pDownloadService;
		m_pDownloadService = nullptr;
	}

	delete m_pFullsizeNvmCache;
	m_pFullsizeNvmCache = nullptr;

	m_bInitialized = estiFALSE;

}//end CstiImageManager::Uninitialize

void CstiImageManager::CacheClear()
{
	std::list<SstiImageInfo>::iterator it;

    stiOSMutexLock(m_Mutex);

	m_RAMCache.clear();

	stiOSMutexUnlock(m_Mutex);
}

/*! \brief Start the CstiImageManager task
 *
 *  \return stiHResult
 *  \retval stiRESULT_SUCCESS if success
 */
stiHResult CstiImageManager::Startup()
{
	stiLOG_ENTRY_NAME (CstiImageManager::Startup);

	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = m_pDownloadService->Startup();
	stiTESTRESULT();

	//
	// Start the task
	//
	hResult = CstiOsTaskMQ::Startup();
	stiTESTRESULT ();

STI_BAIL:

	return (hResult);
}

/*! \brief Task function, executes until the task is shutdown.
 *
 * This task will loop continuously looking for messages in the message
 * queue.  When it receives a message, it will call the appropriate message
 * handling routine.
 * 
 * \return int
 * \retval Always returns 0
 */
int CstiImageManager::Task ()
{
	stiLOG_ENTRY_NAME (CstiImageManager::Task);

	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32Buffer[(stiIMAGE_MAX_MSG_SIZE / sizeof (uint32_t)) + 1];

	fd_set SReadFds;

	int nMsgFd = FileDescriptorGet ();

	struct timeval timeout{};

	//
	// Continue executing until shutdown
	//
	for (;;)
	{
		//
		// Initialize select.
		//
		stiFD_ZERO (&SReadFds);

		// select on the message pipe
		stiFD_SET (nMsgFd, &SReadFds);

		// setup the select timeout
		timeout.tv_sec = 60;
		timeout.tv_usec = 0;

		// wait for data to come in on the message queue
		int nSelectRet = stiOSSelect (stiFD_SETSIZE, &SReadFds, nullptr, nullptr, &timeout);

		if (stiERROR != nSelectRet)
		{
			// Check if ready to read from message queue
			if (stiFD_ISSET (nMsgFd, &SReadFds))
			{
				// read a message from the message queue
				hResult = MsgRecv ((char *)un32Buffer, stiIMAGE_MAX_MSG_SIZE, stiWAIT_FOREVER);

				// Was a message received?
				if (stiIS_ERROR (hResult))
				{
					Callback (estiMSG_CB_ERROR_REPORT, (size_t)m_pstErrorLog);
				}
				else
				{
					// Yes! Process the message.
					auto poEvent = (CstiEvent*)(void *)un32Buffer;

					// Lookup and handle the event
					hResult = EventDoNow (poEvent);

					if (stiIS_ERROR (hResult))
					{
						Callback (estiMSG_CB_ERROR_REPORT, (size_t)m_pstErrorLog);
					} // end if

					// Is the event a "shutdown" message?
					if (estiEVENT_SHUTDOWN == poEvent->EventGet ())
					{
						// Yes.  Time to quit.  Break out of the loop
						break;
					} // end if
				} // end if
			}

			uint32_t now = stiOSTickGet();

			if (   (now < m_LastNvmManifestWriteMsec)
				|| ((now - m_LastNvmManifestWriteMsec) > NVM_MANIFEST_WRITE_MSEC))
			{
				m_pFullsizeNvmCache->WriteManifestIfDirty();

				m_LastNvmManifestWriteMsec = now;
			}

		}
	} // end for

	return (0);

} // end CstiImageManager::Task


/*! \brief Starts the shutdown process of the CstiImageManager task
 *
 * This method is responsible for doing all clean up necessary to gracefully
 * terminate the CstiImageManager task. Note that once this method exits,
 * there will be no more message received from the message queue since the
 * message queue will be deleted and the CstiImageManager task will go away.
 * 
 * \param IN void* poEvent 
 * 
 * \return stiHResult 
 * \retval stiRESULT_SUCCESS if success
 * \retval stiRESULT_() otherwise
 */
stiHResult CstiImageManager::ShutdownHandle (IN void* poEvent)
{
	stiLOG_ENTRY_NAME (CstiImageManager::ShutdownHandle);

	stiHResult hResult = stiRESULT_SUCCESS;
	
	//
	// Shutdown m_pDownloadService
	//
	if (nullptr != m_pDownloadService)
	{
		hResult = m_pDownloadService->Shutdown ();
		m_pDownloadService->WaitForShutdown ();
	}

	CacheClear ();

	// Call the base class method
	hResult = CstiOsTaskMQ::ShutdownHandle ();

	return (hResult);
} // end CstiImageManager::ShutdownHandle

/*! \brief Request an image either from cache or from filesystem
 * 
 * \param SstiImageInfo* pInfo 
 * \param unsigned char** ppData 
 * 
 * \return stiHResult 
 * \retval stiRESULT_SUCCESS if success
 * \retval stiRESULT_() otherwise
 */
stiHResult CstiImageManager::ImageRequest(
	SstiImageInfo *pInfo,
	std::shared_ptr<ImageArray> *pData)
{
	stiLOG_ENTRY_NAME (CstiImageManager::ImageRequest);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (pInfo, stiRESULT_ERROR);

	// If the asked for image is the GUID that indicates no image selected or
	// the GUID isn't valid, give back the silhouette avatar
	if (pInfo->ImageId == NoGUID || !IsGUIDValid(pInfo->ImageId))
	{
		pInfo->ImageId = SilhouetteAvatarGUID;
	}

	// First check the RAM cache.  If we find it here, we're done!
	ImageGetFromRAMCache(pInfo, pData);

	if (!*pData)
	{
		// See if we are already in the process of loading it.  If so,
		// then we'll just wait for that to finish
		pInfo->unRequestId = ImageIsLoading(pInfo);

		if (!pInfo->unRequestId)
		{
			unsigned int index;

			if (ContactImagesEnabledGet() && CoreHasImage(pInfo->ImageId))
			{
				// determine the filename for this image in the NVM cache
				stiOSDynamicDataFolderGet(&pInfo->Filename);

				pInfo->Filename += NVM_CACHE_PATH_SUFFIX;
				pInfo->Filename += "/";
				pInfo->Filename += pInfo->ImageId;

				// First check the NVM cache to see if downloading the image
				// can be avoided.
				bool cache_hit = false;

				if (m_pFullsizeNvmCache->Contains(pInfo->ImageId))
				{
					cache_hit = true;

					pInfo->Filename =
						m_pFullsizeNvmCache->FilePath(pInfo->ImageId);

					m_pFullsizeNvmCache->MoveToFront(pInfo->ImageId);
				}

				if (cache_hit)
				{
					// it exists on the filesystem, so schedule it to be
					// loaded into the RAM cache
					pInfo->unRequestId = NextRequestIdGet();

					auto pInfoCopy = new SstiImageInfo(*pInfo);

					stiOSMutexLock(m_Mutex);
					m_Loading.push_back(*pInfo);
					stiOSMutexUnlock(m_Mutex);

					// Post a message to the message queue
					CstiEvent oEvent(CstiImageManager::estiIMAGE_REQUESTED, (stiEVENTPARAM)pInfoCopy);
					hResult = MsgSend (&oEvent, sizeof (oEvent));
				}
				else if (!cache_hit && m_pCoreService)
				{
					Download download;

					pInfo->unRequestId = NextRequestIdGet();

					download.image = *pInfo;

					stiOSMutexLock(m_Mutex);
					m_Downloading.push_front(download);

					if (m_Downloading.size() > MAX_DOWNLOAD_QUEUE_SIZE)
					{
						m_Downloading.pop_back();
					}

					stiOSMutexUnlock(m_Mutex);

					DownloadSomething();
				}
			}
			else if (StaticManifestSearch(pInfo, &index) == stiRESULT_SUCCESS)
			{
				// This will work for static images that have been explicitly added by the application

				if (m_StaticManifest[index].Data == nullptr)
				{
					// We don't have it, so we'll need to load it from the filesystem

					// Create a request ID so the application we can keep track of who
					// is waiting for which image.
					pInfo->unRequestId = NextRequestIdGet();

					auto pInfoCopy = new SstiImageInfo (*pInfo);
					pInfoCopy->Filename = m_StaticManifest[index].Filename;

					stiOSMutexLock(m_Mutex);
					m_Loading.push_back(*pInfo);
					stiOSMutexUnlock(m_Mutex);

					// Post a message to the message queue
					CstiEvent oEvent(CstiImageManager::estiIMAGE_REQUESTED, (stiEVENTPARAM)pInfoCopy);
					hResult = MsgSend (&oEvent, sizeof (oEvent));
				}
				else
				{
					*pData = m_StaticManifest[index].Data;
				}
			}
		}
	}

STI_BAIL:

	return hResult;
}


/*!
 * \brief Requests image size and moves the image to the front of the cache.
 * 
 * \param pInfo 
 * 
 * \return stiHResult 
 */
stiHResult CstiImageManager::ImageSizeRequest(SstiImageInfo *pInfo)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::list<SstiImageInfo>::iterator it;

	// Lock access to the cache
	stiOSMutexLock(m_Mutex);

	for (it = m_RAMCache.begin(); it != m_RAMCache.end(); it++)
	{
		// Search for the correct GUID first
		if ((*it).ImageId == pInfo->ImageId)
		{
			// Then check the other values
			if ((*it).bDim == pInfo->bDim)
			{
				pInfo->unWidth = (*it).unWidth;
				pInfo->unHeight = (*it).unHeight;
				break;
			}
		}
	}

    if (pInfo->unWidth == 0 && pInfo->unHeight == 0)
	{
		std::vector<SstiImageInfo>::iterator itStatic;

		for (itStatic = m_StaticManifest.begin(); itStatic != m_StaticManifest.end(); itStatic++)
		{
			// Compare the GUID first.
			if ((*itStatic).ImageId == pInfo->ImageId)
			{
				// Then check the other values
				if ((*itStatic).bDim == pInfo->bDim)
				{
					pInfo->unWidth = (*itStatic).unWidth;
					pInfo->unHeight = (*itStatic).unHeight;
					break;
				}
			}
		}
	}

	stiOSMutexUnlock(m_Mutex);

	return hResult;

}

/*! \brief Searches RAM cache for image data
 *
 * This method searches our RAM cache of already-created images to avoid having
 * to load the image again.  If an image is found, it is also moved to the
 * front of the list for faster retrieval next time.
 * 
 * \param SstiImageInfo* pInfo 
 * \param unsigned char** ppData 
 * 
 * \return stiHResult 
 * \retval stiRESULT_SUCCESS if success
 * \retval stiRESULT_() otherwise
 * 
 */
stiHResult CstiImageManager::ImageGetFromRAMCache(
	SstiImageInfo *pInfo,
	std::shared_ptr<ImageArray> *pData)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::list<SstiImageInfo>::iterator it;

	// Lock access to the cache
	stiOSMutexLock(m_Mutex);

	for (it = m_RAMCache.begin(); it != m_RAMCache.end(); it++)
	{
		// Search for the correct GUID first
		if ((*it).ImageId == pInfo->ImageId)
		{
			// Then check the other values
			if ((*it).unWidth == pInfo->unWidth &&
				(*it).unHeight == pInfo->unHeight &&
				(*it).bDim == pInfo->bDim)
			{
				// Move this item to the head of the list because it is now
				// our Most Recently Used (MRU) object.
				if (it != m_RAMCache.begin())
				{
					m_RAMCache.splice(m_RAMCache.begin(), m_RAMCache, it);
				}

				*pData = (*it).Data;

				break;
			}
		}
	}

	stiOSMutexUnlock(m_Mutex);

	return hResult;
}

/*! \brief Adds image data to the RAM cache
 *
 * This method pushes the specified image data onto the front of our list
 * of cached info.  If we exceed our allowable cache size, we drop an
 * image off the back of the list.
 * 
 * 	\param SstiImageInfo *pInfo
 * 
 * \return stiHResult 
 * \retval stiRESULT_SUCCESS if success
 * \retval stiRESULT_() otherwise
 */
stiHResult CstiImageManager::ImageSaveToRAMCache(SstiImageInfo *pInfo)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Lock access to the cache
	stiOSMutexLock(m_Mutex);

	// Check to see if we have reached our limit.
	while (m_RAMCache.size() >= stiIMAGE_RAM_CACHE_SIZE)
	{
		m_RAMCache.pop_back();
	}

	// Add the new item to the front of our list.  It remains the Most
	// Recently Used until something else is added to or read from the
	// cache.
	m_RAMCache.push_front(*pInfo);

	stiOSMutexUnlock(m_Mutex);

	return hResult;
}

/*! \brief Event Handler for the estiIMAGE_REQUESTED message
 *
 * This method is responsible for loading an image from the filesystem and
 * if necessary, resizing it to the requested dimensions.
 * 
 * \param IN void *poEvent
 * 
 *  \return stiHResult
 *  \retval stiRESULT_SUCCESS if success
 *  \retval stiRESULT_ERROR otherwise
 */
stiHResult CstiImageManager::ImageLoad(IN void *poEvent)
{
	stiLOG_ENTRY_NAME (CstiImageManager::ImageLoad);

	stiHResult hResult = stiRESULT_SUCCESS;

	auto pInfo = (SstiImageInfo *)((CstiEvent *)poEvent)->ParamGet();
	stiTESTCOND (pInfo, stiRESULT_ERROR);

	hResult = ImageDecode(pInfo);
	stiTESTRESULT ();

	// Make sure we actually have data
	if (pInfo->Data != nullptr)
	{
		// Everything looks good, let's store it so we don't need to do that again.
		ImageSaveToRAMCache(pInfo);
	}

	// Notify the application that the cookies are done baking.
	// Er...I mean, the image is ready.
	Callback(estiMSG_IMAGE_LOADED, pInfo->unRequestId);

STI_BAIL:
	// BUG 19144: In either case (error or not), pop the SstiImageInfo pointer
	//  that was pushed onto the m_Loading list.
	stiOSMutexLock(m_Mutex);
	m_Loading.pop_front();
	stiOSMutexUnlock(m_Mutex);

	delete pInfo;

	return hResult;
}

/*! \brief Event Handler for the estiIMAGE_ENCODE message
 *
 * This method is responsible for encoding an image from bitmap format to a JPEG
 * on the filesystem and then adding it to the NVM cache.
 *
 * \param IN void *poEvent
 *
 *  \return stiHResult
 *  \retval stiRESULT_SUCCESS if success
 *  \retval stiRESULT_ERROR otherwise
 */
stiHResult CstiImageManager::ImageEncodeAndCache(IN void *poEvent)
{
	stiLOG_ENTRY_NAME (CstiImageManager::ImageEncodeAndCache);

	stiHResult hResult = stiRESULT_SUCCESS;

	auto  pEncodeSpec = (Encode *)((CstiEvent *)poEvent)->ParamGet();
	stiTESTCOND (pEncodeSpec, stiRESULT_ERROR);

	PurgeImageFromCache(pEncodeSpec->id.c_str());

	hResult = EncodeJPEG(*pEncodeSpec);
	stiTESTRESULT ();

	m_pFullsizeNvmCache->Add(pEncodeSpec->id, std::string());

	Callback(estiMSG_IMAGE_ENCODED, pEncodeSpec->request_id);

STI_BAIL:

	delete pEncodeSpec;

	return hResult;
}

#ifdef USE_LIBJPEG_TO_DECODE
struct my_error_mgr
{
	struct jpeg_error_mgr pub;	/* "public" fields */

	jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr *my_error_ptr;

/*!
 * \brief 
 *  It seems silly that we need to create our own error handler for the         
 *  jpeg decoding.  All I want it to do is print an error saying it didn't work,
 *  and return an error code.  But instead, they put an actual call to exit()   
 *  as the default error handler, so the entire application goes down.  What    
 *  is the point of that?  Doesn't that sound rude to you?  Shouldn't it be up  
 *  to the application to decide to exit, and not the choice of some 3rd        
 *  party library?
 *  
 * \param j_common_ptr cinfo 
 */
void my_error_exit(j_common_ptr cinfo)
{
	auto  myerr = (my_error_ptr)cinfo->err;
	(*cinfo->err->output_message) (cinfo);
	longjmp(myerr->setjmp_buffer, 1);
}

int decompressImage (
	SstiImageInfo *pInfo,
	FILE *pSrc,
	int *width,
	int *height)
{
	struct my_error_mgr jerr{};
	struct jpeg_decompress_struct cinfo{};

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;

	if (setjmp(jerr.setjmp_buffer))
	{
		// If we get here, the JPEG code has signaled an error.
		// We need to clean up the JPEG object, close the input file, and return.
		jpeg_destroy_decompress(&cinfo);

		return 1;
	}

	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, pSrc);
	(void) jpeg_read_header(&cinfo, TRUE);

	cinfo.out_color_space = JCS_EXT_BGRX;

	cinfo.do_fancy_upsampling = false;

	jpeg_start_decompress(&cinfo);

	pInfo->Data = std::make_shared<ImageArray> (cinfo.output_width * cinfo.output_height * cinfo.output_components, 0);

	int row_stride = cinfo.output_width * cinfo.output_components;
	unsigned char *pPut = pInfo->Data->data();

	while (cinfo.output_scanline < cinfo.output_height)
	{
		jpeg_read_scanlines(&cinfo, &pPut, 1);
		pPut += row_stride;
	}

	*width = cinfo.output_width;
	*height = cinfo.output_height;

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	return 0;
}
#endif // USE_LIBJPEG_TO_DECODE

/*! \brief Decodes a specified image from the filesystem
 *
 * This method is responsible for opening an image file from the filesystem,
 * decoding it into an RGB buffer and if necessary, resizing it to the
 * requested dimensions.
 *
 * \param SstiImageInfo *pInfo
 *
 * \return stiHResult
 * \retval stiRESULT_SUCCESS if success
 * \retval stiRESULT_() otherwise
 */
stiHResult CstiImageManager::ImageDecode(SstiImageInfo *pInfo)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	FILE *pSrc = nullptr;
	int nWidth = 0, nHeight = 0;
	int result = 0;

	// Look for the requested file.  In some instances we are not given an extension only the name (i.e. image instead of image.jpg)
	try
	{
		if (!exists (pInfo->Filename.c_str ()))
		{
			// If we can't find the file we problaby have a name without the extension so look for a matching name. 
			path filePath = path (pInfo->Filename).parent_path ();
			std::string fileName = path (pInfo->Filename).filename ().string ();

			if (is_directory(filePath))
			{
				for (auto &fileInDir : boost::make_iterator_range(directory_iterator (filePath), {}))
				{
					std::string extFileName = fileInDir.path ().string ();
					if (extFileName.find (fileName) != std::string::npos)
					{
						// We found a matching name so open the file.
						pSrc = fopen (extFileName.c_str (), "rb");
						break;
					}
				}
			}
		}
		else
		{
			pSrc = fopen(pInfo->Filename.c_str (), "rb");
		}
		stiTESTCOND(pSrc != nullptr, stiRESULT_ERROR);
	} 

	catch (const filesystem_error &ex)
	{
		stiTHROW(stiRESULT_ERROR);
	}

#ifdef USE_LIBJPEG_TO_DECODE

	// We use the open-source libjpeg API to decode the image when running on a PC.

	result = decompressImage (pInfo, pSrc, &nWidth, &nHeight);
	stiTESTCOND (result == 0, stiRESULT_ERROR);

#endif // USE_LIBJPEG_TO_DECODE

	// If the info's width or height are 0 we don't know the size so
	// just default to the image's decoded width and height.
	if (pInfo->unWidth == 0)
	{
		pInfo->unWidth = nWidth;
	}

	if (pInfo->unHeight == 0)
	{
		pInfo->unHeight = nHeight;
	}

	if (pInfo->unWidth != nWidth || pInfo->unHeight != nHeight)
	{
		std::shared_ptr<ImageArray> NewData;

		ImageResize(pInfo->Data, &NewData, nWidth, nHeight, pInfo->unWidth, pInfo->unHeight);

		pInfo->Data = NewData;
	}

	// files from the static manifest are already dim on disk, but files
	// from the contact photo NVM cache are never dim on disk so they must be
	// dimmed here when appropriate
	if (   pInfo->bDim
		&& strncmp(m_ImageDirectory.str ().c_str (),
				   pInfo->Filename.c_str(),
				   m_ImageDirectory.str ().length ()) != 0)
	{
		const unsigned int unByteCount = pInfo->unWidth * pInfo->unHeight * 3;

		// RGB888 format
		unsigned char *pData = pInfo->Data->data ();

		for (unsigned int nColorByte = 0; nColorByte < unByteCount; ++nColorByte)
		{
			pData[nColorByte] >>= 1;
		}
	}

STI_BAIL:

	if (pSrc)
	{
		fclose(pSrc);
	}

	return hResult;
}

/*! \brief Resizes image data to new specified dimensions
 *
 * This method uses a very basic resize algorithm that just drops pixels to
 * create a new image of the appropriate size.  We could use a nicer algorithm
 * (bilinear, bicubic, etc.) but we're concerned about the amount of time it
 * would take.
 * \param unsigned char* pOldData 
 * \param unsigned char** pNewData 
 * \param unsigned short unSrcWidth 
 * \param unsigned short unSrcHeight 
 * \param unsigned short unDestWidth 
 * \param unsigned short unDestHeight 
 * 
 * \return stiHResult 
 * \retval stiRESULT_SUCCESS if success
 * \retval stiRESULT_() otherwise
 */
stiHResult CstiImageManager::ImageResize(
	const std::shared_ptr<const ImageArray> &OldData,
	std::shared_ptr<ImageArray> *pNewData,
	unsigned short unSrcWidth, unsigned short unSrcHeight,
	unsigned short unDestWidth, unsigned short unDestHeight)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	int x, y, ex, ey;
	int sw, dw, sh, dh;
	int ySrc = 0;

	*pNewData = std::make_shared<ImageArray>(unDestWidth * unDestHeight * 4, 0);

	dw = unDestWidth << 1;
	dh = unDestHeight << 1;
	sw = unSrcWidth << 1;
	sh = unSrcHeight << 1;
	ey = sh - unDestHeight;

	int nSrcStride = unSrcWidth * 4;

	const unsigned long *pGet = (unsigned long *)OldData->data ();
	auto pPut = (unsigned long *)(*pNewData)->data ();

	y = unDestHeight;

	int nSrcY = 0;

	while (y--)
	{
		ex = sw - unDestWidth;
		pGet = (unsigned long *)(OldData->data () + ySrc);
		x = unDestWidth;

		int nSrcX = 0;

		while (x--)
		{
			*pPut++ = *pGet;

			while (ex >= 0)
			{
				if (nSrcX < unSrcWidth - 1)
				{
					// Move to the next pixel
					pGet++;
					nSrcX++;
				}

				ex -= dw;
			}
			ex += sw;
		}

		while (ey >= 0)
		{
			if (nSrcY < unSrcHeight - 1)
			{
				// Move to the next line
				ySrc += nSrcStride;
				nSrcY++;
			}

			ey -= dh;
		}
		ey += sh;
	}

	return hResult;
}

/*! \brief Builds a log (manifest) of all the images found on the static filesystem
 *
 * This method searches the static image directory for any files it can find.
 * For each one that is found, we create an SstiImageInfo struct with
 * information about the file and add it to our list.  This is so we don't need
 * to search the filesystem every time we request an image.  Note that we do NOT
 * actually load any images here.  We only extract useful information about
 * them.
 *
 * \return stiHResult 
 * \retval stiRESULT_SUCCESS if success
 * \retval stiRESULT_() otherwise
 */
stiHResult CstiImageManager::StaticManifestBuild()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	struct dirent *pFile;
	DIR *pDir = opendir(m_ImageDirectory.str ().c_str ());

	if (pDir)
	{
		// For each file in the directory....
		while ((pFile = readdir(pDir)) != nullptr)
		{
			// Construct the full filename and extract the rest of the information from it.
			SstiImageInfo info;
			info.Filename = m_ImageDirectory.str ();
			info.Filename += "/";
			info.Filename += pFile->d_name;

			if (ImageFileInfoGet(&info) == stiRESULT_SUCCESS)
			{
				ImageAdd(&info);
			}
		}

		closedir(pDir);
	}

	return hResult;
}

/*! \brief Extracts image information from a file
 *
 * This method opens a file for the purpose of finding information about
 * the image.
 *
 * \return stiHResult 
 * \retval stiRESULT_SUCCESS if success
 * \retval stiRESULT_() otherwise
 */
stiHResult CstiImageManager::ImageFileInfoGet(SstiImageInfo *pInfo)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	FILE *pSrc = nullptr;
	size_t index;

#ifdef USE_LIBJPEG_TO_DECODE
	struct jpeg_decompress_struct cinfo{};
	struct jpeg_error_mgr jerr{};
#endif

	// We are only interested in .jpg files right now
	stiTESTCOND_NOLOG((pInfo->Filename.find(".jpg") != std::string::npos), stiRESULT_ERROR);

	// Remove the directory path and extension to get the ImageId
	index = pInfo->Filename.find_last_of('/');
	if (index != std::string::npos)
	{
		pInfo->ImageId = pInfo->Filename.c_str() + index + 1;
	}
	else
	{
		pInfo->ImageId = pInfo->Filename;
	}

	index = pInfo->ImageId.find_last_of('.');
	pInfo->ImageId.erase(index);

	// If the ImageId is not a full GUID, then it's an avatar and we have
	// to construct the GUID with mostly 0's.
	if (pInfo->ImageId.length() < IMAGE_GUID_LENGTH)
	{
		index = pInfo->ImageId.find('#');
		pInfo->unOrder = atoi(pInfo->ImageId.c_str() + index + 1);

		char s[200];
		int number = atoi(pInfo->ImageId.c_str());
		sprintf(s, "00000000-0000-0000-0000-%012d", number);
		pInfo->ImageId = s;
	}

	// If the filename contains the Disabled indicator, we set the Dim flag.
	if (pInfo->Filename.find(stiIMAGE_DISABLED_ID) != std::string::npos)
	{
		pInfo->bDim = true;
	}

	stiTESTCOND(((pSrc = fopen(pInfo->Filename.c_str(), "rb")) != nullptr), stiRESULT_ERROR);

#ifdef USE_LIBJPEG_TO_DECODE
	// Unfortunately we have to go through the motions as if we were going
	// to really decode this jpeg, just to get the dimensions.
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, pSrc);
	stiTESTCOND((jpeg_read_header(&cinfo, true) == JPEG_HEADER_OK), stiRESULT_ERROR);

	pInfo->unWidth = cinfo.image_width;
	pInfo->unHeight = cinfo.image_height;

	jpeg_destroy_decompress(&cinfo);
#endif

	pInfo->Data = nullptr;

STI_BAIL:

	if (pSrc)
	{
		fclose(pSrc);
	}

	return hResult;

}

/*! \brief Adds image information to our manifest
 *
 * This method adds information for a single image to the image manifest.
 * Mostly this is used to list the images on the filesystem, but can also
 * be used to add information about images that are statically built into
 * the application.  In that case, the pData field should not be empty
 * because we won't have a filename to read from.
 *
 * \return stiHResult 
 * \retval stiRESULT_SUCCESS if success
 * \retval stiRESULT_() otherwise
 */
stiHResult CstiImageManager::ImageAdd(SstiImageInfo *pInfo)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::vector<SstiImageInfo>::iterator it;
	bool bSameId = false;

	for (it = m_StaticManifest.begin(); it != m_StaticManifest.end(); it++)
	{
		SstiImageInfo *pCurr = &(*it);
		if (pCurr->unOrder == pInfo->unOrder)
		{
			// Compare GUIDs first
			int diff = pCurr->ImageId.compare(pInfo->ImageId);

			if (diff == 0)
			{
				// If the GUIDs are the same, remember that fact, and then check
				// the size.
				bSameId = true;

				if (pCurr->unWidth < pInfo->unWidth || pCurr->unHeight < pInfo->unHeight)
				{
					// If the GUID is the same and the new size is greater than
					// the current item in the list, then this is where we want
					// to insert the new item.  We do NOT increment m_nNumImages
					// here because we assume we've already counted this GUID.
					m_StaticManifest.insert(it, *pInfo);
					break;
				}
			}
			else if (diff > 0)
			{
				// If the new GUID is smaller than the current item in the list,
				// then this is where we want to insert the new item.
				m_StaticManifest.insert(it, *pInfo);

				// Only increment the image counter if we did not come across the
				// same GUID in the list already.
				if (!bSameId)
				{
					m_nNumImages++;
				}
				break;
			}
		}
		else if (pCurr->unOrder > pInfo->unOrder)
		{
			// If the new Order is smaller than the current item in the list,
			// then this is where we want to insert the new item.
			m_StaticManifest.insert(it, *pInfo);

			// Only increment the image counter if we did not come across the
			// same GUID in the list already.
			if (!bSameId)
			{
				m_nNumImages++;
			}
			break;
		}
	}

	if (it == m_StaticManifest.end())
	{
		// We didn't find a place to insert the new item, so we tack it
		// on the back of the list.
		m_StaticManifest.push_back(*pInfo);

		// Only increment the image counter if we did not come across the same
		// GUID in the list already.
		if (!bSameId)
		{
			m_nNumImages++;
		}
	}

	return hResult;
}

/*! \brief Adds image information to our manifest
 *
 * This method accepts the download URL for an image and adds it to the manifest.
 * Then it will reqeust the image to be downloaded.
 *
 * \return stiHResult 
 * \retval stiRESULT_SUCCESS if success
 * \retval stiRESULT_() otherwise
 */
stiHResult CstiImageManager::ImageAdd(
    const char *pDownloadURL,
    const char *pImageGuid)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiImageInfo *pImageInfo = nullptr;
	char *pUrl = nullptr;
	std::string filePath;

	stiTESTCOND (pDownloadURL != nullptr && pImageGuid != nullptr, stiRESULT_ERROR);

	pImageInfo = new SstiImageInfo;
	stiTESTCOND (pImageInfo != nullptr, stiRESULT_ERROR);

	pImageInfo->unRequestId = NextRequestIdGet();
    pImageInfo->ImageId = pImageGuid;
    filePath = m_pFullsizeNvmCache->FilePath(pImageGuid);
    pImageInfo->Filename = filePath + ".jpg";

	pUrl = new char [strlen(pDownloadURL) + 1];
	stiTESTCOND (pUrl != nullptr, stiRESULT_ERROR);

	strcpy(pUrl, pDownloadURL);

	{ // Braces are needed to fix compile error.
		CstiExtendedEvent oEvent (CstiImageManager::estiIMAGE_ADD, (stiEVENTPARAM)pImageInfo,  (stiEVENTPARAM)pUrl);
		hResult = MsgSend(&oEvent, sizeof(oEvent));
	}

STI_BAIL:
	if (hResult != stiRESULT_SUCCESS)
	{
		if (pImageInfo)
		{
			delete pImageInfo;
			pImageInfo = nullptr;
		}

		if (pUrl)
		{
			delete []pUrl;
			pUrl = nullptr;
		}
	}

	return hResult;
}

stiHResult CstiImageManager::EventImageAdd(void *poEvent)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	char *pDownloadUrl = nullptr;
	Download download;
	auto pInfo = (SstiImageInfo *)((CstiExtendedEvent *)poEvent)->ParamGet(0);
	pDownloadUrl = (char *)((CstiExtendedEvent *)poEvent)->ParamGet(1);

	download.image = *pInfo;
	download.url1 = pDownloadUrl;

	stiOSMutexLock(m_Mutex);
	
    m_Downloading.push_front(download);

    if (m_Downloading.size() > MAX_DOWNLOAD_QUEUE_SIZE)
    {
        m_Downloading.pop_back();
    }

    stiOSMutexUnlock(m_Mutex);

    DownloadSomething();
	
	// Cleanup 							   
	delete [] pDownloadUrl;
	pDownloadUrl = nullptr;

	if (pInfo)
	{
		delete pInfo;
		pInfo = nullptr;
	}

	return hResult;
}


/*! \brief Searches the manifest for the specified image
 *
 * This method searches each entry in the manifest until it finds an exact
 * match of the specified info.  If no exact match is found, we try to
 * find one that at least has the same ImageId.  If that doesn't work
 * we try to find the first one that has the same dimensions.  If that
 * STILL doesn't work, we just return the first one in our list.
 *
 * \param SstiImageInfo* pInfo 
 * \param unsigned int* pIndex 
 * 
 * \return stiHResult 
 * \retval stiRESULT_SUCCESS if success
 * \retval stiRESULT_() otherwise
 */
stiHResult CstiImageManager::StaticManifestSearch(SstiImageInfo *pInfo, unsigned int *pIndex)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	const unsigned int NOT_FOUND = 999999;
	unsigned int IdMatch = NOT_FOUND;
	*pIndex = NOT_FOUND;

	for (unsigned int i = 0; i < m_StaticManifest.size(); i++)
	{
		// Compare the GUID first.
		if (m_StaticManifest[i].ImageId == pInfo->ImageId)
		{
			// If the GUID matches, remember that fact, and then check
			// the other fields
			IdMatch = i + 1;

			if (m_StaticManifest[i].unWidth == pInfo->unWidth &&
				m_StaticManifest[i].unHeight == pInfo->unHeight &&
				m_StaticManifest[i].bDim == pInfo->bDim)
			{
				// An exact match.  We're done here.
				*pIndex = i;
				break;
			}
		}
	}

	if (*pIndex == NOT_FOUND && IdMatch != NOT_FOUND)
	{
		// We matched the Id, but not the size.  Oh well, let's use it anyway!
		*pIndex = IdMatch - 1;
	}

	if (*pIndex == NOT_FOUND)
	{
		// The GUID doesn't exist for some reason, so let's use the first one
		// with the correct dimensions instead
		for (unsigned int i = 0; i < m_StaticManifest.size(); i++)
		{
			if (m_StaticManifest[i].unWidth == pInfo->unWidth &&
				m_StaticManifest[i].unHeight == pInfo->unHeight &&
				m_StaticManifest[i].bDim == pInfo->bDim)
			{
				*pIndex = i;
				break;
			}
		}
	}

	if (*pIndex == NOT_FOUND)
	{
		// Still couldn't find anything.  Return the first one.
		*pIndex = 0;

		stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
}

/*! \brief Returns the GUID of the indexed image.
 *
 * This method looks up the specified index in the manifest to return
 * the associated GUID.
 *
 * \param unsigned int unIndex 
 * 
 * \return std::string& 
 */
std::string CstiImageManager::ImageIdGet(unsigned int unIndex)
{
	unsigned int unCount = 0;
	std::string sPrevId = m_StaticManifest[0].ImageId;

	if (unIndex == 0)
	{
		// Just return the first one.
		return m_StaticManifest[0].ImageId;
	}

	// We can't simply index into the manifest because there could be multiple
	// entries for each Image ID.  So we make sure to only count each ImageId
	// once.  The variable "Count" keeps track of how many different ImageIds
	// we've come across.  When that Count equals the Index that we're looking for
	// then we know we've found the right ImageId.
	unsigned int size = m_StaticManifest.size();
	for (unsigned int i = 1; i < size; i++)
	{
		if (sPrevId != m_StaticManifest[i].ImageId)
		{
			// Since the GUID is different from the last GUID we recorded,
			// we increment our counter and save off this new GUID.
			unCount++;
			if (unIndex == unCount)
			{
				// We found it.  Return the GUID.
				return m_StaticManifest[i].ImageId;
			}

			// Didn't find it yet, so record the latest GUID.
			sPrevId = m_StaticManifest[i].ImageId;
		}
	}

	// We didn't find it, so we'll just return the first GUID.
	return m_StaticManifest[0].ImageId;
}

/*!
 * \brief Determines if an image is already in the process of loading. 
 * 
 * \param SstiImageInfo* pInfo 
 * 
 * \return unsigned int 
 */
unsigned int CstiImageManager::ImageIsLoading(SstiImageInfo *pInfo)
{
	std::list<SstiImageInfo *>::iterator it;
	unsigned int unReqId = 0;

	stiOSMutexLock(m_Mutex);

	for (auto &&imageInfo: m_Loading)
	{
		// Search for the correct GUID first
		if (imageInfo.ImageId == pInfo->ImageId)
		{
			// Then check the other values
			if (imageInfo.unWidth == pInfo->unWidth &&
				imageInfo.unHeight == pInfo->unHeight &&
				imageInfo.bDim == pInfo->bDim)
			{
				unReqId = imageInfo.unRequestId;
				break;
			}
		}
	}

	if (unReqId == 0)
	{
		for (const auto &download: m_Downloading)
		{
			if (download.image.ImageId == pInfo->ImageId)
			{
				unReqId = download.image.unRequestId;
				break;
			}
		}

	}

	stiOSMutexUnlock(m_Mutex);

	return unReqId;
}

/*!
 * \brief 
 * 
 * \return unsigned int 
 */
unsigned int CstiImageManager::NextRequestIdGet ()
{
	stiLOG_ENTRY_NAME (CstiImageManager::NextRequestIdGet);

	unsigned int unReturn;

	// Lock the counter mutex
	stiOSMutexLock(m_Mutex);

	++m_unRequestId;

	if (m_unRequestId == 0)
	{
		m_unRequestId = 1;
	}

	unReturn = m_unRequestId;

	// Unlock
	stiOSMutexUnlock(m_Mutex);

	return (unReturn);
} // end CstiImageManager::NextRequestIdGet

EstiBool CstiImageManager::CoreResponseHandle(CstiCoreResponse *poResponse)
{
	EstiBool bHandled = estiFALSE;

	if (   poResponse->ResponseGet()
		== CstiCoreResponse::eIMAGE_DOWNLOAD_URL_GET_RESULT)
	{
		stiOSMutexLock(m_Mutex);

		for (auto download = m_Downloading.begin();
			 download != m_Downloading.end();
			 ++download)
		{
			if (download->coreUrlRequestId == poResponse->RequestIDGet())
			{
				if (poResponse->ResponseResultGet() == estiOK)
				{
					download->timestamp = poResponse->ResponseStringGet(2);
					download->url1 = poResponse->ResponseStringGet(3);
//					// NOTE: the URL2 was removed from the core response code...
//					download->url2 = poResponse->ResponseStringGet(4);
				}
				else // the request failed...
				{
					m_Downloading.erase(download);

					if (m_pContactManager)
					{
						auto imageId = poResponse->ResponseStringGet(1);
						// If there was an image ID string, call FixContactsForPhotoNotFound()
						if (!imageId.empty ())
						{
							m_pContactManager->FixContactsForPhotoNotFound(imageId);
						}
					}
				}

				bHandled = estiTRUE;
				poResponse->destroy ();
				break;
			}
		}

		if (bHandled && (m_DownloadState == DL_FETCH_URL))
		{
			m_DownloadState = DL_IDLE;
		}

		stiOSMutexUnlock(m_Mutex);

		if (!bHandled)
		{
			// The image download URL request wasn't generated by image manager,
			// but spy on it here anyway to see if it indicates that there's a
			// newer version of an image that image manager knows about.
			if (poResponse->ResponseResultGet() == estiOK)
			{
				auto guid = poResponse->ResponseStringGet(1);
				auto timestamp = poResponse->ResponseStringGet(2);

				m_pFullsizeNvmCache->PurgeIfOlderThan(guid, timestamp);
			}
		}

		if (bHandled)
		{
			DownloadSomething();
		}
	}

	return bHandled;
}

void CstiImageManager::PurgeStaticImage(const char* pGUID)
{
	// evict from static cache (there may be multiple versions)
	stiOSMutexLock(m_Mutex);

	auto itStatic = m_StaticManifest.begin();

	while (itStatic != m_StaticManifest.end())
	{
		if (itStatic->ImageId == pGUID)
		{
			itStatic->Data = nullptr;
            itStatic = m_StaticManifest.erase(itStatic);
			break;
		}
		else
		{
			itStatic++;
		}
	}

	stiOSMutexUnlock(m_Mutex);
}

void CstiImageManager::PurgeImageFromCache(const char* pGUID)
{
	// built in images and avatars shall not be purged...
	if (CoreHasImage(pGUID))
	{
		// evict from NVM caches
		m_pFullsizeNvmCache->Purge(pGUID);

		// evict from RAM cache (there may be multiple versions)
		stiOSMutexLock(m_Mutex);

		auto ram_suspect = m_RAMCache.begin();

		while (ram_suspect != m_RAMCache.end())
		{
			if (ram_suspect->ImageId == pGUID)
			{
				ram_suspect = m_RAMCache.erase(ram_suspect);
			}
			else
			{
				++ram_suspect;
			}
		}

		stiOSMutexUnlock(m_Mutex);
	}
}

void CstiImageManager::PurgeAllContactPhotos()
{
	// don't download any pending contact photos
	stiOSMutexLock(m_Mutex);

	// purge all downloads
	m_Downloading.clear();
	m_DownloadState = DL_IDLE;

	// evict everything from the NVM caches
	m_pFullsizeNvmCache->PurgeAll();

	// evict all contact photos from RAM cache
	auto ram_suspect = m_RAMCache.begin();

	while (ram_suspect != m_RAMCache.end())
	{
		if (CoreHasImage(ram_suspect->ImageId))
		{
			ram_suspect = m_RAMCache.erase(ram_suspect);
		}
		else
		{
			++ram_suspect;
		}
	}

	stiOSMutexUnlock(m_Mutex);
}

stiHResult CstiImageManager::EncodeAndCache(
	uint8_t* bitmap,
	uint32_t width,
	uint32_t height,
	PixelFormat pixel_format,
	const std::string& guid,
	unsigned int* request_id)
{
	stiLOG_ENTRY_NAME (CstiImageManager::EncodeAndCache);

	auto  encode = new Encode();

	encode->id = guid;
	encode->filename = m_pFullsizeNvmCache->FilePath(guid);
	encode->width = width;
	encode->height = height;
	encode->pixel_format = pixel_format;
	encode->bitmap = bitmap;
	encode->request_id = NextRequestIdGet();

	if (request_id)
	{
		*request_id = encode->request_id;
	}

	CstiEvent event(CstiImageManager::estiIMAGE_ENCODE, (stiEVENTPARAM)encode);

	return MsgSend(&event, sizeof(event));
}

std::string CstiImageManager::ImageFileAbsolutePath(const std::string& guid)
{
	std::string path;

	if (CoreHasImage(guid))
	{
		path = m_pFullsizeNvmCache->FilePath(guid);
	}
	else if (   (guid.length() == IMAGE_GUID_LENGTH)
			 && !strncmp(guid.c_str(), AvatarGUIDPrefix, strlen(AvatarGUIDPrefix))
			 && isdigit(guid[34])
			 && isdigit(guid[35]))
	{
		for (auto &image: m_StaticManifest)
		{
			// The first avatar in the manifest with the GUID in question should
			// have the highest resolution because that's how StaticManifestBuild
			// sticks them in there.
			if (image.ImageId == guid)
			{
				path = image.Filename;
				break;
			}
		}
	}

	return path;
}

stiHResult CstiImageManager::ProxyServerSet(const std::string& host, uint16_t port)
{
	return m_pDownloadService ? m_pDownloadService->ProxyServerSet(host, port)
							  : stiRESULT_ERROR;
}

stiHResult CstiImageManager::LoadDownloadedImage(void *poEvent)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	auto  requestId = (unsigned int)((CstiEvent *)poEvent)->ParamGet();

	stiOSMutexLock(m_Mutex);

	for (auto download = m_Downloading.begin();
		 download != m_Downloading.end();
		 ++download)
	{
		if (download->imageDownloadRequestId == requestId)
		{
			// Go get the image's width or height if we don't have it 
			if (download->image.unWidth == 0 ||
				download->image.unHeight == 0)
			{
				ImageFileInfoGet(&download->image);
			}

			// Add image to the cache
			m_pFullsizeNvmCache->Add(
				download->image.ImageId,
				download->timestamp);

			// Post a message to the message queue

			auto pInfoCopy = new SstiImageInfo(download->image);

			CstiEvent oEvent(CstiImageManager::estiIMAGE_REQUESTED, (stiEVENTPARAM)pInfoCopy);
			m_Loading.push_back(download->image);
			hResult = MsgSend (&oEvent, sizeof (oEvent));
			m_Downloading.erase(download);

			break;
		}
	}

	if (m_DownloadState == DL_FETCH_IMAGE)
	{
		m_DownloadState = DL_IDLE;
	}

	DownloadSomething();

	stiOSMutexUnlock(m_Mutex);

	return hResult;
}

stiHResult CstiImageManager::CancelImageDownload(void *poEvent)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bFound = false;
	bool retry = false;

	auto  requestId = (unsigned int)((CstiEvent *)poEvent)->ParamGet();

	stiOSMutexLock(m_Mutex);

	for (auto download = m_Downloading.begin();
		 download != m_Downloading.end();
		 ++download)
	{
		if (download->imageDownloadRequestId == requestId)
		{
			if (download->downloadFailCount++ > 0)
			{
				Callback(estiMSG_IMAGE_LOAD_FAILED, download->image.unRequestId);

				m_Downloading.erase(download);
			}
			else // this is the first failure, swap URLs and try again
			{
				retry = true;

				hResult =
					m_pDownloadService->Schedule(
						download->image.Filename,
						download->url1,
						download->url1,
						&download->imageDownloadRequestId);

				stiTESTRESULT();
			}

			bFound = true;
			break;
		}
	}

	stiTESTCOND (bFound, stiRESULT_ERROR);

STI_BAIL:

	if (!retry && (m_DownloadState == DL_FETCH_IMAGE))
	{
		m_DownloadState = DL_IDLE;
		DownloadSomething();
	}

	stiOSMutexUnlock(m_Mutex);

	return hResult;
}

stiHResult CstiImageManager::HandleImageDownloadCompleted(
	int32_t n32Message,
	size_t MessageParam,
	size_t CallbackParam,
	size_t CallbackFromId)
{
	stiHResult result = stiRESULT_SUCCESS;

	auto  image_mgr =
		reinterpret_cast<CstiImageManager*>(CallbackParam);

	auto  request_id = static_cast<unsigned int>(MessageParam);

	if (n32Message == CstiImageDownloadService::estiMSG_IMAGE_DOWNLOADED)
	{
		// Post a message to the message queue
		CstiEvent oEvent(
			estiIMAGE_DOWNLOAD_COMPLETED,
			(stiEVENTPARAM)request_id);

		result = image_mgr->MsgSend(&oEvent, sizeof(oEvent));
	}
	else if (n32Message == CstiImageDownloadService::estiMSG_IMAGE_DOWNLOAD_ERROR)
	{
		// Post a message to the message queue
		CstiEvent oEvent(
			estiIMAGE_DOWNLOAD_FAILED,
			(stiEVENTPARAM)request_id);

		result = image_mgr->MsgSend(&oEvent, sizeof (oEvent));
	}

	return result;
}

stiHResult CstiImageManager::EncodeJPEG(const Encode& encode_spec)
{
	stiHResult hResult = stiRESULT_SUCCESS;
#ifdef USE_LIBJPEG_TO_DECODE
	struct jpeg_compress_struct cinfo{};
	struct jpeg_error_mgr jerr{};

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	FILE* pJPegFile = fopen(encode_spec.filename.c_str(), "wb");
	stiTESTCOND (pJPegFile != nullptr, stiRESULT_ERROR);

	jpeg_stdio_dest(&cinfo, pJPegFile);

	cinfo.image_width = encode_spec.width;
	cinfo.image_height = encode_spec.height;
	cinfo.input_components = 3;
	cinfo.in_color_space =
		encode_spec.pixel_format == RGB888 ? JCS_RGB : JCS_YCbCr;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 80, TRUE);
	jpeg_start_compress(&cinfo, TRUE);

	while (cinfo.next_scanline < cinfo.image_height)
	{
		JSAMPROW row[1] =
			{
				&encode_spec.bitmap[
					cinfo.next_scanline * encode_spec.width * 3]
			};

		jpeg_write_scanlines(&cinfo, row, 1);
	}

	jpeg_finish_compress(&cinfo);

	fclose(pJPegFile);
	pJPegFile = nullptr;

STI_BAIL:

	jpeg_destroy_compress (&cinfo);
#endif

	return hResult;
}

void CstiImageManager::DownloadSomething()
{
	stiOSMutexLock(m_Mutex);

	if (   (m_DownloadState == DL_IDLE)
		&& !m_Downloading.empty())
	{
		Download& download = m_Downloading.front();

		if (download.url1.empty())
		{
			auto  pRequest = new CstiCoreRequest();

			pRequest->imageDownloadURLGet(
				download.image.ImageId.c_str(),
				CstiCoreRequest::ePUBLIC_ID);

			pRequest->RemoveUponCommunicationError() = estiFALSE;

			auto hResult = m_pCoreService->RequestSend(
					pRequest,
					&download.coreUrlRequestId);

			if (!stiIS_ERROR (hResult))
			{
				m_DownloadState = DL_FETCH_URL;
			}
		}
		else // download URL is known, download it
		{
			stiHResult hResult =
				m_pDownloadService->Schedule(
					download.image.Filename,
					download.url1,
					download.url1,
					&download.imageDownloadRequestId);

			if (stiRESULT_SUCCESS == hResult)
			{
				m_DownloadState = DL_FETCH_IMAGE;
			}
		}
	}

	stiOSMutexUnlock(m_Mutex);
}

void CstiImageManager::ContactManagerSet(IstiContactManager* pContactManager)
{
	m_pContactManager = pContactManager;
}

#endif
