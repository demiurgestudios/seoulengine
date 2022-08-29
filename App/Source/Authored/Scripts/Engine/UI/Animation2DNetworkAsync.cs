// Utility used to give a network time to load asynchronously. Calling
// .Network prior to IsReady() will return a null pointer.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

using System.Diagnostics;

#if SEOUL_WITH_ANIMATION_2D

public sealed class Animation2DNetworkAsync
{
	#region Private member
	readonly Animation2DNetwork m_Animation;
	Table<double, OnReadyFunc> m_aDefer;
	bool m_bReady = false;

	bool ProcessDeferred()
	{
		// Conditions to fulfill to process the deferred list.
		if (null == m_aDefer || !m_Animation.IsReady)
		{
			// Good to go.
			if (null == m_aDefer && m_Animation.IsReady)
			{
				m_bReady = true;
			}

			return m_bReady;
		}

		// If we get here, animation is ready. Need to set this
		// to true *before* calling defer operations so that
		// any operations that might use Call() will invoke
		// immediately.
		m_bReady = true;

		// Walk the list and pcall each deferred operation.
		Table<double, string> aErrors = null;
		foreach ((var _, var v) in ipairs(m_aDefer))
		{
			(var bSuccess, var sErrorMessage) = (Tuple<bool, string>)pcall(v, m_Animation);
			if (!bSuccess)
			{
				if (null == aErrors)
				{
					aErrors = new Table<double, string>();
				}
				table.insert(aErrors, sErrorMessage);
			}
		}

		// Done with defers.
		m_aDefer = null;

		// If any errors, concat and report.
		if (null != aErrors)
		{
			error(table.concat(aErrors));
		}

		// Otherwise, success.
		return m_bReady;
	}
	#endregion

	public Animation2DNetworkAsync(
		MovieClip parent,
		Animation2DNetwork animation)
	{
		m_Animation = animation;

		// Schedule a task to run process deferred.
		VfuncT2<MovieClip, double> deferredProcess = null;
		deferredProcess = (clip, fDeltaTimeInSeconds) =>
		{
			if (ProcessDeferred())
			{
				parent.RemoveEventListener(Event.TICK, deferredProcess);
			}
		};
		parent.AddEventListener(Event.TICK, deferredProcess);
	}

	/// <summary>
	/// Get the underlying network. Returns null until the network is ready.
	/// </summary>
	public Animation2DNetwork Network
	{
		get
		{
			if (m_bReady)
			{
				return m_Animation;
			}
			else
			{
				return null;
			}
		}
	}

	public delegate void OnReadyFunc(Animation2DNetwork animation);

	/// <summary>
	/// Utility, used to defer actions until the underlying network is ready.
	/// </summary>
	/// <param name="func">Action to run</param>
	/// <remarks>
	/// If the network IsReady then the passed in callback will be invoked immediately.
	///
	/// Animation networks load asynchronously. This utility can be useful when you want
	/// to ensure a change will be applied to an animation network as soon as it is ready.
	///
	/// If you need to query animation state, you'll need to access the network via
	/// the Network property, which will return null until the Network is ready.
	/// </remarks>
	public void Call(OnReadyFunc func)
	{
		if (m_bReady)
		{
			func(m_Animation);
			return;
		}

		// Defer.
		if (null == m_aDefer)
		{
			m_aDefer = new Table<double, OnReadyFunc>();
		}

		// Add the defer.
		table.insert(m_aDefer, func);
	}
}

#endif // /#if SEOUL_WITH_ANIMATION_2D
