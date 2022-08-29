/*
	ScriptEngineCamera.cs
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
	public abstract class ScriptEngineCamera
	{
		public abstract void Animate(double a0, double a1, double a2, double a3, double a4, double a5, double a6, double a7, double a8, double a9);
		[Pure] public abstract bool GetEnabled();
		public abstract void SetEnabled(bool a0);
		[Pure] public abstract (double, double, double, double) GetRelativeViewport();
		public abstract void SetRelativeViewport(double a0, double a1, double a2, double a3);
		[Pure] public abstract (double, double, double, double) GetRotation();
		public abstract void SetRotation(double a0, double a1, double a2, double a3);
		[Pure] public abstract (double, double, double) GetPosition();
		public abstract void SetPosition(double a0, double a1, double a2);
		public abstract void SetPerspective(double a0, double a1, double a2, double a3);
		public abstract void SetAsAudioListenerCamera();
	}
}
