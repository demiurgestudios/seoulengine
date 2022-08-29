/**
 * \file MoriartyRPC.h
 * \brief Definitions of RPC calls used by the MoriartyClient.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef MORIARTY_RPC_H
#define MORIARTY_RPC_H

#include "SeoulTypes.h"

namespace Seoul
{

namespace MoriartyRPC
{

/**
 * Moriarty protocol version -- increment this every time you make a
 * non-backwards-compatible change
 */
const UInt32 kProtocolVersion = 0;

/**
 * Magic constant sent after initiating a connection to avoid other applications
 * accidentally connecting to a Moriarty server and trying to use some other
 * protocol.
 */
const UInt32 kConnectMagic = 0x0DDD15C5;

/**
 * Magic constant sent back from the server to the client after a connection is
 * initiated.
 */
const UInt32 kConnectResponseMagic = 0xABADD00D;

/** Flag indicating that a message is an RPC response */
const UInt8 kResponseFlag = 0x80;

/** Enumeration of all RPC types */
enum ERPC
{
	kLogMessage,
	kStatFile,
	kOpenFile,
	kCloseFile,
	kReadFile,
	kWriteFile,
	kSetFileModifiedTime,
	kGetDirectoryListing,
	kCookFile,
	kKeyboardKeyEvent,
	kContentChangeEvent,
	kKeyboardCharEvent,
	kStatFileCacheRefreshEvent,
	kCreateDirPath,
	kDelete,
	kRename,
	kSetReadOnlyBit,
	kCopy,
	kDeleteDirectory,

	kRPCCount  //< Unused, total number of RPCs
};

/** Result codes and error numbers for RPCs */
enum EResult
{
	kResultSuccess,   //< RPC succeeded
	kResultRPCFailed, //< RPC failed to complete (e.g. socket was unexpectedly closed)
	kResultCanceled,  //< RPC was canceled

	// For all of the following codes, the RPC was completed but failed on the
	// server for some other reason
	kResultGenericFailure,    //< Unspecified failure
	kResultFileNotFound,      //< File not found (ENOENT)
	kResultAccessDenied,      //< Access denied (EACCES)
	kResultInvalidFileHandle, //< Invalid file handle (EBADF)

	kMaxResult  //< Unused, total number of result codes
};

/** Types of key events passed via RPC */
enum EKeyEventType
{
	kKeyPressed,
	kKeyReleased,
	kKeyAllReleased,	// Passed when the keyboard loses focus and all keys should be considered released.
};

/** Structure containing a key event received over RPC */
struct KeyEvent
{
	/** A Win32 VK_* style virtual key code. */
	UInt32 m_uVirtualKeyCode;

	/**
	 * Type of event - either KeyPressed, KeyReleased, or AllReleased,
	 * which indicates that the keyboard has lost focus.
	 */
	EKeyEventType m_eKeyEventType;
};

// RPC-specific constants

/** kStatFile flags */
const UInt8 kFlag_StatFile_Directory = 0x01;

/** kGetDirectoryListing flags */
const UInt8 kFlag_GetDirectoryListing_IncludeSubdirectories = 0x01;
const UInt8 kFlag_GetDirectoryListing_Recursive             = 0x02;

/** kCookFile flags */
const UInt8 kFlag_CookFile_CheckTimestamp = 0x01;

}  // namespace MoriartyRPC

} // namespace Seoul

#endif // include guard
