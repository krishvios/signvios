////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: stiSipTools
//
//  File Name:  stiSipTools.cpp
//
//  Abstract:
//	  This class implements the methods specific to a Registration such as the
//	  crdentials and the valid and invalid addresses to manage.
//
////////////////////////////////////////////////////////////////////////////////
//
// Includes
//
#include "stiSipTools.h"
#include "stiTools.h"
#include "stiConfDefs.h"

// Radvision
#include "RvSipContactHeader.h"
#include "RvSipAddress.h"
#include "RvSipAllowHeader.h"
#include "RvSipViaHeader.h"
#include "stiConfDefs.h"
#include "RvSipMsg.h"
#include "RvSipOtherHeader.h"
#include <openssl/evp.h>
#include "stiBase64.h"

//
// Constants
//
const unsigned char SSL_SINFO_KEY[] = {105, 9, 53, 96, 76, 90, 121, 6, 70, 89, 6, 1, 77, 8, 48, 96};
const unsigned char SSL_SINFO_IV[] = {50, 119, 73, 72, 89, 74, 89, 7};

//
// Increment this version number whenever changes in our SIP exchanges change and the remote
// endpoint needs to know how to respond based on those changes.
// TODO: When full WebRTC compatability is achieved this should be changed to be greater or equal to webrtcSSV.
const int g_nSVRSSIPVersion = 5;

//
// Typedefs
//

//
// Forward Declarations
//

// 	This next bit is a neat trick to get it to print the file reference (Do not "optomize" this or it'll break.)
#define _STA(x) #x
#define _STB(x) _STA(x)
#define HERE __FILE__ ":" _STB(__LINE__)
#define DBG_MSG(fmt,...) stiDEBUG_TOOL (g_stiSipMgrDebug, stiTrace (fmt "\n", ##__VA_ARGS__); )


//
// Globals
//


////////////////////////////////////////////////////////////////////////////////
//; ContactHeaderAttach
//
// Description: Attach another contact header to a message
//
// Abstract:
//
// Returns:
//  stiHResult
//
stiHResult ContactHeaderAttach (
	RvSipMsgHandle hMsg, /// the message to attach to
	const char *pszContactString /// The contact string to attach
)
{
	RvSipContactHeaderHandle hContactHeader;
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;

	// Set the stack settings
	stiTESTCOND_NOLOG (nullptr != pszContactString, stiRESULT_ERROR);

	rvStatus = RvSipContactHeaderConstructInMsg (hMsg, RV_TRUE, &hContactHeader);
	stiTESTRVSTATUS ();

	rvStatus = RvSipContactHeaderParse (hContactHeader, (RvChar*)pszContactString);
	stiTESTRVSTATUS ();
	
	//DBG_MSG ("Attached CONTACT header = %s", pszContactString);

STI_BAIL:

	return hResult;
} // end ContactHeaderAttach


////////////////////////////////////////////////////////////////////////////////
//; ContactHeaderAttach
//
// Description: Attach a copy of a free-floating contact header to a message
//
// Abstract:
//
// Returns:
//  stiHResult
//
stiHResult ContactHeaderAttach (
	RvSipMsgHandle hMsg, /// the message to attach to
	RvSipContactHeaderHandle hOriginalContact /// the contact to be cloned into this message
)
{
	RvSipContactHeaderHandle hContactHeader;
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;

	// Set the stack settings
	stiTESTCOND_NOLOG (nullptr != hOriginalContact, stiRESULT_ERROR);

	rvStatus = RvSipContactHeaderConstructInMsg (hMsg, RV_TRUE, &hContactHeader);
	stiTESTRVSTATUS ();

	rvStatus = RvSipContactHeaderCopy (hContactHeader, hOriginalContact);
	stiTESTRVSTATUS ();

STI_BAIL:

	return hResult;
} // end ContactHeaderAttach


