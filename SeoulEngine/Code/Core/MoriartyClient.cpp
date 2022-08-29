/**
 * \file MoriartyClient.cpp
 * \brief Client class for interacting with the Moriarty server tool
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Compress.h"
#include "Logger.h"
#include "MoriartyClient.h"
#include "MoriartyRPC.h"
#include "Path.h"
#include "SeoulSocket.h"
#include "SocketStream.h"

#if SEOUL_WITH_MORIARTY

#include <stdint.h>

namespace Seoul
{

using namespace MoriartyRPC;

SEOUL_STATIC_ASSERT(kRPCCount <= kResponseFlag);

MoriartyClient::MoriartyClient()
	: m_pSocket(SEOUL_NEW(MemoryBudgets::Network) Socket)
	, m_pSocketConnectionMutex(SEOUL_NEW(MemoryBudgets::Network) Mutex)
	, m_pStream(SEOUL_NEW(MemoryBudgets::Network) SocketStream(*m_pSocket))
	, m_bShuttingDown(false)
	, m_bHandshakeCompleted(false)
	, m_bConnecting(false)
	, m_NextToken(1)
	, m_KeyboardKeyEventHandler(nullptr)
	, m_ContentChangeEventHandler(nullptr)
	, m_KeyboardCharEventHandler(nullptr)
	, m_StatFileCacheMutex()
	, m_tStatFileCache()
{
	// Set up RPC handler arrays
	memset(m_aRPCHandlers, 0, sizeof(m_aRPCHandlers));

	RegisterRPCHandler(kStatFile,                   &MoriartyClient::OnStatFile, true);
	RegisterRPCHandler(kOpenFile,                   &MoriartyClient::OnOpenFile, true);
	RegisterRPCHandler(kCloseFile,                  &MoriartyClient::OnCloseFile, true);
	RegisterRPCHandler(kReadFile,                   &MoriartyClient::OnReadFile, true);
	RegisterRPCHandler(kWriteFile,                  &MoriartyClient::OnWriteFile, true);
	RegisterRPCHandler(kSetFileModifiedTime,        &MoriartyClient::OnSetFileModifiedTime, true);
	RegisterRPCHandler(kGetDirectoryListing,        &MoriartyClient::OnGetDirectoryListing, true);
	RegisterRPCHandler(kCookFile,                   &MoriartyClient::OnCookFile, true);
	RegisterRPCHandler(kKeyboardKeyEvent,           &MoriartyClient::OnKeyboardKeyEvent, false);
	RegisterRPCHandler(kContentChangeEvent,         &MoriartyClient::OnContentChangeEvent, false);
	RegisterRPCHandler(kKeyboardCharEvent,          &MoriartyClient::OnKeyboardCharEvent, false);
	RegisterRPCHandler(kStatFileCacheRefreshEvent,  &MoriartyClient::OnStatFileCacheRefreshEvent, false);
	RegisterRPCHandler(kCreateDirPath, &MoriartyClient::OnCreateDirPath, true);
	RegisterRPCHandler(kDelete, &MoriartyClient::OnDelete, true);
	RegisterRPCHandler(kRename, &MoriartyClient::OnRename, true);
	RegisterRPCHandler(kSetReadOnlyBit, &MoriartyClient::OnSetReadOnlyBit, true);
	RegisterRPCHandler(kCopy, &MoriartyClient::OnCopy, true);
	RegisterRPCHandler(kDeleteDirectory, &MoriartyClient::OnDeleteDirectory, true);
}

MoriartyClient::~MoriartyClient()
{
	Disconnect();

	m_pStream.Reset();
	m_pSocket.Reset();
	m_pSocketConnectionMutex.Reset();
}

/**
 * Synchronously connects to the given Moriarty server -- this may block for a
 * non-trivial amount of time in bad network situations.  Must be called before
 * calling any other functions, or they will fail.
 *
 * @param[in] sServerHostname Hostname or IP address of the server to connect
 *              to
 *
 * @return True if the connection succeeded, or false if the connection failed
 */
Bool MoriartyClient::Connect(const String& sServerHostname)
{
	// Clear to the disconnected state before connecting.
	Disconnect();

	// This is a connecting scope.
	ConnectingScope scope(*this);

	// Attempt the connection. Need to release the connecting mutex during this scope.
	m_pSocketConnectionMutex->Unlock();
	Bool const bSuccess = m_pSocket->Connect(Socket::kTCP, sServerHostname, kMoriartyPort);
	m_pSocketConnectionMutex->Lock();

	// If failed, or if we are no longer connecting (m_bConnecting is now false), return immediately.
	if (!bSuccess || !m_bConnecting)
	{
		return false;
	}

	// Disable the Nagle algorithm
	m_pSocket->SetTCPNoDelay(true);

	// Do a version check to make sure we're connecting to a Moriarty server
	// that can handle us
	if (!m_pStream->Write32(kProtocolVersion) ||
		!m_pStream->Write32(kConnectMagic) ||
		!m_pStream->Write32((UInt32)keCurrentPlatform) ||
		!m_pStream->Flush())
	{
		SEOUL_LOG_NETWORK("MoriartyClient::Connect: Failed to send RPC\n");
		goto fail;
	}

	// Read back in server's version
	UInt32 uServerVersion, uServerMagic;
	if (!m_pStream->Read32(uServerVersion) ||
		!m_pStream->Read32(uServerMagic))
	{
		SEOUL_LOG_NETWORK("MoriartyClient::Connect: Failed to receive RPC response\n");
		goto fail;
	}

	// Validate protocol version and magic number
	if (uServerVersion != kProtocolVersion ||
		uServerMagic != kConnectResponseMagic)
	{
		SEOUL_LOG_NETWORK("MoriartyClient::Connect: Bad server response: version=0x%08x magic=0x%08x\n", uServerVersion, uServerMagic);
		goto fail;
	}

	// Start up receive thread
	m_pReceiveThread.Reset(SEOUL_NEW(MemoryBudgets::Network) Thread(SEOUL_BIND_DELEGATE(&MoriartyClient::ReceiveLoop, this)));
	m_pReceiveThread->Start("MoriartyClient worker thread");

	m_bHandshakeCompleted = true;
	return true;

fail:
	m_pSocket->Close();
	m_pStream->Clear();
	return false;
}

