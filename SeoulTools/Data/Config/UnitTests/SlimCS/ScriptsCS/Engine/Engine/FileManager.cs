// Access to the native FileManager singleton.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public static class FileManager
{
	static readonly Native.ScriptEngineFileManager udFileManager = null;

	static FileManager()
	{
		udFileManager = CoreUtilities.NewNativeUserData<Native.ScriptEngineFileManager>();
		if (null == udFileManager)
		{
			error("Failed instantiating ScriptEngineFileManager.");
		}
	}

	public static bool FileExists(object filePathOrFileName)
	{
		var bRet = udFileManager.FileExists(filePathOrFileName);
		return bRet;
	}

	public static Table GetDirectoryListing(string sPath, bool bRecursive, string sFileExtension)
	{
		return udFileManager.GetDirectoryListing(sPath, bRecursive, sFileExtension);
	}

	public static string GetSourceDir()
	{
		var sRet = udFileManager.GetSourceDir();
		return sRet;
	}

	public static string GetVideosDir()
	{
		var sRet = udFileManager.GetVideosDir();
		return sRet;
	}
}
