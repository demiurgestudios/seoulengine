/*
	ScriptUIMovieClipInstance.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) 2018-2022 Demiurge Studios Inc.  All rights reserved.
*/

using System.Diagnostics.Contracts;

namespace Native
{
	public abstract class ScriptUIMovieClipInstance : ScriptUIInstance
	{
		public abstract void AddChild(ScriptUIInstance oInstance, string sName = null, double iDepth = -1);

		public abstract DisplayObject AddChildBitmap(
			FilePath filePath,
			double iWidth,
			double iHeight,
			string sName = null,
			bool? bCenter = null,
			double iDepth = -1,
			bool bPrefetch = false);

		public abstract DisplayObject AddChildFacebookBitmap(string sFacebookUserGuid, double iWidth, double iHeight, FilePath defaultImageFilePath);

		public abstract FxDisplayObject AddChildFx(
			object fxNameOrFilePath,
			double iWidth,
			double iHeight,
			FxFlags? iFxFlags = null,
			Native.ScriptUIInstance udChildNativeInstance = null,
			string sVariationName = null);

		public abstract DisplayObject AddChildHitShape(double fLeft, double fTop, double fRight, double fBottom, string sName = null, double iDepth = -1);
		public abstract DisplayObject AddChildHitShapeFullScreen(string sName = null);
		public abstract DisplayObject AddChildHitShapeWithMyBounds(string sName = null);
		public abstract DisplayObject AddChildStage3D(FilePath filePath, double iWidth, double iHeight, string sName = null, bool? bCenter = null);
		public abstract void AddFullScreenClipper();
		[Pure] public abstract bool GetAbsorbOtherInput();
		[Pure] public abstract int GetChildCount();
		[Pure] public abstract int GetCurrentFrame();
		[Pure] public abstract string GetCurrentLabel();
		[Pure] public abstract double GetDepth3D();
		[Pure] public abstract bool GetExactHitTest();
		[Pure] public abstract bool GetHitTestChildren();
		[Pure] public abstract int GetHitTestChildrenMode();
		[Pure] public abstract bool GetHitTestSelf();
		[Pure] public abstract int GetHitTestSelfMode();
		[Pure] public abstract DisplayObject GetChildAt(double iIndex);
		[Pure] public abstract DisplayObject GetChildByNameFromSubTree(string sName);
		[Pure] public abstract DisplayObject GetChildByPath(params string[] asParts);
		[Pure] public abstract int GetTotalFrames();
		[Pure] public abstract (double, double, double, double) GetHitTestableBounds(double iMask);
		[Pure] public abstract (double, double, double, double) GetHitTestableLocalBounds(double iMask);
		[Pure] public abstract (double, double, double, double) GetHitTestableWorldBounds(double iMask);
		public abstract bool GotoAndPlay(double iFrame);
		public abstract bool GotoAndPlayByLabel(string sLabel);
		public abstract bool GotoAndStop(double iFrame);
		public abstract bool GotoAndStopByLabel(string sLabel);
		[Pure] public abstract bool IsPlaying();
		[Pure] public abstract DisplayObject HitTest(double iMask, double? fX = null, double? fY = null);
		public abstract void IncreaseAllChildDepthByOne();
		public abstract void Play();
		public abstract void RemoveAllChildren();
		public abstract bool RemoveChildAt(double iIndex);
		public abstract bool RemoveChildByName(string sName);
		public abstract void RestoreTypicalDefault();
		public abstract void SetAbsorbOtherInput(bool a0);
		public abstract void SetAutoCulling(bool a0);
		public abstract void SetAutoDepth3D(bool a0);
		public abstract void SetCastPlanarShadows(bool a0);
		public abstract void SetDeferDrawing(bool a0);
		public abstract void SetDepth3D(double a0);
		public abstract void SetDepth3DFromYLoc(double a0);
		public abstract void SetEnterFrame(bool a0);
		public abstract void SetInputActionDisabled(bool a0);
		public abstract void SetTickEvent(bool a0);
		public abstract void SetTickScaledEvent(bool a0);
		public abstract void SetExactHitTest(bool a0);
		public abstract void SetHitTestChildren(bool a0);
		public abstract void SetHitTestChildrenMode(int a0);
		public abstract void SetHitTestSelf(bool a0);
		public abstract void SetHitTestSelfMode(int a0);
		public abstract void SetReorderChildrenFromDepth3D(bool a0);
		public abstract void Stop();
		public abstract void SetChildrenVisible(bool a0);
	}
}