/**
 * Disconnects from the server, which implicitly closes all currently open
 * remote files and cancels any pending asynchronous I/O.  This is
 * automatically called from the destructor.
 */
void MoriartyClient::Disconnect()
{
	// Disconnect block is synchronized around the connection mutex.
	Lock lock(*m_pSocketConnectionMutex);

	// Disconnect if we have a receive thread instance or if
	// a connection is pending.
	if (!m_bConnecting && !m_pReceiveThread.IsValid())
	{
		return;
	}

	// No longer connecting.
	m_bConnecting = false;

	// This cannot be called from the ReceiveLoop thread
	SEOUL_ASSERT(Thread::GetThisThreadId() != m_ReceiveThreadID);

	// Now starting the process of shutting down.
	m_bShuttingDown = true;
	m_bHandshakeCompleted = false;

	// Shutdown and close the socket first to unblock the receiving thread
	m_pSocket->Shutdown();
	m_pSocket->Close();
	m_pStream->Clear();

	// May or may not have a receiving thread at this point,
	// since we can Disconnect() just to cancel a Connect().
	if (m_pReceiveThread.IsValid())
	{
		// Wait for the the receiving thread to finish. Need to release
		// the mutex during this scope.
		m_pSocketConnectionMutex->Unlock();
		m_pReceiveThread->WaitUntilThreadIsNotRunning();
		m_pSocketConnectionMutex->Lock();
		m_pReceiveThread.Reset();
	}

	// The receive loop thread will close the socket
	SEOUL_ASSERT(!m_pSocket->IsConnected());
	SEOUL_ASSERT(m_CallsInProgress.IsEmpty());

	// Done shutting down.
	m_bShuttingDown = false;
}

/**
 * Tests if we are currently connected to a server and not in the process of
 * shutting down
 */
Bool MoriartyClient::IsConnected() const
{
	return m_pSocket->IsConnected() && !m_bShuttingDown && m_bHandshakeCompleted;
}

/** Thread procedure for running the RPC receive loop */
Int MoriartyClient::ReceiveLoop(const Thread& thread)
{
	// Since Windows has no way to get the thread ID of a thread from a
	// thread handle pre-Vista, we need to store our thread ID now to track it
	m_ReceiveThreadID = Thread::GetThisThreadId();

	while (true)
	{
		// Continue to receive messages until we run into a problem
		UInt8 uRPCIndex;
		if (!m_pStream->Read8(uRPCIndex))
		{
			break;
		}

		// Read token, if it's a response RPC
		UInt32 uResponseToken = 0;
		if ((uRPCIndex & kResponseFlag) &&
			!m_pStream->Read32(uResponseToken))
		{
			break;
		}

		// Try to handle the RPC, bail if it's unknown or invalid
		if (!HandleRPC(uRPCIndex, uResponseToken))
		{
			break;
		}
	}

	// Cancel any outstanding RPCs in progress
	CancelCallsInProgress();

	// Close the socket and release any remaining data in the stream.
	{
		Lock lock(*m_pSocketConnectionMutex);
		m_pSocket->Close();
		m_pStream->Clear();
	}

	// Reset state.
	m_ReceiveThreadID = ThreadId();

	return 0;
}

/**
 * Tries to handle an RPC received on the socket
 */
Bool MoriartyClient::HandleRPC(UInt8 uRPCIndex, UInt32 uResponseToken)
{
	// Validate RPC index
	if (!m_aRPCHandlers[uRPCIndex])
	{
		SEOUL_LOG_NETWORK("[MoriartyClient] Invalid/unknown RPC received: 0x%02x\n", uRPCIndex);
		return false;
	}

	// If this is the server sending us an RPC, just run it
	if (!(uRPCIndex & kResponseFlag))
	{
		SEOUL_ASSERT(uResponseToken == 0);
		return ((this ->* m_aRPCHandlers[uRPCIndex])(nullptr) == kResultSuccess);
	}

	// Otherwise, it's the server sending a response to an earlier RPC we sent
	CallInProgress *pCallInfo = nullptr;

	{
		// Find the call for this token
		Lock lock(m_CallsInProgressMutex);
		for (auto callIter = m_CallsInProgress.Begin(); callIter != m_CallsInProgress.End(); ++callIter)
		{
			if ((*callIter)->m_uToken == uResponseToken)
			{
				// Remove the call from the list of calls in progress
				pCallInfo = *callIter;
				m_CallsInProgress.Erase(callIter);
				break;
			}
		}
	}

	if (pCallInfo == nullptr)
	{
		// This shouldn't happen if the server is well-behaved
		SEOUL_LOG_NETWORK("[MoriartyClient] Received RPC response 0x%02x with token 0x%08x but no such call currently in progress\n", uRPCIndex, uResponseToken);
		return false;
	}

	// Check to make sure that this is actually the RPC result we're expecting
	if ((uRPCIndex & ~kResponseFlag) == pCallInfo->m_eRPC)
	{
		// Run the callback and signal the calling thread to wake up
		pCallInfo->m_eResult = (this ->* m_aRPCHandlers[uRPCIndex])(pCallInfo->m_pData);
		pCallInfo->m_Signal.Activate();

		return true;
	}
	else
	{
		SEOUL_LOG_NETWORK("[MoriartyClient] Received RPC response 0x%02x with token 0x%08x but expected RPC response 0x%02x for that call\n", uRPCIndex, uResponseToken, pCallInfo->m_eRPC | kResponseFlag);
		pCallInfo->m_eResult = kResultRPCFailed;
		pCallInfo->m_Signal.Activate();

		return false;
	}
}

/**
 * Sets up internal data structures for an RPC that is beginning
 *
 * @param[in] eRPC Index of the RPC which is beginning
 * @param[in] pData RPC-specific data
 *
 * @return Pointer to a CallInProgress structure identifying the given RPC,
 *         or nullptr if we're not connected or shutting down
 */
MoriartyClient::CallInProgress *MoriartyClient::BeginRPC(ERPC eRPC, void *pData)
{
	Lock lock(m_CallsInProgressMutex);

	if (!IsConnected())
	{
		return nullptr;
	}

	UInt32 uToken = m_NextToken++;
	CallInProgress *pCallInfo = SEOUL_NEW(MemoryBudgets::Network) CallInProgress(eRPC, uToken, pData);

	m_CallsInProgress.PushBack(pCallInfo);

	return pCallInfo;
}

