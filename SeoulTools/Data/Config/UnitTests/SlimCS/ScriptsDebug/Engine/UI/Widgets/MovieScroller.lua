-- Static global class that enables horizontal scrolling
-- of entire UI movies.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local MovieScroller = class('MovieScroller', 'MovieClip')

-- NOTE: All constants, except for deceleration
-- strength, are a factor of the
-- screen width. As a result, valid values
-- are absolute in the range 0 to 1, but
-- some are even more restrictive than that
-- (for example, setting 0 to kfTransitionThreshold
-- will likely result in erroneous behavior).

--- <summary>
--- How aggressively to decelerate.
--- </summary>
--- <remarks>
--- This is the only constant that is not a factored
--- of the screen width. It is a power. As a result,
--- valid values are between 1 and infinity, although
--- valid reasonable values fall between 1 and 2.
--- </remarks>
MovieScroller.kfAutoScrollDecelerationStrength = 1.2

--- <summary>
--- The speed at which the scroller returns
--- to position when the mouse/finger is released.
--- </summary>
MovieScroller.kfAutoScrollSpeedFactor = 0.125--[[1 / 8]]

--- <summary>
--- Distance at which the scroller starts to decelerate
--- when auto scrolling.
--- </summary>
MovieScroller.kfAutoScrollDecelerateDistanceFactor = 0.5

--- <summary>
--- As a factor of the full screen width,
--- the maximum scrolling distance at an edge
--- tile.
--- </summary>
MovieScroller.kfEdgeThresholdFactor = 0.5

--- <summary>
--- The speed at
--- which a finger/mouse flicker
--- will advance to the next tile.
--- </summary>
MovieScroller.kfFlickThresholdFactor = 0.0666666666666667--[[1 / 15]]

--- <summary>
--- As a factor of the full screen width,
--- how far a tile must be dragged before it
--- centers on the next tile in that direction.
--- </summary>
MovieScroller.kfTransitionThresholdFactor = 0.4


--- <summary>
--- Optimization, cache the value of log(10).
--- </summary>
local kfLog10 = 2.302585092994

--- <summary>
--- Used to guarantee stop on auto scroll.
--- Don't tweak unless you know what you're doing.
--- </summary>
local kfSnapThresholdFactor = 0.002

local kExtraSpaceBetweenMovies = 1



















--- <summary>
--- Callback invoked when mouse/finger is pressed on captured
--- MovieClip.
--- </summary>
--- <param name="oMovieClip">MovieClip captured.</param>
--- <param name="iX">World position X of the mouse/finger.</param>
--- <param name="iY">World position Y of the mouse/finger.</param>
--- <param name="bInShape">True if position is in the captured shape, false otherwise.</param>
local function OnMouseDown(self,
	_,
	_,
	_,
	bInShape)

	-- Mouse is down now.
	self.m_bMouseDown = bInShape

	-- Clear mouse move state.
	self.m_bFirstMouseMove = false

	-- Clear some state.
	self.m_fLastMouseX = nil
	self.m_fLastMouseVelocity = 0
end

--- <summary>
--- Callback invoked when a mouse is moved. Only invoked
--- when this MovieClip is captured.
--- </summary>
--- <param name="oMovieClip">MovieClip captured.</param>
--- <param name="iX">World position X of the mouse/finger.</param>
--- <param name="iY">World position Y of the mouse/finger.</param>
--- <param name="bInShape">True if position is in the captured shape, false otherwise.</param>
local function OnMouseMove(self,
	_,
	iX,
	_,
	_)

	-- Nothing to do if mouse/finger is not pressed.
	if not self.m_bMouseDown then

		return
	end

	-- On first move event, just capture state.
	if not self.m_bFirstMouseMove then

		self.m_bFirstMouseMove = true

		-- Cache initial values.
		self.m_fRootTransformX = self.m_RootMovieClip:GetPosition()
		self.m_ReferenceX = iX
		self.m_fLastMouseX = nil
		self.m_fLastMouseVelocity = 0
		return
	end

	-- New position X for the root.
	local fX = self.m_fRootTransformX + (iX - self.m_ReferenceX)

	-- Apply edge clamping if needed.
	local fEdgeFactor = self.m_fWidth * 0.5--[[MovieScroller.kfEdgeThresholdFactor]]

	-- We're scrolling to the left (revealing the right sibling).
	if fX < 0 then

		-- No sibling at all, apply edge resistance to the total shift.
		if nil == self.m_RightSibling then

			fX = -(math.log(-(fX / fEdgeFactor) + 1) / 2.302585092994--[[kfLog10]]) * fEdgeFactor


		-- Sibling, allow one move width of shift before we appy resistance.
		elseif fX < -self.m_fWidth then

			fX = (fX) +(self.m_fWidth)
			fX = -(math.log(-(fX / fEdgeFactor) + 1) / 2.302585092994--[[kfLog10]]) * fEdgeFactor
			fX = (fX) - (self.m_fWidth)
		end


	-- We're scrolling to the right (revealing the left sibling).
	elseif fX > 0 then

		-- No sibling at all, apply edge resistance to the total shift.
		if nil == self.m_LeftSibling then

			fX = math.log(fX / fEdgeFactor + 1) / 2.302585092994--[[kfLog10]] * fEdgeFactor


		-- Sibling, allow one move width of shift before we apply resistance.
		elseif fX > self.m_fWidth then

			fX = (fX) - (self.m_fWidth)
			fX = math.log(fX / fEdgeFactor + 1) / 2.302585092994--[[kfLog10]] * fEdgeFactor
			fX = (fX) +(self.m_fWidth)
		end
	end

	-- Apply the new position.
	local root = self.m_RootMovieClip
	local _, fY = root:GetPosition()
	root:SetPosition(fX, fY)
