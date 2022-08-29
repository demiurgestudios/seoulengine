/*
	ScriptUIInstance.cs
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
	public abstract class ScriptUIInstance
	{
		public abstract int AddMotion(string sType, SlimCS.Vfunc0 callback, params object[] aArgs);
		public abstract void CancelMotion(int a0);
		public abstract int AddTween(TweenTarget eTarget, params object[] aArgs);
		public abstract int AddTweenCurve(TweenType eType, TweenTarget eTarget, params object[] aArgs);
		public abstract void CancelTween(int a0);
		[Pure] public abstract DisplayObject Clone();
		[Pure] public abstract bool GetAdditiveBlend();
		[Pure] public abstract double GetAlpha();
		[Pure] public abstract (double, double, double, double, double, double) GetColorTransform();
		[Pure] public abstract (double, double, double, double) GetBounds();
		[Pure] public abstract (double, double, double, double) GetBoundsIn(ScriptUIInstance targetCoordinateSpace);
		[Pure] public abstract int GetClipDepth();
		[Pure] public abstract int GetDepthInParent();
		[Pure] public abstract bool GetIgnoreDepthProjection();
		[Pure] public abstract (double, double, double, double) GetLocalBounds();
		[Pure] public abstract string GetName();
		[Pure] public abstract string GetFullName();
		[Pure] public abstract MovieClip GetParent();
		[Pure] public abstract (double, double) GetPosition();
		[Pure] public abstract double GetPositionX();
		[Pure] public abstract double GetPositionY();
		[Pure] public abstract double GetRotation();
		[Pure] public abstract (double, double) GetScale();
		[Pure] public abstract double GetScaleX();
		[Pure] public abstract double GetScaleY();
		[Pure] public abstract bool GetScissorClip();
		[Pure] public abstract bool GetVisible();
		[Pure] public abstract bool GetVisibleToRoot();
		[Pure] public abstract (double, double, double, double) GetWorldBounds();
		[Pure] public abstract (double, double) GetWorldPosition();
		[Pure] public abstract (double, double) LocalToWorld(double fX, double fY);
		[Pure] public abstract bool HasParent();
		public abstract bool RemoveFromParent();
		public abstract void SetAdditiveBlend(bool a0);
		public abstract void SetAlpha(double a0);
		public abstract void SetClipDepth(int a0);
		public abstract void SetColorTransform(double a0, double a1, double a2, int a3, int a4, int a5);
		public abstract void SetIgnoreDepthProjection(bool a0);
		public abstract void SetName(string a0);
		public abstract void SetPosition(double a0, double a1);
		public abstract void SetPositionX(double a0);
		public abstract void SetPositionY(double a0);
		public abstract void SetRotation(double a0);
		public abstract void SetScale(double a0, double a1);
		public abstract void SetScaleX(double a0);
		public abstract void SetScaleY(double a0);
		public abstract void SetScissorClip(bool a0);
		public abstract void SetVisible(bool a0);
		public abstract void SetWorldPosition(double a0, double a1);
		[Pure] public abstract double GetWorldDepth3D();
		[Pure] public abstract (double, double) WorldToLocal(double fX, double fY);
		[Pure] public abstract (double, double) GetLocalMousePosition();
		[Pure] public abstract bool Intersects(double fWorldX, double fWorldY, bool bExactHitTest = false);
		[Pure] public abstract string GetDebugName();
		public abstract void SetDebugName(string a0);
	}
}
