// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include <vector>

class BleSendProgress
{
public:
	
	BleSendProgress () = default;
	BleSendProgress (const BleSendProgress &other) = delete;
	BleSendProgress (BleSendProgress &&other) = delete;
	BleSendProgress &operator= (const BleSendProgress &other) = delete;
	BleSendProgress &operator= (BleSendProgress &&Other) = delete;
	virtual ~BleSendProgress() = default;

	void initialize (
		unsigned int totalLength,
		unsigned int maxObjectSize);

	unsigned int currentObjectAvailableLengthGet () const;

	unsigned int bytesSentGet () const;
	void bytesSentSet (unsigned int bytesSent);

	unsigned int bytesReceivedGet () const;
	void bytesReceivedSet (unsigned int bytesReceived);

	void bytesSentAdd (unsigned int bytesSent);

	unsigned int lastExecutedByteGet () const;
	void lastExecutedByteSet (unsigned int lastExecuted);

	bool isComplete () const;
	bool isObjectComplete () const;

	void nextObject ();
	bool isLastObject () const;

	unsigned int percentCompleteGet ();

private:

	unsigned int m_totalLength = 0;
	unsigned int m_maxObjectLength = 0;
	std::vector<unsigned int> m_objectIndices;
	unsigned int m_currentObject = 0;
	unsigned int m_currentObjectLength = 0;
	unsigned int m_bytesSent = 0;
	unsigned int m_bytesReceived = 0;
	unsigned int m_lastExecutedIndex = 0;
};

