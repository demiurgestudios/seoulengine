/**
 * \file NetworkConnection.h
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

#pragma once
#ifndef NETWORK_CONNECTION_H
#define NETWORK_CONNECTION_H

#include "Delegate.h"
#include "ScopedPtr.h"
#include "SeoulString.h"
#include "ThreadId.h"
#include "Vector.h"
namespace Seoul { class Mutex; }
namespace Seoul { class Socket; }
namespace Seoul { class SocketStream; }
namespace Seoul { class Thread; }

#if SEOUL_WITH_NETWORK

namespace Seoul::Network
{

/** Fully configures a network connection opened with a Network::Connection instance. */
struct ConnectionSettings SEOUL_SEALED
{
	ConnectionSettings()
		: m_sHostname()
		, m_iPort(0)
	{
	}

	/** Remote hostname or IP to connect to. */
	String m_sHostname;

	/** Remote public port to connect to. */
	Int32 m_iPort;
}; // struct ConnectionSettings

/** Utility structure contains a received message or a message to send. */
struct Message SEOUL_SEALED
{
	// TODO: We want a union of
	// a relatively small buffer and a heap
	// pointer, to allow small messages to
	// avoid heap allocation.
	typedef Vector<Byte, MemoryBudgets::Network> Data;
	Data m_vData;
}; // struct Message

/**
 * Delegate used to deliver receive messages. By reference, as the
 * receiver is allowed to take ownership of rMessage.
 */
typedef Delegate<void(Message& rMessage)> ConnectionOnReceiveDelegate;

/** Implements synchronous client/server messaging. */
class Connection SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(Connection);

	Connection(const ConnectionOnReceiveDelegate& receiveDelegate);
	~Connection();

	// Open a new connection - force disconnects any existing connection.
	Bool Connect(const ConnectionSettings& settings);

	// Disconnect any current connection - nop if no connection is currently open.
	void Disconnect();

	// Return true if a connection is open and valid, false otherwise.
	Bool IsConnected() const;

	// Synchronously send a message - false value indicates a network failure.
	//
	// WARNING: Synchronous and blocking.
	Bool Send(const Message& message);

private:
	// Thread procedure for running the RPC receive loop
	Int ReceiveLoop(const Thread& thread);

	/** Callback for message receive. */
	ConnectionOnReceiveDelegate m_OnReceiveMessage;

	/** Mutex for serializing writes to the socket */
	ScopedPtr<Mutex> m_pSendMutex;

	/** TCP socket for communications. */
	ScopedPtr<Socket> m_pSocket;

	/**
	 * Mutex used to synchronize Socket::Close() calls and
	 * to synchronize the connection flow in the face of
	 * a connect cancellation.
	 */
	ScopedPtr<Mutex> m_pSocketConnectionMutex;

	/** Socket stream for processing socket data */
	ScopedPtr<SocketStream> m_pStream;

	/** Thread for handling receives and dispatching callbacks */
	ScopedPtr<Thread> m_pReceiveThread;

	/** Thread ID of the receive thread  */
	ThreadId m_ReceiveThreadID;

	/**
	 * Flag indicating that we're trying to shut down, so further RPCs should
	 * fail
	 */
	Atomic32Value<Bool> m_bShuttingDown;

	/**
	 * Flag indicates connection scope.
	 * Used to synchronize a cancellation against a pending connection.
	 */
	Atomic32Value<Bool> m_bConnecting;

	/** Utility for protecting a connection block. */
	struct ConnectingScope SEOUL_SEALED
	{
		ConnectingScope(Connection& r);
		~ConnectingScope();

		Connection& m_r;
	}; // struct ConnectingScope

	SEOUL_DISABLE_COPY(Connection);
}; // class Connection

} // namespace Seoul::Network

#endif // /#if SEOUL_WITH_NETWORK

#endif // include guard
