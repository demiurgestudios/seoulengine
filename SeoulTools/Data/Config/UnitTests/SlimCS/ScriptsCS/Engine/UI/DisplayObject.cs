// Contains DisplayObject, the base class of any script wrapper
// around a native Falcon::Instance.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

using System.Diagnostics;

/// <summary>
/// Utility class, wraps a Native.ScriptUIMovie with a handle style API.
/// </summary>
/// <remarks>
/// Native.ScriptUIMovie is "special". Most native user data that is bound
/// into SlimCS is the native object itself, and the object has garbage collected
/// lifespan.
///
/// In the case of Native.ScriptUIMovie, the object is a pointer to
/// a subclass of Seoul::UIMovie and that pointer can be set to nullptr. Unfortunately,
/// this does *not* invalidate the Native.ScriptUIMovie object (null == m_udNative will
/// still return false, even after the underlying pointer has been set to nullptr), though any
/// attempts to call methods on the object will raise a NullReferenceException. This
/// is similar to a Unity ScriptableObject in behavior.
///
/// NativeScriptUIMovieHandle exists to resolve this issue by wrapping the underlying
/// Native.ScriptUIMovie instance and forcing calls to Target to resolve it, which
/// will only return the instance if the underlying pointer is still valid.
///
/// Note that this class is generally overkill and unneeded. As long as all users
/// of a Native.ScriptUIMovie are internal to that movie, there is no lifespan issue.
///
/// Mainly this class is useful for (e.g.) HTTP callbacks that can return after
/// the UIMovie and its graph have been destroyed (in native), since the GC
/// owned script interface can still exist until those callbacks are released.
/// </remarks>
public sealed class NativeScriptUIMovieHandle : IContextualGameStateSetter
{
	#region Private members
	delegate bool SeoulIsNativeValidDelegate(object o);

	readonly Native.ScriptUIMovie m_udNative;
	#endregion

	public NativeScriptUIMovieHandle(Native.ScriptUIMovie udNative)
	{
		m_udNative = udNative;
	}

	public bool IsAlive
	{
		get
		{
			return dyncast<SeoulIsNativeValidDelegate>(_G["SeoulIsNativeValid"])(m_udNative);
		}
	}

	public void SetGameStateValue(string k, object v)
	{
		string name = null;
#if DEBUG
		{
			var target = Target;
			if (null != target)
			{
				name = target.GetMovieTypeName();
			}
		}
#endif
		DynamicGameStateData.SetValue(k, v, name);
	}

	public Native.ScriptUIMovie Target
	{
		get
		{
			if (IsAlive)
			{
				return m_udNative;
			}
			else
			{
				return null;
			}
		}
	}
}

public class EventDispatcher
{
	public delegate object ListenerDelegate(EventDispatcher oTarget, params object[] aArgs);

	public sealed class ListenerCollection
	{
		#region Private members
		Table<double, ListenerEntry> m_Listeners;
		double m_Count;
		#endregion

		public sealed class ListenerEntry
		{
			public object m_Listener;
		}

		public void Add(object listener)
		{
			if (null == m_Listeners)
			{
				m_Listeners = new Table<double, ListenerEntry>();
			}
			else
			{
				// Check - only register once, nop if already in the list.
				foreach (var (_, o) in ipairs(m_Listeners))
				{
					if (o.m_Listener == listener)
					{
						return;
					}
				}
			}

			// Increment count.
			m_Count++;

			// Add a new entry.
			table.insert(
				m_Listeners,
				new ListenerEntry()
				{
					m_Listener = listener,
				});
		}

		public double Count
		{
			get
			{
				return m_Count;
			}
		}