////////////////////////////////////////////////////////////////////////////////
//; AddressCompare ()
//
// Description: Test two addresses to see if they are the same
//	NOTE: Radvision has three functions to do this, none of which 100% correctly.
//
// Returns:
//  true or false
//
bool AddressCompare (RvSipAddressHandle hAddress1, RvSipAddressHandle hAddress2)
{
	bool bEqual = false;
	int nOriginalPort1 = RvSipAddrUrlGetPortNum (hAddress1);
	int nOriginalPort2 = RvSipAddrUrlGetPortNum (hAddress2);
	if (nOriginalPort1 != nOriginalPort2)
	{
		if (nOriginalPort1 < 0)
		{
			RvSipAddrUrlSetPortNum (hAddress1, nDEFAULT_SIP_LISTEN_PORT);
			bEqual = RvSipAddr3261IsEqual (hAddress1, hAddress2);
			RvSipAddrUrlSetPortNum (hAddress1, nOriginalPort1);
		}
		else if (nOriginalPort2 < 0)
		{
			RvSipAddrUrlSetPortNum (hAddress2, nDEFAULT_SIP_LISTEN_PORT);
			bEqual = RvSipAddr3261IsEqual (hAddress1, hAddress2);
			RvSipAddrUrlSetPortNum (hAddress2, nOriginalPort2);
		}
		else
		{
			bEqual = false;
		}
	}
	else
	{
		bEqual = RvSipAddr3261IsEqual (hAddress1, hAddress2);
	}
	
	return bEqual;
}


/*!\brief returns the provided message in a std::string.
 *
 */
stiHResult sipMessageGet (
	RvSipMsgHandle hMsg,
	RV_LOG_Handle hLog,
	std::string *message)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	HPAGE hPage = nullptr;
	RvUint32 messageSize = 0;
	RvStatus rvStatus = RV_OK;

#ifdef DEBUG_SIP_LOG_MEMORY_POOL
	HRPOOL hPool = RPOOL_Construct (MAX_SIP_MESSAGE_SIZE, 1, hLog, RV_FALSE, "PrintMessageContents");
#else
	HRPOOL hPool = RPOOL_Construct (MAX_SIP_MESSAGE_SIZE, 1, nullptr, RV_FALSE, "PrintMessageContents");
#endif

	// getting the encoded message on an rpool page.
	rvStatus = RvSipMsgEncode (hMsg, hPool, &hPage, &messageSize);
	stiTESTRVSTATUS ();

	message->resize (messageSize);

	// copy the encoded message to an external consecutive buffer
	rvStatus = RPOOL_CopyToExternal (hPool, hPage, 0, (void*)message->c_str (), messageSize);
	stiTESTRVSTATUS ();

STI_BAIL:

	if (hPage)
	{
		RPOOL_FreePage (hPool, hPage);
	}

	RPOOL_Destruct (hPool);

	return hResult;
}


/*!\brief returns the provided address in a std::string.
 *
 */
stiHResult sipAddrGet (
	RvSipAddressHandle hAddress,
	HRPOOL hPool,
	std::string *header)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	HPAGE hPage = nullptr;
	RvUint32 headerSize = 0;
	RvStatus rvStatus = RV_OK;

	// getting the encoded message on an rpool page.
	rvStatus = RvSipAddrEncode (hAddress, hPool, &hPage, &headerSize);
	stiTESTRVSTATUS ();

	// allocate a consecutive buffer
	header->resize(headerSize);

	// copy the encoded header to an external consecutive buffer
	rvStatus = RPOOL_CopyToExternal (hPool, hPage, 0, (void *)header->c_str (), headerSize);
	stiTESTRVSTATUS ();

