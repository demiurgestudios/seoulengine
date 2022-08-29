/*
	ScriptUIFxInstance.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) 2018-2022 Demiurge Studios Inc.  All rights reserved.
*/

using System.Diagnostics.Contracts;

namespace Native
{
	public abstract class ScriptUIFxInstance : ScriptUIInstance
	{
		[Pure] public abstract double GetDepth3D();
		[Pure] public abstract SlimCS.Table GetProperties();
		public abstract void SetDepth3D(double a0);
		public abstract void SetDepth3DBias(double a0);
		public abstract void SetDepth3DNativeSource(SlimCS.Table a0);
		public abstract bool SetRallyPoint(double a0, double a1);
		public abstract void SetTreatAsLooping(bool a0);
		public abstract void Stop(bool a0);
	}
}
