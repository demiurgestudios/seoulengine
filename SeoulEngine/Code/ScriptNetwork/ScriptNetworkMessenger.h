/**
 * \file ScriptNetworkMessenger.h
 * \brief Binder instance for exposing a NetworkMessenger
 * instance to script.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_NETWORK_MESSENGER_H
#define SCRIPT_NETWORK_MESSENGER_H

#include "Prereqs.h"
namespace Seoul { namespace Network { class Messenger; } }
namespace Seoul { namespace Network { struct MessengerSettings; } }
namespace Seoul { namespace Script { class FunctionInterface; } }

#if SEOUL_WITH_NETWORK

namespace Seoul
{

class ScriptNetworkMessenger SEOUL_SEALED
{
public:
	ScriptNetworkMessenger();
	void Construct(const Network::MessengerSettings& settings);
	~ScriptNetworkMessenger();

	// Close the network messenger prematurely.
	void Disconnect();

	// Check if the network messenger is still connected.
	Bool IsConnected() const;

	// Pop a message off the receive queue.
	void ReceiveMessage(Script::FunctionInterface* pInterface);

	// Send a message via the current network messenger.
	void SendMessage(Script::FunctionInterface* pInterface);

private:
	ScopedPtr<Network::Messenger> m_pNetworkMessenger;

	SEOUL_DISABLE_COPY(ScriptNetworkMessenger);
}; // class ScriptNetworkMessenger

} // namespace Seoul

#endif // /#if SEOUL_WITH_NETWORK

#endif // include guard