STI_BAIL:

	if (hPage)
	{
		RPOOL_FreePage (hPool, hPage);
	}

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; AddressPrint ()
//
// Description: Utility to print out a Radvision address structure
//
// Returns:
//  nothing
//
stiHResult AddressPrint (HRPOOL hPool, RvSipAddressHandle hAddress)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string header;

	hResult = sipAddrGet (hAddress, hPool, &header);
	stiTESTRESULT ();

	// print it
	stiTrace ("%s", header.c_str ());

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; AddressStringCreate
//
// Description: Creates a sip address string such as:
//	sip:14445556666@127.0.0.1:5061;transport=tcp
//
// Returns:
//  a string containing the address
//
std::string AddressStringCreate (
	const char *pszLocation, /// The ip address
	const char *pszUser, /// The user portion of the sip address (as in sip:user@ip)
	unsigned int unPort, /// The network port.
	EstiTransport eTransport, /// The transport to use.
	const char *pszProtocol) /// sip or sips
{
	std::stringstream address;

	address << pszProtocol << ":";

	// Determine if we are including a user name
	if (nullptr != pszUser)
	{
		while(pszUser[0] == ' ')
		{
			pszUser = pszUser + 1;
		}

		if (pszUser[0] != '\0')
		{
			address << pszUser << "@";
		}
	}

	if (strchr (pszLocation, ':'))
	{
		// Need to strip off any ipv6 scope and [] brackets are required
		int idx1 = 0;
		
		if (pszLocation[idx1] != '[')
		{
			address << "[";
		}
		
		for (; pszLocation[idx1]; idx1++)
		{
			if (pszLocation[idx1] == '%' || pszLocation[idx1] == ']')
			{
				break;
			}

			address << pszLocation[idx1];
		}

		address << "]";
	}
	else
	{
		address << pszLocation;
	}
	
	// Append port if not zero and not the default SIP listening port
	if (nDEFAULT_SIP_LISTEN_PORT != (signed int)unPort && unPort != 0)
	{
		address << ":" << unPort;
	}

	// Append the transport
	switch (eTransport)
	{
		case estiTransportTCP:

			address << ";transport=tcp";

			break;

		case estiTransportTLS:

			//
			// Only attach the transport if the protocol is not SIPS.
			//
			if (strcmp (pszProtocol, "sips") != 0)
			{
				address << ";transport=tls";
			}

			break;

		case estiTransportUnknown:
		case estiTransportUDP:

			break;
	}

	return address.str ();
}


////////////////////////////////////////////////////////////////////////////////
//; EscapeDisplayName
//
// Description: Utility to escape a SIP display name for transmission.
//
// Returns:
//	None.
//
void EscapeDisplayName (const char *pszDisplayName, std::string *pResult)
{
	pResult->clear ();
	if (pszDisplayName && pszDisplayName[0])
	{
		// Get the display name length after adding delimiters and removing nonprintables
		int nLen= 0;
		for (int i = 0; pszDisplayName[i]; ++i)
		{
			char c = pszDisplayName[i];
			if (c >= 0x20 && c < 0x7f)
			{
				if (c == '"' || c == '%' || c == '\\')
				{
					nLen += 2;
				}
				else
				{
					++nLen;
				}
			}
		}
		// Allocate space for the converted string.
		auto pszUserName = new char [nLen + 1];
		// Convert the display name to a sip-friendly string.
		nLen= 0;
		for (int i = 0; pszDisplayName[i]; ++i)
		{
			char c = pszDisplayName[i];
			if (c >= 0x20 && c <= 0x7f)
			{
				if (c == '"' || c == '%' || c == '\\')
				{
					pszUserName[nLen++] = '\\';
					pszUserName[nLen++] = c;
				}
				else
				{
					pszUserName[nLen++] = c;
				}
			}
		}
		pszUserName[nLen] = '\0';
		pResult->assign (pszUserName);
		delete[] pszUserName;
	}
}

////////////////////////////////////////////////////////////////////////////////
//; AddPlusToPhoneNumber
//
// Description: Add the + symbol to the phone number in a SIP URI
// Prefix the phone number with + if not already there
// URI must match the following format: SIP:12345678901@s.ci.svrs.net
//
// Returns:
//	None.
//
std::string AddPlusToPhoneNumber (const std::string &uri)
{
	std::string result;

	if (!uri.empty ())
	{
		result = uri;
#ifdef ADD_PLUS_TO_PHONENUMBER
		if (0 == stiStrNICmp("sip:1", result.c_str(), 5))
		{
			if (result.find("@") == 15)
			{
				result.insert(4, "+");
			}
		}
#endif
	}

	return result;
}

