/*
	ScriptEngineFileManager.cs
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
	public abstract class ScriptEngineFileManager
	{
		public abstract bool Delete(string a0);
		[Pure] public abstract bool FileExists(object filePathOrFileNameString);
		[Pure] public abstract SlimCS.Table<double, string> GetDirectoryListing(string a0, bool a1, string a2);
		[Pure] public abstract string GetSourceDir();
		[Pure] public abstract string GetVideosDir();
		public abstract bool RenameFile(string a0, string a1);
	}
}
