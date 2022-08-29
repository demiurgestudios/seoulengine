/*
	ScriptUIAnimation2DNetworkInstance.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) Demiurge Studios, Inc.
	
	This source code is licensed under the MIT license.
	Full license details can be found in the LICENSE file
	in the root directory of this source tree.
*/

using System.Diagnostics.Contracts;

namespace Native
{
	public abstract class ScriptUIAnimation2DNetworkInstance : ScriptUIInstance
	{
		public abstract void AddBoneAttachment(double iIndex, ScriptUIInstance oInstance);
		[Pure] public abstract (string, double) GetActiveStatePath();
		public abstract double GetBoneIndex(string sName);
		public abstract (double, double) GetBonePositionByIndex(double iIndex);
		public abstract (double, double) GetBonePositionByName(string sName);
		[Pure] public abstract string GetActivePalette();
		[Pure] public abstract string GetActiveSkin();
		public abstract (double, double) GetLocalBonePositionByIndex(double iIndex);
		public abstract (double, double) GetLocalBonePositionByName(string sName);
		public abstract (double, double) GetLocalBoneScaleByIndex(double iIndex);
		public abstract (double, double) GetLocalBoneScaleByName(string sName);
		[Pure] public abstract double GetCurrentMaxTime();
		[Pure] public abstract double? GetTimeToEvent(string sEventName);
		public abstract (double, double) GetWorldSpaceBonePositionByIndex(double iIndex);
		public abstract (double, double) GetWorldSpaceBonePositionByName(string sName);
		[Pure] public abstract (bool, bool) AllDonePlaying();
		[Pure] public abstract bool IsInStateTransition();
		[Pure] public abstract bool IsReady();
		public abstract void SetCondition(string a0, bool a1);
		public abstract void SetParameter(string a0, double a1);
		public abstract void SetActivePalette(string a0);
		public abstract void SetActiveSkin(string a0);
		public abstract void SetVariableTimeStep(bool a0);
		public abstract void TriggerTransition(string a0);
		public abstract void AddTimestepOffset(double a0);
		public abstract void SetCastShadow(bool a0);
		public abstract void SetShadowOffset(double? fX = null, double? fY = null);
	}
}