////////////////////////////////////////////////////////////////////////////////
//; AddressHeaderStringCreate
//
// Description: Creates a sip address header string such as:
//	To: "display name" <address>;q=1;expires=10
//
// Returns:
//  a string containing the address header.
//
std::string AddressHeaderStringCreate (
	const std::string &headerType, /// Whether this is a From:, To: or Contact: address
	const std::string &address, /// A well-formed sip address. (see AddressStringCreate())
	const std::string &displayName, /// The display name of the user.  Can be empty, which is the default.
	float fOrder, /// set an order or preference when registering contacts
	float fExpiration) /// set an expiration time when registering contacts
{
	std::stringstream addressHeader;
	std::string userName;

	addressHeader << headerType << ":";

	EscapeDisplayName (displayName.c_str(), &userName);
	
	// Formulate optional portions of the string
	if (!userName.empty ())
	{
		addressHeader << " \"" << userName << "\"";
	}

	addressHeader << "<" << address << ">";

	if (fOrder >= 0.0F && fOrder <= 9999.99F)
	{
		addressHeader << ";q=" << fOrder;
	}

	if (fExpiration >= 0.0F && fExpiration <= 9999.99F)
	{
		addressHeader << ";expires=" << fExpiration;
	}

	return addressHeader.str ();
}


////////////////////////////////////////////////////////////////////////////////
//; MethodInAllowVerify
//
// Description: Check that the ALLOW header contains a particular method type.
//
// Abstract:
//
// Returns:
//   This function returns estiTRUE if the ALLOW header contains the passed method
//
void MethodInAllowVerify(
	const vpe::sip::Message &message,
	vpe::sip::Message::Method method,
	bool *pbAllowHeaderFound,
	bool *pbAllowTypeFound)
{
	std::vector<vpe::sip::Message::Method> allowHeader;
	bool present;

	std::tie(allowHeader, present) = message.allowHeaderGet();

	*pbAllowHeaderFound = present;

	if (present)
	{
		*pbAllowTypeFound = std::find(allowHeader.begin(), allowHeader.end(), method) != allowHeader.end();
	}
}


////////////////////////////////////////////////////////////////////////////////
//; InfoPackageVerify
//
// Description: Check that a given INFO package type is supported.
//
// Abstract:
//
// Returns:
//   This function returns estiTRUE if the Recv-Info header contains the package
//
void InfoPackageVerify (
	RvSipMsgHandle hMsg,
	const char *pszInfoPackage,
	bool *pbInfoPackageHeaderFound,
	bool *pbInfoPackageFound)
{
	RvSipHeaderListElemHandle hListElement;
	RvSipOtherHeaderHandle hHeader;
	
	*pbInfoPackageHeaderFound = false;
	*pbInfoPackageFound = false;
	
	hHeader = RvSipMsgGetHeaderByName (hMsg, (RvChar*)RECV_INFO, RVSIP_FIRST_HEADER, &hListElement);
	if (hHeader != nullptr)
	{
		*pbInfoPackageHeaderFound = true;
		RvUint nLength = RvSipOtherHeaderGetStringLength (hHeader, RVSIP_OTHER_HEADER_VALUE);
		auto pszValue = new char[nLength];
		RvStatus rvStatus = RvSipOtherHeaderGetValue (hHeader, pszValue, nLength, &nLength);
		if (rvStatus == RV_OK)
		{
			char *pszLast = pszValue;
			for (char *pszCurrent = nullptr; pszLast != nullptr; pszLast = pszCurrent)
			{
				pszCurrent = strchr (pszLast, ',');
				if (pszCurrent)
				{
					pszCurrent[0] = '\0';
					pszCurrent ++;
				}
				if (0 == strcmp (pszLast, pszInfoPackage))
				{
					*pbInfoPackageFound = true;
					break;
				}
			}
		}

		delete[] pszValue;
	}
}


