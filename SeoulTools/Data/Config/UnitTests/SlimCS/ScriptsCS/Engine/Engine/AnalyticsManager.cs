// Access to the native AnalyticsManager singleton.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public static class AnalyticsManager
{
	static readonly Native.ScriptEngineAnalyticsManager udAnalyticsManager = null;

	static AnalyticsManager()
	{
		udAnalyticsManager = CoreUtilities.NewNativeUserData<Native.ScriptEngineAnalyticsManager>();
		if (null == udAnalyticsManager)
		{
			error("Failed instantiating ScriptEngineAnalyticsManager.");
		}
	}

	public enum ProfileUpdateOp
	{
		/// <summary>
		/// Unknown or invalid.
		/// </summary>
		Unknown,

		/// <summary>
		/// Add a numeric value to an existing numeric value.
		/// </summary>
		Add,

		/// <summary>
		/// Adds values to a list.
		/// </summary>
		Append,

		/// <summary>
		/// Remove a value from an existing list.
		/// </summary>
		Remove,

		/// <summary>
		/// Set a value to a named property, always.
		/// </summary>
		Set,

		/// <summary>
		/// Set a value to a named property only if it is not already set.
		/// </summary>
		SetOnce,

		/// <summary>
		/// Merge a list of values with an existing list of values, deduped.
		/// </summary>
		Union,

		/// <summary>
		/// Permanently delete the list of named properties from the profile.
		/// </summary>
		Unset,
	}

	public static void Flush()
	{
		udAnalyticsManager.Flush();
	}

	public static bool GetAnalyticsSandboxed()
	{
		return udAnalyticsManager.GetAnalyticsSandboxed();
	}

	public static Table GetStateProperties()
	{
		return udAnalyticsManager.GetStateProperties();
	}

	public static void TrackEvent(string sEventName, Table tProperties)
	{
		udAnalyticsManager.TrackEvent(sEventName, tProperties);
	}

	public static void UpdateProfile(ProfileUpdateOp eOp, Table tUpdates)
	{
		udAnalyticsManager.UpdateProfile(eOp, tUpdates);
	}

	public static double GetSessionCount()
	{
		return udAnalyticsManager.GetSessionCount();
	}
}
