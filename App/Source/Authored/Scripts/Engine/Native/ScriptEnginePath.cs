/*
	ScriptEnginePath.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) 2018-2022 Demiurge Studios Inc.  All rights reserved.
*/

using System.Diagnostics.Contracts;

namespace Native
{
	public abstract class ScriptEnginePath
	{
		[Pure] public abstract string Combine(params string[] asArgs);
		[Pure] public abstract string CombineAndSimplify(params string[] asArgs);
		[Pure] public abstract string GetDirectoryName(string a0);
		[Pure] public abstract string GetExactPathName(string a0);
		[Pure] public abstract string GetExtension(string a0);
		[Pure] public abstract string GetFileName(string a0);
		[Pure] public abstract string GetFileNameWithoutExtension(string a0);
		[Pure] public abstract string GetPathWithoutExtension(string a0);
		[Pure] public abstract string GetTempFileAbsoluteFilename();
		[Pure] public abstract string Normalize(string a0);
	}
}
