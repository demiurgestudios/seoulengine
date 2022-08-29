/**
 * \file MoriartyClient.h
 * \brief Client class for interacting with the Moriarty server tool
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef MORIARTY_CLIENT_H
#define MORIARTY_CLIENT_H

#include "StandardPlatformIncludes.h"

#if SEOUL_WITH_MORIARTY

#include "Atomic32.h"
#include "HashTable.h"
#include "FileChangeNotifier.h"
#include "List.h"
#include "MoriartyRPC.h"
#include "Mutex.h"
#include "ScopedPtr.h"
#include "SeoulFile.h"
#include "SeoulSignal.h"
#include "Singleton.h"
#include "StreamBuffer.h"
#include "Thread.h"
#include "Vector.h"

namespace Seoul
{

// Forward declarations
class Socket;
class SocketStream;

/** TCP port used for Moriarty communication */
const UInt16 kMoriartyPort = 22180;

class MoriartyClient : public Singleton<MoriartyClient>
{
public:
	SEOUL_DELEGATE_TARGET(MoriartyClient);

	/** Moriarty file handle data type */
	typedef Int32 FileHandle;

	/** Invalid file handle constant */
	static const FileHandle kInvalidFileHandle = -1;

	MoriartyClient();
	~MoriartyClient();

	// Synchronously connects to the given Moriarty server -- this may block
	// for a non-trivial amount of time in bad network situations.  Must be
	// called before calling any other functions, or they will fail.  Returns
	// true on success, or false on failure.
	Bool Connect(const String& sServerHostname);

	// Disconnects from the server, which implicitly closes all currently open
	// remote files and cancels any pending asynchronous I/O.  This is
	// automatically called from the destructor.
	void Disconnect();

	// Tests if we are currently connected to a server
	Bool IsConnected() const;

	// Log messages to the Moriarty server. Intended
	// to echo all messages passed to Logger::LogMessage().
	Bool LogMessage(const String& sMessage);

	// Gets basic information about a remote file at a given file path.
	// Returns true if successful.
	Bool StatFile(FilePath filePath, FileStat *pOutStat);

	// Opens a remote file and optionally retrieves its basic information.
	// Returns a file handle if successful, or kInvalidFileHandle on failure.
	FileHandle OpenFile(FilePath filePath, File::Mode eMode, FileStat *pOutStat);

	// Closes a file.  Returns true if successful or false if an error occurred
	// (e.g. invalid handle).
	Bool CloseFile(FileHandle file);

	// Reads data from the file at the given offset.  Returns the amount of
	// data read or a negative error code.
	Int64 ReadFile(FileHandle file, void *pBuffer, UInt64 uCount, UInt64 uOffset);

	// Writes data to the file at the given offset.  Returns the amount of
	// data written or a negative error code.
	Int64 WriteFile(FileHandle file, const void *pData, UInt64 uCount, UInt64 uOffset);

	// Sets a remote file's last modified time, in seconds since 1970-01-01 UTC
	Bool SetFileModifiedTime(FilePath filePath, UInt64 uModifiedTime);

	// Gets the list of files and subdirectories in a given directory,
	// optionally recursively
	Bool GetDirectoryListing(FilePath dirPath,
							 Vector<String>& rvResults,
							 Bool bIncludeDirectoriesInResults,
							 Bool bRecursive,
							 const String& sFileExtension);

	// Cooks a remote file.  If the RPC was successful, stores the result (a
	// CookManager::CookResult value) into pOutResult and returns true.
	Bool CookFile(FilePath filePath, Bool bCheckTimestamp, Int32 *pOutResult);

	// Attempt to copy a file, from -> to. Return true on success.
	Bool Copy(FilePath from, FilePath to, Bool bAllowOverwrite);

	// Attempt to create a directory and its parents. Return true on success.
	Bool CreateDirPath(FilePath dirPath);

	// Delete a file. Return true on success.
	Bool Delete(FilePath filePath);

	// Delete a directory. Return true on success.
	Bool DeleteDirectory(FilePath dirPath, Bool bRecursive);

	// Attempt to rename a file or directory, from -> to. Return true on success.
	Bool Rename(FilePath from, FilePath to);

	// Attemp to update the read/write status of a file. Return true on success.
	Bool SetReadOnlyBit(FilePath filePath, Bool bReadOnlyBit);

	/** KeyboardKeyEvent callback type */
	typedef void (*KeyboardKeyEventHandler)(const MoriartyRPC::KeyEvent& keyEvent);

	// Register/unregister the handler that will be invoked when keyboard key
	// events are received.
	void RegisterKeyboardKeyEventHandler(KeyboardKeyEventHandler handler);

	/** ContentChangeEvent callback type */
	typedef void (*ContentChangeEventHandler)(
		FilePath oldFilePath,
		FilePath newFilePath,
		FileChangeNotifier::FileEvent eEvent);

	// Register/unregister the handler that will be invoked when a content
	// change event is received.
	void RegisterContentChangeEventHandler(ContentChangeEventHandler handler);

	/** KeyboardCharEventHandler callback type */
	typedef void (*KeyboardCharEventHandler)(UniChar character);

	// Reigster/unregister the handler that will be invoked when keyboard char
	// events are received.
	void RegisterKeyboardCharEventHandler(KeyboardCharEventHandler handler);

private:
	// Thread procedure for running the RPC receive loop
	Int ReceiveLoop(const Thread& thread);

	// Tries to handle an RPC received on the socket
	Bool HandleRPC(UInt8 uRPCIndex, UInt32 uResponseToken);

