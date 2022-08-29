// Access to the native Renderer singleton.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public static class Renderer
{
	static readonly Native.ScriptEngineRenderer udRenderer = null;

	static Renderer()
	{
		udRenderer = CoreUtilities.NewNativeUserData<Native.ScriptEngineRenderer>();
		if (null == udRenderer)
		{
			error("Failed instantiating ScriptEngineRenderer.");
		}
	}

	public static double GetViewportAspectRatio()
	{
		var fRet = udRenderer.GetViewportAspectRatio();
		return fRet;
	}

	public static (double, double) GetViewportDimensions()
	{
		(double varX, double varY) = udRenderer.GetViewportDimensions();
		return (varX, varY);
	}

	public static void PrefetchFx(Native.FilePath filePath)
	{
		udRenderer.PrefetchFx(filePath);
	}
}
