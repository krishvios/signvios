//*****************************************************************************
//
// FileName:        CstiMFBufferWrapper.cpp
//
// Abstract:        Declaration of the CstiMFBufferWrapper class used to wrap a byte array into
//					a Media Foundation buffer.
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//*****************************************************************************
#include "CstiMFBufferWrapper.h"

CstiMFBufferWrapper::CstiMFBufferWrapper (BYTE *pbData, DWORD cbLength) :
	m_nRefCount (1),
	m_cbMaxLength (cbLength),
	m_cbLength (cbLength),
	m_pbData (pbData)
{
	dataMutex = CreateMutex (NULL, FALSE, "CstiMFBufferWrapperMutex");
}

CstiMFBufferWrapper::~CstiMFBufferWrapper ()
{
	if (m_pbData)
	{
		delete[] m_pbData;
	}
	CloseHandle (dataMutex);
}

// Function to create a new IMediaBuffer object and return 
// an AddRef'd interface pointer.
HRESULT CstiMFBufferWrapper::Create (IMFMediaBuffer **ppBuffer, BYTE *pbData, DWORD cbLength)
{
	HRESULT hr = S_OK;

	if (ppBuffer == NULL)
	{
		return E_POINTER;
	}

	*ppBuffer = new CstiMFBufferWrapper (pbData, cbLength);

	return hr;
}

// IUnknown methods.
STDMETHODIMP CstiMFBufferWrapper::QueryInterface (REFIID riid, void **ppv)
{
	if (ppv == NULL)
	{
		return E_POINTER;
	}
	else if (riid == IID_IMFMediaBuffer || riid == IID_IUnknown)
	{
		*ppv = static_cast<IMFMediaBuffer *>(this);
		AddRef ();
		return S_OK;
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
}

STDMETHODIMP_ (ULONG) CstiMFBufferWrapper::AddRef ()
{
	return InterlockedIncrement (&m_nRefCount);
}

STDMETHODIMP_ (ULONG) CstiMFBufferWrapper::Release ()
{
	LONG lRef = InterlockedDecrement (&m_nRefCount);
	if (lRef == 0)
	{
		delete this;
		// m_cRef is no longer valid! Return lRef.
	}
	return lRef;
}


STDMETHODIMP CstiMFBufferWrapper::SetCurrentLength (DWORD cbCurrentLength)
{
	if (cbCurrentLength > m_cbMaxLength)
	{
		return E_INVALIDARG;
	}
	m_cbLength = cbCurrentLength;
	return S_OK;
}

STDMETHODIMP CstiMFBufferWrapper::GetCurrentLength (DWORD *pcbCurrentLength)
{
	if (pcbCurrentLength)
	{
		*pcbCurrentLength = m_cbLength;
		return S_OK;
	}
	return E_POINTER;
}

STDMETHODIMP CstiMFBufferWrapper::GetMaxLength (DWORD *pcbMaxLength)
{
	if (pcbMaxLength == NULL)
	{
		return E_POINTER;
	}
	*pcbMaxLength = m_cbMaxLength;
	return S_OK;
}

STDMETHODIMP CstiMFBufferWrapper::Lock (BYTE **ppbBuffer, DWORD *pcbMaxLength, DWORD *pcbLength)
{
	DWORD dwWaitResult = WaitForSingleObject (dataMutex, 300);
	if (dwWaitResult == WAIT_OBJECT_0)
	{
		// Either parameter can be NULL, but not both.
		if (ppbBuffer == NULL && pcbLength == NULL)
		{
			return E_POINTER;
		}
		if (ppbBuffer)
		{
			*ppbBuffer = m_pbData;
		}
		if (pcbLength)
		{
			*pcbLength = m_cbLength;
		}
		if (pcbMaxLength)
		{
			*pcbMaxLength = m_cbMaxLength;
		}
		return S_OK;
	}
	return E_POINTER;
}

STDMETHODIMP CstiMFBufferWrapper::Unlock ()
{
	ReleaseMutex (dataMutex);
	return S_OK;
}