		public (object, bool) Dispatch(EventDispatcher target, string sEventName, params object[] vararg)
		{
			if (null == m_Listeners)
			{
				return (null, false);
			}

			// Manually check i, in case there is an add mid-stream.
			// We want to match AS semantics, which says that
			// listeners added during dispatch are not invoked
			// until the next dispatch.
			var listeners = m_Listeners;
			var count = lenop(listeners);
			var i = 1;
			object toReturn = null;
			var bReturn = false;
			while (i <= count)
			{
				// Removed, cleanup now.
				var e = listeners[i];
				if (null == e.m_Listener)
				{
					table.remove(listeners, i);
					count--;
				}
				else
				{
					// Invoke.
					toReturn = dyncast<ListenerDelegate>(e.m_Listener)(target, vararg);
					bReturn = true;

					// Advance.
					i++;
				}
			}

			return (toReturn, bReturn);
		}

		public void Remove(object listener)
		{
			// Check.
			if (null == m_Listeners)
			{
				return;
			}

			// Find and remove.
			foreach (var (i, o) in ipairs(m_Listeners))
			{
				if (o.m_Listener == listener)
				{
					// Decrement.
					m_Count--;

					// Null out, will be removed by dispatch passes.
					o.m_Listener = null;
					return;
				}
			}
		}
	}

	protected Table<string, ListenerCollection> m_tEventListeners;

	public const string CHANGE = "change";

	public virtual MovieClip GetParent()
	{
		return null;
	}

	public void AddEventListener(string sName, object listener)
	{
		if (null == sName)
		{
			error("called AddEventListener with a nil sName value.", 2);
		}
		if (null == listener)
		{
			error("called AddEventListener with a nil listener value.", 2);
		}

		var tEventListeners = m_tEventListeners;
		if (null == tEventListeners)
		{
			tEventListeners = new Table<string, ListenerCollection> { };
			m_tEventListeners = tEventListeners;
		}

		var listeners = tEventListeners[sName];
		if (null == listeners)
		{
			listeners = new ListenerCollection();
			tEventListeners[sName] = listeners;
		}

		// Convenience handling for Event.ENTER_FRAME and Event.TICK
		var udNativeInstance = dyncast<Native.ScriptUIMovieClipInstance>(rawget(this, "m_udNativeInstance"));
		if (null != udNativeInstance && 0 == listeners.Count)
		{
			if (Event.ENTER_FRAME == sName)
			{
				udNativeInstance.SetEnterFrame(true);
			}
			else if (Event.TICK == sName)
			{
				udNativeInstance.SetTickEvent(true);
			}
			else if (Event.TICK_SCALED == sName)
			{
				udNativeInstance.SetTickScaledEvent(true);
			}
		}

		listeners.Add(listener);
	}

	public void RemoveEventListener(string sName, object listener)
	{
		if (null == sName)
		{
			error("called RemoveEventListener with a nil sName value.", 2);
		}

		var tEventListeners = m_tEventListeners;
		if (null == tEventListeners)
		{
			return;
		}

		var listeners = tEventListeners[sName];
		if (null == listeners)
		{
			return;
		}

		listeners.Remove(listener);

		// Convenience handling for Event.ENTER_FRAME and Event.TICK
		var udNativeInstance = dyncast<Native.ScriptUIMovieClipInstance>(rawget(this, "m_udNativeInstance"));
		if (null != udNativeInstance && 0 == listeners.Count)
		{
			if (Event.ENTER_FRAME == sName)
			{
				udNativeInstance.SetEnterFrame(false);
			}
			else if (Event.TICK == sName)
			{
				udNativeInstance.SetTickEvent(false);
			}
			else if (Event.TICK_SCALED == sName)
			{
				udNativeInstance.SetTickScaledEvent(false);
			}
		}
	}

	public void RemoveAllEventListenersOfType(string sName)
	{
		if (null == sName)
		{
			error("called RemoveEventListener with a nil sName value.", 2);
		}

		var tEventListeners = m_tEventListeners;
		if (null == tEventListeners)
		{
			return;
		}

		var tListeners = tEventListeners[sName];
		if (null == tListeners)
		{
			return;
		}

		rawset(tEventListeners, sName, null);

		// Convenience handling fo Event.ENTER_FRAME and Event.TICK
		var udNativeInstance = dyncast<Native.ScriptUIMovieClipInstance>(rawget(this, "m_udNativeInstance"));
		if (null != udNativeInstance)
		{
			if (Event.ENTER_FRAME == sName)
			{
				udNativeInstance.SetEnterFrame(false);
			}
			else if (Event.TICK == sName)
			{
				udNativeInstance.SetTickEvent(false);
			}
		}
	}

