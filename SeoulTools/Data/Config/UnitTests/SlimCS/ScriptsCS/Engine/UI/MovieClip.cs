// MovieClip, the base class of any script wrapper
// around a Falcon::MovieClipInstance.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

/// <summary>
/// Class types that associate contextual information
/// with the setter of a game state data value.
/// </summary>
/// <remarks>
/// Context is used for developer hot loading, to determine
/// when and if a game state data value should be backed up
/// and restored.
/// </remarks>
public interface IContextualGameStateSetter
{
	void SetGameStateValue(string k, object v);
}

public class MovieClip : DisplayObject, IContextualGameStateSetter
{
	delegate object BindAndConstructRootMovieClipDelegate(object udNativeInterface, Native.ScriptUIInstance udNativeInstance, string sOptionalClassName);

	delegate object GetSetterFunctionDelegate(string sName);

	public delegate bool AllowClickPassthroughToProceedDelegate(MovieClip oMovieClip, double iX, double iY);

	protected static string ALLOW_CLICK_PASSTHROUGH_EVENT = "allowclickpassthrough";

	// utility, equivalent to the implementation of New in class,
	// except that the constructor name is always 'Constructor'.
	static void InvokeConstructor(Table tClass, DisplayObject oInstance, params object[] vararg0)
	{
		if (null == tClass)
		{
			return;
		}

		var ctor = tClass["Constructor"];
		if (null != ctor)
		{
			dyncast<ConstructorDelegate>(ctor)(oInstance, vararg0);
		}
	}

	static MovieClip InPlaceNew(Table tClass, MovieClip oInstance, params object[] vararg0)
	{
		setmetatable(oInstance, tClass);
		InvokeConstructor(tClass, oInstance, vararg0);
		return oInstance;
	}

	public void AddChild(DisplayObject oDisplayObject, string sName = null, int iDepth = -1)
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).AddChild(
			oDisplayObject.NativeInstance,
			sName,
			iDepth);
	}

#if SEOUL_WITH_ANIMATION_2D
	public Animation2DNetworkAsync AddChildAnimationNetwork(
		Native.FilePath networkFilePath,
		Native.FilePath dataFilePath,
		object callback = null,
		bool bStartAlphaZero = false)
	{
		var network = dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).AddChildAnimationNetwork(
			networkFilePath,
			dataFilePath,
			callback);

		// Use alpha 0 here instead of visible - for legacy reasons (from ActionScript),
		// visibility is effectively "enabled/disabled". It disables update actions in
		// addition to rendering visibility.
		if (bStartAlphaZero)
		{
			network.SetAlpha(0);
		}

		return new Animation2DNetworkAsync(this, network);
	}
