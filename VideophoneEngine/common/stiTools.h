/*!
* \file stiTools.h
* \brief See stiTools.cpp.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once

//
// Includes
//
#include "stiDefs.h"
#include "stiSVX.h"
#include "stiError.h"
#include <vector>
#include <string>
#include <ctime>
#include <functional>
#include <cctype>
#include <sstream>

#include <memory>
#include <chrono>

// Add make_unique as we don't currently support c++14. Use custom namespace to avoid future conflict
namespace sci
{
	template<typename T, typename... Ts>
	typename std::enable_if
	<
	    !std::is_array<T>::value,
	    std::unique_ptr<T>
	>::type
	make_unique(Ts&&... params)
	{
		return std::unique_ptr<T>(new T(std::forward<Ts>(params)...));
	}

	template <class T>
	typename std::enable_if
	<
	    std::is_array<T>::value,
	    std::unique_ptr<T>
	>::type
	make_unique(std::size_t n)
	{
	    typedef typename std::remove_extent<T>::type RT;
	    return std::unique_ptr<T>(new RT[n]);
	}}


//
// Constants
//

//
// Typedefs
//
enum EstiRounding
{
	estiTRUNCATE,
	estiROUND_UP,
};

enum EstiTIME_UNITS
{
	estiDAYS	= 0x01,
	estiHOURS	= 0x02,
	estiMINUTES = 0x04,
	estiSECONDS = 0x08,
};

//
// Forward Declarations
//

//
// Globals
//

//
// Function Declarations
//

/*!
 * \brief Canonical number validate
 * 
 * \param pszNumber
 * 
 * \return EstiBool 
 */
void ByteStreamNALUnitFind (
	uint8_t *pun8Buffer,
	uint32_t un32BufferLength,
	uint8_t **ppun8NALUnit,
	uint32_t *pun32HeaderLength);

/*!
 * \brief Canonical number validate
 * 
 * \param pszNumber
 * 
 * \return EstiBool 
 */
EstiBool CanonicalNumberValidate (
	const char *pszNumber);
	
/*!
 * \brief E164 Alias Validate
 * 
 * \param pszAlias
 * 
 * \return EstiBool 
 */
EstiBool E164AliasValidate (
	const char *pszAlias);
	
/*!
 * \brief Email address validate
 * 
 * \param pszAddress
 * 
 * \return EstiBool 
 */
EstiBool EmailAddressValidate (
	const char *pszAddress);
	
/*!
 * \brief Validates that the string is an IPv4 address with no port or an IPv6 address
 * 
 * \param pszAddress
 * 
 * \return EstiBool 
 */
EstiBool IPAddressValidate (
	const char *pszAddress);

/*!
 * \brief Validates that the string is an IPv4 address with no port
 *
 * \param pszAddress
 *
 * \return EstiBool
 */
EstiBool IPv4AddressValidate (
	const char *pszAddress);

/*!
 * \brief Validates that the string is an IPv6 address with no port
 *
 * \param pszAddress
 *
 * \return EstiBool
 */
EstiBool IPv6AddressValidate (
	const char *pszAddress);
	
/*!
 * \brief Validates that the string is an IPv4 address with optional port or an IPv6 address
 * 
 * \param pszAddress
 * 
 * \return EstiBool 
 */
EstiBool IPAddressValidateEx (
	const char *pszAddress);

/*!
 * \brief Validates that the string is an IPv4 address with optional port
 *
 * \param pszAddress
 *
 * \return EstiBool
 */
EstiBool IPv4AddressValidateEx (
	const char *pszAddress);

/*!
 * \brief Validates that the string is an IPv6 address with optional port
 *
 * \param pszAddress
 *
 * \return EstiBool
 */
EstiBool IPv6AddressValidateEx (
	const char *pszAddress);

	
/*! \brief Converts IPv4 and IPv6 addresses from binary to text form
 *
 * \param addr
 * \param pIPAddress
 *
 * \return hResult
 */
