//*****************************************************************************
//
// FileName:        CstiMFBufferWrapper.h
//
// Abstract:        Declaration of the CstiMFBufferWrapper class used to wrap a byte array into
//					a Media Foundation buffer.
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//*****************************************************************************
#pragma once
#include "CstiMediaFoundation.h"

class CstiMFBufferWrapper : public IMFMediaBuffer
{
private:
	const DWORD  m_cbMaxLength;
	LONG         m_nRefCount;  // Reference count

	CstiMFBufferWrapper (DWORD cbMaxLength, HRESULT& hr);
	CstiMFBufferWrapper (BYTE *pbData, DWORD cbLength);
	~CstiMFBufferWrapper ();
public:
	BYTE         *m_pbData;
	DWORD        m_cbLength;
	HANDLE		dataMutex;

	static HRESULT Create (IMFMediaBuffer **ppBuffer, BYTE *pbData, DWORD cbLength);

	// IUnknown methods.
	STDMETHODIMP QueryInterface (REFIID riid, void **ppv);
	STDMETHODIMP_ (ULONG) AddRef ();
	STDMETHODIMP_ (ULONG) Release ();

	// IMFMediaBuffer methods
	STDMETHODIMP SetCurrentLength (DWORD cbCurrentLength);
	STDMETHODIMP GetCurrentLength (DWORD *pcbCurrentLength);
	STDMETHODIMP GetMaxLength (DWORD *pcbMaxLength);
	STDMETHODIMP Lock (BYTE **ppbBuffer, DWORD *pcbMaxLength, DWORD *pcbCurrentLength);
	STDMETHODIMP Unlock ();
};