#endif // /#if SEOUL_WITH_ANIMATION_2D

	public DisplayObject AddChildBitmap(
		Native.FilePath filePath,
		double iWidth,
		double iHeight,
		string sName = null,
		bool? bCenter = null,
		double iDepth = -1,
		bool bPrefetch = false)
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).AddChildBitmap(
			filePath,
			iWidth,
			iHeight,
			sName,
			bCenter,
			iDepth,
			bPrefetch);
	}

	public DisplayObject AddChildFacebookBitmap(
		string sFacebookUserGuid,
		double iWidth,
		double iHeight,
		Native.FilePath defaultImageFilePath)
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).AddChildFacebookBitmap(
			sFacebookUserGuid,
			iWidth,
			iHeight,
			defaultImageFilePath);
	}

	public FxDisplayObject AddChildFx(
		object fxNameOrFilePath,
		double iWidth,
		double iHeight,
		FxFlags? iFxFlags = null,
		MovieClip oParentIfWorldspace = null,
		string sVariationName = null)
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).AddChildFx(
			fxNameOrFilePath,
			iWidth,
			iHeight,
			iFxFlags,
			oParentIfWorldspace?.m_udNativeInstance,
			sVariationName);
	}

	public DisplayObject AddChildHitShape(
		double fLeft,
		double fTop,
		double fRight,
		double fBottom,
		string sName = null,
		double iDepth = -1)
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).AddChildHitShape(
			fLeft,
			fTop,
			fRight,
			fBottom,
			sName,
			iDepth);
	}

	public DisplayObject AddChildHitShapeFullScreen(string sName = null)
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).AddChildHitShapeFullScreen(sName);
	}

	public DisplayObject AddChildHitShapeWithMyBounds(string sName = null)
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).AddChildHitShapeWithMyBounds(sName);
	}

	public DisplayObject AddChildStage3D(
		Native.FilePath filePath,
		double iWidth,
		double iHeight,
		string sName = null,
		bool? bCenter = null)
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).AddChildStage3D(
			filePath,
			iWidth,
			iHeight,
			sName,
			bCenter);
	}

	public (double, double, double, double) GetHitTestableBounds(double iMask)
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).GetHitTestableBounds(iMask);
	}

	public (double, double, double, double) GetHitTestableLocalBounds(double iMask)
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).GetHitTestableLocalBounds(iMask);
	}

	public (double, double, double, double) GetHitTestableWorldBounds(double iMask)
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).GetHitTestableWorldBounds(iMask);
	}

	public (double, double, double, double) GetBoundsUsingAuthoredBoundsIfPresent(bool bWorldBounds = false)
	{
		var bounds = GetChildByPath<MovieClip>("mcBounds");
		if (bWorldBounds)
		{
			if (null != bounds)
			{
				return bounds.GetWorldBounds();
			}
			return GetWorldBounds();
		}

		if (null != bounds)
		{
			(var fLeft, var fTop, var fRight, var fBottom) = bounds.GetBounds();
			(var fPosX, var fPosY) = GetPosition();
			(var fScaleX, var fScaleY) = GetScale();
			fLeft = fScaleX * fLeft + fPosX;
			fRight = fScaleX * fRight + fPosX;
			fTop = fScaleY * fTop + fPosY;
			fBottom = fScaleY * fBottom + fPosY;
			return (fLeft, fTop, fRight, fBottom);
		}
		return GetBounds();
	}

	// See DisplayObject::GetBoundsIn for more information
	public (double, double, double, double) GetBoundsInUsingAuthoredBoundsIfPresent(DisplayObject targetCoordinateSpace)
	{
		var bounds = GetChildByPath<MovieClip>("mcBounds");
		if (null != bounds)
		{
			return bounds.GetBoundsIn(targetCoordinateSpace);
		}
		return GetBoundsIn(targetCoordinateSpace);
	}

	public (double, double, double, double) GetHitTestableBoundsUsingAuthoredBoundsIfPresent(double iHitMask, bool bWorldBounds = false)
	{
		var bounds = GetChildByPath<MovieClip>("mcBounds");
		if (bWorldBounds)
		{
			if (null != bounds)
			{
				return bounds.GetHitTestableWorldBounds(iHitMask);
			}
			return GetWorldBounds();
		}

		if (null != bounds)
		{
			(var fLeft, var fTop, var fRight, var fBottom) = bounds.GetHitTestableBounds(iHitMask);
			(var fPosX, var fPosY) = GetPosition();
			(var fScaleX, var fScaleY) = GetScale();
			fLeft = fScaleX * fLeft + fPosX;
			fRight = fScaleX * fRight + fPosX;
			fTop = fScaleY * fTop + fPosY;
			fBottom = fScaleY * fBottom + fPosY;
			return (fLeft, fTop, fRight, fBottom);
		}
		return GetHitTestableBounds(iHitMask);
	}

	/// <summary>
	/// Retrieve a child by absolute index.
	/// </summary>
	/// <param name="iIndex">0-based index of the child DisplayObject to retrieve.</param>
	/// <returns>A DisplayObject at the given index or null if an invalid index.</returns>
	public DisplayObject GetChildAt(double iIndex)
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).GetChildAt(iIndex);
	}

	/// <summary>
	/// Retrieve a child DisplayObject of the current MovieClip and cast it to the given type.
	/// </summary>
	/// <typeparam name="T">Coerced target type of the DisplayObject.</typeparam>
	/// <param name="asParts">Path to the child to retrieve.</param>
	/// <returns>The target DisplayObject or nil if no such DisplayObject.</returns>
	/// <remarks>
	/// The instance retrieve will be wrapped in a script class binding as needed
	/// (e.g. if the return instance is of type Button, a new Button script class
	/// will be instantiated and wrapped around the native instance).
	/// 
	/// Note that the child is expected to already be of that type and this will
	/// not allow you to convert to a more derived type.
	/// (IE making a MovieClip into something more specific will not work).
	/// </remarks>
	public T GetChildByPath<T>(params string[] asParts)
		where T : DisplayObject
	{
		var child = dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).GetChildByPath(asParts);
