/**
 * \file SeoulSocket.cpp
 * \brief Platform-independent socket class implementation
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "Mutex.h"
#include "SeoulSocket.h"

#if SEOUL_PLATFORM_WINDOWS
#include <ws2tcpip.h>
#define SEOUL_SOCKET_CHAR_T WCHAR
#define ADDRESS_CHAR_P "%ls"
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <unistd.h>
#define SEOUL_SOCKET_CHAR_T char
#define ADDRESS_CHAR_P "%s"
#endif

namespace Seoul
{

/** Platform independent application of settings that can be configured once for a socket. */
static void ApplyImmutableSettings(SocketHandle socket, const SocketSettings& settings)
{
	// Receive timeout - only set if non-zero.
	if (!settings.m_ReceiveTimeout.IsZero())
	{
#if SEOUL_PLATFORM_WINDOWS
		auto const v = (DWORD)(settings.m_ReceiveTimeout.GetMicroseconds() / 1000); // Value in milliseconds.
#else
		auto const& rv = settings.m_ReceiveTimeout.GetTimeValue();
		struct timeval v;
		v.tv_sec = (decltype(timeval::tv_sec))rv.tv_sec;
		v.tv_usec = (decltype(timeval::tv_usec))rv.tv_usec;
#endif

		SEOUL_VERIFY(setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&v, sizeof(v)) == 0);
	}
}

/** Platform-independent helper function for closing a socket handle */
static void CloseSocket(SocketHandle socket)
{
#if SEOUL_PLATFORM_WINDOWS
	SEOUL_VERIFY(closesocket(socket) == 0);
#else
	SEOUL_VERIFY(close(socket) == 0);
#endif
}

#if SEOUL_LOGGING_ENABLED
/** Converts an addrinfo structure into a human-readable string */
static void AddressInfoToString(const addrinfo *pAddressInfo, SEOUL_SOCKET_CHAR_T *pOutBuffer, socklen_t uBufferSize)
{
#if SEOUL_PLATFORM_WINDOWS
	DWORD dwBufferSize = (DWORD)uBufferSize;
	SEOUL_VERIFY(WSAAddressToStringW(pAddressInfo->ai_addr, (DWORD)pAddressInfo->ai_addrlen, nullptr, pOutBuffer, &dwBufferSize) == 0);
#else
	void *pAddress = (pAddressInfo->ai_family == AF_INET ?
					  (void *)&((sockaddr_in *)pAddressInfo->ai_addr)->sin_addr :
					  (void *)&((sockaddr_in6 *)pAddressInfo->ai_addr)->sin6_addr);
	SEOUL_VERIFY(inet_ntop(pAddressInfo->ai_family, pAddress, pOutBuffer, uBufferSize) != nullptr);
#endif
}
#endif

/** Constructs a default invalid address */
SocketAddress::SocketAddress()
	: m_eType(Socket::kUnknown)
	, m_pAddressInfo(nullptr)
{
}

/** Destroys the socket address and frees any associated resources */
SocketAddress::~SocketAddress()
{
	if (m_pAddressInfo != nullptr)
	{
		freeaddrinfo(m_pAddressInfo);
	}
}

/**
 * Initializes an address from a hostname and a port, optionally doing a DNS
 * lookup if the hostname is not an IP address.
 *
 * @param[in] eType Type of socket which will be used for connecting to this address
 * @param[in] sHostname Hostname of the peer, either as a domain name
 *              (e.g. "www.demiurgestudios.com") or as an IP address of some
 *              flavor (e.g. "192.168.1.100" or "::1")
 * @param[in] uPort Port of the desired service, e.g. 80 for HTTP
 * @param[in] bDoNameLookup True if we should do a synchronous DNS lookup of the
 *              given hostname, or false if it's already a resolved IP address
 *
 * @return True if the address was successfully initialized, or false if an
 *         error occurred (e.g. DNS lookup failed)
 */