	/// <summary>
	/// Fire any listeners registered with oTarget.
	/// </summary>
	/// <param name="oTarget">EventDispatcher that contains listeners.</param>
	/// <param name="sEventName">Target event to dispatch.</param>
	/// <param name="vararg0">Args to pass to the listeners.</param>
	/// <returns>Return value from the last invoked listener, and true/false if any listeners were invoked.</returns>
	public (object, bool) DispatchEvent(EventDispatcher target, string sEventName, params object[] vararg)
	{
		object toReturn = null;
		bool bReturn = false;
		var tEventListeners = target.m_tEventListeners;
		if (null != tEventListeners)
		{
			var listeners = tEventListeners[sEventName];
			if (null != listeners)
			{
				(toReturn, bReturn) = listeners.Dispatch(target, sEventName, vararg);
			}
		}

		return (toReturn, bReturn);
	}

	/// <summary>
	/// Fire any listeners registered with oTarget. Also bubbles to parents unless the event is handled.
	/// </summary>
	/// <param name="oTarget">EventDispatcher that contains listeners.</param>
	/// <param name="sEventName">Target event to dispatch.</param>
	/// <param name="vararg0">Args to pass to the listeners.</param>
	/// <returns>Return value from the last invoked listener, and true/false if any listeners were invoked.</returns>
	public (object, bool) DispatchEventAndBubble(EventDispatcher target, string sEventName, params object[] vararg)
	{
		object toReturn = null;
		bool bReturn = false;
		var tEventListeners = target.m_tEventListeners;
		if (null != tEventListeners)
		{
			var listeners = tEventListeners[sEventName];
			if (null != listeners)
			{
				(toReturn, bReturn) = listeners.Dispatch(target, sEventName, vararg);
			}
		}

		if (bReturn)
		{
			return (toReturn, bReturn);
		}

		var parent = target.GetParent();
		if (null != parent)
		{
			return parent.DispatchEventAndBubble(parent, sEventName, vararg);
		}

		return (toReturn, bReturn);
	}
}

public interface IVisible
{
	bool GetVisible();
	void SetVisible(bool value);
}

public class DisplayObject : EventDispatcher, IVisible
{
	public const double kfFadeInTimeSecs = 0.2;

	public delegate void ConstructorDelegate(DisplayObject oInstance, params object[] vararg0);

	protected delegate object NativeFunctionDelegate(object udNativeInstance, params object[] aArgs);

	public delegate object NativeWrapper(DisplayObject self, params object[] aArgs);

	// Hooks called form native on hot load events.
#if DEBUG
	public virtual void OnHotLoadBeginHandler()
	{
		// Nop
	}

	public virtual void OnHotLoadEndHandler()
	{
		// Nop
	}

	public void RegisterForHotLoadEvents()
	{
		GetRootMovieClip().RegisterForHotLoad(this);
	}
#endif

	#region Protected members
	// Setup via some low-level mechanics of DisplayObject
	// creation so otherwise never explicitly assigned
	// from the C# compiler's view.
	#pragma warning disable CS0649
	protected Native.ScriptUIInstance m_udNativeInstance;
	protected Native.ScriptUIMovie m_udNativeInterface;
	#pragma warning restore CS0649
	#endregion

	/// <summary>
	/// Handle wrapper variation of GetNative().
	/// </summary>
	/// <returns>A newly instantiated handle wrapper around GetNative().</returns>
	/// <remarks>
	/// The handle wrapper ensures that the native API is valid before it is returned.
	/// If the native API is not valid, calls to Target will return null instead.
	/// </remarks>
	public NativeScriptUIMovieHandle BindNativeHandle()
	{
		return new NativeScriptUIMovieHandle(m_udNativeInterface);
	}