#if DEBUG
		return (T)child;
#else
		return dyncast<T>(child);
#endif
	}

	public DisplayObject GetChildByName(string sName)
	{
		return GetChildByPath<DisplayObject>(sName);
	}

	public DisplayObject GetChildByNameFromSubTree(string sName)
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).GetChildByNameFromSubTree(sName);
	}

	/// <summary>
	/// Retrieve the 0-based index of the child in the current
	/// parent.
	/// </summary>
	/// <param name="child">The child instance to search for.</param>
	/// <returns>The 0-based index if found, or -1 if the child is not a child of this parent.</returns>
	public int GetChildIndex(DisplayObject child)
	{
		var iCount = GetChildCount();
		for (int i = 0; i < iCount; ++i)
		{
			var d = GetChildAt(i);
			if (d == child)
			{
				return i;
			}
		}

		return -1;
	}

	public void IncreaseAllChildDepthByOne()
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).IncreaseAllChildDepthByOne();
	}

	/// <summary>
	/// Alternative to AddChildBitmap that returns an unparented
	/// Bitmap instance with no configuration.
	/// </summary>
	/// <returns>An unparented Bitmap instance.</returns>
	public Bitmap NewBitmap()
	{
		return m_udNativeInterface.NewBitmap();
	}

	public MovieClip NewMovieClip(string sClassName, params object[] varargs)
	{
		return m_udNativeInterface.NewMovieClip(sClassName, varargs);
	}

	public T CreateMovieClip<T>(params object[] varargs)
	{
		return dyncast<T>(m_udNativeInterface.NewMovieClip(typeof(T).Name, varargs));
	}

	// Fire an 'out-of-band', manual hit test against the Falcon scene graph,
	// starting at self. Not part of normal input handling, can be used for
	// special cases.
	public DisplayObject HitTest(double iMask, double? fX = null, double? fY = null)
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).HitTest(
			iMask,
			fX,
			fY);
	}

	// UI system BroadcastEvent calls are normally delivered only to the root
	// MovieClip. This function hooks a function into the root MovieClip that
	// will forward BroadcastEvent calls to self.
	public void RegisterBroadcastEventHandlerWithRootMovie(string sKey, object oFunction)
	{
		if (null != sKey)
		{
			var oRootMC = GetRootMovieClip();

			if (null == oRootMC)
			{
				CoreNative.Warn(
					$"RegisterBroadcastEventHandlerWithRootMovie failed to register key '{sKey}'" +
					$" because no root movie clip was found for MovieClip {GetName()}. " +
					"Did you attempt to register a movie clip for an event before the move clip was parented?");
				return;
			}

			var oOriginalFunction = dyncast<Table>(oRootMC)[sKey];
			if (null != oOriginalFunction)
			{
				dyncast<Table>(oRootMC)[sKey] = (Vvargs)((vararg0) =>
				{
					dyncast<Vvargs>(oOriginalFunction)(vararg0);
					dyncast<Vvargs>(oFunction)(vararg0);
				});
			}
			else
			{
				dyncast<Table>(oRootMC)[sKey] = oFunction;
			}
		}
	}

	// General purpose functionality similar to RootMovieClip:RootAnimOut(),
	// except targeting an inner MovieClip.
	public void AnimOutAndDisableInput(string sEventName, Vvargs completionCallback, string sAnimOutFrameLabel)
	{
		// Optional argument.
		sAnimOutFrameLabel = sAnimOutFrameLabel ?? "AnimOut";

		var iHitTestSelfMask = GetHitTestSelfMode();
		var iHitTestChildrenMask = GetHitTestChildrenMode();
		SetHitTestSelf(false);
		SetHitTestChildren(false);

		// Setup an event listener for the AnimOut event.
		var callbackContainer = new Table { };
		callbackContainer["m_Callback"] = (Vvargs)((vararg0) =>
		{
			// Invoke the specific completion callback.
			if (null != completionCallback)
			{
				completionCallback(vararg0);
			}

			// If input was enabled prior to AnimOut, re-enable it now.
			SetHitTestSelfMode(iHitTestSelfMask);
			SetHitTestChildrenMode(iHitTestChildrenMask);

			// Remove the event listener.
			RemoveEventListener(sEventName, callbackContainer["m_Callback"]);
		});

		// Add the event listener.
		AddEventListener(sEventName, callbackContainer["m_Callback"]);

		// Play the AnimOut frame.
		GotoAndPlayByLabel(sAnimOutFrameLabel);
	}

	public void UnsetGameStateValue(string k)
	{
		SetGameStateValue(k, null);
	}

	public T GetGameStateValue<T>(string k)
	{
		return DynamicGameStateData.GetValue<T>(k);
	}

	public T GetGameStateValue<T>(string k, T defaultVal)
	{
		T output = GetGameStateValue<T>(k);
		if (output == null)
		{
			return defaultVal;
		}
		return output;
	}

	public void SetGameStateValue(string k, object v)
	{
		string name = null;
#if DEBUG
		name = GetNative().GetMovieTypeName();
#endif
		DynamicGameStateData.SetValue(k, v, name);
	}

	public void AddFullScreenClipper()
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).AddFullScreenClipper();
	}

	public bool GetAbsorbOtherInput()
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).GetAbsorbOtherInput();
	}

	public int GetChildCount()
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).GetChildCount();
	}

	public int GetCurrentFrame()
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).GetCurrentFrame();
	}

	public string GetCurrentLabel()
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).GetCurrentLabel();
	}

	public double GetDepth3D()
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).GetDepth3D();
	}

	public bool GetExactHitTest()
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).GetExactHitTest();
	}

	public bool GetHitTestChildren()
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).GetHitTestChildren();
	}

	public bool GetHitTestClickInputOnly()
	{
		return 0 != (GetHitTestSelfMode() & HitTestMode.m_iClickInputOnly);
	}

	public bool GetHitTestChildrenClickInputOnly()
	{
		return 0 != (GetHitTestChildrenMode() & HitTestMode.m_iClickInputOnly);
	}

	public int GetHitTestChildrenMode()
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).GetHitTestChildrenMode();
	}

	public bool GetHitTestSelf()
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).GetHitTestSelf();
	}

	public int GetHitTestSelfMode()
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).GetHitTestSelfMode();
	}

	public int GetTotalFrames()
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).GetTotalFrames();
	}

	public bool GotoAndPlay(double iFrame)
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).GotoAndPlay(iFrame);
	}

	public bool GotoAndPlayByLabel(string sLabel)
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).GotoAndPlayByLabel(sLabel);
	}

	public bool GotoAndStop(double iFrame)
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).GotoAndStop(iFrame);
	}

	public bool GotoAndStopByLabel(string sLabel)
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).GotoAndStopByLabel(sLabel);
	}

	public bool HasChild(DisplayObject child)
	{
		return child.GetParent() == this;
	}

	public bool HasDescendant(DisplayObject descendant)
	{
		while (true)
		{
			MovieClip parent = descendant.GetParent();
			if (parent == null)
			{
				return false;
			}
			if (parent == this)
			{
				return true;
			}
			descendant = parent;
		}
	}

	public bool IsPlaying()
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).IsPlaying();
	}

	public virtual void Play()
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).Play();
	}

	public void RemoveAllChildren()
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).RemoveAllChildren();
	}

	public bool RemoveChildAt(double iIndex)
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).RemoveChildAt(iIndex);
	}

	public bool RemoveChildByName(string sName)
	{
		return dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).RemoveChildByName(sName);
	}

	public void RestoreTypicalDefault()
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).RestoreTypicalDefault();
	}

	public void SetAbsorbOtherInput(bool bAbsorbOtherInput)
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).SetAbsorbOtherInput(bAbsorbOtherInput);
	}

	public void SetDepth3D(double fDepth3D)
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).SetDepth3D(fDepth3D);
	}

	public void SetDepth3DFromYLoc(double fYLoc)
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).SetDepth3DFromYLoc(fYLoc);
	}

	public void SetInputActionDisabled(bool bInputActionDisabled)
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).SetInputActionDisabled(bInputActionDisabled);
	}

	public void SetEnterFrame(bool bEnableEnterFrame)
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).SetEnterFrame(bEnableEnterFrame);
	}

	public void SetAutoCulling(bool bEnableAutoCulling)
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).SetAutoCulling(bEnableAutoCulling);
	}

	public void SetAutoDepth3D(bool bEnableAutoDepth3D)
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).SetAutoDepth3D(bEnableAutoDepth3D);
	}

	public void SetCastPlanarShadows(bool bCastPlanarShadows)
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).SetCastPlanarShadows(bCastPlanarShadows);
	}

	public void SetDeferDrawing(bool bDeferDrawing)
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).SetDeferDrawing(bDeferDrawing);
	}

	public void SetChildrenVisible(bool bVisible)
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).SetChildrenVisible(bVisible);
	}

	public void SetTickEvent(bool bEnableTickEvent)
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).SetTickEvent(bEnableTickEvent);
	}

	public void SetExactHitTest(bool bExactHitTest)
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).SetExactHitTest(bExactHitTest);
	}

	public void SetHitTestChildren(bool bHitTestChildren)
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).SetHitTestChildren(bHitTestChildren);
	}

	public void SetHitTestChildrenMode(int iMask)
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).SetHitTestChildrenMode(iMask);
	}

	public void SetHitTestClickInputOnly(bool value)
	{
		var mode = GetHitTestSelfMode();
		if (value)
		{
			mode |= HitTestMode.m_iClickInputOnly;
		}
		else
		{
			mode &= ~HitTestMode.m_iClickInputOnly;
		}

		SetHitTestSelfMode(mode);
	}

	public void SetHitTestChildrenClickInputOnly(bool value)
	{
		var mode = GetHitTestChildrenMode();
		if (value)
		{
			mode |= HitTestMode.m_iClickInputOnly;
		}
		else
		{
			mode &= ~HitTestMode.m_iClickInputOnly;
		}

		SetHitTestChildrenMode(mode);
	}

	public void SetHitTestSelf(bool bHitTestSelf)
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).SetHitTestSelf(bHitTestSelf);
	}

	public void SetHitTestSelfMode(int iMask)
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).SetHitTestSelfMode(iMask);
	}

	public void SetReorderChildrenFromDepth3D(bool bReorder)
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).SetReorderChildrenFromDepth3D(bReorder);
	}

	public virtual void Stop()
	{
		dyncast<Native.ScriptUIMovieClipInstance>(m_udNativeInstance).Stop();
	}

#if DEBUG
	public bool IsInSceneGraph()
	{
		RootMovieClip rootMovieClip = GetRootMovieClip();
		if (null == rootMovieClip)
		{
			return false;
		}

		MovieClip currentMovieClip = this;
		if (currentMovieClip == rootMovieClip)
		{
			return true;
		}

		while (true)
		{
			if (currentMovieClip.GetParent() == rootMovieClip)
			{
				return true;
			}
			else if (currentMovieClip.GetParent() == null)
			{
				return false;
			}

			currentMovieClip = currentMovieClip.GetParent();
		}
	}
#endif //DEBUG
}