Bool SocketAddress::InitializeForConnect(Socket::ESocketType eType, const String& sHostname, UInt16 uPort, Bool bDoNameLookup /*= true*/)
{
	SEOUL_ASSERT(eType == Socket::kTCP || eType == Socket::kUDP);
	m_eType = eType;

	SEOUL_ASSERT(m_pAddressInfo == nullptr);

	// Convert the port to a string for getaddrinfo
	char sPortStr[8];
	SNPRINTF(sPortStr, sizeof(sPortStr), "%hu", uPort);

	// Prepare hints structure for getaddrinfo
	addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;  // Don't care if it's IPv4 or IPv6
	hints.ai_socktype = (eType == Socket::kTCP ? SOCK_STREAM : SOCK_DGRAM);
	hints.ai_flags = AI_NUMERICSERV;  // Don't do a service name lookup

	if (!bDoNameLookup)
	{
		hints.ai_flags |= AI_NUMERICHOST;  // Don't do a DNS lookup
	}

	int nStatus;
	if ((nStatus = getaddrinfo(sHostname.CStr(), sPortStr, &hints, &m_pAddressInfo)) != 0)
	{
		SEOUL_LOG_NETWORK("getaddrinfo(%s) failed: %s (error %d)\n", sHostname.CStr(), gai_strerror(nStatus), nStatus);
		return false;
	}

	return true;
}

/**
 * Initializes an address for binding to our local network interface
 *
 * @param[in] eType Type of socket which will be used for connecting to this address
 * @param[in] uPort Port of the desired service, e.g. 80 for HTTP
 *
 * @return True if the address was successfully initialized, or false if an
 *         error occurred
 */
Bool SocketAddress::InitializeForBind(Socket::ESocketType eType, UInt16 uPort)
{
	SEOUL_ASSERT(eType == Socket::kTCP || eType == Socket::kUDP);
	m_eType = eType;

	SEOUL_ASSERT(m_pAddressInfo == nullptr);

	// Convert the port to a string for getaddrinfo
	char sPortStr[8];
	SNPRINTF(sPortStr, sizeof(sPortStr), "%hu", uPort);

	// Prepare hints structure for getaddrinfo
	addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;  // Don't care if it's IPv4 or IPv6
	hints.ai_socktype = (eType == Socket::kTCP ? SOCK_STREAM : SOCK_DGRAM);
	hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;  // Fill in my IP for me & don't do a service name lookup

	int nStatus;
	if ((nStatus = getaddrinfo(nullptr, sPortStr, &hints, &m_pAddressInfo)) != 0)
	{
		SEOUL_LOG_NETWORK("getaddrinfo failed: %s\n", gai_strerror(nStatus));
		return false;
	}

	return true;
}

/** Counter used for ensuring we only do a single StaticInitialize */
Atomic32 Socket::s_nStaticInitCount(0);

/**
 * Performs platform-specific socket initialization.  Must be called
 * before any sockets are created.
 */
void Socket::StaticInitialize()
{
	// Avoid multiple initializations
	if (s_nStaticInitCount++ > 0)
	{
		return;
	}

#if SEOUL_PLATFORM_WINDOWS
	WSADATA wsaData;
	int nResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	SEOUL_ASSERT_MESSAGE(nResult == 0, String::Printf("WSAStartup failed: error %d", nResult).CStr());
	SEOUL_LOG_NETWORK("Initialized WinSock: version=%u.%u\n  Description: %s\n  System status: %s\n", wsaData.wVersion, wsaData.wHighVersion, wsaData.szDescription, wsaData.szSystemStatus);
#endif
}

/**
 * Performs platform-specific socket deinitialization.  Socket functions cannot
 * be called after this, unless StaticInitialize() is called again.
 */
