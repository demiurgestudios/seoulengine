/*
	ScriptUIBitmapInstance.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) Demiurge Studios, Inc.
	
	This source code is licensed under the MIT license.
	Full license details can be found in the LICENSE file
	in the root directory of this source tree.
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