/**
 * Waits for a response to the RPC in progress identified by pCallInfo
 *
 * @param[in] pCallInfo Structure identifying the RPC in progress
 *
 * @return Result code for the RPC
 */
EResult MoriartyClient::WaitForResponse(CallInProgress *pCallInfo)
{
	pCallInfo->m_Signal.Wait();

	EResult eResult = pCallInfo->m_eResult;

	SEOUL_DELETE pCallInfo;

	if (eResult == kResultRPCFailed)
	{
		SEOUL_LOG_NETWORK("[MoriartyClient] Protocol error, disconnecting\n");
		Disconnect();
	}

	return eResult;
}

/** Cancels all RPCs currently in progress */
void MoriartyClient::CancelCallsInProgress()
{
	Lock lock(m_CallsInProgressMutex);

	// Cancel each call and wake up any waiting threads
	for (auto callIter = m_CallsInProgress.Begin(); callIter != m_CallsInProgress.End(); ++callIter)
	{
		(*callIter)->m_eResult = kResultCanceled;
		(*callIter)->m_Signal.Activate();
	}

	m_CallsInProgress.Clear();
}

/** Registers an RPC handler function */
void MoriartyClient::RegisterRPCHandler(ERPC eRPC, RPCHandler pHandler, Bool bIsResponse)
{
	UInt8 uRPCIndex = (UInt8)(bIsResponse ? (eRPC | kResponseFlag) : eRPC);
	SEOUL_ASSERT(!m_aRPCHandlers[uRPCIndex]);
	m_aRPCHandlers[uRPCIndex] = pHandler;
}

/**
 * Send log text to the Moriarty server.
 *
 * @param[in] sMessage The text to send. Can be multiline and should
 * not include any logger decorations (timestamps).
 *
 * @return True if the message was sent to the server successfully, false otherwise.
 */
Bool MoriartyClient::LogMessage(const String& sMessage)
{
	// Can't send messages if the client is not connected.
	if (!IsConnected())
	{
		return false;
	}

	// Lock the send mutex and transmit the log data.
	Lock lock(m_SendMutex);

	if (!m_pStream->Write8((UInt8)kLogMessage) ||
		!m_pStream->Write(sMessage) ||
		!m_pStream->Flush())
	{
		return false;
	}

	return true;
}

/**
 * Gets basic information about a remote file at a given file path.
 *
 * @param[in] filePath Remote file path to stat
 * @param[out] pOutStat Receives file information, if successful
 *
 * @return True if successful, or false if an error occurred
 */
Bool MoriartyClient::StatFile(FilePath filePath, FileStat *pOutStat)
{
	// Can't stat an invalid FilePath.
	if (!filePath.IsValid())
	{
		return false;
	}

	// Lookup the FilePath in the local cache if it is valid.
	if (filePath.IsValid())
	{
		Lock lock(m_StatFileCacheMutex);
		if (m_tStatFileCache.GetValue(filePath, *pOutStat))
		{
			return true;
		}
	}

	CallInProgress *pCallInfo = BeginRPC(kStatFile, pOutStat);
	if (pCallInfo == nullptr)
	{
		return false;
	}

	{
		Lock lock(m_SendMutex);

		if (!m_pStream->Write8((UInt8)kStatFile) ||
			!m_pStream->Write32(pCallInfo->m_uToken) ||
			!m_pStream->Write(filePath) ||
			!m_pStream->Flush())
		{
			return false;
		}
	}

	if (WaitForResponse(pCallInfo) == kResultSuccess)
	{
		// If we have a valid file path, refresh the cached
		// stat file structure.
		if (filePath.IsValid())
		{
			Lock lock(m_StatFileCacheMutex);
			SEOUL_VERIFY(m_tStatFileCache.Overwrite(filePath, *pOutStat).Second);
		}

		return true;
	}

	return false;
}

/**
 * Handler for the StatFile RPC
 */
EResult MoriartyClient::OnStatFile(void *pData)
{
	FileStat *pOutStat = (FileStat *)pData;

	UInt8 uStatus, uFlags;
	if (!m_pStream->Read8(uStatus) ||
		!m_pStream->Read8(uFlags) ||
		!m_pStream->Read64(pOutStat->m_uFileSize) ||
		!m_pStream->Read64(pOutStat->m_uModifiedTime) ||
		uStatus >= kMaxResult)
	{
		return kResultRPCFailed;
	}

	pOutStat->m_bIsDirectory = ((uFlags & kFlag_StatFile_Directory) != 0);
	return (EResult)uStatus;
}

/** Data used for the OpenFile RPC */
struct OpenFileData
{
	OpenFileData()
		: m_iFileHandle(MoriartyClient::kInvalidFileHandle)
	{
	}

	Int32 m_iFileHandle;
	FileStat m_Stat;
};

/**
 * Opens a remote file and optionally retrieves its basic information.
 *
 * @param[in] filePath Remote file path to open
 * @param[in] eMode File mode to open the file with
 * @param[out] pOutStat Receives basic file information, if non-nullptr
 *
 * @return A file handle if successful, or kInvalidFileHandle on failure.
 */
MoriartyClient::FileHandle MoriartyClient::OpenFile(FilePath filePath, File::Mode eMode, FileStat *pOutStat)
{
	OpenFileData data;
	CallInProgress *pCallInfo = BeginRPC(kOpenFile, &data);
	if (pCallInfo == nullptr)
	{
		return kInvalidFileHandle;
	}

	{
		Lock lock(m_SendMutex);

		if (!m_pStream->Write8((UInt8)kOpenFile) ||
			!m_pStream->Write32(pCallInfo->m_uToken) ||
			!m_pStream->Write(filePath) ||
			!m_pStream->Write8((UInt8)eMode) ||
			!m_pStream->Flush())
		{
			return kInvalidFileHandle;
		}
	}

	if (WaitForResponse(pCallInfo) != kResultSuccess)
	{
		return kInvalidFileHandle;
	}

	// If we have a valid file path, refresh the cached
	// stat file structure.
	if (filePath.IsValid())
	{
		Lock lock(m_StatFileCacheMutex);
		SEOUL_VERIFY(m_tStatFileCache.Overwrite(filePath, data.m_Stat).Second);
	}

	if (pOutStat != nullptr)
	{
		*pOutStat = data.m_Stat;
	}

	return data.m_iFileHandle;
}

