// Root MovieClip that allows non movie clips to register for ticks and
// register broadcast handlers. Registering must happen at the class
// level rather than the instance level
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public class GlobalBroadcastHandler : RootMovieClip
{
	public delegate void MovieClipVararg(MovieClip oMovieClip, params object[] aArgs);

	public delegate void TickFunctionDelegate();

	static SlimCS.Table BroadcastList = new Table { };
	static TickFunctionDelegate TickFunction = null;

	public GlobalBroadcastHandler()
	{
		// Take the list of registered handlers and set them to be functions
		//  on this instance
		foreach ((var sKey, var oFn) in pairs(BroadcastList))
		{
			dyncast<Table>(this)[sKey] = (MovieClipVararg)((_, vararg0) =>
			{
				dyncast<Vvargs>(oFn)(vararg0);
			});
		}

		// Call registered tick functions
		AddEventListener(
			Event.TICK,
			(MovieClipVararg)((oMovieClip, aArgs) =>
			{
				if (null != TickFunction)
				{
					TickFunction();
				}
			}));
	}

	// Register broadcast event handlers
	public static void RegisterGlobalBroadcastEventHandler(string sKey, object oFunction)
	{
		if (null != sKey)
		{
			if (null != BroadcastList[sKey])
			{
				var oOriginalFunction = BroadcastList[sKey];
				BroadcastList[sKey] = (Vvargs)((vararg0) =>
				{
					dyncast<Vvargs>(oOriginalFunction)(vararg0);
					dyncast<Vvargs>(oFunction)(vararg0);
				});
			}
			else
			{
				BroadcastList[sKey] = oFunction;
			}
		}
	}

	// Register tick functions
	public static void RegisterGlobalTickEventHandler(TickFunctionDelegate oFunction)
	{
		if (null != TickFunction)
		{
			var oOriginalFunction = TickFunction;
			TickFunction = (TickFunctionDelegate)(() =>
			{
				oOriginalFunction();
				oFunction();
			});
		}
		else
		{
			TickFunction = oFunction;
		}
	}
}