end

--- <summary>
--- Callback invoked when the mouse/finger is released.
--- </summary>
--- <param name="oMovieClip">Instance of the captured shape on release.</param>
--- <param name="iX">World space position X of the mouse or finger.</param>
--- <param name="iY">World space position Y of the mouse or finger.</param>
--- <param name="bInShape">True if the released shape was active, false otherwise.</param>
--- <param name="iInputCaptureHitTestMask">Filtering mask on mouse/finger release.</param>
local function OnMouseUp(self,
	_,
	_,
	_,
	_,
	_)

	self.m_bMouseDown = false

	-- Only apply process if some movement occured.
	if self.m_bFirstMouseMove then

		-- Get the position between modified.
		local fBaseX = self.m_RootMovieClip:GetPosition()

		-- If we have a left tile, potentially move towards it.
		if nil ~= self.m_LeftSibling then

			-- Handle transition.
			if not SlimCSStringLib.IsNullOrEmpty(self.m_aScrolledTriggerNames[1]) and
			   0 == self.m_fTarget and
			   (
			   fBaseX > self.m_fWidth * 0.4--[[MovieScroller.kfTransitionThresholdFactor]] or
			   self.m_fLastMouseVelocity >= self.m_fWidth * 0.0666666666666667--[[MovieScroller.kfFlickThresholdFactor]]) then

				UIManager.TriggerTransition(self.m_aScrolledTriggerNames[1])
				self.m_fTarget = self.m_fWidth
			end
		end

		-- If we have a right tile, potentially move towards it.
		if nil ~= self.m_RightSibling then

			-- Handle transition.
			if not SlimCSStringLib.IsNullOrEmpty(self.m_aScrolledTriggerNames[3]) and
			   0 == self.m_fTarget and
			   (
			   fBaseX < -self.m_fWidth * 0.4--[[MovieScroller.kfTransitionThresholdFactor]] or
			   self.m_fLastMouseVelocity <= -self.m_fWidth * 0.0666666666666667--[[MovieScroller.kfFlickThresholdFactor]]) then

				UIManager.TriggerTransition(self.m_aScrolledTriggerNames[3])
				self.m_fTarget = -self.m_fWidth
			end
		end
	end

	-- Clear mouse move state.
	self.m_bFirstMouseMove = false

	-- Mouse velocity is only tracked while the mouse is held.
	self.m_fLastMouseVelocity = 0
end

