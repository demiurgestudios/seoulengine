/*
	ScriptUIBitmapInstance.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) 2018-2022 Demiurge Studios Inc.  All rights reserved.
*/

namespace Native
{
	public abstract class ScriptUIBitmapInstance : ScriptUIInstance
	{
		public abstract void ResetTexture();
		public abstract void SetIndirectTexture(string symbol, double iWidth, double iHeight);
		public abstract void SetTexture(FilePath filePath, double iWidth, double iHeight, bool bPrefetch);
	}
}
