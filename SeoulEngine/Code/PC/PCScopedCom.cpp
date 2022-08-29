/**
 * \file PCScopedCom.cpp
 * \brief Utility to initialize and uninitialize COM services
 * for operations that depend on COM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "PCScopedCom.h"

#define SEOUL_ENABLE_CTLMGR 1 // For CoCreateInstance, etc.

#include "Platform.h"
#include "PCScopedCom.h"

// Pre-declarations for the COM API.
extern "C"
{

HRESULT WINAPI CoInitializeEx(_In_opt_ LPVOID pvReserved,_In_ DWORD dwCoInit);
void WINAPI CoUninitialize(void);

} // extern "C"

namespace Seoul
{

PCScopedCom::PCScopedCom()
	: m_eResult(::CoInitializeEx(nullptr, 0x0 /* COINIT_MULTITHREADED */ | 0x4 /* COINIT_DISABLE_OLE1DDE */))
{
	// TODO: Verify here in case one of our third parties (unexpectedly)
	// initializes COM. If that is the case, this value will be something other
	// than S_OK or S_FALSE (RPC_E_CHANGED_MODE is most likely).
	SEOUL_ASSERT(SUCCEEDED(m_eResult));
}

PCScopedCom::~PCScopedCom()
{
	// TODO: Not entirely clear, but based on additional interpretations from
	// the internet (see also: https://github.com/mlabbe/nativefiledialog/blob/master/src/nfd_win.cpp#L52),
	// it appears that we must call CoUninitialize if CoInitializeEx() returned S_TRUE or S_FALSE
	// but otherwise we must not.
	//
	// "success" in the below is likely equal to HRESULT codes that are considered
	// success codes (S_TRUE and S_FALSE).
	//
	// See also: https://docs.microsoft.com/en-us/windows/win32/api/objbase/nf-objbase-coinitialize#remarks
	//   "Typically, the COM library is initialized on a thread only once.
	//    Subsequent calls to CoInitialize or CoInitializeEx on the same thread
	//    will succeed, as long as they do not attempt to change the concurrency model,
	//    but will return S_FALSE. To close the COM library gracefully, each successful
	//    call to CoInitialize or CoInitializeEx, including those that return S_FALSE, must
	//    be balanced by a corresponding call to CoUninitialize. However, the first thread
	//    in the application that calls CoInitialize with 0 (or CoInitializeEx with COINIT_APARTMENTTHREADED)
	//    must be the last thread to call CoUninitialize. Otherwise, subsequent calls to CoInitialize on
	//    the STA will fail and the application will not work."
	if (SUCCEEDED(m_eResult))
	{
		::CoUninitialize();
	}
}

} // namespace Seoul
