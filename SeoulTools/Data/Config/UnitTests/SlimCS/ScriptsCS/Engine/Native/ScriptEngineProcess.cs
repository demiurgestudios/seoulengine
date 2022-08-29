/*
	ScriptEngineProcess.cs
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
	public abstract class ScriptEngineProcess
	{
		public abstract bool CheckRunning();
		[Pure] public abstract int GetReturnValue();
		public abstract bool Kill(int a0);
		public abstract void SetStdErrChannel(object a0);
		public abstract void SetStdOutChannel(object a0);
		public abstract bool Start();
		public abstract int WaitUntilProcessIsNotRunning();
	}
}
