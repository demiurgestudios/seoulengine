/*
	ScriptEngineRenderer.cs
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
	public abstract class ScriptEngineRenderer
	{
		[Pure] public abstract double GetViewportAspectRatio();
		[Pure] public abstract (double, double) GetViewportDimensions();
		[Pure] public abstract void PrefetchFx(FilePath filePath);
	}
}