void Socket::StaticShutdown()
{
	// Avoid multiple shutdowns
	if (--s_nStaticInitCount > 0)
	{
		return;
	}

#if SEOUL_PLATFORM_WINDOWS
	SEOUL_VERIFY(WSACleanup() == 0);
#endif
}

/** Returns the last error code from a socket function */
int Socket::GetLastSocketError()
{
#if SEOUL_PLATFORM_WINDOWS
	return WSAGetLastError();
#else
	return errno;
#endif
}

/** Returns the last error code from a hostname-related function */
int Socket::GetLastHostnameError()
{
#if SEOUL_PLATFORM_WINDOWS
	return WSAGetLastError();
#else
	return h_errno;
#endif
}

/** Logs a human-readable error message */
void Socket::LogError(const char *sFunctionName, int nErrorCode)
{
#if SEOUL_LOGGING_ENABLED
#if SEOUL_PLATFORM_WINDOWS
	char sMessage[256];
	DWORD uLength;
	uLength = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
							 nullptr,
							 nErrorCode,
							 0,
							 sMessage,
							 SEOUL_ARRAY_COUNT(sMessage),
							 nullptr);
	SEOUL_ASSERT(uLength != 0);

	// Strip trailing newline, if present
	if (sMessage[uLength - 1] == '\n')
	{
		if (uLength > 1 && sMessage[uLength - 2] == '\r')
		{
			sMessage[uLength - 2] = 0;
		}
		else
		{
			sMessage[uLength - 1] = 0;
		}
	}

	SEOUL_LOG_NETWORK("%s failed: %s (error %d)\n", sFunctionName, sMessage, nErrorCode);
#else
	SEOUL_LOG_NETWORK("%s failed: %s (error %d)\n", sFunctionName, strerror(nErrorCode), nErrorCode);
#endif
#endif
}

/**
 * Constructs a socket that is unusable until Connect(), ConnectAsync(), or
 * Bind() is called
 */
Socket::Socket(const SocketSettings& settings)
	: m_Settings(settings)
	, m_pInitializingMutex(SEOUL_NEW(MemoryBudgets::Network) Mutex)
	, m_eType(kUnknown)
	, m_Socket(kInvalidSocketHandle)
	, m_bInitializing(false)
	, m_bInitialized(false)
	, m_bConnected(false)
{
}

/** Destroys the socket and cancels any pending asynchronous requests */
Socket::~Socket()
{
	Close();
}

/**
 * Closes the socket and cancels any pending asynchronous requests.  The socket
 * cannot be used again unless Connect(), ConnectAsync(), or Bind() is called
 * again.
 */
void Socket::Close()
{
	// Entire Close block exclusive to
	// initialization.
	Lock lock(*m_pInitializingMutex);

	// Nothing to do if not initializing or initialized.
	if (!m_bInitializing && !m_bInitialized)
	{
		return;
	}

	CloseSocket(m_Socket);

	m_bInitializing = false;
	m_bInitialized = false;
	m_bConnected = false;
	m_Socket = kInvalidSocketHandle;
}

/**
 * Shuts down the socket for reading and writing.  See the shutdown(2) man
 * page.
 *
 * This does not need to be called before Close(), but if other threads are
 * currently blocked inside of a Send/Receive, then this should be called to
 * unblock them before closing (although that they will not unblock until the
 * other peer also shuts down its connection).
 *
 * @return True if the shutdown succeeded, or false if an error occurred
 */
Bool Socket::Shutdown()
{
	if (!m_bInitialized)
	{
		return false;
	}

#if SEOUL_PLATFORM_WINDOWS
	int nResult = shutdown(m_Socket, SD_BOTH);
#else
	int nResult = shutdown(m_Socket, SHUT_RDWR);
#endif

	if (nResult != 0)
	{
		LogError("shutdown", GetLastSocketError());
		return false;
	}

	return true;
}

