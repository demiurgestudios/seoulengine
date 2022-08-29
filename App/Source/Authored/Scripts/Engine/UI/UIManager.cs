// Access to the native UIManager singleton.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

using System.Diagnostics;

public static class UIManager
{
	static readonly Native.ScriptUIManager udUIManager = null;

	static UIManager()
	{
		// Before further action, capture any methods that are not safe to be called
		// from off the main thread (static initialization occurs on a worker thread).
		CoreUtilities.MainThreadOnly(
			astable(typeof(UIManager)),
			nameof(DebugLogEntireUIState),
			nameof(GetRootMovieClip),
			nameof(GetStateMachineCurrentStateId)
#if DEBUG
			,
			nameof(GotoState),
			nameof(ValidateUiFiles)
#endif
		);

		udUIManager = CoreUtilities.NewNativeUserData<Native.ScriptUIManager>();
		if (null == udUIManager)
		{
			error("Failed instantiating ScriptUIManager.");
		}
	}

	public static bool BroadcastEvent(string sEvent, params object[] vararg0)
	{
		var bRet = udUIManager.BroadcastEvent(sEvent, vararg0);
		return bRet;
	}

	public static bool BroadcastEventTo(string sTargetType, string sEvent, params object[] vararg0)
	{
		var bRet = udUIManager.BroadcastEventTo(sTargetType, sEvent, vararg0);
		return bRet;
	}

	public static RootMovieClip GetRootMovieClip(string sStateMachine, string sTarget)
	{
		return udUIManager.GetRootMovieClip(sStateMachine, sTarget);
	}

	public static bool GetCondition(string sName)
	{
		var bCondition = udUIManager.GetCondition(sName);
		return bCondition;
	}

	public static double GetPerspectiveFactorAdjustment()
	{
		var fFactor = udUIManager.GetPerspectiveFactorAdjustment();
		return fFactor;
	}

	public static bool PersistentBroadcastEvent(string sEvent, params object[] vararg0)
	{
		var bRet = udUIManager.PersistentBroadcastEvent(sEvent, vararg0);
		return bRet;
	}

	public static bool PersistentBroadcastEventTo(string sTargetType, string sEvent, params object[] vararg0)
	{
		var bRet = udUIManager.PersistentBroadcastEventTo(sTargetType, sEvent, vararg0);
		return bRet;
	}

	public static void SetCondition(string sName, bool bValue)
	{
		udUIManager.SetCondition(sName, bValue);
	}

	public static void SetPerspectiveFactorAdjustment(double fFactor)
	{
		udUIManager.SetPerspectiveFactorAdjustment(fFactor);
	}

	public static void SetStage3DSettings(string sName)
	{
		udUIManager.SetStage3DSettings(sName);
	}

	public static void TriggerTransition(string sName)
	{
		udUIManager.TriggerTransition(sName);
#if DEBUG
		if (CoreNative.IsLogChannelEnabled(LoggerChannel.UIDebug))
		{
			// Complementary to similar logging that will occur in native,
			// completes the stack.
			CoreNative.LogChannel(LoggerChannel.UIDebug, debug.traceback());
		}
#endif
	}

	public static double GetViewportAspectRatio()
	{
		return udUIManager.GetViewportAspectRatio();
	}

	[Conditional("DEBUG")]
	public static void DebugLogEntireUIState()
	{
		udUIManager.DebugLogEntireUIState();
	}

	public static (double, double) ComputeWorldSpaceDepthProjection(double posX, double posY, double worldDepth)
	{
		return udUIManager.ComputeWorldSpaceDepthProjection(posX, posY, worldDepth);
	}

	public static (double, double) ComputeInverseWorldSpaceDepthProjection(double posX, double posY, double worldDepth)
	{
		return udUIManager.ComputeInverseWorldSpaceDepthProjection(posX, posY, worldDepth);
	}

	public static string GetStateMachineCurrentStateId(string sStateName)
	{
		return udUIManager.GetStateMachineCurrentStateId(sStateName);
	}

	public static void AddToInputWhitelist(MovieClip movieClip)
	{
		udUIManager.AddToInputWhitelist(dyncast<Native.ScriptUIMovieClipInstance>(rawget(movieClip, "m_udNativeInstance")));
	}

	public static void ClearInputWhitelist()
	{
		udUIManager.ClearInputWhitelist();
	}

	public static string GetPlainTextString(string text)
	{
		return udUIManager.GetPlainTextString(text);
	}

	public static bool HasPendingTransitions()
	{
		return udUIManager.HasPendingTransitions();
	}

	public static void RemoveFromInputWhitelist(MovieClip movieClip)
	{
		udUIManager.RemoveFromInputWhitelist(dyncast<Native.ScriptUIMovieClipInstance>(rawget(movieClip, "m_udNativeInstance")));
	}

	public static void SetInputActionsEnabled(bool bEnabled)
	{
		udUIManager.SetInputActionsEnabled(bEnabled);
	}

#if DEBUG
	public static void GotoState(string stateMachineName, string stateName)
	{
		udUIManager.GotoState(stateMachineName, stateName);
	}

	public static bool ValidateUiFiles(string sExcludeWildcard)
	{
		return udUIManager.ValidateUiFiles(sExcludeWildcard);
	}

	public static void ShelveDataForHotLoad(string sId, object data)
	{
		udUIManager.ShelveDataForHotLoad(sId, data);
	}

	public static T UnshelveDataFromHotLoad<T>(string sId)
	{
		return dyncast<T>(udUIManager.UnshelveDataFromHotLoad(sId));
	}
#endif // /#if DEBUG

	/// <summary>
	/// Special handling around condition variables used to control transition from
	/// patching to full game state in a game application.
	/// </summary>
	/// <param name="bForceImmediate">If true, restart occurs without delay. Otherwise, it can be gated by game state.</param>
	public static void TriggerRestart(bool bForceImmediate)
	{
		udUIManager.TriggerRestart(bForceImmediate);
	}
}
