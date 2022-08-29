// Static global class that enables horizontal scrolling
// of entire UI movies.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public sealed class MovieScroller : MovieClip
{
	// NOTE: All constants, except for deceleration
	// strength, are a factor of the
	// screen width. As a result, valid values
	// are absolute in the range 0 to 1, but
	// some are even more restrictive than that
	// (for example, setting 0 to kfTransitionThreshold
	// will likely result in erroneous behavior).

	/// <summary>
	/// How aggressively to decelerate.
	/// </summary>
	/// <remarks>
	/// This is the only constant that is not a factored
	/// of the screen width. It is a power. As a result,
	/// valid values are between 1 and infinity, although
	/// valid reasonable values fall between 1 and 2.
	/// </remarks>
	public const double kfAutoScrollDecelerationStrength = 1.2;

	/// <summary>
	/// The speed at which the scroller returns
	/// to position when the mouse/finger is released.
	/// </summary>
	public const double kfAutoScrollSpeedFactor = 1.0 / 8.0;

	/// <summary>
	/// Distance at which the scroller starts to decelerate
	/// when auto scrolling.
	/// </summary>
	public const double kfAutoScrollDecelerateDistanceFactor = 0.5;

	/// <summary>
	/// As a factor of the full screen width,
	/// the maximum scrolling distance at an edge
	/// tile.
	/// </summary>
	public const double kfEdgeThresholdFactor = 0.5;

	/// <summary>
	/// The speed at
	/// which a finger/mouse flicker
	/// will advance to the next tile.
	/// </summary>
	public const double kfFlickThresholdFactor = 1.0 / 15.0;

	/// <summary>
	/// As a factor of the full screen width,
	/// how far a tile must be dragged before it
	/// centers on the next tile in that direction.
	/// </summary>
	public const double kfTransitionThresholdFactor = 0.4;

	#region Private members
	/// <summary>
	/// Optimization, cache the value of log(10).
	/// </summary>
	const double kfLog10 = 2.302585092994;

	/// <summary>
	/// Used to guarantee stop on auto scroll.
	/// Don't tweak unless you know what you're doing.
	/// </summary>
	const double kfSnapThresholdFactor = 0.002;

	const double kExtraSpaceBetweenMovies = 1;

	RootMovieClip m_RootMovieClip;
	Table<double, string> m_aScrolledMovieNames;
	Table<double, string> m_aScrolledTriggerNames;
	bool m_bMouseDown;
	bool m_bFirstMouseMove;
	double m_ReferenceX;
	double m_fRootTransformX;
	RootMovieClip m_LeftSibling;
	RootMovieClip m_RightSibling;
	double m_fWidth;
	double m_fTarget;
	double m_fLastMouseVelocity;
	double? m_fLastMouseX;
	VfuncT2<MovieClip, double> m_TickFunc;
	MouseDelegate m_MouseDownFunc;
	MouseDelegate m_MouseMoveFunc;
	MouseUpDelegate m_MouseUpFunc;

	/// <summary>
	/// Callback invoked when mouse/finger is pressed on captured
	/// MovieClip.
	/// </summary>
	/// <param name="oMovieClip">MovieClip captured.</param>
	/// <param name="iX">World position X of the mouse/finger.</param>
	/// <param name="iY">World position Y of the mouse/finger.</param>
	/// <param name="bInShape">True if position is in the captured shape, false otherwise.</param>
	void OnMouseDown(
		MovieClip oMovieClip,
		double iX,
		double iY,
		bool bInShape)
	{
		// Mouse is down now.
		m_bMouseDown = bInShape;

		// Clear mouse move state.
		m_bFirstMouseMove = false;

		// Clear some state.
		m_fLastMouseX = null;
		m_fLastMouseVelocity = 0;
	}

	/// <summary>
	/// Callback invoked when a mouse is moved. Only invoked
	/// when this MovieClip is captured.
	/// </summary>
	/// <param name="oMovieClip">MovieClip captured.</param>
	/// <param name="iX">World position X of the mouse/finger.</param>
	/// <param name="iY">World position Y of the mouse/finger.</param>
	/// <param name="bInShape">True if position is in the captured shape, false otherwise.</param>
	void OnMouseMove(
		MovieClip oMovieClip,
		double iX,
		double iY,
		bool bInShape)
	{
		// Nothing to do if mouse/finger is not pressed.
		if (!m_bMouseDown)
		{
			return;
		}

		// On first move event, just capture state.
		if (!m_bFirstMouseMove)
		{
			m_bFirstMouseMove = true;

			// Cache initial values.
			(m_fRootTransformX, _) = m_RootMovieClip.GetPosition();
			m_ReferenceX = iX;
			m_fLastMouseX = null;
			m_fLastMouseVelocity = 0;
			return;
		}

		// New position X for the root.
		var fX = m_fRootTransformX + (iX - m_ReferenceX);

		// Apply edge clamping if needed.
		var fEdgeFactor = m_fWidth * kfEdgeThresholdFactor;

		// We're scrolling to the left (revealing the right sibling).
		if (fX < 0.0)
		{
			// No sibling at all, apply edge resistance to the total shift.
			if (null == m_RightSibling)
			{
				fX = -(math.log(-(fX / fEdgeFactor) + 1.0) / kfLog10) * fEdgeFactor;
			}

			// Sibling, allow one move width of shift before we appy resistance.
			else if (fX < -m_fWidth)
			{
				fX += m_fWidth;
				fX = -(math.log(-(fX / fEdgeFactor) + 1.0) / kfLog10) * fEdgeFactor;
				fX -= m_fWidth;
			}
		}

		// We're scrolling to the right (revealing the left sibling).
		else if (fX > 0.0)
		{
			// No sibling at all, apply edge resistance to the total shift.
			if (null == m_LeftSibling)
			{
				fX = math.log(fX / fEdgeFactor + 1.0) / kfLog10 * fEdgeFactor;
			}

			// Sibling, allow one move width of shift before we apply resistance.
			else if (fX > m_fWidth)
			{
				fX -= m_fWidth;
				fX = math.log(fX / fEdgeFactor + 1.0) / kfLog10 * fEdgeFactor;
				fX += m_fWidth;
			}
		}

		// Apply the new position.
		var root = m_RootMovieClip;
		(var _, var fY) = root.GetPosition();
		root.SetPosition(fX, fY);
	}

	/// <summary>
	/// Callback invoked when the mouse/finger is released.
	/// </summary>
	/// <param name="oMovieClip">Instance of the captured shape on release.</param>
	/// <param name="iX">World space position X of the mouse or finger.</param>
	/// <param name="iY">World space position Y of the mouse or finger.</param>
	/// <param name="bInShape">True if the released shape was active, false otherwise.</param>
	/// <param name="iInputCaptureHitTestMask">Filtering mask on mouse/finger release.</param>
	void OnMouseUp(
		MovieClip oMovieClip,
		double iX,
		double iY,
		bool bInShape,
		double iInputCaptureHitTestMask)
	{
		m_bMouseDown = false;

		// Only apply process if some movement occured.
		if (m_bFirstMouseMove)
		{
			// Get the position between modified.
			(var fBaseX, var _) = m_RootMovieClip.GetPosition();

			// If we have a left tile, potentially move towards it.
			if (null != m_LeftSibling)
			{
				// Handle transition.
				if (!string.IsNullOrEmpty(m_aScrolledTriggerNames[1]) &&
					0.0 == m_fTarget &&
					(
						fBaseX > m_fWidth * kfTransitionThresholdFactor ||
						m_fLastMouseVelocity >= m_fWidth * kfFlickThresholdFactor))
				{
					UIManager.TriggerTransition(m_aScrolledTriggerNames[1]);
					m_fTarget = m_fWidth;
				}
			}

			// If we have a right tile, potentially move towards it.
			if (null != m_RightSibling)
			{
				// Handle transition.
				if (!string.IsNullOrEmpty(m_aScrolledTriggerNames[3]) &&
					0.0 == m_fTarget &&
					(
						fBaseX < -m_fWidth * kfTransitionThresholdFactor ||
						m_fLastMouseVelocity <= -m_fWidth * kfFlickThresholdFactor))
				{
					UIManager.TriggerTransition(m_aScrolledTriggerNames[3]);
					m_fTarget = -m_fWidth;
				}
			}
		}

		// Clear mouse move state.
		m_bFirstMouseMove = false;

		// Mouse velocity is only tracked while the mouse is held.
		m_fLastMouseVelocity = 0;
	}

	/// <summary>
	/// Per frame update operations.
	/// </summary>
	/// <param name="fDeltaTimeInSeconds">Length of the frame tick in seconds.</param>
	void Tick(double fDeltaTimeInSeconds)
	{
		// If we entered from the left or right, set our starting offset.
		if (UIManager.GetCondition("MovieScroller_FromLeft"))
		{
			UIManager.SetCondition("MovieScroller_FromLeft", false);
			(var fX, var fY) = m_RootMovieClip.GetPosition();
			m_RootMovieClip.SetPosition(fX + 2.0 * m_fWidth, fY);
		}
		else if (UIManager.GetCondition("MovieScroller_FromRight"))
		{
			UIManager.SetCondition("MovieScroller_FromRight", false);
			(var fX, var fY) = m_RootMovieClip.GetPosition();
			m_RootMovieClip.SetPosition(fX - 2.0 * m_fWidth, fY);
		}

		// Wait for scroller to settle before allowing input.
		var root = m_RootMovieClip;
		(var fBaseX, var fBaseY) = root.GetPosition();
		var bTest = m_fTarget == fBaseX;

		// Track mouse movement velocity in world terms.
		if (m_bMouseDown)
		{
			if (null == m_fLastMouseX || fDeltaTimeInSeconds <= 0.0)
			{
				m_fLastMouseX = fBaseX;
			}
			else
			{
				m_fLastMouseVelocity = (fBaseX - dyncast<double>(m_fLastMouseX)) / fDeltaTimeInSeconds;
				m_fLastMouseX = fBaseX;
			}
		}

		// Reset values.
		else
		{
			m_fLastMouseVelocity = 0;
			m_fLastMouseX = null;
		}

		// If we haven't settled and the mouse is not held,
		// return to center.
		if (!bTest && !m_bMouseDown)
		{
			// Total distance left to auto scroll.
			var fDistance = math.abs(m_fTarget - fBaseX);

			// Adjustment to apply - constants were tuned at 60 FPS, so we
			// rescale delta time by that value.
			var fAdjust = kfAutoScrollSpeedFactor * m_fWidth * (fDeltaTimeInSeconds * 60.0);

			// We start decelerating when we're closer than this distance.
			var fSlowdownDistance = kfAutoScrollDecelerateDistanceFactor * m_fWidth;

			// If the total distance we can move will place us within the slowdown
			// distance, split it into two halves - slowed and unslowed - so we
			// don't lose any energy due to time step.
			if (fDistance - fAdjust <= fSlowdownDistance)
			{
				// Apply any unslowed adjustment first.
				if (fDistance > fSlowdownDistance)
				{
					var fUnslowedAdjust = fDistance - fSlowdownDistance;
					fAdjust -= fUnslowedAdjust;
					fDistance -= fUnslowedAdjust;

					// Negate if necessary and then apply.
					if (fBaseX > m_fTarget)
					{
						fUnslowedAdjust = -fUnslowedAdjust;
					}

					// Apply.
					fBaseX += fUnslowedAdjust;
				}

				// Apply slodown.
				var fSlowdown = fDistance / fSlowdownDistance;
				fSlowdown = math.pow(fSlowdown, kfAutoScrollDecelerationStrength);
				fAdjust *= fSlowdown;
			}

			// Swap the directionality of adjustment as needed.
			if (fBaseX > m_fTarget)
			{
				fAdjust = -fAdjust;
			}

			// Apply.
			fBaseX += fAdjust;

			// If we've either gone passed, or if we're within
			// a threshold, snap to target.
			if (fBaseX <= m_fTarget && fAdjust < 0.0 ||
				fBaseX >= m_fTarget && fAdjust > 0.0 ||
				math.abs(fBaseX - m_fTarget) <= kfSnapThresholdFactor * m_fWidth)
			{
				fBaseX = m_fTarget;
			}

			// Done, set new position.
			root.SetPosition(fBaseX, fBaseY);
		}

		// Update test state based on scrolling flag.
		SetHitTestChildren(bTest);

		// Match siblings to our position.
		if (null != m_LeftSibling)
		{
			(var _, var fY) = m_LeftSibling.GetPosition();
			m_LeftSibling.SetPosition(fBaseX - m_fWidth, fY);
		}
		if (null != m_RightSibling)
		{
			(var _, var fY) = m_RightSibling.GetPosition();
			m_RightSibling.SetPosition(fBaseX + m_fWidth, fY);
		}
	}
	#endregion

	public MovieScroller()
	{
		m_TickFunc = (VfuncT2<MovieClip, double>)((oMovieClip, fDeltaTimeInSeconds) =>
		{
			Tick(fDeltaTimeInSeconds);
		});
		m_MouseDownFunc = (MouseDelegate)((oMovieClip, iX, iY, bInShape) =>
		{
			OnMouseDown(oMovieClip, iX, iY, bInShape);
		});
		m_MouseMoveFunc = (MouseDelegate)((oMovieClip, iX, iY, bInShape) =>
		{
			OnMouseMove(oMovieClip, iX, iY, bInShape);
		});
		m_MouseUpFunc = (MouseUpDelegate)((oMovieClip, iX, iY, bInShape, iInputCaptureHitTestMask) =>
		{
			OnMouseUp(oMovieClip, iX, iY, bInShape, iInputCaptureHitTestMask);
		});
	}

	/// <summary>
	/// Called by the owner root movie to signal Viewport changed events.
	/// </summary>
	public void OnViewportChanged(double fAspectRatio)
	{
		// Early out if no root.
		if (null == m_RootMovieClip)
		{
			return;
		}

		// Drop all children.
		RemoveAllChildren();

		// Re-add hit tester, viewport sized.
		AddChildHitShapeFullScreen();

		// Re-add the full screen clippers.
		m_RootMovieClip.AddFullScreenClipper();
		m_LeftSibling?.AddFullScreenClipper();
		m_RightSibling?.AddFullScreenClipper();

		// Cache our width for tiling.
		(var fLeft, var _, var fRight, var _) = GetBounds();
		m_fWidth = fRight - fLeft + kExtraSpaceBetweenMovies;
	}

	public bool IsAtTarget()
	{
		(var fBaseX, var _) = m_RootMovieClip.GetPosition();
		return math.abs(m_fTarget - fBaseX) < 0.0001;
	}

	/// <summary>
	/// Call once prior to releasing this scroller.
	/// </summary>
	public void Deinit()
	{
		RemoveEventListener(MouseEvent.MOUSE_UP, m_MouseUpFunc);
		RemoveEventListener(MouseEvent.MOUSE_MOVE, m_MouseMoveFunc);
		RemoveEventListener(MouseEvent.MOUSE_DOWN, m_MouseDownFunc);
		RemoveEventListener(Event.TICK, m_TickFunc);

		m_fLastMouseX = null;
		m_fLastMouseVelocity = 0;
		m_fTarget = 0;
		m_fWidth = 0;
		m_RightSibling = null;
		m_LeftSibling = null;
		m_fRootTransformX = 0;
		m_ReferenceX = 0;
		m_bFirstMouseMove = false;
		m_bMouseDown = false;
		m_aScrolledTriggerNames = null;
		m_aScrolledMovieNames = null;
		m_RootMovieClip = null;
	}

	/// <summary>
	/// Call once immediately after instantiating a new scroller,
	/// *after* adding this clip to its parent.
	/// </summary>
	/// <param name="root">The root that contains this croller.</param>
	/// <param name="aScrolledMovieNames">Scroller set. We're expected to be on movie at middle element.</param>
	/// <param name="aScrolledTriggerNames">Triggers to fire to advance to the next or previous state when scrolling.</param>
	public void Init(
		RootMovieClip root,
		Table<double, string> aScrolledMovieNames,
		Table<double, string> aScrolledTriggerNames)
	{
		// Cache siblings.
		m_RootMovieClip = root;
		m_aScrolledMovieNames = aScrolledMovieNames;
		m_aScrolledTriggerNames = aScrolledTriggerNames;

		// Hit tester, viewport sized.
		AddChildHitShapeFullScreen();

		// Also, adding a full clipper. This needs to be the last element
		// of the root.
		root.AddFullScreenClipper();

		// Also, track neighbors if they exist, and give them a full screen clipper
		// also.
		if (!string.IsNullOrEmpty(m_aScrolledMovieNames[1]))
		{
			m_LeftSibling = root.GetSiblingRootMovieClip(m_aScrolledMovieNames[1]);
			m_LeftSibling?.AddFullScreenClipper();
		}
		if (!string.IsNullOrEmpty(m_aScrolledMovieNames[3]))
		{
			m_RightSibling = root.GetSiblingRootMovieClip(m_aScrolledMovieNames[3]);
			m_RightSibling?.AddFullScreenClipper();
		}

		// Listeners for tick and input events.
		AddEventListener(Event.TICK, m_TickFunc);
		AddEventListener(MouseEvent.MOUSE_DOWN, m_MouseDownFunc);
		AddEventListener(MouseEvent.MOUSE_MOVE, m_MouseMoveFunc);
		AddEventListener(MouseEvent.MOUSE_UP, m_MouseUpFunc);

		// Hit testable by default.
		SetHitTestChildren(true);
		SetHitTestSelfMode(HitTestMode.m_iHorizontalDragInputOnly);

		// Cache our width for tiling.
		(var fLeft, var _, var fRight, var _) = GetBounds();
		m_fWidth = fRight - fLeft + kExtraSpaceBetweenMovies;
	}
}
