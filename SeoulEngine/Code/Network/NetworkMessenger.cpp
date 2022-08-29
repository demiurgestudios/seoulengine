/**
 * \file NetworkMessenger.cpp
 * \brief Network::Messenger implements a synchronous communication
 * layer, using a Network::Connection instance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EncryptXXTEA.h"
#include "NetworkConnection.h"
#include "NetworkMessenger.h"
#include "SeoulSignal.h"
#include "Thread.h"

#if SEOUL_WITH_NETWORK

namespace Seoul::Network
{

// Time to periodically release signal lock to check for a lost connection.
static const UInt32 kuSignalWaitTimeInMilliseconds = 1000u;

static inline Bool DecryptMessage(UInt32 const aKey[EncryptXXTEA::kuKeyLengthInUInt32], Message& rMessage)
{
	// Invalid size.
	if (rMessage.m_vData.IsEmpty())
	{
		return false;
	}

	// Invalid size.
	if (0u != (rMessage.m_vData.GetSizeInBytes() % sizeof(UInt32)))
	{
		return false;
	}

	UInt32 uSizeInUInt32 = (rMessage.m_vData.GetSizeInBytes() / sizeof(UInt32));
	UInt32* pBuffer = (UInt32*)rMessage.m_vData.Data();
	EncryptXXTEA::DecryptInPlace(pBuffer, uSizeInUInt32, aKey);

	// Get the actual message size.
	UInt32 uMessageSizeInBytes = pBuffer[uSizeInUInt32 - 1u];

	// Sanity check size.
	if (uMessageSizeInBytes > rMessage.m_vData.GetSizeInBytes() - sizeof(UInt32))
	{
		return false;
	}

	// Resize and return success.
	rMessage.m_vData.Resize(uMessageSizeInBytes);
	return true;
}

static inline void EncryptMessage(UInt32 const aKey[EncryptXXTEA::kuKeyLengthInUInt32], Message& rMessage)
{
	UInt32 uMessageSizeInBytes = rMessage.m_vData.GetSizeInBytes();

	// Pad to UInt32 size, then add 1 for the message size.
	UInt32 uEncryptedSizeInBytes = (UInt32)RoundUpToAlignment((size_t)uMessageSizeInBytes, sizeof(UInt32)) + sizeof(UInt32);

	// Pad to the new size.
	rMessage.m_vData.Resize(uEncryptedSizeInBytes, (Byte)0);

	// Get pointers and size.
	UInt32 uSizeInUInt32 = (uEncryptedSizeInBytes / sizeof(UInt32));
	UInt32* pBuffer = (UInt32*)rMessage.m_vData.Data();

	// Add the message size to the data.
	pBuffer[uSizeInUInt32 - 1u] = uMessageSizeInBytes;

	// Encrypt the data in-place.
	EncryptXXTEA::EncryptInPlace(pBuffer, uSizeInUInt32, aKey);
}

Messenger::Messenger(const MessengerSettings& settings)
	: m_Settings(settings)
	, m_eState(MessengerState::kConnecting)
	, m_pNetworkConnection()
	, m_pConnectionThread()
	, m_ConnectionThreadID()
	, m_SendBuffer()
	, m_pSendSignal(SEOUL_NEW(MemoryBudgets::Network) Signal)
	, m_ReceiveBuffer()
	, m_bShuttingDown(false)
{
	m_pNetworkConnection.Reset(SEOUL_NEW(MemoryBudgets::Network) Connection(SEOUL_BIND_DELEGATE(&Messenger::OnReceiveMessage, this)));
	m_pConnectionThread.Reset(SEOUL_NEW(MemoryBudgets::Network) Thread(SEOUL_BIND_DELEGATE(&Messenger::ConnectionLoop, this)));
	m_pConnectionThread->Start("Network::Messenger Thread");
}

Messenger::~Messenger()
{
	// Disconnect
	Disconnect();

	// Clear buffers.
	ClearBuffers();
}

/**
 * Explicitly close the current connection. Once in the kDisconnected
 * state, this Network::Messenger instance is idle and a new one must
 * be created to form a new connection.
 */
void Messenger::Disconnect()
{
	// Nothign to do if we've already cleaned up m_pNetworkConnection.
	if (!m_pNetworkConnection.IsValid())
	{
		return;
	}

	// Sanity check
	SEOUL_ASSERT(Thread::GetThisThreadId() != m_ConnectionThreadID);

	// Tell the connection thread we're shutting down.
	m_bShuttingDown = true;

	// Disconnect the connection.
	m_pNetworkConnection->Disconnect();

	// Wake up the connection thread.
	m_pSendSignal->Activate();

	// Wait for the thread to finish.
	(void)m_pConnectionThread->WaitUntilThreadIsNotRunning();
	m_pConnectionThread.Reset();

	// Destroy the connection.
	m_pNetworkConnection.Reset();

	// The connection thread cleared the connection id.
	SEOUL_ASSERT(ThreadId() == m_ConnectionThreadID);

	// Must now be in the kDisconnected state.
	SEOUL_ASSERT(MessengerState::kDisconnected == (MessengerState)m_eState);

	// Clear buffers.
	ClearBuffers();

	// Unset the shutting down flag.
	m_bShuttingDown = false;
}

