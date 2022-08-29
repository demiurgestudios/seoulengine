// Access to native WordFilter objects.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public static class WordFilter
{
	public static Native.ScriptEngineWordFilter Load(string configPath)
	{
		var udWordFilter = CoreUtilities.NewNativeUserData<Native.ScriptEngineWordFilter>(configPath);
		if (null == udWordFilter)
		{
			error("Failed instantiating ScriptEngineWordFilter.");
		}

		return udWordFilter;
	}
}