/**
 * Synchronously connects to the given host.  This may block for non-trivial
 * amounts of time.
 *
 * @param[in] eType Type of socket to connect with (TCP or UDP).  For UDP
 *              sockets, this does not initiate a connection, but rathersets
 *              the default destination address for future Send() calls
 * @param[in] sHostname Hostname to connect to, either as a domain name or an
 *              IP address string
 * @param[in] uPort Port to connect to
 *
 * @return True if the connection was successful, or false if the connection
 *         failed
 */
Bool Socket::Connect(ESocketType eType, const String& sHostname, UInt16 uPort)
{
	SocketAddress address;
	return (address.InitializeForConnect(eType, sHostname, uPort, true) &&
			Connect(address));
}

/**
 * Synchronously connects to the given host.  This may block for non-trivial
 * amounts of time.
 *
 * @param[in] address Host address to connect to.  Must have been initialized
 *              using SocketAddress::InitializeForConnect().
 *
 * @return True if the connection was successful, or false if the connection
 *         failed
 */
Bool Socket::Connect(const SocketAddress& address)
{
	SEOUL_ASSERT(!m_bInitialized);

	// Initializing block scope.
	InitializingScope scope(*this);

	// Try to create the socket and connect it for each possible address in the
	// address list
	for (const addrinfo *pAddressInfo = address.GetAddressInfo();
		 pAddressInfo != nullptr;
		 pAddressInfo = pAddressInfo->ai_next)
	{
#if SEOUL_LOGGING_ENABLED
		SEOUL_SOCKET_CHAR_T sAddressStr[INET6_ADDRSTRLEN];
		AddressInfoToString(pAddressInfo, sAddressStr, sizeof(sAddressStr));
		SEOUL_LOG_NETWORK("connect: Trying " ADDRESS_CHAR_P "\n", sAddressStr);
#endif

		m_Socket = socket(pAddressInfo->ai_family, pAddressInfo->ai_socktype, pAddressInfo->ai_protocol);
		if (m_Socket == kInvalidSocketHandle)
		{
			LogError("socket", GetLastSocketError());
			continue;
		}

		// Apply settings that will never change.
		ApplyImmutableSettings(m_Socket, m_Settings);

		// Need to unlock and lock around the call to connect(), to allow
		// cancellation of the connect.
		m_pInitializingMutex->Unlock();
		Bool const bSuccess = (0 == connect(m_Socket, pAddressInfo->ai_addr, (Int)pAddressInfo->ai_addrlen));
		m_pInitializingMutex->Lock();

		// Log the error, if there was one.
		if (!bSuccess)
		{
			LogError("connect", GetLastSocketError());
		}

		// If initializing is now false, it means the connection was cancelled,
		// so just return immediately. Close() cleaned up our state already.
		if (!m_bInitializing)
		{
			return false;
		}

		// On failure, but still initializing, cleanup this attempt and try again.
		if (!bSuccess)
		{
			CloseSocket(m_Socket);
			m_Socket = kInvalidSocketHandle;
			continue;
		}

		// Success!
		m_bInitialized = true;
		m_bConnected = true;
		m_eType = address.GetSocketType();
		return true;
	}

	return false;
}

/**
 * Binds the socket to a local network interface
 *
 * @param[in] address Local network interface to bind to.  Must have been
 *              initialized using SocketAddress::InitializeForBind().
 */
