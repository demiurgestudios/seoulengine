// SeoulEngine path manipulation and sanitizer functions.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public static class Path
{
	static readonly Native.ScriptEnginePath udPath = null;

	static Path()
	{
		udPath = CoreUtilities.NewNativeUserData<Native.ScriptEnginePath>();
		if (null == udPath)
		{
			error("Failed instantiating ScriptEnginePath.");
		}
	}

	public static string Combine(params string[] aArgs)
	{
		var sRet = udPath.Combine(aArgs);
		return sRet;
	}

	public static string CombineAndSimplify(params string[] aArgs)
	{
		var sRet = udPath.CombineAndSimplify(aArgs);
		return sRet;
	}

	public static string GetDirectoryName(string sPath)
	{
		var sRet = udPath.GetDirectoryName(sPath);
		return sRet;
	}

	public static string GetExactPathName(string sPath)
	{
		var sRet = udPath.GetExactPathName(sPath);
		return sRet;
	}

	public static string GetExtension(string sPath)
	{
		var sRet = udPath.GetExtension(sPath);
		return sRet;
	}

	public static string GetFileName(string sPath)
	{
		var sRet = udPath.GetFileName(sPath);
		return sRet;
	}

	public static string GetFileNameWithoutExtension(string sPath)
	{
		var sRet = udPath.GetFileNameWithoutExtension(sPath);
		return sRet;
	}

	public static string GetPathWithoutExtension(string sPath)
	{
		var sRet = udPath.GetPathWithoutExtension(sPath);
		return sRet;
	}

	public static string Normalize(string sPath)
	{
		var sRet = udPath.Normalize(sPath);
		return sRet;
	}
}
