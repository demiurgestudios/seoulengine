/**
 * \file ScriptNetworkMessenger.cpp
 * \brief Binder instance for exposing a NetworkMessenger
 * instance to script.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "NetworkMessenger.h"
#include "ReflectionDefine.h"
#include "ScriptFunctionInterface.h"
#include "ScriptNetworkMessenger.h"
#include "ScriptVm.h"

#if SEOUL_WITH_NETWORK

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptNetworkMessenger, TypeFlags::kDisableCopy)
	SEOUL_METHOD(Disconnect)
	SEOUL_METHOD(IsConnected)
	SEOUL_METHOD(ReceiveMessage)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "object", "")
	SEOUL_METHOD(SendMessage)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "object msgBytes")
SEOUL_END_TYPE()

ScriptNetworkMessenger::ScriptNetworkMessenger()
	: m_pNetworkMessenger()
{
}

ScriptNetworkMessenger::~ScriptNetworkMessenger()
{
}

/**
 * To be called immediately after ScriptNetworkMessenger().
 *
 * Required. Exists to workaround the fact that Reflection
 * only supports instantiation via a default constructor.
 */
void ScriptNetworkMessenger::Construct(const Network::MessengerSettings& settings)
{
	m_pNetworkMessenger.Reset(SEOUL_NEW(MemoryBudgets::Network) Network::Messenger(settings));
}

/** Close the network messenger prematurely. */
void ScriptNetworkMessenger::Disconnect()
{
	m_pNetworkMessenger->Disconnect();
}

/** Check if the network messenger is still connected. */
Bool ScriptNetworkMessenger::IsConnected() const
{
	return (Network::MessengerState::kDisconnected != m_pNetworkMessenger->GetState());
}

/** Pop a message off the receive queue. */
void ScriptNetworkMessenger::ReceiveMessage(Script::FunctionInterface* pInterface)
{
	Network::Message* pMessage = m_pNetworkMessenger->ReceiveMessage();
	if (nullptr == pMessage || pMessage->m_vData.IsEmpty())
	{
		pInterface->PushReturnNil();
		return;
	}

	// Push the data as a byte buffer.
	Script::ByteBuffer byteBuffer;
	byteBuffer.m_pData = pMessage->m_vData.Get(0u);
	byteBuffer.m_zDataSizeInBytes = pMessage->m_vData.GetSizeInBytes();
	pInterface->PushReturnByteBuffer(byteBuffer);

	// Cleanup the message instance.
	SafeDelete(pMessage);
}

/** Send a message via the current network messenger. */
void ScriptNetworkMessenger::SendMessage(Script::FunctionInterface* pInterface)
{
	Byte const* sData = nullptr;
	UInt32 zDataSizeInBytes = 0u;
	if (!pInterface->GetString(1u, sData, zDataSizeInBytes))
	{
		pInterface->RaiseError(1, "expected binary message data in string.");
		return;
	}

	Network::Message networkMessage;
	if (zDataSizeInBytes > 0u)
	{
		networkMessage.m_vData.Resize(zDataSizeInBytes);
		memcpy(networkMessage.m_vData.Get(0u), (void const*)sData, (size_t)zDataSizeInBytes);
	}
	m_pNetworkMessenger->SendMessage(networkMessage);
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_NETWORK