/**
 * Handler for the OpenFile RPC
 */
EResult MoriartyClient::OnOpenFile(void *pData)
{
	OpenFileData *pOpenFileData = (OpenFileData *)pData;

	UInt8 uStatus, uFlags;
	if (!m_pStream->Read8(uStatus) ||
		!m_pStream->Read32(pOpenFileData->m_iFileHandle) ||
		!m_pStream->Read8(uFlags) ||
		!m_pStream->Read64(pOpenFileData->m_Stat.m_uFileSize) ||
		!m_pStream->Read64(pOpenFileData->m_Stat.m_uModifiedTime) ||
		uStatus >= kMaxResult)
	{
		return kResultRPCFailed;
	}

	pOpenFileData->m_Stat.m_bIsDirectory = (uFlags != 0);
	return (EResult)uStatus;
}

/**
 * Closes a remote file.
 *
 * @param[in] file File to close
 *
 * @return true if successful or false if an error occurred (e.g. invalid
 *         handle).
 */
Bool MoriartyClient::CloseFile(FileHandle file)
{
	CallInProgress *pCallInfo = BeginRPC(kCloseFile, nullptr);
	if (pCallInfo == nullptr)
	{
		return false;
	}

	{
		Lock lock(m_SendMutex);

		if (!m_pStream->Write8((UInt8)kCloseFile) ||
			!m_pStream->Write32(pCallInfo->m_uToken) ||
			!m_pStream->Write32(file) ||
			!m_pStream->Flush())
		{
			return false;
		}
	}

	return (WaitForResponse(pCallInfo) == kResultSuccess);
}

/**
 * Handler for the CloseFile RPC
 */
EResult MoriartyClient::OnCloseFile(void *pData)
{
	UInt8 uStatus;
	if (!m_pStream->Read8(uStatus) ||
		uStatus >= kMaxResult)
	{
		return kResultRPCFailed;
	}

	return (EResult)uStatus;
}

/** Data used for the OpenFile RPC */
struct ReadFileData
{
	ReadFileData()
		: m_pOutBuffer(nullptr)
		, m_uBufferSize(0)
		, m_nBytesRead(-1)
	{
	}

	void *m_pOutBuffer;
	UInt64 m_uBufferSize;
	Int64 m_nBytesRead;
};

/**
 * Reads data from the file at the given offset.  The file must have been
 * opened in a readable mode.
 *
 * @param[in] file File to read from
 * @param[out] pBuffer Buffer to read the file data into
 * @param[in] uCount Maximum amount of data to read
 * @param[in] uOffset File offset to read from
 *
 * @return The amount of data read if successful, or -1 on failure.
 *         May return 0 if you try to read at/past EOF.
 */
Int64 MoriartyClient::ReadFile(FileHandle file, void *pBuffer, UInt64 uCount, UInt64 uOffset)
{
	SEOUL_ASSERT(uCount <= (UInt64)INT64_MAX && uCount <= SIZE_MAX);
	SEOUL_ASSERT(uOffset + uCount >= uOffset);

	ReadFileData data;
	data.m_pOutBuffer = pBuffer;
	data.m_uBufferSize = uCount;

	CallInProgress *pCallInfo = BeginRPC(kReadFile, &data);
	if (pCallInfo == nullptr)
	{
		return false;
	}

	{
		Lock lock(m_SendMutex);

		if (!m_pStream->Write8((UInt8)kReadFile) ||
			!m_pStream->Write32(pCallInfo->m_uToken) ||
			!m_pStream->Write32(file) ||
			!m_pStream->Write64(uCount) ||
			!m_pStream->Write64(uOffset) ||
			!m_pStream->Flush())
		{
			return -1;
		}
	}

	EResult eResult = WaitForResponse(pCallInfo);

	return (eResult == kResultSuccess ? data.m_nBytesRead : -1);
}

/**
 * Handler for the ReadFile RPC
 */
EResult MoriartyClient::OnReadFile(void *pData)
{
	ReadFileData *pReadFileData = (ReadFileData *)pData;

	UInt8 bCompressed;
	UInt8 uStatus;
	UInt64 uBytesRead;
	if (!m_pStream->Read8(bCompressed) ||
		!m_pStream->Read8(uStatus) ||
		!m_pStream->Read64(uBytesRead) ||
		uStatus >= kMaxResult ||
		uBytesRead > (UInt64)INT64_MAX ||
		uBytesRead > (UInt64)SIZE_MAX ||
		uBytesRead > pReadFileData->m_uBufferSize ||
		!m_pStream->ReadImmediate(pReadFileData->m_pOutBuffer, (SocketStream::SizeType)uBytesRead))
	{
		return kResultRPCFailed;
	}

	// If the data was sent compressed, uncompress it now.
	if (0 != bCompressed)
	{
		// Something went wrong, can't uncompressed data this large.
		if (uBytesRead > (UInt64)UIntMax)
		{
			return kResultRPCFailed;
		}

		// Uncompress the data.
		void* pUncompressedData = nullptr;
		UInt32 zUncompressedSize = 0;
		if (!LZ4Decompress(
			pReadFileData->m_pOutBuffer,
			(UInt32)uBytesRead,
			pUncompressedData,
			zUncompressedSize,
			MemoryBudgets::Network) ||
			(UInt64)zUncompressedSize > pReadFileData->m_uBufferSize)
		{
			MemoryManager::Deallocate(pUncompressedData);
			return kResultRPCFailed;
		}

		// Copy the uncompressed data to the output buffer, update the
		// bytes read value, and free the intermediate buffer.
		memcpy(pReadFileData->m_pOutBuffer, pUncompressedData, zUncompressedSize);
		uBytesRead = (UInt64)zUncompressedSize;
		MemoryManager::Deallocate(pUncompressedData);
	}

	pReadFileData->m_nBytesRead = (Int64)uBytesRead;

	return (EResult)uStatus;
}

/**
 * Writes data to the file at the given offset.  The file must have been opened
 * in a writable mode.
 *
 * @param[in] file File to write to
 * @param[in] pData Data to write
 * @param[in] uCount Number of bytes to write
 * @param[in] uOffset File offset to write at
 *
 * @return The amount of data written if successful, or -1 on failure.
 */