	/// <summary>
	/// Retrieve a reference to the native movie API.
	/// </summary>
	/// <returns>Native movie API</returns>
	/// <remarks>
	/// The native movie API allows access to native functionality
	/// available via Seoul::UIMovie, Seoul::ScriptUIMovie, etc.
	///
	/// NOTE: All method calls will fail if IsNativeValid() returns
	/// true for the returned instance. This only occurs if the underlying
	/// native movie has been destroyed. This is not typical - it can occur
	/// if "dangling" references are maintained (e.g. in an HTTP callback).
	/// </remarks>
	public Native.ScriptUIMovie GetNative()
	{
		return m_udNativeInterface;
	}

	public T GetNative<T>()
		where T : Native.ScriptUIMovie
	{
		return dyncast<T>(m_udNativeInterface);
	}

	delegate bool SeoulIsNativeValidDelegate(object o);

	public bool IsNativeValid()
	{
		return dyncast<SeoulIsNativeValidDelegate>(_G["SeoulIsNativeValid"])(GetNative());
	}

	public override MovieClip GetParent()
	{
		return m_udNativeInstance.GetParent();
	}

	// Return the root MovieClip for this DisplayObject.
	public RootMovieClip GetRootMovieClip()
	{
		if (IsNativeValid())
		{
			return m_udNativeInterface.GetRootMovieClip();
		}
		else
		{
			return null;
		}
	}

	/// <summary>
	/// Convenience alias.
	/// </summary>
	/// <param name="sType">Motion type to instantiate.</param>
	/// <param name="aArgs">Arguments to pass to the instantiated motion.</param>
	/// <returns>Unique id of the instantiated motion util.</returns>
	public int AddMotionNoCallback(string sType, params object[] aArgs)
	{
		return m_udNativeInstance.AddMotion(sType, callback: null, aArgs: aArgs);
	}

	public int AddMotion(string sType, Vfunc0 callback, params object[] aArgs)
	{
		return m_udNativeInstance.AddMotion(sType, callback, aArgs);
	}

	public int AddTween(TweenTarget eTarget, params object[] aArgs)
	{
		return m_udNativeInstance.AddTween(eTarget, aArgs);
	}

	public int AddTweenCurve(TweenType eType, TweenTarget eTarget, params object[] aArgs)
	{
		return m_udNativeInstance.AddTweenCurve(eType, eTarget, aArgs);
	}

	public void CancelMotion(int iIdentifier)
	{
		m_udNativeInstance.CancelMotion(iIdentifier);
	}

	public void CancelTween(int iIdentifier)
	{
		m_udNativeInstance.CancelTween(iIdentifier);
	}

	public DisplayObject Clone()
	{
		return m_udNativeInstance.Clone();
	}

	public double GetAlpha()
	{
		return m_udNativeInstance.GetAlpha();
	}

	public (double, double, double, double) GetBounds()
	{
		return m_udNativeInstance.GetBounds();
	}

	/// <summary>
	/// Return the bounds of this DisplayObject in the coordinate space of targetCoordinateSpace.
	/// </summary>
	/// <param name="targetCoordinateSpace">DisplayObject that defines the coordinate space of the returned bounds.</param>
	/// <returns>Bounds as (left, top, right, bottom)</returns>
	/// <example>
	/// var (l, t, r, b) = obj.GetBoundsIn(do);                    // Bounds in local space of the object - equivalent to GetLocalBounds().
	///     (l, t, r, b) = obj.GetBoundsIn(do.GetParent());        // Bounds in the space of the parent - equivalent to GetBounds().
	///     (l, t, r, b) = obj.GetBoundsIn(do.GetRootMovieClip()); // Bounds in world space - equivalent to GetWorldBounds().
	/// </example>
	public (double, double, double, double) GetBoundsIn(DisplayObject targetCoordinateSpace)
	{
		return m_udNativeInstance.GetBoundsIn(targetCoordinateSpace.m_udNativeInstance);
	}

	public double GetClipDepth()
	{
		return m_udNativeInstance.GetClipDepth();
	}

	public int GetDepthInParent()
	{
		return m_udNativeInstance.GetDepthInParent();
	}