stiHResult stiInetAddrToString (
	const addrinfo *addr,
	std::string *IPAddress);

	
/*!
 * \brief Get Host by Name
 * 
 * \param pszName 
 * \param pszBuf 
 * \param nBufLen 
 * 
 * \return stiHResult 
 */
stiHResult stiDNSGetHostByName (
	const char *pszName,
	const char *pszAltName,
	std::string *pHost);

/*!
 * \brief Create sockaddr_storage
 *
 * \param pszIPAddress
 * \param un16Port
 * \param eSockType
 * \param pstSockAddrStorage
 *
 * \return stiHResult
 */
stiHResult stiSockAddrStorageCreate (
	const char *pszIPAddress,
	uint16_t un16Port,
	int eSockType,
	sockaddr_storage *pstSockAddrStorage);

/*!
 * \brief Convert NAT64 IPv6 address to IPv4
 *
 * \param ipAddress
 * \param mappedIpAddress
 *
 * \return stiHResult
 */
stiHResult stiMapIPv6AddressToIPv4 (std::string ipAddress, std::string *mappedIpAddress);

/*!
 * \brief Convert IPv4 address to IPv6
 *
 * \param ipAddress
 * \param mappedIpAddress
 *
 * \return stiHResult
 */
stiHResult stiMapIPv4AddressToIPv6 (const std::string& ipAddress, std::string *mappedIpAddress);

/*!
 * \brief Get Local IP
 * 
 * \param pLocalIp
 * \param eIpAddressType
 * 
 * \return stiHResult 
 */
stiHResult stiGetLocalIp (
	std::string *pLocalIp,
	EstiIpAddressType eIpAddressType);
				
/*!
 * \brief Get subnet mask ip address
 * 
 * \param pSubnetMask
 * \param eIpAddressType
 * 
 * \return stiHResult
 */
stiHResult stiGetNetSubnetMaskAddress (
	std::string *pSubnetMask,
	EstiIpAddressType eIpAddressType);

/*!
 * \brief Get DNS IP address
 * 
 * \param nIndex 
 * \param pDNSAddress
 * \param eIpAddressType
 * 
 * \return stiHResult
 */
stiHResult stiGetDNSAddress (
	int nIndex,
	std::string *pDNSAddress,
	EstiIpAddressType eIpAddressType);

/*!
 * \brief Get default gateway address
 * 
 * \param pDefaultGatewayAddress
 * \param eIpAddressType
 * 
 * \return stiHResult
 */
stiHResult stiGetDefaultGatewayAddress (
	std::string *pDefaultGatewayAddress,
	EstiIpAddressType eIpAddressType);


/*!
 * \brief Copy a file
 *
 * If `dstFileName` already exists, it is overwritten.
 *
 * \param srcFileName
 * \param dstFileName
 *
 * \return stiHResult
 */
stiHResult stiCopyFile (
	const std::string &srcFileName,
	const std::string &dstFileName);


/*!
 * \brief String compare ignore case
 * Positive number to indicate that "first" would appear alphabetically
 * after "last". 
 *  
 * \param pString1 The first string 
 * \param pString2 The second string
 * 
 * \retval Positive number to indicate that "first" would appear alphabetically
 * after "last".
 * \retval 0 indicates that the two strings are the same.
 * \retval Negative number to indicate that "first" would appear alphabetically
 * before "last".
 */
int stiStrICmp (
	const char *pString1,
	const char *pString2);
	
/*!
 * \brief String compare ignore case n characters
 * 
 * \param pString1 The first string
 * \param pString2 The second string
 * \param count 
 * 
 * \retval Positive number to indicate that "first" would appear alphabetically
 * after "last".
 * \retval 0 indicates that the two strings are the same.
 * \retval Negative number to indicate that "first" would appear alphabetically
 * before "last".
 */
int stiStrNICmp (
	const char *pString1,
	const char *pString2,
	size_t count);