Int64 MoriartyClient::WriteFile(FileHandle file, const void *pData, UInt64 uCount, UInt64 uOffset)
{
	SEOUL_ASSERT(uCount <= (UInt64)INT64_MAX && uCount <= SIZE_MAX);
	SEOUL_ASSERT(uOffset + uCount >= uOffset);

	Int64 nBytesWritten = -1;
	CallInProgress *pCallInfo = BeginRPC(kWriteFile, &nBytesWritten);
	if (pCallInfo == nullptr)
	{
		return -1;
	}

	{
		Lock lock(m_SendMutex);

		if (!m_pStream->Write8((UInt8)kWriteFile) ||
			!m_pStream->Write32(pCallInfo->m_uToken) ||
			!m_pStream->Write32(file) ||
			!m_pStream->Write64(uCount) ||
			!m_pStream->Write64(uOffset) ||
			!m_pStream->WriteImmediate(pData, (SocketStream::SizeType)uCount))
		{
			return -1;
		}
	}

	if (WaitForResponse(pCallInfo) != kResultSuccess)
	{
		return -1;
	}

	return nBytesWritten;
}

/**
 * Handler for the WriteFile RPC
 */
EResult MoriartyClient::OnWriteFile(void *pData)
{
	UInt8 uStatus;
	UInt64 uBytesWritten;
	if (!m_pStream->Read8(uStatus) ||
		!m_pStream->Read64(uBytesWritten) ||
		uBytesWritten > (UInt64)INT64_MAX)
	{
		return kResultRPCFailed;
	}

	Int64 *pnBytesWritten = (Int64 *)pData;
	*pnBytesWritten = (Int64)uBytesWritten;

	return (EResult)uStatus;
}

/**
 * Sets a remote file's last modified time, in seconds since 1970-01-01 UTC
 *
 * @param[in] filePath Path of the file
 * @param[in] uModifiedTime New last modified time of the file, in seconds
 *              since 1970-01-01 UTC
 *
 * @return True if the file's last modified time was successfully updated, or
 *         false if an error occurred
 */
Bool MoriartyClient::SetFileModifiedTime(FilePath filePath, UInt64 uModifiedTime)
{
	CallInProgress *pCallInfo = BeginRPC(kSetFileModifiedTime, nullptr);
	if (pCallInfo == nullptr)
	{
		return false;
	}

	{
		Lock lock(m_SendMutex);

		if (!m_pStream->Write8((UInt8)kSetFileModifiedTime) ||
			!m_pStream->Write32(pCallInfo->m_uToken) ||
			!m_pStream->Write(filePath) ||
			!m_pStream->Write64(uModifiedTime) ||
			!m_pStream->Flush())
		{
			return false;
		}
	}

	return (WaitForResponse(pCallInfo) == kResultSuccess);
}

/**
 * Handler for the SetFileModifiedTime RPC
 */
EResult MoriartyClient::OnSetFileModifiedTime(void *pData)
{
	UInt8 uStatus;
	if (!m_pStream->Read8(uStatus) ||
		uStatus >= kMaxResult)
	{
		return kResultRPCFailed;
	}

	return (EResult)uStatus;
}

struct MoriartyClientGetDirectoryListingResults
{
	MoriartyClientGetDirectoryListingResults(FilePath dirPath, Vector<String>& rvResults)
		: m_DirPath(dirPath)
		, m_rvResults(rvResults)
	{
	}

	FilePath m_DirPath;
	Vector<String>& m_rvResults;
};

/**
 * Gets the list of files and subdirectories in a given directory, optionally
 * recursively
 *
 * @param[in] dirPath Path of the directory to enumerate
 * @param[out] rvResults Receives the contents of the directory on success
 * @param[in] bIncludeDirectoriesInResults True if subdirectories should also
 *              be included in the results, or false if only regular files
 *              should be included
 * @param[in] bRecursive True if a recursive directory listing should be
 *              performed, or false to only include the direct contents of the
 *              directory
 * @param[in] sFileExtension If non-empty, a filter suffix (such as ".json")
 *              to only include results which end in that suffix
 *
 * @return True if the directory was successfully enumerated, or false if an
 *         error occurred
 */
Bool MoriartyClient::GetDirectoryListing(FilePath dirPath,
										 Vector<String>& rvResults,
										 Bool bIncludeDirectoriesInResults,
										 Bool bRecursive,
										 const String& sFileExtension)
{
	MoriartyClientGetDirectoryListingResults results(dirPath, rvResults);
	CallInProgress *pCallInfo = BeginRPC(kGetDirectoryListing, &results);
	if (pCallInfo == nullptr)
	{
		return false;
	}

	// Set up flags byte
	UInt8 uFlags = 0;
	if (bIncludeDirectoriesInResults)
	{
		uFlags |= kFlag_GetDirectoryListing_IncludeSubdirectories;
	}
	if (bRecursive)
	{
		uFlags |= kFlag_GetDirectoryListing_Recursive;
	}

	{
		Lock lock(m_SendMutex);

		if (!m_pStream->Write8((UInt8)kGetDirectoryListing) ||
			!m_pStream->Write32(pCallInfo->m_uToken) ||
			!m_pStream->Write(dirPath) ||
			!m_pStream->Write8(uFlags) ||
			!m_pStream->Write(sFileExtension) ||
			!m_pStream->Flush())
		{
			return false;
		}
	}

	return (WaitForResponse(pCallInfo) == kResultSuccess);
}

/**
 * Handler for the GetDirectoryListing RPC
 */
EResult MoriartyClient::OnGetDirectoryListing(void *pData)
{
	MoriartyClientGetDirectoryListingResults* pResults = (MoriartyClientGetDirectoryListingResults*)pData;

	UInt8 uStatus;
	if (!m_pStream->Read8(uStatus) ||
		!m_pStream->Read(pResults->m_rvResults) ||
		uStatus >= kMaxResult)
	{
		return kResultRPCFailed;
	}

	// If the directory listing failed, we should not have any directory
	// contents
	SEOUL_ASSERT(uStatus == kResultSuccess || pResults->m_rvResults.GetSize() == 0);

	// Fixup paths.
	String sAbsoluteBasePath = pResults->m_DirPath.GetAbsoluteFilename();
	UInt32 const nResults = pResults->m_rvResults.GetSize();
	for (UInt32 i = 0; i < nResults; ++i)
	{
		pResults->m_rvResults[i] = Path::Combine(sAbsoluteBasePath, pResults->m_rvResults[i]);
	}

	return (EResult)uStatus;
}

