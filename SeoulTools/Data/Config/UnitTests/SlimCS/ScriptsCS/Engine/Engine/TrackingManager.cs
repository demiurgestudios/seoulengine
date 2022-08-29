// Access to the native TrackingManager singleton.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public static class TrackingManager
{
	static readonly Native.ScriptEngineTrackingManager udTrackingManager = null;

	static TrackingManager()
	{
		udTrackingManager = CoreUtilities.NewNativeUserData<Native.ScriptEngineTrackingManager>();
		if (null == udTrackingManager)
		{
			error("Failed instantiating ScriptEngineTrackingManager.");
		}
	}

	public static string ExternalTrackingUserID
	{
		get
		{
			return udTrackingManager.GetExternalTrackingUserID();
		}
	}

	public static void TrackEvent(string sEventName)
	{
		udTrackingManager.TrackEvent(sEventName);
	}
}
