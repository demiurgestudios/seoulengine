/**
 * \file NetworkMessenger.h
 * \brief Network::Messenger implements a synchronous communication
 * layer, using a Network::Connection instance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef NETWORK_MESSENGER_H
#define NETWORK_MESSENGER_H

#include "AtomicRingBuffer.h"
#include "EncryptXXTEA.h"
#include "NetworkConnection.h"
#include "ThreadId.h"
namespace Seoul { namespace Network { struct Message; } }
namespace Seoul { class Signal; }
namespace Seoul { class Thread; }

#if SEOUL_WITH_NETWORK

namespace Seoul::Network
{

/** Configuration settings for a Messanger, passed to Messanger(). */
struct MessengerSettings
{
	MessengerSettings()
		: m_ConnectionSettings()
	{
		memset(&m_aKey[0], 0, sizeof(m_aKey));
	}

	ConnectionSettings m_ConnectionSettings;
	UInt32 m_aKey[EncryptXXTEA::kuKeyLengthInUInt32];
};

/** Internal utility structure used for queueing receive and send messages. */
typedef AtomicRingBuffer<Message*> MessengerRingBuffer;

/**
 * State of a Messenger.
 *
 * Network::Messenger starts in the kConnectingState, eventually
 * reaches the kConnected state (unless told to disconnect prior
 * to this), and then ends in the kDisconnected state, either
 * from a call to Disconnect() or due to a premature network
 * disconnect.
 *
 * Once in the kDisconnected state, a new Network::Messenger must
 * be created to re-establish the connection.
 */
enum class MessengerState
{
	/** Initial state - trying to connect to the endpoint. */
	kConnecting = 0,

	/** Normal state - active connection with the endpoint. */
	kConnected,

	/** End state - premature or deliberate disconnect from the endpoint. */
	kDisconnected,
};

/**
 * High-level messenger implementation, on top of Network::Connection. Used
 * in conjunction with a client message format, implements a full
 * synchronous remote communication pipe.
 */
class Messenger SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(Messenger);

	Messenger(const MessengerSettings& settings);
	~Messenger();

	// Disconnect the active connection, or cancel a pending connection.
	// Enter the kDisconnected state.
	void Disconnect();

	// Get the Network::Messenger state. Once the Network::Messenger
	// reaches the kDisconnected state, a new Network::Messenger
	// must be created to form a new connection.
	MessengerState GetState() const
	{
		return (MessengerState)m_eState;
	}

	// Consume the next queued message - returns nullptr if no message
	// is pending.
	Message* ReceiveMessage();

	// Asynchronously send a message.
	//
	// WARNING: Takes ownership of the data in rMessage.
	void SendMessage(Message& rMessage);

private:
	// Internal, clear buffers. Used in destructor and for disconnect.
	void ClearBuffers();

	// Thread that manages the server connection.
	Int ConnectionLoop(const Thread& thread);

	// Binding into Network::Connection for message receive.
	void OnReceiveMessage(Message& rMessage);

	/** Fixed configuration of this Network::Messenger. */
	MessengerSettings const m_Settings;

	/** Tracking of Network::Messenger state. */
	Atomic32Value<MessengerState> m_eState;

	/** Shared reference to the Network::Connection used by this Network::Messenger. */
	ScopedPtr<Connection> m_pNetworkConnection;

	/** Thread for handling the connection to the server. */
	ScopedPtr<Thread> m_pConnectionThread;

	/** Thread ID of the connection thread.  */
	ThreadId m_ConnectionThreadID;

	/** RingBuffer for outgoing messages. */
	MessengerRingBuffer m_SendBuffer;

	/** Signal for waking up the connection thread. */
	ScopedPtr<Signal> m_pSendSignal;

	/** RingBuffer for incoming messages. */
	MessengerRingBuffer m_ReceiveBuffer;

	/** Flag to communication to the connection thread to close the connection. */
	Atomic32Value<Bool> m_bShuttingDown;

	SEOUL_DISABLE_COPY(Messenger);
}; // class Messenger

} // namespace Seoul::Network

#endif // /#if SEOUL_WITH_NETWORK

#endif // include guard