/**
 * Cooks a remote file.
 *
 * @param[in] filePath Remote file to cook
 * @param[in] bCheckTimeStamp If true, the file is only cooked if it is
 *              out-of-date (i.e. the source file is newer than the cooked
 *              file)
 * @param[out] pOutResult On a successful RPC, receives the result of the cook,
 *              as a CookManager::CookResult value
 *
 * @return True if the RPC was successful, or false if the RPC failed
 */
Bool MoriartyClient::CookFile(FilePath filePath, Bool bCheckTimestamp, Int32 *pOutResult)
{
	CallInProgress *pCallInfo = BeginRPC(kCookFile, pOutResult);
	if (pCallInfo == nullptr)
	{
		return false;
	}

	UInt8 uFlags = (bCheckTimestamp ? kFlag_CookFile_CheckTimestamp : 0);

	{
		Lock lock(m_SendMutex);

		if (!m_pStream->Write8((UInt8)kCookFile) ||
			!m_pStream->Write32(pCallInfo->m_uToken) ||
			!m_pStream->Write(filePath) ||
			!m_pStream->Write8(uFlags) ||
			!m_pStream->Flush())
		{
			return false;
		}
	}

	return (WaitForResponse(pCallInfo) == kResultSuccess);
}

/**
 * Attempt to copy a file, from -> to.
 *
 * @return true on success.
 */
Bool MoriartyClient::Copy(FilePath from, FilePath to, Bool bAllowOverwrite /* = false */)
{
	CallInProgress *pCallInfo = BeginRPC(kCopy, nullptr);
	if (pCallInfo == nullptr)
	{
		return false;
	}

	{
		Lock lock(m_SendMutex);

		if (!m_pStream->Write8((UInt8)kCopy) ||
			!m_pStream->Write32(pCallInfo->m_uToken) ||
			!m_pStream->Write(from) ||
			!m_pStream->Write(to) ||
			!m_pStream->Write8((UInt8)(bAllowOverwrite ? 1 : 0)) ||
			!m_pStream->Flush())
		{
			return false;
		}
	}

	return (WaitForResponse(pCallInfo) == kResultSuccess);
}

/**
 * Attempt to create a directory and its parents.
 *
 * @return true on success.
 */
Bool MoriartyClient::CreateDirPath(FilePath dirPath)
{
	CallInProgress *pCallInfo = BeginRPC(kCreateDirPath, nullptr);
	if (pCallInfo == nullptr)
	{
		return false;
	}

	{
		Lock lock(m_SendMutex);

		if (!m_pStream->Write8((UInt8)kCreateDirPath) ||
			!m_pStream->Write32(pCallInfo->m_uToken) ||
			!m_pStream->Write(dirPath) ||
			!m_pStream->Flush())
		{
			return false;
		}
	}

	return (WaitForResponse(pCallInfo) == kResultSuccess);
}

/**
 * Delete a file.
 *
 * @return true on success.
 */
Bool MoriartyClient::Delete(FilePath filePath)
{
	CallInProgress *pCallInfo = BeginRPC(kDelete, nullptr);
	if (pCallInfo == nullptr)
	{
		return false;
	}

	{
		Lock lock(m_SendMutex);

		if (!m_pStream->Write8((UInt8)kDelete) ||
			!m_pStream->Write32(pCallInfo->m_uToken) ||
			!m_pStream->Write(filePath) ||
			!m_pStream->Flush())
		{
			return false;
		}
	}

	return (WaitForResponse(pCallInfo) == kResultSuccess);
}

/**
 * Delete a directory.
 *
 * @return true on success.
 */
Bool MoriartyClient::DeleteDirectory(FilePath dirPath, Bool bRecursive)
{
	CallInProgress *pCallInfo = BeginRPC(kDeleteDirectory, nullptr);
	if (pCallInfo == nullptr)
	{
		return false;
	}

	{
		Lock lock(m_SendMutex);

		if (!m_pStream->Write8((UInt8)kDeleteDirectory) ||
			!m_pStream->Write32(pCallInfo->m_uToken) ||
			!m_pStream->Write(dirPath) ||
			!m_pStream->Write8((UInt8)(bRecursive ? 1 : 0)) ||
			!m_pStream->Flush())
		{
			return false;
		}
	}

	return (WaitForResponse(pCallInfo) == kResultSuccess);
}

/**
 * Attempt to rename a file or directory, from -> to.
 *
 * @return true on success.
 */
Bool MoriartyClient::Rename(FilePath from, FilePath to)
{
	CallInProgress *pCallInfo = BeginRPC(kRename, nullptr);
	if (pCallInfo == nullptr)
	{
		return false;
	}

	{
		Lock lock(m_SendMutex);

		if (!m_pStream->Write8((UInt8)kRename) ||
			!m_pStream->Write32(pCallInfo->m_uToken) ||
			!m_pStream->Write(from) ||
			!m_pStream->Write(to) ||
			!m_pStream->Flush())
		{
			return false;
		}
	}

	return (WaitForResponse(pCallInfo) == kResultSuccess);
}

/**
 * Attemp to update the read/write status of a file.
 *
 * @return true on success.
 */
Bool MoriartyClient::SetReadOnlyBit(FilePath filePath, Bool bReadOnlyBit)
{
	CallInProgress *pCallInfo = BeginRPC(kSetReadOnlyBit, nullptr);
	if (pCallInfo == nullptr)
	{
		return false;
	}

	{
		Lock lock(m_SendMutex);

		if (!m_pStream->Write8((UInt8)kSetReadOnlyBit) ||
			!m_pStream->Write32(pCallInfo->m_uToken) ||
			!m_pStream->Write(filePath) ||
			!m_pStream->Write8((UInt8)(bReadOnlyBit ? 1 : 0)) ||
			!m_pStream->Flush())
		{
			return false;
		}
	}

	return (WaitForResponse(pCallInfo) == kResultSuccess);
}

/**
 * Handler for the CookFile RPC
 */
