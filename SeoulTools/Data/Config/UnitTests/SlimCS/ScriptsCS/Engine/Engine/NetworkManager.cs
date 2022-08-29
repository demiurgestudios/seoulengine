// Access to the native NetworkManager
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

#if SEOUL_WITH_ANIMATION_2D
public static class NetworkManager
{
	static readonly Native.ScriptNetworkManager udNetworkManager = null;

	static NetworkManager()
	{
		udNetworkManager = CoreUtilities.NewNativeUserData<Native.ScriptNetworkManager>();
		if (null == udNetworkManager)
		{
			error("Failed instantiating ScriptNetworkManager.");
		}
	}

	public static Native.ScriptNetworkMessenger NewMessenger(string hostname, int port, string encryptionKeyBase32)
	{
		return udNetworkManager.NewMessenger(hostname, port, encryptionKeyBase32);
	}
}
#endif // /#if SEOUL_WITH_ANIMATION_2D