	/** Info about an RPC currently in progress */
	struct CallInProgress
	{
		CallInProgress(MoriartyRPC::ERPC eRPC, UInt32 uToken, void *pData)
			: m_eRPC(eRPC)
			, m_uToken(uToken)
			, m_pData(pData)
			, m_eResult(MoriartyRPC::kResultRPCFailed)
		{
		}

		/** RPC we're waiting on */
		MoriartyRPC::ERPC m_eRPC;

		/** Token we're waiting on */
		UInt32 m_uToken;

		/** RPC-specific data */
		void *m_pData;

		/** Event being waited on by the calling thread */
		Signal m_Signal;

		/** Result code for the call */
		MoriartyRPC::EResult m_eResult;
	};

	// Sets up internal data structures for an RPC that is beginning
	CallInProgress *BeginRPC(MoriartyRPC::ERPC eRPC, void *pData);

	// Waits for a response to the given RPC in progress
	MoriartyRPC::EResult WaitForResponse(CallInProgress *pCall);

	// Cancels all RPCs currently in progress
	void CancelCallsInProgress();

	/** RPC handler function type */
	typedef MoriartyRPC::EResult (MoriartyClient::*RPCHandler)(void *pData);

	// Registers an RPC handler function
	void RegisterRPCHandler(MoriartyRPC::ERPC eRPC, RPCHandler pHandler, Bool bIsResponse);

	// RPC Handlers
	MoriartyRPC::EResult OnStatFile(void *pData);
	MoriartyRPC::EResult OnOpenFile(void *pData);
	MoriartyRPC::EResult OnCloseFile(void *pData);
	MoriartyRPC::EResult OnReadFile(void *pData);
	MoriartyRPC::EResult OnWriteFile(void *pData);
	MoriartyRPC::EResult OnSetFileModifiedTime(void *pData);
	MoriartyRPC::EResult OnGetDirectoryListing(void *pData);
	MoriartyRPC::EResult OnCookFile(void *pData);
	MoriartyRPC::EResult OnKeyboardKeyEvent(void *pData);
	MoriartyRPC::EResult OnContentChangeEvent(void *pData);
	MoriartyRPC::EResult OnKeyboardCharEvent(void *pData);
	MoriartyRPC::EResult OnStatFileCacheRefreshEvent(void *pData);
	MoriartyRPC::EResult OnCreateDirPath(void *pData);
	MoriartyRPC::EResult OnDelete(void *pData);
	MoriartyRPC::EResult OnRename(void *pData);
	MoriartyRPC::EResult OnSetReadOnlyBit(void *pData);
	MoriartyRPC::EResult OnCopy(void *pData);
	MoriartyRPC::EResult OnDeleteDirectory(void *pData);

private:
	/** TCP socket for communicating with the Moriarty server */
	ScopedPtr<Socket> m_pSocket;

	/**
	 * Mutex used to synchronize Socket::Close() calls and
	 * to synchronize the connection flow in the face of
	 * a connect cancellation.
	 */
	ScopedPtr<Mutex> m_pSocketConnectionMutex;

	/** Socket stream for processing socket data */
	ScopedPtr<SocketStream> m_pStream;

	/**
	 * Flag indicating that we're trying to shut down, so further RPCs should
	 * fail
	 */
	Atomic32Value<Bool> m_bShuttingDown;

	/** Flag indicating if the startup handshake has completed */
	Atomic32Value<Bool> m_bHandshakeCompleted;

	/**
	 * Flag indicates connection scope.
	 * Used to synchronize a cancellation against a pending connection.
	 */
	Atomic32Value<Bool> m_bConnecting;

	/** Thread for handling receives and dispatching callbacks */
	ScopedPtr<Thread> m_pReceiveThread;

	/** Thread ID of the receive thread  */
	ThreadId m_ReceiveThreadID;

	/** Mutex for serializing writes to the socket */
	Mutex m_SendMutex;

	/** Next RPC token to be used */
	Atomic32 m_NextToken;

	/** Array of RPC handler functions */
	RPCHandler m_aRPCHandlers[256];

	/** List of RPCs currently in progress */
	List<CallInProgress*> m_CallsInProgress;

	/** Mutex controlling access to m_CallsInProgress */
	Mutex m_CallsInProgressMutex;

	/** Handler that will be invoked with any key events, if registered */
	KeyboardKeyEventHandler m_KeyboardKeyEventHandler;

	/** Handler that will be invoked with any content change events, if registered */
	ContentChangeEventHandler m_ContentChangeEventHandler;

	/** Handler that will be invoked with any char events, if registered */
	KeyboardCharEventHandler m_KeyboardCharEventHandler;

	/** Mutex used to protect accesses to m_tStatFileCache */
	Mutex m_StatFileCacheMutex;

	/** Table used to cache stat file responses */
	typedef HashTable<FilePath, FileStat, MemoryBudgets::Io> StatFileCache;
	StatFileCache m_tStatFileCache;

	/** Utility for protecting a connection block. */
	struct ConnectingScope SEOUL_SEALED
	{
		ConnectingScope(MoriartyClient& r)
			: m_r(r)
		{
			// Lock the mutex and set initializing to true.
			m_r.m_pSocketConnectionMutex->Lock();
			m_r.m_bConnecting = true;
		}

		~ConnectingScope()
		{
			// Unset m_bInitializing and unlock the mutex.
			m_r.m_bConnecting = false;
			m_r.m_pSocketConnectionMutex->Unlock();
		}

		MoriartyClient& m_r;
	}; // struct ConnectingScope
}; // class MoriartyClient

} // namespace Seoul

#endif  // SEOUL_WITH_MORIARTY

#endif  // include guard