/** Get the next queued received message. */
Message* Messenger::ReceiveMessage()
{
	return m_ReceiveBuffer.Pop();
}

/**
 * Queue a message for send - will be delivered asynchronously on the sender thread.
 *
 * \warning Takes ownership of the data contained in rMessage.
 */
void Messenger::SendMessage(Message& rMessage)
{
	// Dispose immediately without queueing if kDisconnected.
	if (MessengerState::kDisconnected == (MessengerState)m_eState)
	{
		Message disposeMessage;
		disposeMessage.m_vData.Swap(rMessage.m_vData);
		return;
	}

	// Create a new message instance.
	Message* pMessage = SEOUL_NEW(MemoryBudgets::Network) Message;

	// Consume the data.
	pMessage->m_vData.Swap(rMessage.m_vData);

	// Queue the message for send and (potentially) wake up the send thread.
	m_SendBuffer.Push(pMessage);
	m_pSendSignal->Activate();
}

/** Internal, clear buffers. Used in destructor and for disconnect. */
void Messenger::ClearBuffers()
{
	Message* pMessage = nullptr;
	while (nullptr != (pMessage = m_SendBuffer.Pop()))
	{
		SafeDelete(pMessage);
	}

	while (nullptr != (pMessage = m_ReceiveBuffer.Pop()))
	{
		SafeDelete(pMessage);
	}
}

/** Thread that manages the server connection. */
Int Messenger::ConnectionLoop(const Thread& thread)
{
	// Set the thread id.
	m_ConnectionThreadID = Thread::GetThisThreadId();

	// Keep trying to connect until we succeed.
	while (!m_bShuttingDown && !m_pNetworkConnection->IsConnected())
	{
		// If connection fails, sleep and then continue.
		if (!m_pNetworkConnection->Connect(m_Settings.m_ConnectionSettings))
		{
			// Wait on the signal.
			m_pSendSignal->Wait(kuSignalWaitTimeInMilliseconds);
			continue;
		}
	}

	// Now in the kConnected state if not shutting down and connected.
	if (!m_bShuttingDown && m_pNetworkConnection->IsConnected())
	{
		m_eState = MessengerState::kConnected;
	}

	// Send loop
	while (!m_bShuttingDown)
	{
		// Get the next message to send.
		Message* pMessage = m_SendBuffer.Pop();

		// If we have no message to send, wait on the send signal.
		if (nullptr == pMessage)
		{
			// Wait on the signal.
			m_pSendSignal->Wait(kuSignalWaitTimeInMilliseconds);

			// Check if we still have a connection - if not, break out
			// of the loop.
			if (!m_pNetworkConnection->IsConnected())
			{
				break;
			}

			// Otherwise, try again.
			continue;
		}

		// If enabled, encrypt the data.
		if (m_Settings.m_aKey[0] != 0)
		{
			EncryptMessage(m_Settings.m_aKey, *pMessage);
		}

		// We have a message to send. If this fails, break out of the
		// inner loop to reset the connection.
		Bool const bResult = m_pNetworkConnection->Send(*pMessage);

		// Release the message instance.
		SafeDelete(pMessage);

		// Breakout of the inner loop, disconnect event.
		if (!bResult)
		{
			break;
		}
	}

	// If we get here, flush the send buffer, since a disconnect
	// event has occured.
	{
		Message* pMessage = nullptr;
		while (nullptr != (pMessage = m_SendBuffer.Pop()))
		{
			SafeDelete(pMessage);
		}
	}

	// Now disconnected.
	m_eState = MessengerState::kDisconnected;

	// Clear the thread id.
	m_ConnectionThreadID = ThreadId();

	// Done.
	return 0;
}

/** Binding into Connection for message receive. */
void Messenger::OnReceiveMessage(Message& rMessage)
{
	// Create a heap allocated message instance.
	Message* pMessage = SEOUL_NEW(MemoryBudgets::Network) Message;

	// Consume the data.
	pMessage->m_vData.Swap(rMessage.m_vData);

	// If enabled, decrypt the data.
	if (m_Settings.m_aKey[0] != 0)
	{
		// If decryption fails, just dispose of the message.
		if (!DecryptMessage(m_Settings.m_aKey, *pMessage))
		{
			SafeDelete(pMessage);
			return;
		}
	}

	// Enqueue for consumption by the user context.
	m_ReceiveBuffer.Push(pMessage);
}

} // namespace Seoul::Network

#endif // /#if SEOUL_WITH_NETWORK

