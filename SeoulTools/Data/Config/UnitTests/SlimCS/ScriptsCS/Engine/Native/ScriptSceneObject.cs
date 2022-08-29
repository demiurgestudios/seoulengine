/*
	ScriptSceneObject.cs
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
	public abstract class ScriptSceneObject
	{
		[Pure] public abstract string GetId();
		public abstract string ResolveRelativeId(string a0);
		[Pure] public abstract void GetPosition(SlimCS.Table a0);
		[Pure] public abstract void GetRotation(SlimCS.Table a0);
		[Pure] public abstract double GetFxDuration();
		public abstract void SetLookAt(double a0, double a1, double a2);
		public abstract void SetPosition(double a0, double a1, double a2);
		public abstract void SetRotation(double a0, double a1, double a2, double a3);
		public abstract bool StartFx();
		public abstract void StopFx(SlimCS.Table a0);
		public abstract void SetMeshVisible(bool a0);
		[Pure] public abstract void TransformPosition(SlimCS.Table a0);
	}
}
