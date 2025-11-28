/*!
 * \file CstiImageManager.h
 * \brief See CstiImageManager.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

#ifndef CSTIIMAGEMANAGER_H
#define CSTIIMAGEMANAGER_H

//
// Includes
//
#include <vector>
#include <list>
#include <string>
#include "CstiOsTaskMQ.h"
#include "IstiImageManager.h"
#include "CstiEvent.h"
#include "stiEventMap.h"
#include "../contacts/IstiContactManager.h"
#include <sstream>

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//
class CstiCoreResponse;
class CstiCoreService;
class CstiImageDownloadService;
class CstiNvmImageCache;

//
// Globals
//

/*!
 * \brief Image manager 
 */
class CstiImageManager : public CstiOsTaskMQ, public IstiImageManager
{
public:

	/*! \enum EEventType
	*/ 
	enum EEventType
	{
		estiIMAGE_REQUESTED = estiEVENT_NEXT,
		estiIMAGE_ENCODE,
		estiIMAGE_DOWNLOAD_COMPLETED,
		estiIMAGE_DOWNLOAD_FAILED,
		estiIMAGE_ADD
	};

	/*!
	 * \brief Image Manager constructor
	 * 
	 * \param pCoreService Pointer to Core Service module
	 * \param PstiObjectCallback pfnAppCallback 
	 * \param size_t CallbackParam
	 */
	CstiImageManager(
		CstiCoreService *pCoreService,
		PstiObjectCallback pfnAppCallback,
		size_t CallbackParam);

	/*!
	 * \brief Destructor
	 */
	~CstiImageManager() override;

	/*!
	 * \brief Initialize to be called once on startup
	 * 
	 * \return stiHResult 
	 */
	stiHResult Initialize(
		EstiBool bContactImagesEnabled);

	/*!
	 * \brief Start up 
	 * 	Starts Image Manager 
	 * 
	 * \return stiHResult 
	 */
	stiHResult Startup() override;

	/*!
	 * \brief Sets up a proxy server for contact photo download requests.
	 *
	 * \param host hostname or address of the proxy server or empty for none
	 * \param port port of the proxy server (ignored when host is empty)
	 *
	 * \return stiHResult
	 */
	stiHResult ProxyServerSet(const std::string& host, uint16_t port);

	/*!
	 * \brief Adds an image to the image manager
	 * 
	 * \param SstiImageInfo* Info 
	 * 
	 * \return stiHResult 
	 */
	stiHResult ImageAdd(SstiImageInfo *Info) override;

	/*!
	 * \brief Adds image
	 * 
	 * \param char *pDownloadURL 
	 * \param char *pImageGuid 
	 * 
	 * \return stiHResult 
	 */
	stiHResult ImageAdd(
        const char *pDownloadURL,
        const char *pImageGuid) override;


	/*!
	 * \brief Request image from the manager
	 * 
	 * \param SstiImageInfo* pInfo 
	 * \param unsigned char** pData 
	 * 
	 * \return stiHResult 
	 */
	stiHResult ImageRequest(SstiImageInfo *pInfo, std::shared_ptr<ImageArray> *pData) override;

	/*!
	 * \brief Requests image size and moves the image to the front of the cache.
	 * 
	 * \param pInfo 
	 * 
	 * \return stiHResult 
	 */
	stiHResult ImageSizeRequest(SstiImageInfo *pInfo) override;

	/*!
	 * \brief returns number of images
	 * 
	 * \return int 
	 */
	int ImageCountGet() override { return m_nNumImages; }

	/*!
	 * \brief Returns image by ID
	 * 
	 * \param unsigned int unIndex 
	 * 
	 * \return std::string& 
	 */
	std::string ImageIdGet(unsigned int unIndex) override;

	/*!
	 * \brief Causes the specified image to be purged from all image caches
	 *
	 * \param pGUID globally unique identifier of the image to purge
	 */
	void PurgeImageFromCache(const char* pGUID) override;

	/*!
	 * \brief Causes the specified image to be purged from the static cache
	 *
	 * \param pGUID globally unique identifier of the image to purge
	 */
	void PurgeStaticImage(const char* pGUID) override;

	/*!
	 * \brief Causes all core provided contact photos to be purged from all image caches
	 */
	void PurgeAllContactPhotos() override;

	/*!
	 * \brief Encodes the specified bitmap into JPEG format and adds the result
	 * to the image cache.
	 *
	 * This method doesn't actually do the work, it just queues the action to be
	 * handled asynchronously later.
	 *
	 * @param bitmap pointer to the bitmap pixels.  One should not free the
	 *               resources allocated to this pointer until after a response
	 *               to this request has been received.
	 * @param width width of the bitmap in pixels
	 * @param height height of the bitmap in pixels
	 * @param pixel_format pixel format of the source bitmap
	 * @param guid globally unique identifier for the image
	 * @param request_id when not NULL, this field is populated with the unique
	 *                   identifier for this request
	 * @return stiRESULT_SUCCESS on success otherwise fail
	 */
	stiHResult EncodeAndCache(
		uint8_t* bitmap,
		uint32_t width,
		uint32_t height,
		PixelFormat pixel_format,
		const std::string& guid,
		unsigned int* request_id = nullptr) override;