Bool Socket::Bind(const SocketAddress& address)
{
	SEOUL_ASSERT(!m_bInitialized);

	// Initializing block scope.
	InitializingScope scope(*this);

	// Try to create the socket and bind it for each possible address in the
	// address list
	for (const addrinfo *pAddressInfo = address.GetAddressInfo();
		 pAddressInfo != nullptr;
		 pAddressInfo = pAddressInfo->ai_next)
	{
#if SEOUL_LOGGING_ENABLED
		SEOUL_SOCKET_CHAR_T sAddressStr[INET6_ADDRSTRLEN];
		AddressInfoToString(pAddressInfo, sAddressStr, sizeof(sAddressStr));
		SEOUL_LOG_NETWORK("bind: Trying " ADDRESS_CHAR_P "\n", sAddressStr);
#endif

		m_Socket = socket(pAddressInfo->ai_family, pAddressInfo->ai_socktype, pAddressInfo->ai_protocol);
		if (m_Socket == kInvalidSocketHandle)
		{
			LogError("socket", GetLastSocketError());
			continue;
		}

		// Apply settings that will never change.
		ApplyImmutableSettings(m_Socket, m_Settings);

		// Need to unlock and lock around the call to bind(), to allow
		// cancellation of the connect.
		m_pInitializingMutex->Unlock();
		Bool const bSuccess = (0 == bind(m_Socket, pAddressInfo->ai_addr, (Int)pAddressInfo->ai_addrlen));
		m_pInitializingMutex->Lock();

		// Log the error, if there was one.
		if (!bSuccess)
		{
			LogError("bind", GetLastSocketError());
		}

		// If initializing is now false, it means the connection was cancelled,
		// so just return immediately. Close() cleaned up our state already.
		if (!m_bInitializing)
		{
			return false;
		}

		// On failure, but still initializing, cleanup this attempt and try again.
		if (!bSuccess)
		{
			CloseSocket(m_Socket);
			m_Socket = kInvalidSocketHandle;
			continue;
		}

		// Success!
		m_bInitialized = true;
		m_eType = address.GetSocketType();
		return true;
	}

	return false;
}

/**
 * Begins listening on the socket for incoming connections.  The socket must
 * have been previously bound successfully with a call to Bind().
 *
 * @param[in] nBacklog A hint to the OS of what limit to apply to the length
 *              of the backlog of incoming connections which have not yet
 *              been accepted.  10 is a typical value.
 */
Bool Socket::Listen(int nBacklog)
{
	SEOUL_ASSERT(m_bInitialized);

	if (listen(m_Socket, nBacklog) != 0)
	{
		LogError("listen", GetLastSocketError());
		return false;
	}

	return true;
}

/**
 * Accepts a connection (Listen() must have previously been called) and returns
 * the new socket.  Blocks until a connection is received.
 *
 * The returned socket should be deleted with SEOUL_DELETE when it is no longer
 * needed.
 *
 * @return The socket for the new connection, or nullptr if an error occurred
 */
Socket *Socket::Accept()
{
	SEOUL_ASSERT(m_bInitialized);

	// Accept the connection and ignore the peer's address (that can be
	// retrieved later with getpeername(3) if so desired)
	SocketHandle newSocketHandle = accept(m_Socket, nullptr, nullptr);
	if (newSocketHandle == kInvalidSocketHandle)
	{
		LogError("accept", GetLastSocketError());
		return nullptr;
	}

	// Apply settings that will never change.
	ApplyImmutableSettings(newSocketHandle, m_Settings);

	// Create new Socket object for the new socket
	Socket *pNewSocket = SEOUL_NEW(MemoryBudgets::Network) Socket;
	pNewSocket->m_bInitialized = true;
	pNewSocket->m_bConnected = true;
	pNewSocket->m_eType = m_eType;
	pNewSocket->m_Socket = newSocketHandle;

	return pNewSocket;
}

/**
 * Sets the socket to be either blocking or non-blocking.  The default is
 * blocking.
 *
 * @param[in] bBlocking True to set the socket to be blocking, or false to set
 *              it to be non-blocking
 */
void Socket::SetBlocking(Bool bBlocking)
{
#if SEOUL_PLATFORM_WINDOWS
	unsigned long uNonBlocking = (unsigned long)!bBlocking;
	SEOUL_VERIFY(ioctlsocket(m_Socket, FIONBIO, &uNonBlocking) == 0);
#else
	int nFlags = fcntl(m_Socket, F_GETFL);
	if (bBlocking)
	{
		nFlags &= ~O_NONBLOCK;
	}
	else
	{
		nFlags |= O_NONBLOCK;
	}

	SEOUL_VERIFY(fcntl(m_Socket, F_SETFL, nFlags) == 0);
#endif
}

