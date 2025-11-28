/*!
 * \file IstiImageManager.h
 * \brief Interface for the ImageManager
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

#ifndef ISTIIMAGEMANAGER_H
#define ISTIIMAGEMANAGER_H

//
// Includes
//
#include <string>
#include "stiDefs.h"
#include "stiError.h"
#include <memory>
#include <algorithm>

//
// Constants
//
/// length of an image GUID: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
const unsigned int IMAGE_GUID_LENGTH = 36;

/// The unique image identifier for the silhouette avatar.
const char* const SilhouetteAvatarGUID = "00000000-0000-0000-0000-000000000001";

/// The unique image identifier that means no image has been assigned
const char* const NoGUID = "00000000-0000-0000-0000-000000000000";

/// The first part of the GUID that is the same for all avatars and endpoint custom images
const char* const AvatarGUIDPrefix = "00000000-0000-0000-0000-0000000000";

//
// Typedefs
//

#if 1
using ImageArray = std::vector<unsigned char>;
#else
static int g_ImageArrayCount = 0;

class ImageArray : public std::vector<unsigned char>
{
public:
	ImageArray ()
	:
		std::vector<unsigned char>()
	{
		++g_ImageArrayCount;
		stiTrace ("Image Array Count = %d\n", g_ImageArrayCount);
	}

	ImageArray (unsigned int a, int b)
	:
		std::vector<unsigned char>(a, b)
	{
		++g_ImageArrayCount;
		stiTrace ("Image Array Count = %d\n", g_ImageArrayCount);
	}

	~ImageArray ()
	{
		--g_ImageArrayCount;
		stiTrace ("Image Array Count = %d\n", g_ImageArrayCount);
	}
};
#endif


/*! 
 * \struct SstiImageInfo
 *  
 * \brief This structure defines information about an image  
 *  
 */
typedef struct SstiImageInfo
{
	SstiImageInfo() = default;

	~SstiImageInfo () = default;

	std::string ImageId;
	std::string Filename;
	unsigned short unWidth{0};
	unsigned short unHeight{0};
	unsigned int unRequestId{0};
	unsigned int unOrder{0};
	bool bDim{false};
	bool bDeletable{false};
	std::shared_ptr<ImageArray> Data;

} SstiImageInfo;

//
// Forward Declarations
//
class IstiContactManager;

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
bool CoreHasImage(const std::string& imageId);


/*!
 * \brief Interface class for Image Manager
 */
class IstiImageManager
{
public:
	/** supported bitmap pixel formats */
	enum PixelFormat
	{
		YUV444,
		RGB888
	};


	/*!
	 * \brief Determines whether or not the specified image identifier is a
	 *  valid image GUID.
	 *
	 * This method ensures that the GUID is of valid length and format.  It does
	 * not actually ensure that the specified GUID refers to an image that
	 * exists.
	 *
	 * A true result from this method does not imply that the specified GUID
	 * refers to an image that actually exists.
	 *
	 * \return true if the specified GUID is valid, false otherwise
	 */
	static bool IsGUIDValid(const std::string& guid)
	{
		static const char* TEMPLATE = NoGUID;

		bool valid = guid.length() == IMAGE_GUID_LENGTH;

		if (valid)
		{
			for (unsigned int i = 0; i < IMAGE_GUID_LENGTH; ++i)
			{
				const char& T(TEMPLATE[i]);
				const char& C(guid[i]);

				if (   (   (T == '0')
						&& !islower(C)
						&& !isdigit(C))
					|| (   (T != '0')
						&& (T != C)))
				{
					valid = false;
					break;
				}
			}
		}

		return valid;
	}


	/*!
	 * \brief Adds image
	 * 
	 * \param Info 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult ImageAdd(SstiImageInfo *Info) = 0;

	/*!
	 * \brief Adds image
	 * 
	 * \param pDownloadURL 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult ImageAdd(
		const char *pDownloadURL,
		const char *pImageGuid) = 0;

	/*!
	 * \brief Requests Image
	 * 
	 * \param pInfo 
	 * \param pData 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult ImageRequest(SstiImageInfo *pInfo, std::shared_ptr<ImageArray> *pData) = 0;

	/*!
	 * \brief Requests image size and moves the image to the front of the cache.
	 * 
	 * \param pInfo 
	 * 
	 * \return stiHResult 
	 */
	virtual stiHResult ImageSizeRequest(SstiImageInfo *pInfo) = 0;

	/*!
	 * \brief Returns the count of images
	 * 
	 * \return int 
	 */
	virtual int ImageCountGet() = 0;

	/*!
	 * \brief Gets image by ID
	 * 
	 * \param unIndex 
	 * 
	 * \return std::string& 
	 */
	virtual std::string ImageIdGet(unsigned int unIndex) = 0;

	/*!
	 * \brief Causes the specified image to be purged from the static cache
	 *
	 * \param pGUID globally unique identifier of the image to purge
	 */
	virtual void PurgeStaticImage(const char* pGUID) = 0;

	/*!
	 * \brief Causes the specified image to be purged from all image caches
	 *
	 * \param pGUID globally unique identifier of the image to purge
	 */
	virtual void PurgeImageFromCache(const char* pGUID) = 0;

	/*!
	 * \brief Causes all core provided contact photos to be purged from all image caches
	 */
	virtual void PurgeAllContactPhotos() = 0;

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
	 * @param stiRESULT_SUCCESS on success otherwise fail
	 */
	virtual stiHResult EncodeAndCache(
		uint8_t* bitmap,
		uint32_t width,
		uint32_t height,
		PixelFormat pixel_format,
		const std::string& guid,
		unsigned int* request_id = nullptr) = 0;

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
	virtual std::string ImageFileAbsolutePath(const std::string& guid) = 0;

	/*!
	 * \brief Associates the specified contact manager with the image manager.
	 */
	virtual void ContactManagerSet(IstiContactManager* pContactManager) = 0;

	/*!
	 * \brief Sets the ContactImagesEnabled flag
	 */
	virtual void ContactImagesEnabledSet (EstiBool bContactImagesEnabled) = 0;

	/*!
	 * \brief Gets the ContactImagesEnabled flag
	 */
	virtual EstiBool ContactImagesEnabledGet () = 0;

private:

};


#endif //#ifndef ISTIIMAGEMANAGER_H