EResult MoriartyClient::OnCookFile(void *pData)
{
	Int32 *pResult = (Int32 *)pData;

	FilePath filePath;
	FileStat fileStat;
	fileStat.m_bIsDirectory = false;
	UInt8 uStatus;
	if (!m_pStream->Read(filePath) ||
		!m_pStream->Read64(fileStat.m_uFileSize) ||
		!m_pStream->Read64(fileStat.m_uModifiedTime) ||
		!m_pStream->Read8(uStatus) ||
		!m_pStream->Read32(*pResult) ||
		uStatus >= kMaxResult)
	{
		return kResultRPCFailed;
	}

	// If we have a valid file path, refresh the cached
	// stat file structure.
	if (filePath.IsValid())
	{
		Lock lock(m_StatFileCacheMutex);
		SEOUL_VERIFY(m_tStatFileCache.Overwrite(filePath, fileStat).Second);
	}

	return (EResult)uStatus;
}

/**
 * Set the keyboard key event handler.
 *
 * @param[in] handler Function that will be invoked when a keyboard key event
 * is received if non-nullptr.
 */
void MoriartyClient::RegisterKeyboardKeyEventHandler(KeyboardKeyEventHandler handler)
{
	m_KeyboardKeyEventHandler = handler;
}

/**
 * Handler for the KeyboardKeyEvent RPC
 */
EResult MoriartyClient::OnKeyboardKeyEvent(void *pData)
{
	UInt32 uVirtualKeyCode;
	UInt8 uKeyEventType;
	if (!m_pStream->Read32(uVirtualKeyCode) ||
		!m_pStream->Read8(uKeyEventType))
	{
		return kResultRPCFailed;
	}

	// If a keyboard key event handler is registered, invoke
	// it with the received key event.
	if (m_KeyboardKeyEventHandler != nullptr)
	{
		KeyEvent keyEvent;
		keyEvent.m_uVirtualKeyCode = uVirtualKeyCode;
		keyEvent.m_eKeyEventType = (MoriartyRPC::EKeyEventType)uKeyEventType;

		m_KeyboardKeyEventHandler(keyEvent);
	}

	return kResultSuccess;
}

/**
 * Set the content change event handler.
 *
 * @param[in] handler Function that will be invoked when a content change event
 * is received if non-nullptr.
 */
void MoriartyClient::RegisterContentChangeEventHandler(ContentChangeEventHandler handler)
{
	m_ContentChangeEventHandler = handler;
}

/**
 * Handler for the ContentChangeEvent RPC
 */
EResult MoriartyClient::OnContentChangeEvent(void *pData)
{
	FilePath oldFilePath;
	FilePath newFilePath;
	FileStat fileStat;
	fileStat.m_bIsDirectory = false;
	UInt8 uFileChangeEvent;
	if (!m_pStream->Read(oldFilePath) ||
		!m_pStream->Read(newFilePath) ||
		!m_pStream->Read64(fileStat.m_uFileSize) ||
		!m_pStream->Read64(fileStat.m_uModifiedTime) ||
		!m_pStream->Read8(uFileChangeEvent))
	{
		return kResultRPCFailed;
	}

	// If a console command event handler is registered, invoke
	// it with the received command.
	if (m_ContentChangeEventHandler != nullptr)
	{
		m_ContentChangeEventHandler(oldFilePath, newFilePath, (FileChangeNotifier::FileEvent)uFileChangeEvent);
	}

	// If we have a valid file path, refresh the cached
	// stat file structure.
	if (newFilePath.IsValid())
	{
		Lock lock(m_StatFileCacheMutex);
		SEOUL_VERIFY(m_tStatFileCache.Overwrite(newFilePath, fileStat).Second);
	}

	return kResultSuccess;
}

/**
 * Set the keyboard char event handler.
 *
 * @param[in] handler Function that will be invoked when a content change event
 * is received if non-nullptr.
 */
void MoriartyClient::RegisterKeyboardCharEventHandler(KeyboardCharEventHandler handler)
{
	m_KeyboardCharEventHandler = handler;
}

/**
 * Handler for the KeyboardCharEvent RPC
 */
EResult MoriartyClient::OnKeyboardCharEvent(void *pData)
{
	UInt32 uUnicodeCharacter;
	if (!m_pStream->Read32(uUnicodeCharacter))
	{
		return kResultRPCFailed;
	}

	// If a keyboard char event handler is registered, invoke
	// it with the received character.
	if (m_KeyboardCharEventHandler != nullptr)
	{
		m_KeyboardCharEventHandler((UniChar)uUnicodeCharacter);
	}

	return kResultSuccess;
}

/** Internal utility structure used to unpack cache refresh data. */
struct StatFileCacheRefreshEntry
{
	StatFileCacheRefreshEntry()
		: m_uFileSize(0)
		, m_uModifiedTime(0)
		, m_FilePath()
	{
	}

	/** File size, in bytes */
	UInt64 m_uFileSize;

	/** File's last modified time, in seconds since 1970-01-01 UTC */
	UInt64 m_uModifiedTime;

	/** FilePath to the file. */
	FilePath m_FilePath;
};

// TODO: These are essentially cut and paste from equivalent functions in SocketStream.

/** Read a UInt32 net value from an arbitrary byte buffer. */
inline static Bool Read(UInt8 const*& rp, UInt8 const* const pEnd, UInt32& ru)
{
	if (rp + sizeof(UInt32) > pEnd)
	{
		return false;
	}

	memcpy(&ru, rp, sizeof(UInt32));
	rp += sizeof(UInt32);

#if SEOUL_LITTLE_ENDIAN
	ru = EndianSwap32(ru);
#endif

	return true;
}

/** Read a UInt64 net value from an arbitrary byte buffer. */
inline static Bool Read(UInt8 const*& rp, UInt8 const* const pEnd, UInt64& ru)
{
	if (rp + sizeof(UInt64) > pEnd)
	{
		return false;
	}

	memcpy(&ru, rp, sizeof(UInt64));
	rp += sizeof(UInt64);

#if SEOUL_LITTLE_ENDIAN
	ru = EndianSwap64(ru);
#endif

	return true;
}

/** Read a String net value from an arbitrary byte buffer. */
inline static Bool Read(UInt8 const*& rp, UInt8 const* const pEnd, String& rsString)
{
	UInt32 uLength;
	if (!Read(rp, pEnd, uLength))
	{
		return false;
	}

	if (uLength > 0x01000000)
	{
		return false;
	}

	if (uLength == 0)
	{
		rsString.Clear();
		return true;
	}

	rsString.Assign((Byte const*)rp, uLength);
	rp += uLength;
	return true;
}