--- <summary>
--- Per frame update operations.
--- </summary>
--- <param name="fDeltaTimeInSeconds">Length of the frame tick in seconds.</param>
local function Tick(self, fDeltaTimeInSeconds)

	-- If we entered from the left or right, set our starting offset.
	if UIManager.GetCondition 'MovieScroller_FromLeft' then

		UIManager.SetCondition('MovieScroller_FromLeft', false)
		local fX, fY = self.m_RootMovieClip:GetPosition()
		self.m_RootMovieClip:SetPosition(fX + 2 * self.m_fWidth, fY)

	elseif UIManager.GetCondition 'MovieScroller_FromRight' then

		UIManager.SetCondition('MovieScroller_FromRight', false)
		local fX, fY = self.m_RootMovieClip:GetPosition()
		self.m_RootMovieClip:SetPosition(fX - 2 * self.m_fWidth, fY)
	end

	-- Wait for scroller to settle before allowing input.
	local root = self.m_RootMovieClip
	local fBaseX, fBaseY = root:GetPosition()
	local bTest = self.m_fTarget == fBaseX

	-- Track mouse movement velocity in world terms.
	if self.m_bMouseDown then

		if nil == self.m_fLastMouseX or fDeltaTimeInSeconds <= 0 then

			self.m_fLastMouseX = fBaseX

		else

			self.m_fLastMouseVelocity = (fBaseX - self.m_fLastMouseX) / fDeltaTimeInSeconds
			self.m_fLastMouseX = fBaseX
		end


	-- Reset values.
	else

		self.m_fLastMouseVelocity = 0
		self.m_fLastMouseX = nil
	end

	-- If we haven't settled and the mouse is not held,
	-- return to center.
	if not bTest and not self.m_bMouseDown then

		-- Total distance left to auto scroll.
		local fDistance = math.abs(self.m_fTarget - fBaseX)

		-- Adjustment to apply - constants were tuned at 60 FPS, so we
		-- rescale delta time by that value.
		local fAdjust = 0.125--[[MovieScroller.kfAutoScrollSpeedFactor]] * self.m_fWidth * (fDeltaTimeInSeconds * 60)

		-- We start decelerating when we're closer than this distance.
		local fSlowdownDistance = 0.5--[[MovieScroller.kfAutoScrollDecelerateDistanceFactor]] * self.m_fWidth

		-- If the total distance we can move will place us within the slowdown
		-- distance, split it into two halves - slowed and unslowed - so we
		-- don't lose any energy due to time step.
		if fDistance - fAdjust <= fSlowdownDistance then

			-- Apply any unslowed adjustment first.
			if fDistance > fSlowdownDistance then

				local fUnslowedAdjust = fDistance - fSlowdownDistance
				fAdjust = (fAdjust) - (fUnslowedAdjust)
				fDistance = (fDistance) - (fUnslowedAdjust)

				-- Negate if necessary and then apply.
				if fBaseX > self.m_fTarget then

					fUnslowedAdjust = -fUnslowedAdjust
				end

				-- Apply.
				fBaseX = (fBaseX) +(fUnslowedAdjust)
			end

			-- Apply slodown.
			local fSlowdown = fDistance / fSlowdownDistance
			fSlowdown = math.pow(fSlowdown, 1.2--[[MovieScroller.kfAutoScrollDecelerationStrength]])
			fAdjust = (fAdjust) *(fSlowdown)
		end

		-- Swap the directionality of adjustment as needed.
		if fBaseX > self.m_fTarget then

			fAdjust = -fAdjust
		end

		-- Apply.
		fBaseX = (fBaseX) +(fAdjust)

		-- If we've either gone passed, or if we're within
		-- a threshold, snap to target.
		if fBaseX <= self.m_fTarget and fAdjust < 0 or
		   fBaseX >= self.m_fTarget and fAdjust > 0 or
		   math.abs(fBaseX - self.m_fTarget) <= 0.002--[[kfSnapThresholdFactor]] * self.m_fWidth then

			fBaseX = self.m_fTarget
		end

		-- Done, set new position.
		root:SetPosition(fBaseX, fBaseY)
	end

	-- Update test state based on scrolling flag.
	self:SetHitTestChildren(bTest)

	-- Match siblings to our position.
	if nil ~= self.m_LeftSibling then

		local _, fY = self.m_LeftSibling:GetPosition()
		self.m_LeftSibling:SetPosition(fBaseX - self.m_fWidth, fY)
	end
	if nil ~= self.m_RightSibling then

		local _, fY = self.m_RightSibling:GetPosition()
		self.m_RightSibling:SetPosition(fBaseX + self.m_fWidth, fY)
	end
end


function MovieScroller:Constructor() self.m_bMouseDown = false; self.m_bFirstMouseMove = false; self.m_ReferenceX = 0; self.m_fRootTransformX = 0; self.m_fWidth = 0; self.m_fTarget = 0; self.m_fLastMouseVelocity = 0;

	self.m_TickFunc = function(_, fDeltaTimeInSeconds)

		Tick(self, fDeltaTimeInSeconds)
	end
	self.m_MouseDownFunc = function(oMovieClip, iX, iY, bInShape)

		OnMouseDown(self, oMovieClip, iX, iY, bInShape)
	end
	self.m_MouseMoveFunc = function(oMovieClip, iX, iY, bInShape)

		OnMouseMove(self, oMovieClip, iX, iY, bInShape)
	end
	self.m_MouseUpFunc = function(oMovieClip, iX, iY, bInShape, iInputCaptureHitTestMask)

		OnMouseUp(self, oMovieClip, iX, iY, bInShape, iInputCaptureHitTestMask)
	end
end