EstiMediaType SorensonMediaTypeGet (RvSdpMediaType eSdpMediaType, const char *pszMediaType)
{
	EstiMediaType eMediaType = estiMEDIA_TYPE_UNKNOWN;

	switch (eSdpMediaType)
	{
		case RV_SDPMEDIATYPE_VIDEO:
			eMediaType = estiMEDIA_TYPE_VIDEO;
			break;

		case RV_SDPMEDIATYPE_AUDIO:
			eMediaType = estiMEDIA_TYPE_AUDIO;
			break;

		case RV_SDPMEDIATYPE_UNKNOWN:
			// Need to also look at the name to know if it is a text channel
			if (0 == strcmp ("text", pszMediaType))
			{
				eMediaType = estiMEDIA_TYPE_TEXT;
			}
			break;

		default:
			break;
	}

	return eMediaType;
}


RvSdpMediaType RadvisionMediaTypeGet (EstiMediaType eMediaType)
{
	RvSdpMediaType eSdpMediaType;

	switch (eMediaType)
	{
		case estiMEDIA_TYPE_VIDEO:
			eSdpMediaType = RV_SDPMEDIATYPE_VIDEO;
			break;

		case estiMEDIA_TYPE_AUDIO:
			eSdpMediaType = RV_SDPMEDIATYPE_AUDIO;
			break;

		case estiMEDIA_TYPE_TEXT:
		default:
			eSdpMediaType = RV_SDPMEDIATYPE_UNKNOWN;
			break;
	}

	return eSdpMediaType;
}


/*!\brief Decrypts a string buffer
 *
 * \param pszEncryptedString The encrypted string buffer
 * \param nEncryptedLEngth The length of the encrypted string
 * \param pDecryptedString The decrypted string is stored in this buffer
 *
 * The linux command line equivalent is:
 * 		echo what_you_are_decoding | base64 -d | openssl bf -K 690935604c5a7906465906014d083060 -iv 32774948594a5907 -d
 */
stiHResult DecryptString (
	const unsigned char *pszEncryptedString,
	unsigned int encryptedLength,
	std::string *pDecryptedString)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult = 0;
	unsigned char *pOutputBuffer = nullptr;
	int nOutputLength = 0;
	int nTmpLength = 0;
	char *pBase64Decoded = nullptr;
	int nBase64DecodedLength = 0;

	auto ctx = EVP_CIPHER_CTX_new();

	nResult = EVP_CipherInit(ctx, EVP_bf_cbc(), SSL_SINFO_KEY, SSL_SINFO_IV, 0);

	//
	// Decode the Base64 string
	// The decoded string will always be shorter than the encoded string so allocate
	// a buffer that is equal in length to the encoded string.
	//
	stiTESTCOND(encryptedLength > 0, stiRESULT_ERROR);
	pBase64Decoded = new char [encryptedLength];

	stiBase64Decode (pBase64Decoded, &nBase64DecodedLength, (const char *)pszEncryptedString);

	//
	// Decrypt the encrypted string
	//
	pOutputBuffer = new unsigned char [nBase64DecodedLength + EVP_MAX_BLOCK_LENGTH];

	stiTESTCOND (nResult == 1, stiRESULT_ERROR);

	nResult = EVP_CipherUpdate(ctx, pOutputBuffer, &nOutputLength, (unsigned char *)pBase64Decoded, nBase64DecodedLength);
	stiTESTCOND (nResult == 1, stiRESULT_ERROR);

	nResult = EVP_CipherFinal_ex(ctx, &pOutputBuffer[nOutputLength], &nTmpLength);
	stiTESTCOND (nResult == 1, stiRESULT_ERROR);

	nOutputLength += nTmpLength;

	pOutputBuffer[nOutputLength] = '\0';

	//
	// Copy the decrypted buffer to the output parameter.
	//
	*pDecryptedString += (char *)pOutputBuffer;

STI_BAIL:

	EVP_CIPHER_CTX_free(ctx);

	if (pOutputBuffer)
	{
		delete [] pOutputBuffer;
		pOutputBuffer = nullptr;
	}

	if (pBase64Decoded)
	{
		delete [] pBase64Decoded;
		pBase64Decoded = nullptr;
	}

	return hResult;
}