	/*!
	 * \brief Gets the path on the local filesystem to the specified image file.
	 *
	 * This method doesn't ensure or imply that the requested image actually
	 * exists on the filesystem.  It simply says where it would be hiding if it
	 * were to exist.
	 *
	 * For avatars, this method returns the path to the highest resolution
	 * version available.
	 *
	 * @param guid globally unique identifier for the image
	 * @return full absolute path to the requested image file or empty string if
	 *         the path isn't known
	 */
	std::string ImageFileAbsolutePath(const std::string& guid) override;

	/*!
	 * \brief Responds to relevant CoreResponses
	 *
	 * \param poResponse The CoreResponse object
	 *
	 * \return True if the response was handled, False otherwise
	 */
	EstiBool CoreResponseHandle(CstiCoreResponse *poResponse);

	/*!
	 * \brief Associates the specified contact manager with the image manager.
	 */
	void ContactManagerSet(IstiContactManager* pContactManager) override;

	/*!
	 * \brief Sets the ContactImagesEnabled flag.
	 */
	void ContactImagesEnabledSet (EstiBool bContactImagesEnabled) override { m_bContactImagesEnabled = bContactImagesEnabled; }

	/*!
	 * \brief Returns the ContactImagesEnabled flag.
	 */
	EstiBool ContactImagesEnabledGet () override { return m_bContactImagesEnabled; }

private:

	struct Download
	{
		SstiImageInfo image;

		unsigned int coreUrlRequestId{0};
		unsigned int imageDownloadRequestId{0};

		unsigned int downloadFailCount{0};

		std::string url1;
		std::string url2;
		std::string timestamp;

		Download() = default;
	};

	struct Encode
	{
		std::string id;
		std::string filename;
		uint32_t width{0};
		uint32_t height{0};
		PixelFormat pixel_format;
		uint8_t* bitmap{nullptr};

		unsigned int request_id{0};
	};

	enum DownloadState
	{
		DL_IDLE,
		DL_FETCH_URL,
		DL_FETCH_IMAGE
	};

	typedef std::list<Download> DownloadList;

	stiMUTEX_ID m_Mutex{};

	EstiBool m_bInitialized;

	int m_nNumImages;   // Does not count every size of each image!
	unsigned int m_unRequestId;

	std::vector<SstiImageInfo> m_StaticManifest;  // Logs what images we are able to load from the static filesystem
	std::list<SstiImageInfo> m_RAMCache;       // Stores copies of images so we don't need to load them again
	std::list<SstiImageInfo> m_Loading;     // Our queue of images that are in the process of loading.
											// (This makes sure we don't try to load the same image multiple times.)
	CstiCoreService *m_pCoreService;
	DownloadList m_Downloading; ///< queue of images that are in the process of being downloaded

	CstiNvmImageCache *m_pFullsizeNvmCache; ///< images that currently exist in the full-size NVM cache in MRU order
	CstiImageDownloadService *m_pDownloadService;
	DownloadState m_DownloadState;
	uint32_t m_LastNvmManifestWriteMsec;

	IstiContactManager* m_pContactManager;

	EstiBool m_bContactImagesEnabled;

	std::stringstream m_ImageDirectory;

	//
	// Member functions
	//
	void Uninitialize();
	void CacheClear();

	int Task () override;

	stiHResult StaticManifestBuild();
	stiHResult StaticManifestSearch(SstiImageInfo *pInfo, unsigned int *pIndex);

	stiHResult ImageFileInfoGet(SstiImageInfo *pinfo);
	stiHResult ImageGetFromRAMCache(SstiImageInfo *pInfo, std::shared_ptr<ImageArray> *pData);
	stiHResult ImageSaveToRAMCache(SstiImageInfo *pInfo);
	stiHResult ImageDecode(SstiImageInfo *pInfo);
	stiHResult ImageResize(
		const std::shared_ptr<const ImageArray> &OldData,
		std::shared_ptr<ImageArray> *pNewData,
		unsigned short unSrcWidth, unsigned short unSrcHeight,
		unsigned short unDestWidth, unsigned short unDestHeight);

	unsigned int ImageIsLoading(SstiImageInfo *pInfo);

	unsigned int NextRequestIdGet();

	//
	// Event Handlers
	//
	stiDECLARE_EVENT_MAP(CstiImageManager);
	stiDECLARE_EVENT_DO_NOW(CstiImageManager);

	using CstiOsTaskMQ::ShutdownHandle;
	stiHResult stiEVENTCALL ShutdownHandle(void *poEvent);

	stiHResult stiEVENTCALL EventImageAdd(void *poEvent);
	stiHResult stiEVENTCALL ImageLoad(void *poEvent);
	stiHResult stiEVENTCALL ImageEncodeAndCache(void *poEvent);

	stiHResult stiEVENTCALL LoadDownloadedImage(void *poEvent);
	stiHResult stiEVENTCALL CancelImageDownload(void *poEvent);

	stiHResult EncodeJPEG(const Encode& encode_spec);
	void DownloadSomething();

	/*!\brief Callback function to handle image download service messages.
	 *
	 * \param n32Message the type of message
	 * \param MessageParam data specific to the message
	 * \param CallbackParam points to the instantiated CstiImageManager object
	 * \param CallbackFromId
	 * \return stiRESULT_SUCCESS if success
	 * \note This is a static function
	 */
	static stiHResult HandleImageDownloadCompleted(
		int32_t n32Message,
		size_t MessageParam,
		size_t CallbackParam,
		size_t CallbackFromId);
};

#endif
