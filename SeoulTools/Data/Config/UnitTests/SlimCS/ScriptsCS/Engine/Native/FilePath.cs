/*
	FilePath.cs
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
	public abstract class FilePath
	{
		interface IStatic
		{
			FilePath CreateConfigFilePath(string a0);
			FilePath CreateContentFilePath(string a0);
		}

		static IStatic s_udStaticApi;

		static FilePath()
		{
			s_udStaticApi = SlimCS.dyncast<IStatic>(CoreUtilities.DescribeNativeUserData("FilePath"));
		}

		public static extern bool operator<(FilePath a0, FilePath a1);
		public static extern bool operator>(FilePath a0, FilePath a1);
		[Pure] public abstract string __tostring();
		[Pure] public abstract int GetDirectory();
		[Pure] public abstract string GetRelativeFilenameWithoutExtension();
		[Pure] public abstract string GetAbsoluteFilename();
		public abstract void SetDirectory(object a0);
		[Pure] public abstract new int GetType();
		public abstract void SetType(object a0);
		[Pure] public abstract string ToSerializedUrl();
		[Pure] public abstract new string ToString();

		public static FilePath CreateConfigFilePath(string a0)
		{
			return s_udStaticApi.CreateConfigFilePath(a0);
		}

		[Pure] public abstract bool IsValid();

		public static FilePath CreateContentFilePath(string a0)
		{
			return s_udStaticApi.CreateContentFilePath(a0);
		}
	}
}