	public (double, double, double, double, double, double) GetColorTransform()
	{
		return m_udNativeInstance.GetColorTransform();
	}

	public string GetDebugName()
	{
#if DEBUG
		return m_udNativeInstance.GetDebugName();
#else
		return string.Empty;
#endif
	}

	/// <summary>
	/// Return the object space (*not* local space) height of the DisplayObject.
	/// </summary>
	/// <returns>Height in pixels.</returns>
	/// <remarks>
	/// This is equivalent to:
	///   var (_, t, _, b) = GetBounds();
	///   return (b - t);
	/// </remarks>
	public double GetHeight()
	{
		var (_, t, _, b) = GetBounds();
		return b - t;
	}

	public bool GetIgnoreDepthProjection()
	{
		return m_udNativeInstance.GetIgnoreDepthProjection();
	}

	public (double, double, double, double) GetLocalBounds()
	{
		return m_udNativeInstance.GetLocalBounds();
	}

	public (double, double) GetLocalMouse()
	{
		return m_udNativeInstance.GetLocalMousePosition();
	}

	public string GetName()
	{
		return m_udNativeInstance.GetName();
	}

	public string GetFullName()
	{
		return m_udNativeInstance.GetFullName();
	}

	public Native.ScriptUIInstance NativeInstance
	{
		get
		{
			return m_udNativeInstance;
		}
	}

	public virtual (double, double) GetPosition()
	{
		return m_udNativeInstance.GetPosition();
	}

	public double GetPositionX()
	{
		return m_udNativeInstance.GetPositionX();
	}

	public double GetPositionY()
	{
		return m_udNativeInstance.GetPositionY();
	}

	public double GetRotation()
	{
		return m_udNativeInstance.GetRotation();
	}

	public (double, double) GetScale()
	{
		return m_udNativeInstance.GetScale();
	}

	public double GetScaleX()
	{
		return m_udNativeInstance.GetScaleX();
	}

	public double GetScaleY()
	{
		return m_udNativeInstance.GetScaleY();
	}

	public bool GetScissorClip()
	{
		return m_udNativeInstance.GetScissorClip();
	}

	public bool GetVisible()
	{
		return m_udNativeInstance.GetVisible();
	}

	public bool GetVisibleToRoot()
	{
		return m_udNativeInstance.GetVisibleToRoot();
	}

	/// <summary>
	/// Return the object space (*not* local space) width of the DisplayObject.
	/// </summary>
	/// <returns>Width in pixels.</returns>
	/// <remarks>
	/// This is equivalent to:
	///   var (l, _, r, _) = GetBounds();
	///   return (r - l);
	/// </remarks>
	public double GetWidth()
	{
		var (l, _, r, _) = GetBounds();
		return r - l;
	}

	public (double, double, double, double) GetWorldBounds()
	{
		return m_udNativeInstance.GetWorldBounds();
	}

	public double GetWorldDepth3D()
	{
		return m_udNativeInstance.GetWorldDepth3D();
	}

	public (double, double) GetWorldPosition()
	{
		return m_udNativeInstance.GetWorldPosition();
	}

	public (double, double) LocalToWorld(double fX, double fY)
	{
		return m_udNativeInstance.LocalToWorld(fX, fY);
	}

	public bool HasParent()
	{
		return m_udNativeInstance.HasParent();
	}

	/// <summary>
	/// Given a world space point, check if it intersects this DisplayObject.
	/// </summary>
	/// <param name="fWorldX">X position in world space.</param>
	/// <param name="fWorldY">Y position in world space.</param>
	/// <param name="bExactHitTest">
	/// If true, exactly triangles of the Shape
	/// are used (when applicable). Otherwise, only tests against the bounds.
	/// </param>
	/// <returns>true on intersection, false otherwise.</returns>
	/// <remarks></remarks>
	public bool Intersects(double fWorldX, double fWorldY, bool bExactHitTest = false)
	{
		return m_udNativeInstance.Intersects(fWorldX, fWorldY, bExactHitTest);
	}

	public void RemoveFromParent()
	{
		m_udNativeInstance.RemoveFromParent();
	}

