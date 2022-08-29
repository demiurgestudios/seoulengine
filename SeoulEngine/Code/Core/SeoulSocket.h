/**
 * \file SeoulSocket.h
 * \brief Platform-independent and protocol-independent socket class
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_SOCKET_H
#define SEOUL_SOCKET_H

#include "Atomic32.h"
#include "Mutex.h"
#include "ScopedPtr.h"
#include "SeoulString.h"
#include "SeoulTime.h"

#if SEOUL_PLATFORM_WINDOWS
#include "Platform.h"
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#endif

struct addrinfo;

namespace Seoul
{

// Platform-dependent socket handle type
#if SEOUL_PLATFORM_WINDOWS
typedef SOCKET SocketHandle;
const SocketHandle kInvalidSocketHandle = INVALID_SOCKET;
#else
typedef int SocketHandle;
const SocketHandle kInvalidSocketHandle = (SocketHandle)-1;
#endif

// FIXME: Figure out the correct platform-specific values
/** Maximum transfer unit (MTU) for the current platform */
const int kSocketMTUSize = 1492;

class Mutex;
class SocketAddress;

/** Immutable configuration settings, cannot be configured after a socket is created. */
struct SocketSettings SEOUL_SEALED
{
	/** Timeout on receive operations. Default of 0 indicates "infinite timeout. */
	TimeInterval m_ReceiveTimeout;
}; // struct SocketSettings

/**
 * Platform-independent socket class.  Supports synchronous and asynchronous
 * operations on TCP and UDP sockets, as is IP version-agnostic.
 *
 * The typical order of function calls is usually one of the following:
 *
 * TCP client: Connect, (Send|Receive)*, Close
 * TCP server: Bind, Listen, Accept, (Send|Receive)* on the new socket, Close
 * UDP client/server: Bind, (SendTo|ReceiveFrom)*, Close
 */
class Socket SEOUL_SEALED
{
public:
	/** Supported socket types */
	enum ESocketType
	{
		kTCP,
		kUDP,
		kUnknown,  //< Do not use
	};

	// Performs platform-specific socket initialization.  Must be called
	// before any sockets are created.
	static void StaticInitialize();

	// Performs platform-specific socket deinitialization.  Socket functions
	// cannot be called after this, unless StaticInitialize() is called again.
	static void StaticShutdown();

	// Returns the last error code from a socket function
	static int GetLastSocketError();

	// Returns the last error code from a hostname-related socket function
	static int GetLastHostnameError();

	Socket(const SocketSettings& settings = SocketSettings());
	~Socket();

	// Closes the socket.  The socket is automatically closed when this object
	// is destroyed.
	void Close();

	// Shuts down the socket for reading and writing.  See the shutdown(2)
	// man page.  Returns true on success of false on failure.
	Bool Shutdown();

	// Synchronously connects to the given host.  This may block for
	// non-trivial amounts of time.  Returns true on success or false on
	// failure.
	Bool Connect(ESocketType eType, const String& sHostname, UInt16 uPort);

	// Synchronously connects to the given host.  This may block for
	// non-trivial amounts of time.  Returns true on success or false on
	// failure.
	Bool Connect(const SocketAddress& address);

	/** Tests if we are currently connected to another peer */
	Bool IsConnected() const { return m_bConnected; }

	// Binds the socket to a local network interface
	Bool Bind(const SocketAddress& address);

	// Begins listening on the socket (Bind() must have previously been called)
	Bool Listen(int nBacklog);

	// Accepts a connection (Listen() must have previously been called) and
	// returns the new socket.  Blocks until a connection is received.  The
	// returned socket should be deleted with SEOUL_DELETE when it's no longer
	// needed.
	Socket *Accept();

	// Sets the socket to be either blocking or non-blocking.  The default is
	// blocking.
	void SetBlocking(Bool bBlocking);

	// Sets or unsets the TCP_NODELAY flag (TCP only) to enable/disable the
	// Nagle algorithm
	void SetTCPNoDelay(Bool bNoDelay);

	// Sends data to the connected peer.  May send less data than requested.
	// Returns the amount of data sent or a negative error code.
	int Send(const void *pData, UInt32 zDataSize);