/*!\brief Encrypts a string buffer
 *
 * \param inputString The string buffer to be encrypted
 * \param pEncryptedString The encrypted string is stored in this buffer
 */
stiHResult EncryptString (
	const std::string &inputString,
	std::string *encryptedString)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult = 0;
	unsigned char *pOutputBuffer = nullptr;
	int nOutputLength = 0;
	int nTmpLength = 0;
	char *pBase64Buffer = nullptr;

	auto ctx = EVP_CIPHER_CTX_new();

	EVP_CipherInit(ctx, EVP_bf_cbc(), SSL_SINFO_KEY, SSL_SINFO_IV, 1);

	pOutputBuffer = new unsigned char [inputString.size () + EVP_MAX_BLOCK_LENGTH];

	nResult = EVP_CipherUpdate(ctx, pOutputBuffer, &nOutputLength, (const unsigned char *)inputString.c_str (), inputString.size ());
	stiTESTCOND (nResult == 1, stiRESULT_ERROR);

	pOutputBuffer[nOutputLength] = '\0';

	nResult = EVP_CipherFinal_ex(ctx, &pOutputBuffer[nOutputLength], &nTmpLength);
	stiTESTCOND (nResult == 1, stiRESULT_ERROR);

	nOutputLength += nTmpLength;

	//
	// Base64 encode
	//
	pBase64Buffer = new char [nOutputLength * 2];

	stiBase64Encode (pBase64Buffer, (char *)pOutputBuffer, nOutputLength);

	//
	// Copy the encoded buffer to the output parameter.
	//
	*encryptedString = pBase64Buffer;