	public bool GetAdditiveBlend()
	{
		return m_udNativeInstance.GetAdditiveBlend();
	}

	public void SetAdditiveBlend(bool bAdditiveBlend)
	{
		m_udNativeInstance.SetAdditiveBlend(bAdditiveBlend);
	}

	public void SetAlpha(double fAlpha)
	{
		m_udNativeInstance.SetAlpha(fAlpha);
	}

	public void SetClipDepth(int iDepth)
	{
		m_udNativeInstance.SetClipDepth(iDepth);
	}

	public void SetColorTransform(double fMulR, double fMulG, double fMulB, int iAddR, int iAddG, int iAddB)
	{
		m_udNativeInstance.SetColorTransform(fMulR, fMulG, fMulB, iAddR, iAddG, iAddB);
	}

	[Conditional("DEBUG")]
	public void SetDebugName(string sName)
	{
		m_udNativeInstance.SetDebugName(sName);
	}

	/// <summary>
	/// Adjust the scale of this DisplayObject so that its
	/// height (bottom - top bounds) is equalt to the specified
	/// value.
	/// </summary>
	/// <param name="value">Desired height.</param>
	/// <remarks>
	/// If the local (bottom - top) of the DisplayObject is 0, this method
	/// is a no-op.
	/// </remarks>
	public void SetHeight(double value)
	{
		var (_, t, _, b) = GetLocalBounds();
		var lh = b - t;

		if (lh < 1e-6)
		{
			return;
		}

		var (sx, sy) = GetScale();
		sy = value / lh;
		SetScale(sx, sy);
	}

	public void SetIgnoreDepthProjection(bool b)
	{
		m_udNativeInstance.SetIgnoreDepthProjection(b);
	}

	public void SetName(string sName)
	{
		m_udNativeInstance.SetName(sName);
	}

	public void SetPosition(double fX, double fY)
	{
		m_udNativeInstance.SetPosition(fX, fY);
	}

	public void SetPositionX(double value)
	{
		m_udNativeInstance.SetPositionX(value);
	}

	public void SetPositionY(double value)
	{
		m_udNativeInstance.SetPositionY(value);
	}

	public void SetRotation(double fAngleInDegrees)
	{
		m_udNativeInstance.SetRotation(fAngleInDegrees);
	}

	public void SetScale(double fX, double fY)
	{
		m_udNativeInstance.SetScale(fX, fY);
	}

	public void SetScaleX(double value)
	{
		m_udNativeInstance.SetScaleX(value);
	}

	public void SetScaleY(double value)
	{
		m_udNativeInstance.SetScaleY(value);
	}

	public void SetScissorClip(bool bEnable)
	{
		m_udNativeInstance.SetScissorClip(bEnable);
	}

	public virtual void SetVisible(bool bVisible)
	{
		m_udNativeInstance.SetVisible(bVisible);
	}

	/// <summary>
	/// Adjust the scale of this DisplayObject so that its
	/// width (right - left bounds) is equalt to the specified
	/// value.
	/// </summary>
	/// <param name="value">Desired width.</param>
	/// <remarks>
	/// If the local (right - left) of the DisplayObject is 0, this method
	/// is a no-op.
	/// </remarks>
	public void SetWidth(double value)
	{
		var (l, _, r, _) = GetLocalBounds();
		var lw = r - l;

		if (lw < 1e-6)
		{
			return;
		}

		var (sx, sy) = GetScale();
		sx = value / lw;
		SetScale(sx, sy);
	}

	public void SetWorldPosition(double fX, double fY)
	{
		m_udNativeInstance.SetWorldPosition(fX, fY);
	}

	public (double, double) WorldToLocal(double fX, double fY)
	{
		return m_udNativeInstance.WorldToLocal(fX, fY);
	}

	/// <summary>
	/// Convenience, adds a tween to transition this DisplayObject's
	/// Alpha from 0 to 1.
	/// </summary>
	public void FadeIn()
	{
		SetAlpha(0);
		AddTween(TweenTarget.m_iAlpha, 0, 1, kfFadeInTimeSecs);
	}
}
