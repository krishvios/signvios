///
/// \file CstiUpdate.h
/// \brief Declaration of the Update class for ntouch
///
/// Sorenson Communications Inc. Confidential. --  Do not distribute
/// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
///
#pragma once

//
// Includes
//
#include "CstiEventQueue.h"
#include "IUpdate.h"

#include "CstiUpdateConnection.h"
#include "CstiUpdateWriter.h"

//
// Constants
//

//
// Typedefs
//
//
// Globals
//

//
// Class Declaration
//
class CstiUpdate : public IUpdate, public CstiEventQueue
{
public:
	
	CstiUpdate ();
	
	CstiUpdate (const CstiUpdate &other) = delete;
	CstiUpdate (CstiUpdate &&other) = delete;
	CstiUpdate &operator= (const CstiUpdate &other) = delete;
	CstiUpdate &operator= (CstiUpdate &&other) = delete;

	~CstiUpdate () override;

	stiHResult Startup ();
	
	stiHResult Initialize ();

	//
	// Methods called by the Application
	//
	void updateDownload () override;
	void updateInstall () override;
	void update (const std::string &url) override;

	void pause () override;

	void resume () override;

private:

	/*!
	* \enum EState
	*
	* \brief Update progress states
	*
	*/
	enum EState
	{
		eUNINITIALIZED,
		eIDLE,
		eCONNECTING,
		eCONNECTING_PAUSED,
		eCONNECTED,
		eDOWNLOADING,
		ePROGRAMMING,
		eNETWORK_ERROR,
		eCRC_ERROR,
		eFLASH_ERROR,
		eDOWNLOAD_PAUSED,
		eUPDATE_COMPLETE
	}; // end EState
	
	EState stateGet ();
	
	void stateSet (
		EState eState);
	
	stiHResult downloadProcessStart ();
	
	bool updateDownloaded ();
	
	int imageSizeGet ();
	int bytesDownloadedGet ();
	int bytesWrittenGet ();

	stiHResult writeImageToFlash ();
	
	stiHResult cleanup ();
		
	stiHResult nextBufferGet ();

	
	stiHResult oldImageReplace (
		const char * pszNewImagePath);
	
	stiHResult oldImageGet (
		const char ** ppszOldImagePath,
		long * pnHighestTimeStamp);
		
	stiHResult imageTimeStampSet (
		const char * pszImagePath,
		long nTimeStamp);
	
	stiHResult imageCopy (
		const char * pszSrcImagePath,
		const char * pszDstImagePath);
	
	// Events
	void eventUpdate (
		const std::string &url);
	void eventNextBufferGet ();
	void eventWriteToFlash ();
	void eventBufferWritten (
		char * buffer,
		int bytesWritten);
	
	CstiSignalConnection::Collection m_signalConnections;
	
	CstiUpdateConnection *m_pUpdateConnection {nullptr};
	
	std::unique_ptr<CstiUpdateWriter> m_updateWriter;
	
	EState m_eState {eUNINITIALIZED};		// Current state
	
	int m_nTotalDataLength {0};
	int m_nBytesDownloaded {0};
	int m_nBytesWritten {0};
	
	int m_nProgressPercent {0};
	unsigned int m_nProgress {0};

	bool m_bUpdateDownloaded {0};
	int m_nPauseCount {0};
	bool m_bResume {0};

	int m_nMaxDownloadLength{0};

	std::list<char *> m_EmptyList;
	std::list<char *> m_FullList;

	std::string m_updateUrl;
};

// end file CstiUpdate.h