/** Read a FilePath net value from an arbitrary byte buffer. */
inline static Bool Read(UInt8 const*& rp, UInt8 const* const pEnd, FilePath& rFilePath)
{
	if (rp + 16 > pEnd)
	{
		return false;
	}

	UInt8 const uDirectory = *rp;
	++rp;
	UInt8 const uType = *rp;
	++rp;

	String sRelativePathWithoutExtension;
	if (!Read(rp, pEnd, sRelativePathWithoutExtension))
	{
		return false;
	}

	// Normalize slashes - arbitrary choose the "\" as the "net" version.
	sRelativePathWithoutExtension =
		sRelativePathWithoutExtension.ReplaceAll("\\", Path::DirectorySeparatorChar());

	rFilePath.SetDirectory((GameDirectory)uDirectory);
	rFilePath.SetType((FileType)uType);
	rFilePath.SetRelativeFilenameWithoutExtension(FilePathRelativeFilename(sRelativePathWithoutExtension));
	return true;
}

/** Unpack already uncomrpessed StatFileCache data. */
inline static Bool DecodeStatFileCacheRefreshUncompressedData(
	void const* pData,
	UInt32 uDataSizeInBytes,
	Vector<StatFileCacheRefreshEntry, MemoryBudgets::Network>& rvEntries)
{
	UInt8 const* p = (UInt8 const*)pData;
	UInt8 const* const pEnd = (p + uDataSizeInBytes);

	Vector<StatFileCacheRefreshEntry, MemoryBudgets::Network> vEntries;
	while (p < pEnd)
	{
		StatFileCacheRefreshEntry entry;
		if (!Read(p, pEnd, entry.m_FilePath))
		{
			return false;
		}

		if (!Read(p, pEnd, entry.m_uFileSize))
		{
			return false;
		}

		if (!Read(p, pEnd, entry.m_uModifiedTime))
		{
			return false;
		}

		vEntries.PushBack(entry);
	}

	rvEntries.Swap(vEntries);
	return true;
}

EResult MoriartyClient::OnStatFileCacheRefreshEvent(void* /*pData*/)
{
	// Read the size of the compressed data.
	UInt32 uDataSize;
	if (!m_pStream->Read32(uDataSize))
	{
		return kResultRPCFailed;
	}

	// If we have data, uncompressed it.
	if (uDataSize > 0)
	{
		// Read the compressed data.
		Byte* pData = (Byte*)MemoryManager::AllocateAligned(uDataSize, MemoryBudgets::Network, kLZ4MinimumAlignment);
		if (!m_pStream->ReadImmediate(pData, (SocketStream::SizeType)uDataSize))
		{
			MemoryManager::Deallocate(pData);
			return kResultRPCFailed;
		}

		// Uncompress the data.
		void* pUncompressedData = nullptr;
		UInt32 zUncompressedSize = 0;
		if (!LZ4Decompress(
			pData,
			uDataSize,
			pUncompressedData,
			zUncompressedSize,
			MemoryBudgets::Network))
		{
			MemoryManager::Deallocate(pData);
			return kResultRPCFailed;
		}

		// Release the uncompressed data.
		MemoryManager::Deallocate(pData);

		// Unpack the entries.
		Vector<StatFileCacheRefreshEntry, MemoryBudgets::Network> vEntries;
		if (!DecodeStatFileCacheRefreshUncompressedData(pUncompressedData, zUncompressedSize, vEntries))
		{
			MemoryManager::Deallocate(pUncompressedData);
			return kResultRPCFailed;
		}

		// Release the compressed data.
		MemoryManager::Deallocate(pUncompressedData);

		// Process the entries.
		UInt32 const uEntries = vEntries.GetSize();
		{
			Lock lock(m_StatFileCacheMutex);
			for (UInt32 i = 0; i < uEntries; ++i)
			{
				StatFileCacheRefreshEntry const entry = vEntries[i];
				FilePath const filePath = entry.m_FilePath;
				FileStat fileStat;
				fileStat.m_bIsDirectory = false;
				fileStat.m_uFileSize = entry.m_uFileSize;
				fileStat.m_uModifiedTime = entry.m_uModifiedTime;
				if (filePath.IsValid())
				{
					SEOUL_VERIFY(m_tStatFileCache.Overwrite(filePath, fileStat).Second);
				}
			}
		}
	}

	return kResultSuccess;
}

MoriartyRPC::EResult MoriartyClient::OnCreateDirPath(void *pData)
{
	UInt8 uStatus;
	if (!m_pStream->Read8(uStatus) ||
		uStatus >= kMaxResult)
	{
		return kResultRPCFailed;
	}

	return (EResult)uStatus;
}

MoriartyRPC::EResult MoriartyClient::OnDelete(void *pData)
{
	UInt8 uStatus;
	if (!m_pStream->Read8(uStatus) ||
		uStatus >= kMaxResult)
	{
		return kResultRPCFailed;
	}

	return (EResult)uStatus;
}

MoriartyRPC::EResult MoriartyClient::OnRename(void *pData)
{
	UInt8 uStatus;
	if (!m_pStream->Read8(uStatus) ||
		uStatus >= kMaxResult)
	{
		return kResultRPCFailed;
	}

	return (EResult)uStatus;
}

MoriartyRPC::EResult MoriartyClient::OnSetReadOnlyBit(void *pData)
{
	UInt8 uStatus;
	if (!m_pStream->Read8(uStatus) ||
		uStatus >= kMaxResult)
	{
		return kResultRPCFailed;
	}

	return (EResult)uStatus;
}

MoriartyRPC::EResult MoriartyClient::OnCopy(void *pData)
{
	UInt8 uStatus;
	if (!m_pStream->Read8(uStatus) ||
		uStatus >= kMaxResult)
	{
		return kResultRPCFailed;
	}

	return (EResult)uStatus;
}

MoriartyRPC::EResult MoriartyClient::OnDeleteDirectory(void *pData)
{
	UInt8 uStatus;
	if (!m_pStream->Read8(uStatus) ||
		uStatus >= kMaxResult)
	{
		return kResultRPCFailed;
	}

	return (EResult)uStatus;
}

} // namespace Seoul

#endif  // SEOUL_WITH_MORIARTY