/**
 * Sets or unsets the TCP_NODELAY flag on the socket (TCP only) to enable or
 * disable the Nagle algorithm.
 *
 * The default is unset, which means that the network stack will wait for up to
 * 200 ms or a full packet before it actually sends data.  This has higher
 * network utilization (and therefore throughput) but higher latency when
 * sending lots of small packets, but it has little effect on well-behaved
 * applications which send appropriately-buffered large packets.
 *
 * @param[in] bNoDelay True to enable the TCP_NODELAY option, or false to
 *              disable it
 */
void Socket::SetTCPNoDelay(Bool bNoDelay)
{
	SEOUL_ASSERT(m_bInitialized);
	SEOUL_ASSERT(m_eType == kTCP);

	int nFlagValue = (bNoDelay ? 1 : 0);
	SEOUL_VERIFY(setsockopt(m_Socket, IPPROTO_TCP, TCP_NODELAY, (char *)&nFlagValue, sizeof(nFlagValue)) == 0);
}

/**
 * Sends data to the connected peer.  May send less data than requested.
 *
 * @param[in] pData Data to send
 * @param[in] zDataSize Length of data to send
 *
 * @return The amount of data sent or a negative error code
 */
int Socket::Send(const void *pData, UInt32 zDataSize)
{
	if (!m_bInitialized || !m_bConnected)
	{
		return -1;
	}

	// The Winsock send() takes a const char*
	return (int) send(m_Socket, (const char *)pData, (Int)zDataSize, 0);
}

/**
 * Sends all of the data to the connected peer.  Only usable on blocking
 * sockets, but is guaranteed to send all of the data if an error does not
 * occur.
 *
 * @param[in] pData Data to send
 * @param[in] zDataSize Length of data to send
 *
 * @return The amount of data sent or a negative error code
 */
int Socket::SendAll(const void *pData, UInt32 zDataSize)
{
	if (!m_bInitialized || !m_bConnected)
	{
		return -1;
	}

	SEOUL_ASSERT(zDataSize <= INT_MAX);

	const char *pDataPtr = (const char *)pData;
	int nBytesLeft = (int)zDataSize;

	// Keep trying to send until we send everything or an error occurs
	while (nBytesLeft > 0)
	{
		int nBytesSent = (int) send(m_Socket, pDataPtr, nBytesLeft, 0);
		if (nBytesSent < 0)
		{
			return nBytesSent;
		}

		pDataPtr += nBytesSent;
		nBytesLeft -= nBytesSent;
	}

	return (int)zDataSize;
}

/**
 * Receives data from the connected peer.
 *
 * @param[out] pBuffer Buffer to receive data into
 * @param[in] zBufferSize Maximum amount of data to receive
 *
 * @return The amount of data received or a negative error code.
 */
int Socket::Receive(void *pBuffer, UInt32 zBufferSize)
{
	if (!m_bInitialized || !m_bConnected)
	{
		return -1;
	}

	// The Winsock recv takes a char*
	return (int) recv(m_Socket, (char *)pBuffer, (Int)zBufferSize, 0);
}

/**
 * Receives data from the connected peer.  Continues to receive until the
 * given buffer is filled, the socket is shut down, or an error occurs.
 *
 * @param[out] pBuffer Buffer to receive data into
 * @param[in] zBufferSize Amount of data to receive
 *
 * @return The amount of data received, 0 if no data was received and the
 *         socket was shut down cleanly, or a negative error code if an error
 *         occurred.
 */