--- <summary>
--- Called by the owner root movie to signal Viewport changed events.
--- </summary>
function MovieScroller:OnViewportChanged(_)

	-- Early out if no root.
	if nil == self.m_RootMovieClip then

		return
	end

	-- Drop all children.
	self:RemoveAllChildren()

	-- Re-add hit tester, viewport sized.
	self:AddChildHitShapeFullScreen()

	-- Re-add the full screen clippers.
	self.m_RootMovieClip:AddFullScreenClipper()
	if self.m_LeftSibling then self.m_LeftSibling:AddFullScreenClipper() end
	if self.m_RightSibling then self.m_RightSibling:AddFullScreenClipper() end

	-- Cache our width for tiling.
	local fLeft, _, fRight = self:GetBounds()
	self.m_fWidth = fRight - fLeft + 1--[[kExtraSpaceBetweenMovies]]
end

function MovieScroller:IsAtTarget()

	local fBaseX = self.m_RootMovieClip:GetPosition()
	return math.abs(self.m_fTarget - fBaseX) < 0.0001
end

--- <summary>
--- Call once prior to releasing this scroller.
--- </summary>
function MovieScroller:Deinit()

	self:RemoveEventListener('mouseUp'--[[MouseEvent.MOUSE_UP]], self.m_MouseUpFunc)
	self:RemoveEventListener('mouseMove'--[[MouseEvent.MOUSE_MOVE]], self.m_MouseMoveFunc)
	self:RemoveEventListener('mouseDown'--[[MouseEvent.MOUSE_DOWN]], self.m_MouseDownFunc)
	self:RemoveEventListener('tick'--[[Event.TICK]], self.m_TickFunc)

	self.m_fLastMouseX = nil
	self.m_fLastMouseVelocity = 0
	self.m_fTarget = 0
	self.m_fWidth = 0
	self.m_RightSibling = nil
	self.m_LeftSibling = nil
	self.m_fRootTransformX = 0
	self.m_ReferenceX = 0
	self.m_bFirstMouseMove = false
	self.m_bMouseDown = false
	self.m_aScrolledTriggerNames = nil
	self.m_aScrolledMovieNames = nil
	self.m_RootMovieClip = nil
end

--- <summary>
--- Call once immediately after instantiating a new scroller,
--- *after* adding this clip to its parent.
--- </summary>
--- <param name="root">The root that contains this croller.</param>
--- <param name="aScrolledMovieNames">Scroller set. We're expected to be on movie at middle element.</param>
--- <param name="aScrolledTriggerNames">Triggers to fire to advance to the next or previous state when scrolling.</param>
function MovieScroller:Init(
	root,
	aScrolledMovieNames,
	aScrolledTriggerNames)

	-- Cache siblings.
	self.m_RootMovieClip = root
	self.m_aScrolledMovieNames = aScrolledMovieNames
	self.m_aScrolledTriggerNames = aScrolledTriggerNames

	-- Hit tester, viewport sized.
	self:AddChildHitShapeFullScreen()

	-- Also, adding a full clipper. This needs to be the last element
	-- of the root.
	root:AddFullScreenClipper()

	-- Also, track neighbors if they exist, and give them a full screen clipper
	-- also.
	if not SlimCSStringLib.IsNullOrEmpty(self.m_aScrolledMovieNames[1]) then

		self.m_LeftSibling = root:GetSiblingRootMovieClip(self.m_aScrolledMovieNames[1])
		if self.m_LeftSibling then self.m_LeftSibling:AddFullScreenClipper() end
	end
	if not SlimCSStringLib.IsNullOrEmpty(self.m_aScrolledMovieNames[3]) then

		self.m_RightSibling = root:GetSiblingRootMovieClip(self.m_aScrolledMovieNames[3])
		if self.m_RightSibling then self.m_RightSibling:AddFullScreenClipper() end
	end

	-- Listeners for tick and input events.
	self:AddEventListener('tick'--[[Event.TICK]], self.m_TickFunc)
	self:AddEventListener('mouseDown'--[[MouseEvent.MOUSE_DOWN]], self.m_MouseDownFunc)
	self:AddEventListener('mouseMove'--[[MouseEvent.MOUSE_MOVE]], self.m_MouseMoveFunc)
	self:AddEventListener('mouseUp'--[[MouseEvent.MOUSE_UP]], self.m_MouseUpFunc)

	-- Hit testable by default.
	self:SetHitTestChildren(true)
	self:SetHitTestSelfMode(2--[[HitTestMode.m_iHorizontalDragInputOnly]])

	-- Cache our width for tiling.
	local fLeft, _, fRight = self:GetBounds()
	self.m_fWidth = fRight - fLeft + 1--[[kExtraSpaceBetweenMovies]]
end
return MovieScroller
