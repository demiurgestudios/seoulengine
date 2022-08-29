// RootMovieClip is a convenience class, meant to be subclassed
// by classes which match the root MovieClip of a Falcon/Flash
// scene graph.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public class RootMovieClip : MovieClip
{
	// Hooks called form native on reload events.
#if DEBUG
	Table<DisplayObject, DisplayObject> m_tHotLoadTargets;

	public void OnHotLoadBegin()
	{
		if (null == m_tHotLoadTargets)
		{
			return;
		}

		foreach (var (inst, _) in pairs(m_tHotLoadTargets))
		{
			inst.OnHotLoadBeginHandler();
		}
	}

	public void OnHotLoadEnd()
	{
		if (null == m_tHotLoadTargets)
		{
			return;
		}

		foreach (var (inst, _) in pairs(m_tHotLoadTargets))
		{
			inst.OnHotLoadEndHandler();
		}
	}

	public void RegisterForHotLoad(DisplayObject inst)
	{
		if (null == m_tHotLoadTargets)
		{
			m_tHotLoadTargets = new Table<DisplayObject, DisplayObject>();
		}

		m_tHotLoadTargets[inst] = inst;
	}
#endif

	readonly Table<double, TaskFunc> m_aRootEveryTickTasks = new Table<double, TaskFunc>();
	readonly Table<double, TaskFunc> m_aRootTasks = new Table<double, TaskFunc>();
	TableV<Native.ScriptEngineHTTPRequest, bool> m_tPendingRequests = new TableV<Native.ScriptEngineHTTPRequest, bool>();
	bool m_bRootIsInAnimOut = false;
	protected MovieClip m_mcTop;

	public delegate void TaskFunc(double fDeltaTimeInSeconds);

	void RunTask(TaskFunc task, double fDeltaTimeInSeconds)
	{
		// Early out if no task.
		if (null == task)
		{
			return;
		}

		// Execute and report.
		(var bStatus, var msg) = coroutine.resume(task, fDeltaTimeInSeconds);
		if (!bStatus)
		{
			error(msg);
		}
	}

	public RootMovieClip()
	{
#if DEBUG
		DynamicGameStateData.RegisterRootMovieClip(this, GetNative().GetMovieTypeName());
#endif
	}

	public void AddPendingRequest(Native.ScriptEngineHTTPRequest req)
	{
		if (null != req)
		{
			m_tPendingRequests[req] = true;
		}
	}

	public void RemovePendingRequest(Native.ScriptEngineHTTPRequest req)
	{
		if (null != req)
		{
			m_tPendingRequests[req] = null;
		}
	}

	public virtual void Destructor()
	{
		foreach ((var req, var _) in pairs(m_tPendingRequests))
		{
			req.Cancel();
		}

		m_tPendingRequests = new TableV<Native.ScriptEngineHTTPRequest, bool> { };

#if DEBUG
		DynamicGameStateData.UnregisterRootMovieClip(this);
#endif
	}

	// Get if this root is animating out, typically to perform
	// a state transition to a new UI state.
	public bool IsInAnimOut()
	{
		return m_bRootIsInAnimOut;
	}

	// Utility for handling the common pattern of animating out a root MovieClip.
	// Disables input on the entire Falcon scene graph until the animation completes.
	public void RootAnimOut(string sEventName, MovieClip oMovieClip, Vfunc0 completionCallback, string sOptionalAnimOutFrameLabel = null)
	{
		// Optional argument.
		sOptionalAnimOutFrameLabel = sOptionalAnimOutFrameLabel ?? "AnimOut";

		var udNative = GetNative();

		// Disable input while animating out.
		var bAcceptingInput = udNative.AcceptingInput();
		udNative.SetAcceptInput(false);

		// Setup an event listener for the AnimOut event.
		var callbackContainer = new Table { };
		callbackContainer["m_Callback"] = (Vfunc0)(() =>
		{
			// Invoke the specific completion callback.
			if (null != completionCallback)
			{
				completionCallback();
			}

			// If input was enabled prior to AnimOut, re-enable it now.
			if (bAcceptingInput)
			{
				udNative.SetAcceptInput(true);
			}

			// Remove the event listener.
			oMovieClip.RemoveEventListener(sEventName, callbackContainer["m_Callback"]);

			// No longer in AnimOut.
			m_bRootIsInAnimOut = false;
		});

		// Add the event listener.
		oMovieClip.AddEventListener(sEventName, callbackContainer["m_Callback"]);

		// Play the AnimOut frame.
		m_bRootIsInAnimOut = true;
		oMovieClip.GotoAndPlayByLabel(sOptionalAnimOutFrameLabel);
	}

	/// <summary>
	/// Hook to override for root movie animation out events. Standard
	/// setup - can be customized, see RootStandardAnimOut().
	/// </summary>
	public virtual void AnimOutComplete()
	{
		// Nop by default.
	}

	/// <summary>
	/// Standard configuration for RootAnimOut. Usually,
	/// you can call this if your timeline and instance names
	/// match our common naming convention.
	/// </summary>
	public void RootStandardAnimOut()
	{
		RootAnimOut(
			"AnimOutComplete",
			m_mcTop,
			() =>
			{
				AnimOutComplete();
			});
	}

	/// <summary>
	/// Simple cooperative multi-tasking task functionality.
	/// </summary>
	/// <param name="func">The task to run.</param>
	/// <param name="everyTickeveryTick">
	/// If true, task is run once every frame. By default,
	/// tasks are run in sequence and given a single time
	/// slice - meaning, if task 0 and task 1 are scheduled,
	/// only task 0 will run until it completes.
	/// </param>
	/// <remarks>
	/// Add a task, and the native UI code will invoke YieldToTasks()
	/// once per frame. This allows the implementation of func to
	/// use coroutine.yield() to timeslice the body of the task.
	/// </remarks>
	public object AddTask(TaskFunc func, bool everyTick = false)
	{
		var task = coroutine.create(func);
		if (everyTick)
		{
			table.insert(m_aRootEveryTickTasks, task);
		}
		else
		{
			table.insert(m_aRootTasks, task);
		}

		return task;
	}

	public void RemoveTask(object task)
	{
		foreach ((var index, var taskObj) in ipairs(m_aRootEveryTickTasks))
		{
			if (taskObj == task)
			{
				table.remove(m_aRootEveryTickTasks, index);
				return;
			}
		}

		foreach ((var index, var taskObj) in ipairs(m_aRootTasks))
		{
			if (taskObj == task)
			{
				table.remove(m_aRootTasks, index);
				return;
			}
		}
	}

	/// <summary>
	/// Return true if this root has any pending tasks to process.
	/// </summary>
	public bool HasRootTasks
	{
		get
		{
			return null != m_aRootTasks[1];
		}
	}

	/// <summary>
	/// Called once per-frame by native UI code. Used to advance
	/// the queue of tasks.
	/// </summary>
	public void YieldToTasks(double fDeltaTimeInSeconds)
	{
		// Once a yield tasks.
		{
			var tasks = m_aRootTasks;
			var task = tasks[1];
			while (null != task && coroutine.status(task) == "dead")
			{
				table.remove(tasks, 1);
				task = tasks[1];
			}

			RunTask(task, fDeltaTimeInSeconds);
		}

		// Every yield tasks.
		{
			var tasks = m_aRootEveryTickTasks;
			double i = 1;
			var task = tasks[i];
			while (null != task)
			{
				// Remove the task - don't advance, tasks[i] is a new task.
				if (coroutine.status(task) == "dead")
				{
					table.remove(tasks, i);
				}

				// Otherwise, execute and advance.
				else
				{
					RunTask(task, fDeltaTimeInSeconds);
					++i;
				}

				// Acquire the next task.
				task = tasks[i];
			}
		}
	}

	public virtual void HANDLER_OnViewportChanged(double fAspectRatio)
	{
		var scrolledMovie = GetChildByPath<MovieScroller>("UIMovieScroller");
		scrolledMovie?.OnViewportChanged(fAspectRatio);
	}

	public delegate void MemberMethodDelegate(object member, params object[] varargs);

	/// <summary>
	/// Utility invoked by native to invoke a method on a nested member field.
	/// </summary>
	/// <param name="sMember">Name of the field to target the invoke.</param>
	/// <param name="sMethod">Name of the method to invoke.</param>
	/// <param name="varargs">Arguments to the method.</param>
	public void CallMemberMethod(string sMember, string sMethod, params object[] varargs)
	{
		var member = dyncast<object>(dyncast<Table>(this)[sMember]);
		dyncast<MemberMethodDelegate>(dyncast<Table>(member)[sMethod])(member, varargs);
	}

	/// <summary>
	/// Base handling for enter state. Configures movie scrollers if defined.
	/// </summary>
	/// <param name="sPreviousState">Name of the state we're leaving - may be empty.</param>
	/// <param name="sNextState">Name of the state we're entering - may be empty.</param>
	/// <param name="bWasInPreviousState">True if we were in the previous state, false otherwise.</param>
	public virtual void OnEnterState(string sPreviousState, string sNextState, bool bWasInPreviousState)
	{
		// Cleanup any existing scroller first.
		var scrolledMovie = GetChildByPath<MovieScroller>("UIMovieScroller");
		scrolledMovie?.Deinit();
		scrolledMovie?.RemoveFromParent();

		// Check if the state we're entering has screen scrollers defined.
		var native = GetNative();
		var aScrolledMovies = dyncast<Table<double, string>>(native.GetStateConfigValue("ScrolledMovies"));

		// We have a proper set of screen scrolls, check if we're
		// the primary (we're the middle element).
		if (null != aScrolledMovies && lenop(aScrolledMovies) == 3)
		{
			// We're the primary if our class name matches
			// the primary name.
			var sName = aScrolledMovies[2];
			var sClassName = getmetatable(this)["m_sClassName"];
			if (sName == sClassName)
			{
				// Grab triggers now as well.
				var aScrolledTriggers = dyncast<Table<double, string>>(native.GetStateConfigValue("ScrolledTriggers"));

				// We're the primary, setup a scroller.
				SetupScrolledMovie(aScrolledMovies, aScrolledTriggers);
			}
		}
	}

	public virtual void OnExitState(string sPrevState, string sNextState, bool bIsInNextState)
	{
	}

	public virtual void OnLoad()
	{
	}

	/// <summary>
	/// Add a MovieScroller MovieClip as the last child of this root.
	/// </summary>
	/// <param name="aScrolledMovies">List of scrollers defined.</param>
	/// <param name="aScrolledTriggers">List of scroller triggers defined.</param>
	void SetupScrolledMovie(
		Table<double, string> aScrolledMovies,
		Table<double, string> aScrolledTriggers)
	{
		// Create and initialize.
		var screenScroller = (MovieScroller)NewMovieClip("MovieScroller");
		AddChild(screenScroller, "UIMovieScroller");
		screenScroller.Init(this, aScrolledMovies, aScrolledTriggers);
	}

	/// <summary>
	/// The height of the Flash stage, in pixels.
	/// </summary>
	public double GetMovieHeight()
	{
		return GetNative().GetMovieHeight();
	}

	/// <summary>
	/// The width of the Flash stage, in pixels.
	/// </summary>
	public double GetMovieWidth()
	{
		return GetNative().GetMovieWidth();
	}

	/// <summary>
	/// Look through all sibling movies and return the root
	/// of the first movie found with the given type name.
	/// </summary>
	/// <param name="sSiblingName">Name of the movie to get a root from.</param>
	/// <returns></returns>
	public RootMovieClip GetSiblingRootMovieClip(string sSiblingName)
	{
		return m_udNativeInterface.GetSiblingRootMovieClip(sSiblingName);
	}

	public string GetUIStateConfigString(string key)
	{
		return dyncast<string>(GetNative().GetStateConfigValue(key));
	}

	protected FxDisplayObject AttachVFXToMovie(MovieClip parent, string vfxName)
	{
		var flags = FxFlags.m_iFollowBone;
		FxDisplayObject fx = parent.AddChildFx(vfxName, 0, 0, flags);
		return fx;
	}
}

public static class NativeScriptUIMovieExtensions
{
	delegate void DynamicCallDelegate(Native.ScriptUIMovie movie, params object[] aArgs);

	public static void DynamicCall(this Native.ScriptUIMovie movie, string methodName, params object[] aArgs)
	{
		dyncast<DynamicCallDelegate>(dyncast<Table>(movie)[methodName])(movie, aArgs);
	}
}
