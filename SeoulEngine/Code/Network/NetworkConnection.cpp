/**
 * \file NetworkConnection.cpp
 * \brief Low-level synchronous communication. Implements the lowest
 * level of send/receive communication with a remote socket. Typically,
 * used in conjunction with a Network::Messenger and client-specific message
 * encoding to implement a complete remote communication pipe.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Mutex.h"
#include "NetworkConnection.h"
#include "SeoulSocket.h"
#include "SocketStream.h"
#include "Thread.h"

#if SEOUL_WITH_NETWORK

namespace Seoul::Network
{

/** Absolute max message size. */
static const UInt32 kuMaxMessageSize = (1 << 26);

/** Message header data - currently just the message size in bytes, as a UInt16. */
struct ConnectionMessageHeader SEOUL_SEALED
{
	UInt16 m_uBodySize;

	Bool operator==(const ConnectionMessageHeader& b) const
	{
		return (m_uBodySize == b.m_uBodySize);
	}

	Bool operator!=(const ConnectionMessageHeader& b) const
	{
		return !(*this == b);
	}
};

// Special header value indicating an extra header is present/required.
static const ConnectionMessageHeader kHeaderHasExtraHeader = { UInt16Max };

/**
 * Message extra header data - if ConnectionMessageHeader is
 * set to the extra value, this header will be present immediately
 * after the required header.
 */
struct ConnectionMessageExtraHeader SEOUL_SEALED
{
	UInt32 m_uBodySize;
};

Connection::Connection(const ConnectionOnReceiveDelegate& receiveDelegate)
	: m_OnReceiveMessage(receiveDelegate)
	, m_pSendMutex(SEOUL_NEW(MemoryBudgets::Network) Mutex)
	, m_pSocket(SEOUL_NEW(MemoryBudgets::Network) Socket)
	, m_pSocketConnectionMutex(SEOUL_NEW(MemoryBudgets::Network) Mutex)
	, m_pStream(SEOUL_NEW(MemoryBudgets::Network) SocketStream(*m_pSocket))
	, m_pReceiveThread(nullptr)
	, m_ReceiveThreadID()
	, m_bShuttingDown(false)
	, m_bConnecting(false)
{
}

Connection::~Connection()
{
	Disconnect();

	m_pStream.Reset();
	m_pSocket.Reset();
	m_pSocketConnectionMutex.Reset();
}

/**
 * Synchronously connects to the given server -- this may block for a
 * non-trivial amount of time in bad network situations.  Must be called before
 * calling any other functions, or they will fail.
 *
 * @param[in] sServerHostname Hostname or IP address of the server to connect
 *              to.
 * @Param[in] iPort Port of the server to connect to.
 *
 * @return True if the connection succeeded, or false if the connection failed
 */
Bool Connection::Connect(const ConnectionSettings& settings)
{
	// Clear to the disconnected state before connecting.
	Disconnect();

	// This is a connecting scope.
	ConnectingScope scope(*this);

	// Attempt the connection. Need to release the connecting mutex during this scope.
	m_pSocketConnectionMutex->Unlock();
	Bool const bSuccess = m_pSocket->Connect(Socket::kTCP, settings.m_sHostname, settings.m_iPort);
	m_pSocketConnectionMutex->Lock();

	// If failed, or if we are no longer connecting (m_bConnecting is now false), return immediately.
	if (!bSuccess || !m_bConnecting)
	{
		return false;
	}

	// Disable the Nagle algorithm
	m_pSocket->SetTCPNoDelay(true);

	// Start up receive thread
	m_pReceiveThread.Reset(SEOUL_NEW(MemoryBudgets::Network) Thread(SEOUL_BIND_DELEGATE(&Connection::ReceiveLoop, this)));
	m_pReceiveThread->Start("Network::Connection Thread");

	return true;
}

/**
 * Disconnects from the server, which implicitly closes all currently open
 * remote files and cancels any pending asynchronous I/O.  This is
 * automatically called from the destructor.
 */
