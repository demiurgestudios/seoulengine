/**
 * \file ScriptNetworkManager.cpp
 * \brief Binder instance for instantiating NetworkMessenger
 * instances into script.
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
#include "ScriptNetworkManager.h"
#include "ScriptNetworkMessenger.h"
#include "StringUtil.h"

#if SEOUL_WITH_NETWORK

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptNetworkManager, TypeFlags::kDisableCopy)
	SEOUL_METHOD(NewMessenger)
	SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "Native.ScriptNetworkMessenger", "string hostname, int port, string encryptionKeyBase32")
SEOUL_END_TYPE()

ScriptNetworkManager::ScriptNetworkManager()
{
}

ScriptNetworkManager::~ScriptNetworkManager()
{
}

bool DecodeUint32(String const& str, UInt offset, UInt32& result)
{
	if (str.GetSize() < offset + 8) {
		return false;
	}

	result = (
		((UInt32)DecodeHexChar(str[offset    ])) << 28 |
		((UInt32)DecodeHexChar(str[offset + 1])) << 24 |
		((UInt32)DecodeHexChar(str[offset + 2])) << 20 |
		((UInt32)DecodeHexChar(str[offset + 3])) << 16 |
		((UInt32)DecodeHexChar(str[offset + 4])) << 12 |
		((UInt32)DecodeHexChar(str[offset + 5])) <<  8 |
		((UInt32)DecodeHexChar(str[offset + 6])) <<  4 |
		((UInt32)DecodeHexChar(str[offset + 7]))
	);
	return true;
}

void ScriptNetworkManager::NewMessenger(Script::FunctionInterface* pInterface)
{
	String sHostname;
	if (!pInterface->GetString(1, sHostname))
	{
		pInterface->RaiseError(1, "invalid argument, expected string hostname.");
		return;
	}

	Int32 iPort = 0;
	if (!pInterface->GetInteger(2, iPort))
	{
		pInterface->RaiseError(2, "invalid argument, expected integer port.");
		return;
	}

	String sKeyBase32;
	if (!pInterface->GetString(3, sKeyBase32))
	{
		pInterface->RaiseError(3, "invalid argument, expected string encryption key.");
		return;
	}

	UInt32 uKey0, uKey1, uKey2, uKey3;
	if (!DecodeUint32(sKeyBase32, 0, uKey0) || !DecodeUint32(sKeyBase32, 8, uKey1) ||
		!DecodeUint32(sKeyBase32, 16, uKey2) || !DecodeUint32(sKeyBase32, 24, uKey3))
	{
		pInterface->RaiseError(3, "invalid argument, encryption key must be 32 hex characters.");
		return;
	}

	// Make settings.
	Network::MessengerSettings settings;
	settings.m_ConnectionSettings.m_sHostname = sHostname;
	settings.m_ConnectionSettings.m_iPort = iPort;
	settings.m_aKey[0] = uKey0;
	settings.m_aKey[1] = uKey1;
	settings.m_aKey[2] = uKey2;
	settings.m_aKey[3] = uKey3;

	// Instantiate the instance.
	ScriptNetworkMessenger* pScriptNetworkMessenger = pInterface->PushReturnUserData<ScriptNetworkMessenger>();
	pScriptNetworkMessenger->Construct(settings);
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_NETWORK