STI_BAIL:

	EVP_CIPHER_CTX_free(ctx);

	if (pOutputBuffer)
	{
		delete [] pOutputBuffer;
		pOutputBuffer = nullptr;
	}

	if (pBase64Buffer)
	{
		delete [] pBase64Buffer;
		pBase64Buffer = nullptr;
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; SipUriCreate
//
// Description: Creates a SIP URI string using a user and domain formatted as "sip:user@domain".
//
// Returns:
//   Nothing.
//
void SipUriCreate (
	const std::string &user,
	const std::string &domain,
	std::string *uri)
{
	*uri = "sip:";

	if (!user.empty ())
	{
		*uri += user;
		*uri += "@";
	}

	*uri += domain;
}


/*!\brief Grabs the Received value from the Via header.
 *
 * \param RvSipMsgHandle SIP message handle to search for Via header.
 * \return A string containing the address if found in the Via header. Otherwise an empty string.
 */
viaHeader viaHeaderGet (
	RvSipMsgHandle message)
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);
	unsigned int actualLen = RV_ADDRESS_MAXSTRINGSIZE + 1;
	RvSipHeaderListElemHandle listElement = nullptr;
	RvStatus rvStatus;
	auto buffer = sci::make_unique<char[]>(actualLen);
	viaHeader header;
	int rPort = UNDEFINED;
	RvBool useRPort = false;
	
	// Get the Via at the top of the list
	auto  via = static_cast<RvSipViaHeaderHandle>(RvSipMsgGetHeaderByType (
		message, RVSIP_HEADERTYPE_VIA, RVSIP_FIRST_HEADER, &listElement));
	stiTESTCOND (via != nullptr, stiRESULT_ERROR);
	
	stiTESTCOND (buffer != nullptr, stiRESULT_MEMORY_ALLOC_ERROR);
	
	// Get Transport
	header.transport = RvSipViaHeaderGetTransport (via);
	
	// Get the Host
	rvStatus = RvSipViaHeaderGetHost (via, buffer.get (), actualLen, &actualLen);
	if (rvStatus == RV_ERROR_INSUFFICIENT_BUFFER)
	{
		buffer = sci::make_unique<char[]>(actualLen);
		stiTESTCOND (buffer != nullptr, stiRESULT_MEMORY_ALLOC_ERROR);
		rvStatus = RvSipViaHeaderGetHost (via, buffer.get (), actualLen, &actualLen);
	}
	stiTESTRVSTATUS ();
	
	if (IPAddressValidate (buffer.get ()))
	{
		header.hostAddr.assign (buffer.get ());
	}
	
	// Get the Host Port
	header.hostPort = RvSipViaHeaderGetPortNum (via);
	
	// Get the branch currently not used but still part of the Via.
	rvStatus = RvSipViaHeaderGetBranchParam(via, buffer.get (), actualLen, &actualLen);
	if (rvStatus == RV_ERROR_INSUFFICIENT_BUFFER)
	{
		buffer = sci::make_unique<char[]>(actualLen);
		stiTESTCOND (buffer != nullptr, stiRESULT_MEMORY_ALLOC_ERROR);
		rvStatus = RvSipViaHeaderGetBranchParam(via, buffer.get (), actualLen, &actualLen);
	}
	
	// Unless we start using the branch this is more of a sanity check to look for problems with SIP messages.
	stiASSERTMSG(rvStatus == RV_OK, "Unable to get branch parameter from via header rvStatus = %d", rvStatus);
	if (rvStatus == RV_OK)
	{
		header.branch.assign (buffer.get ());
	}
	
	// get the received param on the via header
	rvStatus = RvSipViaHeaderGetReceivedParam (via, buffer.get(), actualLen, &actualLen); // Get the length
	if (rvStatus == RV_ERROR_INSUFFICIENT_BUFFER)
	{
		buffer = sci::make_unique<char[]>(actualLen);
		stiTESTCOND (buffer != nullptr, stiRESULT_MEMORY_ALLOC_ERROR);
		
		rvStatus = RvSipViaHeaderGetReceivedParam (via, buffer.get(), actualLen, &actualLen);
	}

	if  (rvStatus != RV_ERROR_NOT_FOUND)
	{
		stiTESTRVSTATUS ();
	}
	
	if (IPAddressValidate (buffer.get ()))
	{
		header.receivedAddr.assign (buffer.get ());
	}
	
	// Get the RPort
	rvStatus = RvSipViaHeaderGetRportParam (via, &rPort, &useRPort);
	
	if (useRPort)
	{
		header.rPort = rPort;
		stiTESTRVSTATUS ();
	}

STI_BAIL:
	
	return header;
}


std::string dhvUriCreate (const std::string &uri, const std::string &participantType)
{
	// Attempt to format the address to call the Hearing dialplan on the MCU.
	auto position = uri.find(DIALPLAN_URI_HEADER);
	std::string dhvUri;
	
	if (position != std::string::npos)
	{
		dhvUri = participantType + uri.substr (position);
	}
	else
	{
		dhvUri = uri;
	}
	
	return dhvUri;
}

////////////////////////////////////////////////////////////////////////////////
//; UnescapeDisplayName
//
// Description: Utility to translate an escaped SIP display name back to readable form.
//
// Returns:
//	None.
//
void UnescapeDisplayName (const char *pszEscapedName, std::string *pResult)
{
	pResult->clear ();
	if (pszEscapedName && pszEscapedName[0])
	{
		auto pszUnescapedName = new char[strlen (pszEscapedName) + 1]; // It will always be the same length or shorter
		int nResultIndex = 0;
		for (int i = 0; pszEscapedName[i]; ++pszEscapedName)
		{
			switch (pszEscapedName[i])
			{
				case '\\':
					if (pszEscapedName[i + 1])
					{
						pszUnescapedName[nResultIndex++] = pszEscapedName[++i];
					}
					break;
				case '"':
					break;
				default:
					pszUnescapedName[nResultIndex++] = pszEscapedName[i];
			}
		}
		pszUnescapedName[nResultIndex] = '\0';
		pResult->assign (pszUnescapedName);
		delete [] pszUnescapedName;
		pszUnescapedName = nullptr;
	}
}

// end file stiSipTools.cpp
