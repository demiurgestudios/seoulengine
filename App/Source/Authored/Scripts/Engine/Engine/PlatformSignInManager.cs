// Access to the native PlatformSignInManager singleton.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public static class PlatformSignInManager
{
	static readonly Native.ScriptEnginePlatformSignInManager udPlatformSignInManager = null;

	static PlatformSignInManager()
	{
		udPlatformSignInManager = CoreUtilities.NewNativeUserData<Native.ScriptEnginePlatformSignInManager>();
		if (null == udPlatformSignInManager)
		{
			error("Failed instantiating ScriptEnginePlatformSignInManager.");
		}
	}

	public static double GetStateChangeCount()
	{
		return udPlatformSignInManager.GetStateChangeCount();
	}

	public static bool IsSignedIn()
	{
		return udPlatformSignInManager.IsSignedIn();
	}

	public static bool IsSigningIn()
	{
		return udPlatformSignInManager.IsSigningIn();
	}

	public static bool IsSignInSupported()
	{
		return udPlatformSignInManager.IsSignInSupported();
	}

	public static void SignIn()
	{
		udPlatformSignInManager.SignIn();
	}

	public static void SignOut()
	{
		udPlatformSignInManager.SignOut();
	}
}