/*!
 * \brief Converts time string to time_t
 * 
 * \param pszTime 
 * \param pTime 
 * 
 * \return stiHResult 
 */
stiHResult TimeConvert (
	const char * pszTime,
	time_t *pTime);

/*!
 * \brief Time difference in days
 * 
 * \param tTime1 
 * \param tTime2 
 * \param eRoundMethod 
 * 
 * \return int 
 */
int TimeDifferenceInDays (
	time_t tTime1, 
	time_t tTime2, 
	EstiRounding eRoundMethod = estiROUND_UP);

double TimeDifferenceInSeconds (
	const timespec &Time1,
	const timespec &Time2);
	
	
/*!
 * \brief Time difference to string
 * 
 * \param dDiffTime 
 * \param pszBuff 
 * \param nBuffLen 
 * \param unUnitsMask 
 * \param eRoundMethod 
 * 
 * \return char* 
 */
char *TimeDiffToString (
	double dDiffTime,
	char *pszBuff,
	int nBuffLen,
	unsigned int unUnitsMask,
	EstiRounding eRoundMethod = estiROUND_UP);

/*!
 * \brief Replace string
 * 
 * \param ppszDest 
 * \param pszSrc 
 * 
 * \return stiHResult 
 */
stiHResult ReplaceString(
	char **ppszDest,
	const char *pszSrc);

/*!
 * \brief Parse URL 
 * Splits URL into server name and file 
 * 
 * \param pszUrl    Incoming URL to parse
 * \param pProtocol i.e. ftp, http, https, etc. 
 * \param pServer   domain name e.g. www.sorenson.com
 * \param pFile 	What comes after the domain Name e.g. /local/xyz.combo
 * 
 * \return bool 
 */
bool ParseUrl (
	const char *pszUrl,
	std::string *pProtocol,
	std::string *pServer,
	std::string *pFile);


/*!
 * \brief Picture size get
 * 
 * \param unCols 
 * \param unRows 
 * 
 * \return EstiPictureSize 
 * 	\li estiSQCIF - Sub-Quarter CIF - 128 x 96 pixels.   
 *  \li estiQCIF - Quarter CIF - 176 x 144 pixels.      
 *  \li estiSIF - SIF - 352 x 240 pixels.              
 *  \li estiCIF - CIF - 352 x 288 pixels.              
 *  \li estiCUSTOM - Custom picture size.                 
 *  \li estiFULL_SCREEN - Full Screen mode.                
 */
EstiPictureSize PictureSizeGet (
	unsigned int unCols,
	unsigned int unRows);

/*!
 * \brief Computes the difference between two timeval structures.
 * 
 * It is assumed that the timeval inputs are normalized.
 * 
 * \param pDifference The result of the subtraction
 * \param pTime1 The first time value.
 * \param pTime2 The second time value.
 * 
 */
int timevalSubtract (
	timeval *pDifference,
	const timeval *pTime1,
	const timeval *pTime2);

/*!
 * \brief Computes the sum of two timeval structures.
 * 
 * \param pSum The result of the addition
 * \param pTime1 The first time value.
 * \param pTime2 The second time value.
 * 
 */
void timevalAdd (
	timeval *pSum,
	const timeval *pTime1,
	const timeval *pTime2);

/*!
 * \brief Computes the difference between two timespec structures.
 * 
 * It is assumed that the timespec inputs are normalized.
 * 
 * \param pDifference The result of the subtraction
 * \param Time1 The first time value.
 * \param Time2 The second time value.
 * 
 */
int timespecSubtract (
	timespec *pDifference,
	const timespec &Time1,
	const timespec &Time2);

/*!
 * \brief Computes the sum of two timespec structures.
 * 
 * \param pSum The result of the addition
 * \param Time1 The first time value.
 * \param Time2 The second time value.
 * 
 */
void timespecAdd (
	timespec *pSum,
	const timespec &Time1,
	const timespec &Time2);

