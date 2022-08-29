/*
	ScriptEngineHTTPRequest.cs
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
	public abstract class ScriptEngineHTTPRequest
	{
		public abstract void AddHeader(string a0, string a1);
		public abstract void AddPostData(string a0, string a1);
		public abstract void SetLanesMask(int a0);
		public abstract void Start();
		public abstract void StartWithPlatformSignInIdToken();
		public abstract void Cancel();
	}
}