int Socket::ReceiveAll(void *pBuffer, UInt32 zBufferSize)
{
	if (!m_bInitialized || !m_bConnected)
	{
		return -1;
	}

	SEOUL_ASSERT(zBufferSize <= INT_MAX);

	char *pBufferPtr = (char *)pBuffer;
	int nBytesLeft = (int)zBufferSize;
	while (nBytesLeft > 0)
	{
		int nBytesReceived = (int) recv(m_Socket, pBufferPtr, nBytesLeft, 0);
		if (nBytesReceived <= 0)
		{
			return nBytesReceived;
		}

		pBufferPtr += nBytesReceived;
		nBytesLeft -= nBytesReceived;
	}

	return (int)zBufferSize;
}

/**
 * Sends data to the given unconnected peer (UDP only).  May send less data
 * than requested.
 *
 * @param[in] pData Data to send
 * @param[in] zDataSize Length of data to send
 * @param[in] address Address to send the data to
 * @param[in] uAddressLength Length in bytes of the given address
 *
 * @return The amount of data sent or a negative error code.
 */
int Socket::SendTo(const void *pData, UInt32 zDataSize, const sockaddr& address, size_t uAddressLength)
{
	if (!m_bInitialized)
	{
		return -1;
	}

	SEOUL_ASSERT(m_eType == kUDP);

	// The Winsock sendto takes a const char*
	return (int) sendto(m_Socket, (const char *)pData, (Int)zDataSize, 0, (const sockaddr *)&address, (Int)uAddressLength);
}

/**
 * Sends all of the data to the given unconnected peer (UDP only).  Only
 * usable on blocking sockets, but is guaranteed to send all of the data if
 * an error does not occur.
 *
 * @param[in] pData Data to send
 * @param[in] zDataSize Length of data to send
 * @param[in] address Address to send the data to
 * @param[in] uAddressLength Length in bytes of the given address
 *
 * @return The amount of data sent or a negative error code.
 */
int Socket::SendToAll(const void *pData, UInt32 zDataSize, const sockaddr& address, size_t uAddressLength)
{
	if (!m_bInitialized)
	{
		return -1;
	}

	SEOUL_ASSERT(m_eType == kUDP);
	SEOUL_ASSERT(zDataSize <= INT_MAX);

	const char *pDataPtr = (const char *)pData;
	int nBytesLeft = (int)zDataSize;

	while (nBytesLeft > 0)
	{
		int nBytesSent = (int) sendto(m_Socket, pDataPtr, nBytesLeft, 0, (const sockaddr *)&address, (Int)uAddressLength);
		if (nBytesSent < 0)
		{
			return nBytesSent;
		}

		pDataPtr += nBytesSent;
		nBytesLeft -= nBytesSent;
	}

	return (int)zDataSize;
}

/**
 * Receives data from an unconnected peer (UDP only).  If any data was
 * received, the peer address from which it was received is stored into
 * outAddress.
 *
 * @param[out] pBuffer Buffer to receive data into
 * @param[in] zBufferSize Maximum amount of data to receive
 * @param[out] outAddress If any data was received, this receives the source
 *               address of the data
 * @param[out] uOutAddressLength If any data was received, this receives the
 *               length of the source address of the data
 *
 * @return The amount of data received or a negative error code
 */
int Socket::ReceiveFrom(void *pBuffer, UInt32 zBufferSize, sockaddr_storage& outAddress, size_t& uOutAddressLength)
{
	if (!m_bInitialized)
	{
		return -1;
	}

	SEOUL_ASSERT(m_eType == kUDP);

	// Convert platform-specific address length into platform-agnostic length
#if SEOUL_PLATFORM_WINDOWS
	int addressLength = 0;
#else
	socklen_t addressLength = 0;
#endif

	// The Winsock recvfrom takes a char*
	int nResult = (int) recvfrom(m_Socket, (char *)pBuffer, (Int)zBufferSize, 0, (sockaddr *)&outAddress, &addressLength);
	uOutAddressLength = addressLength;

	return nResult;
}

} // namespace Seoul
