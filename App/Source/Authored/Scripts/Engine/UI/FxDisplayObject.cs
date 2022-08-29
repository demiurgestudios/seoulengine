// Placeholder API for the native ScriptUIFxInstance.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public sealed class FxDisplayObject : DisplayObject
{
	// TODO: Remove the need for this hook. It's low-level
	// and unexpected in SlimCS.
	//
	// __cinit when defined is run on the class table immediately
	// before super propagation occurs. Meaning, the class is
	// not yet constructed (the static constructor has not run yet)
	// but all members defined below will have already been added
	static void __cinit()
	{
		// Get the native ScriptUIFxInstance description and bind
		// wrapper methods into the FxDisplayObject class description.
		{
			var tUserDataDesc = CoreUtilities.DescribeNativeUserData("ScriptUIFxInstance");
			foreach ((var name, var func) in pairs(tUserDataDesc))
			{
				if (type(func) == "function")
				{
					rawset(
						astable(typeof(FxDisplayObject)),
						name,
						(NativeWrapper)((self, vararg0) =>
						{
							var udNativeInstance = (DisplayObject)rawget(self, "m_udNativeInstance");
							return dyncast<NativeWrapper>(func)(udNativeInstance, vararg0);
						}));
				}
			}
		}
	}

	public void SetDepth3DSource(DisplayObject source)
	{
		var udSourceInstance = dyncast<Native.ScriptUIInstance>(rawget(source, "m_udNativeInstance"));
		SetDepth3DNativeSource(udSourceInstance);
	}

	// These functions are defined as placeholders to allow concrete
	// definitions in script. They will be replaced with their native
	// hookups.
	//
	// PLACEHOLDERS
	public double GetDepth3D()
	{
		error("placeholder not replaced");
		return 0;
	}

	public Table GetProperties()
	{
		error("placeholder not replaced");
		return null;
	}

	public void SetDepth3D(double fDepth3D)
	{
		var _ = fDepth3D;
		error("placeholder not replaced");
	}

	public void SetDepth3DBias(double fDepth3DBias)
	{
		var _ = fDepth3DBias;
		error("placeholder not replaced");
	}

	public void SetDepth3DNativeSource(Native.ScriptUIInstance source)
	{
		var _ = source;
		error("placeholder not replaced");
	}

	public bool SetRallyPoint(double fX, double fY)
	{
		(var _, var _) = (fX, fY);
		error("placeholder not replaced");
		return false;
	}

	public void SetTreatAsLooping(bool b)
	{
		var _ = b;
		error("placeholder not replaced");
	}

	public void Stop(bool bStopImmediately = false)
	{
		var _ = bStopImmediately;
		error("placeholder not replaced");
	}

	// END PLACEHOLDERS
}
