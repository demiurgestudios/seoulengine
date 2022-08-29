// Placeholder API for the native ScriptUIAnimation2DNetworkInstance.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

using System.Diagnostics;

#if SEOUL_WITH_ANIMATION_2D

public sealed class Animation2DNetwork : DisplayObject
{
	#region Private member
	// Utility to check for ready state - all methods in this instance
	// depend on IsReady() and we raise an error if they're called
	// otherwise (the Animation2DNetworkAsync utility is used
	// to enforce this in practice).
	[Conditional("DEBUG")]
	void CheckReady()
	{
		if (!dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).IsReady())
		{
			error($"Animation2DNetwork instance '{GetName()}' is not ready.", 1);
		}
	}
	#endregion

	public string ActivePalette
	{
		get
		{
			CheckReady();
			return dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).GetActivePalette();
		}

		set
		{
			CheckReady();
			dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).SetActivePalette(value);
		}
	}

	public string ActiveSkin
	{
		get
		{
			CheckReady();
			return dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).GetActiveSkin();
		}

		set
		{
			CheckReady();
			dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).SetActiveSkin(value);
		}
	}

	public (string, double) GetActiveStatePath()
	{
		CheckReady();
		return dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).GetActiveStatePath();
	}

	public bool IsReady
	{
		get
		{
			return dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).IsReady();
		}
	}

	public void AddBoneAttachment(double iIndex, DisplayObject attachment)
	{
		CheckReady();
		dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).AddBoneAttachment(
			iIndex,
			attachment.NativeInstance);
	}

	public void AddTimestepOffset(double fTimestepOffset)
	{
		CheckReady();
		dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).AddTimestepOffset(fTimestepOffset);
	}

	public (bool, bool) AllDonePlaying()
	{
		CheckReady();
		return dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).AllDonePlaying();
	}

	public double CurrentMaxTime
	{
		get
		{
			CheckReady();
			return dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).GetCurrentMaxTime();
		}
	}

	public double GetBoneIndex(string sName)
	{
		CheckReady();
		return dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).GetBoneIndex(sName);
	}

	public (double, double) GetBonePositionByName(string sName)
	{
		CheckReady();
		return dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).GetBonePositionByName(sName);
	}

	public (double, double) GetBonePositionByIndex(double iIndex)
	{
		CheckReady();
		return GetBonePositionByIndex(iIndex);
	}

	public (double, double) GetLocalBonePositionByName(string sName)
	{
		CheckReady();
		return dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).GetLocalBonePositionByName(sName);
	}

	public (double, double) GetLocalBonePositionByIndex(double iIndex)
	{
		CheckReady();
		return dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).GetLocalBonePositionByIndex(iIndex);
	}

	public (double, double) GetLocalBoneScaleByIndex(double iIndex)
	{
		CheckReady();
		return dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).GetLocalBoneScaleByIndex(iIndex);
	}

	public (double, double) GetLocalBoneScaleByName(string sName)
	{
		CheckReady();
		return dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).GetLocalBoneScaleByName(sName);
	}

	public (double, double) GetWorldSpaceBonePositionByIndex(double iIndex)
	{
		CheckReady();
		return dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).GetWorldSpaceBonePositionByIndex(iIndex);
	}

	public (double, double) GetWorldSpaceBonePositionByName(string sName)
	{
		CheckReady();
		return dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).GetWorldSpaceBonePositionByName(sName);
	}

	public double? GetTimeToEvent(string sEventName)
	{
		CheckReady();
		return dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).GetTimeToEvent(sEventName);
	}

	public bool IsInStateTransition
	{
		get
		{
			CheckReady();
			return dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).IsInStateTransition();
		}
	}

	public void SetCastShadow(bool bCast)
	{
		CheckReady();
		dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).SetCastShadow(bCast);
	}

	public void SetCondition(string sName, bool bValue)
	{
		CheckReady();
		dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).SetCondition(sName, bValue);
	}

	public void SetShadowOffset(double fX, double fY)
	{
		CheckReady();
		dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).SetShadowOffset(fX, fY);
	}

	public void SetParameter(string sName, double fValue)
	{
		CheckReady();
		dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).SetParameter(sName, fValue);
	}

	public void TriggerTransition(string sName)
	{
		CheckReady();
		dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).TriggerTransition(sName);
	}

	public bool VariableTimeStep
	{
		set
		{
			CheckReady();
			dyncast<Native.ScriptUIAnimation2DNetworkInstance>(m_udNativeInstance).SetVariableTimeStep(value);
		}
	}
}

#endif // /#if SEOUL_WITH_ANIMATION_2D