/*!
 * \brief Split an ip:port string into two separate strings.
 * 
 * \param pszAddressAndPort the address to parse
 * \param pszAddress fill this string in with the address portion (you must make sure it is large enough)
 * \param pszPort fill this string in with the port portion if it exists (you must make sure it is large enough)
 * 
 * \return stiHResult
 */
stiHResult AddressAndPortParse (
	const char *pszAddressAndPort,
	char *pszAddress,
	char *pszPort);


/*!
 * \brief Split an ip:port string into two separate strings.
 * 
 * \param pszAddressAndPort the address to parse
 * \param pAddress Fills this string in with the address portion
 * \param pun16Port Contains the port portion if it exists.  Otherwise, it is set to zero.
 * 
 * \return stiHResult
 */
stiHResult AddressAndPortParse (
	const char *pszAddressAndPort,
	std::string *pAddress,
	uint16_t *pun16Port);



/*!
 * \brief Split a name into a first/last name pair. If there are no spaces
 *  in the name, then pszFirstName will contain the entire name and pszLastName
 *  will be NULL.
 *
 * \param pszFullName The name to split
 * \param pszFirstName Fills this string in with the first name
 * \param pszLastName Fills this string in with the last name
 *
 * \return stiHResult
 */
stiHResult NameSplit (const char *pszFullName,
	char **pszFirstName,
	char **pszLastName);


/*!\brief Get a time value for use in duration calculations
 *
 * Note: This is not the time from the wall clock
 * and should not be used for call placement time within
 * a call history list
 *
 * \param pTime On return, contains a representation of the time
 */
void TimeGet (
	time_t *pTime);

void TimeGet (
	timespec *pTimeVal);

#if (SUBDEVICE != SUBDEV_RCU)
stiHResult EncryptString (
	const char *pszString,
	const unsigned char *pun8Key,
	const unsigned char *pun8InitVector,
	std::string *pEncryptedString);


stiHResult DecryptString (
	const char *pszEncryptedString,
	const unsigned char *pun8Key,
	const unsigned char *pun8InitVector,
	std::string *pDecryptedString);
#endif

std::string *LeftTrim(
	std::string *pString);

std::string *RightTrim(
	std::string *pString);

std::string *Trim(
	std::string *pString);


size_t stiSafeMemcpy (
	void *pDstBuffer,
	size_t DstLength,
	const void *pSrcBuffer,
	size_t SrcLength);

#if APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
namespace std {
	template<typename T>
	string to_string(T &&t) {
		// When we use C++ 11, change this to use move semantics.
		std::ostringstream stm;
		stm << t;
		return stm.str();
	}
}
#endif

std::vector<std::string> splitString (
	const std::string &str,
	char delimiter);


template<typename T>
constexpr  bool isOneOf(T t, T v)
{
	return t == v;
}

template<typename T, typename... Args>
constexpr bool isOneOf(T t, T v, Args... args)
{
	return t == v || isOneOf(t, args...);
}

std::string productNameConvert (ProductType productType);

std::string phoneTypeConvert (ProductType productType);

std::string VideoCodecToString (EstiVideoCodec codec);
std::string AudioCodecToString (EstiAudioCodec codec);
std::string InterfaceModeToString (EstiInterfaceMode mode);
std::string NetworkTypeToString (NetworkType type);

/*
 * return a const string either "true" or "false" for debugging
 * using a char * instead of a std::string because about the only use would be in stiTrace
 */
const char *torf(bool value);

/*
 * return a new uri with the protocol scheme changed to the passed in value.
 * 
 */
void protocolSchemeChange (std::string &uri, const std::string &scheme);

/*
* return a formatted string using printf semantics
*/
std::string formattedString (const char* format, ...);

/*
* return an 'hr:min:sec.ms' string representation of a chrono timepoint: "00:00:00.000"
*/
std::string timepointToTimestamp (std::chrono::system_clock::time_point timepoint);

// end file stiTools.h
