// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2018 Sorenson Communications, Inc. -- All rights reserved

#include "BleSendProgress.h"

void BleSendProgress::initialize (
		unsigned int totalLength,
		unsigned int maxObjectLength)
{
	m_totalLength = totalLength;
	m_maxObjectLength = maxObjectLength;
	m_objectIndices.clear ();

	if (m_maxObjectLength < m_totalLength)
	{
		unsigned int numOfObjects = (m_totalLength + m_maxObjectLength - 1) / m_maxObjectLength;

		for (unsigned int i = 0; i < numOfObjects; i++)
		{
			m_objectIndices.push_back (i * m_maxObjectLength);
		}

		m_currentObjectLength = m_maxObjectLength;
	}
	else
	{
		m_objectIndices.push_back (0);
		m_currentObjectLength = m_totalLength;
	}

	m_currentObject = 0;
	m_bytesSent = 0;
	m_bytesReceived = 0;
	m_lastExecutedIndex = 0;
}


unsigned int BleSendProgress::currentObjectAvailableLengthGet () const
{
	if (isLastObject ())
	{
		return m_totalLength - m_bytesSent;
	}
	
	return m_objectIndices[m_currentObject + 1] - m_bytesSent;
}


unsigned int BleSendProgress::bytesSentGet () const
{
	return m_bytesSent;
}


void BleSendProgress::bytesSentSet (unsigned int bytesSent)
{
	m_bytesSent = bytesSent;
}


unsigned int BleSendProgress::bytesReceivedGet () const
{
	return m_bytesReceived;
}


void BleSendProgress::bytesReceivedSet (unsigned int bytesReceived)
{
	m_bytesReceived = bytesReceived;
}


void BleSendProgress::bytesSentAdd (unsigned int bytesSent)
{
	m_bytesSent += bytesSent;
}


unsigned int BleSendProgress::lastExecutedByteGet () const
{
	return m_lastExecutedIndex;
}


void BleSendProgress::lastExecutedByteSet (unsigned int lastExecuted)
{
	m_lastExecutedIndex = lastExecuted;
}


bool BleSendProgress::isComplete () const
{
	return m_bytesSent >= m_totalLength;
}


bool BleSendProgress::isObjectComplete () const
{
	if (isLastObject ())
	{
		return isComplete ();
	}
	else if ((m_bytesSent - m_objectIndices[m_currentObject]) >= m_maxObjectLength)
	{
		return true;
	}

	return false;
}


void BleSendProgress::nextObject ()
{
	// Do not allow indexing to next object if there isn't one
	if (!isLastObject ())
	{
		m_currentObject++;
	}
}


bool BleSendProgress::isLastObject () const
{
	return (m_objectIndices.size () - m_currentObject) == 1;
}

unsigned int BleSendProgress::percentCompleteGet ()
{
	return m_bytesSent * 100 / m_totalLength;
}

