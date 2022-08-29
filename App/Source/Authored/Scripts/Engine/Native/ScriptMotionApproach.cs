/*
	ScriptMotionApproach.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) 2018-2022 Demiurge Studios Inc.  All rights reserved.
*/

using System.Diagnostics.Contracts;

namespace Native
{
	public abstract class ScriptMotionApproach : ScriptMotion
	{
		[Pure] public abstract double GetDistanceToTarget();
		public abstract void SetAccelerationMag(double a0);
		public abstract void SetApproachRange(double a0);
		public abstract void SetApproachTarget(double a0, double a1, double a2);
		public abstract void SetEnterApproachRangeCallback(SlimCS.Table a0);
		public abstract void SetLeaveApproachRangeCallback(SlimCS.Table a0);
		public abstract void SetReverseAccelerationMag(double a0);
		public abstract void SetVelocityToFacing(double a0);
	}
}