	// Sends all of the data to the connected peer.  Only usable on blocking
	// sockets, but is guaranteed to send all of the data if an error does not
	// occur.
	int SendAll(const void *pData, UInt32 zDataSize);

	// Receives data from the connected peer.  Returns the amount of data
	// received or a negative error code.
	int Receive(void *pBuffer, UInt32 zBufferSize);

	// Continues receiving until the buffer is completely filled or an error
	// occurs
	int ReceiveAll(void *pBuffer, UInt32 zBufferSize);

	// Sends data to the given unconnected peer (UDP only).  May send less data
	// than requested.  Returns the amount of data sent or a negative error
	// code.
	int SendTo(const void *pData, UInt32 zDataSize, const sockaddr& address, size_t uAddressLength);

	// Sends all of the data to the given unconnected peer (UDP only).  Only
	// usable on blocking sockets, but is guaranteed to send all of the data if
	// an error does not occur.
	int SendToAll(const void *pData, UInt32 zDataSize, const sockaddr& address, size_t uAddressLength);

	// Receives data from an unconnected peer (UDP only).  Returns the amount
	// of data received or a negative error code.  If any data was received,
	// the peer address from which it was received is stored into outAddress.
	int ReceiveFrom(void *pBuffer, UInt32 zBufferSize, sockaddr_storage& outAddress, size_t& uOutAddressLength);

	// Logs a human-readable error message
	static void LogError(const char *sFunction, int nErrorCode);

private:
	// Counter used for ensuring we only do a single StaticInitialize
	static Atomic32 s_nStaticInitCount;

	/** Immutable socket configuration. */
	SocketSettings const m_Settings;

	/** Synchronization around initialization block. */
	ScopedPtr<Mutex> m_pInitializingMutex;

	/** Type of this socket */
	ESocketType m_eType;

	/** Socket handle/file descriptor */
	SocketHandle m_Socket;

	/** In the process of connecting or binding. */
	Atomic32Value<Bool> m_bInitializing;

	/** Have we been initialized yet? */
	Atomic32Value<Bool> m_bInitialized;

	/** Are we connected to another peer? */
	Atomic32Value<Bool> m_bConnected;

	/** Utility for protecting an initialization block (connect or bind). */
	struct InitializingScope SEOUL_SEALED
	{
		InitializingScope(Socket& r)
			: m_r(r)
		{
			// Lock the mutex and set initializing to true.
			m_r.m_pInitializingMutex->Lock();
			m_r.m_bInitializing = true;
		}

		~InitializingScope()
		{
			// Unset m_bInitializing and unlock the mutex.
			m_r.m_bInitializing = false;
			m_r.m_pInitializingMutex->Unlock();
		}

		Socket& m_r;
	}; // struct InitializingScope

	SEOUL_DISABLE_COPY(Socket);
}; // class Socket

/** Class representing a socket endpoint, i.e. a network address */
class SocketAddress
{
public:
	// Constructs a default invalid address
	SocketAddress();

	// Destroys the address and frees associated resources
	~SocketAddress();

	// Initializes an address from a hostname (e.g. "www.demiurgestudios.com",
	// "192.168.1.100", or "::1") and a port, optionally doing a DNS lookup if
	// the hostname is not an IP address.  Returns true if successful, or false
	// on failure (e.g. DNS lookup failed).
	Bool InitializeForConnect(Socket::ESocketType eType, const String& sHostname, UInt16 uPort, Bool bDoNameLookup = true);

	// Initializes an address for binding to our local network interface
	Bool InitializeForBind(Socket::ESocketType eType, UInt16 uPort);

	/** Gets the desired socket type for sockets using this address */
	Socket::ESocketType GetSocketType() const { return m_eType; }

	/** Gets the linked list of resolved address info */
	const addrinfo *GetAddressInfo() const
	{
		return m_pAddressInfo;
	}

private:
	SEOUL_DISABLE_COPY(SocketAddress);

	/** Desired socket type for sockets using this address */
	Socket::ESocketType m_eType;

	/** Linked list of info about the resolved address */
	addrinfo *m_pAddressInfo;
}; // class Socket

} // namespace Seoul

#endif  // Include guard
