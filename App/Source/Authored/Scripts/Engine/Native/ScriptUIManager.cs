/*
	ScriptUIManager.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) 2018-2022 Demiurge Studios Inc.  All rights reserved.
*/

using System.Diagnostics.Contracts;

namespace Native
{
	public abstract class ScriptUIManager
	{
		public abstract bool BroadcastEvent(string sEvent, params object[] aArgs);
		public abstract bool BroadcastEventTo(string sEvent, string sTarget, params object[] aArgs);
		[Pure] public abstract RootMovieClip GetRootMovieClip(string sStateMachine, string sTarget);
		[Pure] public abstract bool GetCondition(string a0);
		[Pure] public abstract double GetPerspectiveFactorAdjustment();
		public abstract void SetCondition(string a0, bool a1);
		public abstract bool PersistentBroadcastEvent(string sEvent, params object[] aArgs);
		public abstract bool PersistentBroadcastEventTo(string sEvent, string sTarget, params object[] aArgs);
		public abstract void SetPerspectiveFactorAdjustment(double a0);
		public abstract void SetStage3DSettings(string a0);
		public abstract void TriggerTransition(string a0);
		[Pure] public abstract double GetViewportAspectRatio();
		[Pure] public abstract void DebugLogEntireUIState();
		[Pure] public abstract (double, double) ComputeWorldSpaceDepthProjection(double fX, double fY, double fDepth);
		[Pure] public abstract (double, double) ComputeInverseWorldSpaceDepthProjection(double fX, double fY, double fDepth);
		[Pure] public abstract string GetStateMachineCurrentStateId(string sStateName);
		public abstract void AddToInputWhitelist(Native.ScriptUIMovieClipInstance movieClip);
		public abstract void ClearInputWhitelist();
		public abstract void RemoveFromInputWhitelist(Native.ScriptUIMovieClipInstance movieClip);
		public abstract void SetInputActionsEnabled(bool bEnabled);
		[Pure] public abstract bool HasPendingTransitions();
		public abstract void GotoState(string a0, string a1);
		[Pure] public abstract bool ValidateUiFiles(string sExcludeWildcard);
		public abstract void TriggerRestart(bool a0);
		[Pure] public abstract string GetPlainTextString(string input);
		public abstract void ShelveDataForHotLoad(string sId, object data);
		public abstract object UnshelveDataFromHotLoad(string sId);
	}
}