void Connection::Disconnect()
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

	// Done shutting down.
	m_bShuttingDown = false;
}

/**
 * Tests if we are currently connected to a server and not in the process of
 * shutting down
 */
Bool Connection::IsConnected() const
{
	return m_pSocket->IsConnected() && !m_bShuttingDown;
}

/**
 * Synchronously send a message.
 *
 * @return True on success, false on failure. False implies
 * a network failure.
 *
 * \warning Synchronous and blocking.
 */
Bool Connection::Send(const Message& message)
{
	// Lock the send mutex and try to send the data.
	Lock lock(*m_pSendMutex);

	// The body size is m_vData size.
	UInt32 uBodySize = message.m_vData.GetSize();

	// If uBodySize >= UInt16Max, need to write out an extra header
	// and write kHeaderHasExtraHeader for the header value.
	if (uBodySize >= (UInt32)UInt16Max)
	{
		// Indicate there is an extra header.
		if (!m_pStream->Write16(kHeaderHasExtraHeader.m_uBodySize))
		{
			return false;
		}

		// Write the extra header.
		if (!m_pStream->Write32(uBodySize))
		{
			return false;
		}
	}
	// Otherwise, just write the header.
	else
	{
		if (!m_pStream->Write16((UInt16)message.m_vData.GetSize()))
		{
			return false;
		}
	}

	if (!message.m_vData.IsEmpty())
	{
		if (!m_pStream->Write(message.m_vData.Get(0u), message.m_vData.GetSize()))
		{
			return false;
		}
	}

	if (!m_pStream->Flush())
	{
		return false;
	}

	return true;
}

/** Thread procedure for running the RPC receive loop */
Int Connection::ReceiveLoop(const Thread& thread)
{
	// Since Windows has no way to get the thread ID of a thread from a
	// thread handle pre-Vista, we need to store our thread ID now to track it
	m_ReceiveThreadID = Thread::GetThisThreadId();

	// Loop until we're told to stop, or until a read operation fails.
	while (true)
	{
		UInt32 uBodySize = 0u;
		{
			// Read the header bytes - the socket is blocking, so a false
			// value here means a lost connection or terminal error,
			// so break out of the receive loop.
			ConnectionMessageHeader header;
			if (!m_pStream->Read16(header.m_uBodySize))
			{
				break;
			}

			// Check for and read the extra header.
			if (header == kHeaderHasExtraHeader)
			{
				ConnectionMessageExtraHeader extraHeader;
				if (!m_pStream->Read32(extraHeader.m_uBodySize))
				{
					break;
				}

				// Body size is the extra header.
				uBodySize = extraHeader.m_uBodySize;
			}
			else
			{
				// Otherwise, body size is the main header.
				uBodySize = header.m_uBodySize;
			}
		}

		// Sanity check the body size - ignore this data if invalid.
		if (uBodySize > kuMaxMessageSize)
		{
			continue;
		}

		Message message;

		// Populate the message data if the body size is non-zero.
		if (uBodySize > 0u)
		{
			message.m_vData.Resize(uBodySize);

			// Read the body - a failure here indicates a lost connection
			// or other network failure, so block out of the receive loop.
			if (!m_pStream->Read(message.m_vData.Get(0u), uBodySize))
			{
				break;
			}
		}

		// Send through the received data to the consumer code.
		m_OnReceiveMessage(message);
	}

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

Connection::ConnectingScope::ConnectingScope(Connection& r)
	: m_r(r)
{
	// Lock the mutex and set initializing to true.
	m_r.m_pSocketConnectionMutex->Lock();
	m_r.m_bConnecting = true;
}

Connection::ConnectingScope::~ConnectingScope()
{
	// Unset m_bInitializing and unlock the mutex.
	m_r.m_bConnecting = false;
	m_r.m_pSocketConnectionMutex->Unlock();
}

} // namespace Seoul::Network

#endif // /#if SEOUL_WITH_NETWORK
