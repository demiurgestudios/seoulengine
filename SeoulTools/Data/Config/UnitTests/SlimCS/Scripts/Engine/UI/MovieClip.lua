-- MovieClip, the base class of any script wrapper
-- around a Falcon::MovieClipInstance.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



--- <summary>
--- Class types that associate contextual information
--- with the setter of a game state data value.
--- </summary>
--- <remarks>
--- Context is used for developer hot loading, to determine
--- when and if a game state data value should be backed up
--- and restored.
--- </remarks>
interface 'IContextualGameStateSetter'




local MovieClip = class('MovieClip', 'DisplayObject', nil, nil, 'IContextualGameStateSetter')







MovieClip.ALLOW_CLICK_PASSTHROUGH_EVENT = 'allowclickpassthrough'

-- utility, equivalent to the implementation of New in class,
-- except that the constructor name is always 'Constructor'.
local function InvokeConstructor(tClass, oInstance, ...)

	if nil == tClass then

		return
	end

	local ctor = tClass.Constructor
	if nil ~= ctor then

		ctor(oInstance, ...)
	end
end

local function InPlaceNew(tClass, oInstance, ...)

	setmetatable(oInstance, tClass)
	InvokeConstructor(tClass, oInstance, ...)
	return oInstance
end

function MovieClip:AddChild(oDisplayObject, sName, iDepth)

	self.m_udNativeInstance:AddChild(
		oDisplayObject:get_NativeInstance(),
		sName,
		iDepth)
end


function MovieClip:AddChildAnimationNetwork(
	networkFilePath,
	dataFilePath,
	callback,
	bStartAlphaZero)

	local network = self.m_udNativeInstance:AddChildAnimationNetwork(
		networkFilePath,
		dataFilePath,
		callback)

	-- Use alpha 0 here instead of visible - for legacy reasons (from ActionScript),
	-- visibility is effectively "enabled/disabled". It disables update actions in
	-- addition to rendering visibility.
	if bStartAlphaZero then

		network:SetAlpha(0)
	end

	return Animation2DNetworkAsync:New(self, network)
end
-- /#if SEOUL_WITH_ANIMATION_2D

function MovieClip:AddChildBitmap(
	filePath,
	iWidth,
	iHeight,
	sName,
	bCenter,
	iDepth,
	bPrefetch)

	return self.m_udNativeInstance:AddChildBitmap(
		filePath,
		iWidth,
		iHeight,
		sName,
		bCenter,
		iDepth,
		bPrefetch)
end

function MovieClip:AddChildFacebookBitmap(
	sFacebookUserGuid,
	iWidth,
	iHeight,
	defaultImageFilePath)

	return self.m_udNativeInstance:AddChildFacebookBitmap(
		sFacebookUserGuid,
		iWidth,
		iHeight,
		defaultImageFilePath)
end

function MovieClip:AddChildFx(
	fxNameOrFilePath,
	iWidth,
	iHeight,
	iFxFlags,
	oParentIfWorldspace,
	sVariationName)

	return self.m_udNativeInstance:AddChildFx(
		fxNameOrFilePath,
		iWidth,
		iHeight,
		iFxFlags,
		oParentIfWorldspace and oParentIfWorldspace.m_udNativeInstance,
		sVariationName)
end

function MovieClip:AddChildHitShape(
	fLeft,
	fTop,
	fRight,
	fBottom,
	sName,
	iDepth)

	return self.m_udNativeInstance:AddChildHitShape(
		fLeft,
		fTop,
		fRight,
		fBottom,
		sName,
		iDepth)
end

function MovieClip:AddChildHitShapeFullScreen(sName)

	return self.m_udNativeInstance:AddChildHitShapeFullScreen(sName)
end

function MovieClip:AddChildHitShapeWithMyBounds(sName)

	return self.m_udNativeInstance:AddChildHitShapeWithMyBounds(sName)
end

function MovieClip:AddChildStage3D(
	filePath,
	iWidth,
	iHeight,
	sName,
	bCenter)

	return self.m_udNativeInstance:AddChildStage3D(
		filePath,
		iWidth,
		iHeight,
		sName,
		bCenter)
end

function MovieClip:GetHitTestableBounds(iMask)

	return self.m_udNativeInstance:GetHitTestableBounds(iMask)
end

function MovieClip:GetHitTestableLocalBounds(iMask)

	return self.m_udNativeInstance:GetHitTestableLocalBounds(iMask)
end

function MovieClip:GetHitTestableWorldBounds(iMask)

	return self.m_udNativeInstance:GetHitTestableWorldBounds(iMask)
end

function MovieClip:GetBoundsUsingAuthoredBoundsIfPresent(bWorldBounds)

	local bounds = self:GetChildByPath(MovieClip, 'mcBounds')
	if bWorldBounds then

		if nil ~= bounds then

			return bounds:GetWorldBounds()
		end
		return self:GetWorldBounds()
	end

	if nil ~= bounds then

		local fLeft, fTop, fRight, fBottom = bounds:GetBounds()
		local fPosX, fPosY = self:GetPosition()
		local fScaleX, fScaleY = self:GetScale()
		fLeft = fScaleX * fLeft + fPosX
		fRight = fScaleX * fRight + fPosX
		fTop = fScaleY * fTop + fPosY
		fBottom = fScaleY * fBottom + fPosY
		return fLeft, fTop, fRight, fBottom
	end
	return self:GetBounds()
end

-- See DisplayObject::GetBoundsIn for more information
function MovieClip:GetBoundsInUsingAuthoredBoundsIfPresent(targetCoordinateSpace)

	local bounds = self:GetChildByPath(MovieClip, 'mcBounds')
	if nil ~= bounds then

		return bounds:GetBoundsIn(targetCoordinateSpace)
	end
	return self:GetBoundsIn(targetCoordinateSpace)
end

function MovieClip:GetHitTestableBoundsUsingAuthoredBoundsIfPresent(iHitMask, bWorldBounds)

	local bounds = self:GetChildByPath(MovieClip, 'mcBounds')
	if bWorldBounds then

		if nil ~= bounds then

			return bounds:GetHitTestableWorldBounds(iHitMask)
		end
		return self:GetWorldBounds()
	end

	if nil ~= bounds then

		local fLeft, fTop, fRight, fBottom = bounds:GetHitTestableBounds(iHitMask)
		local fPosX, fPosY = self:GetPosition()
		local fScaleX, fScaleY = self:GetScale()
		fLeft = fScaleX * fLeft + fPosX
		fRight = fScaleX * fRight + fPosX
		fTop = fScaleY * fTop + fPosY
		fBottom = fScaleY * fBottom + fPosY
		return fLeft, fTop, fRight, fBottom
	end
	return self:GetHitTestableBounds(iHitMask)
end

--- <summary>
--- Retrieve a child by absolute index.
--- </summary>
--- <param name="iIndex">0-based index of the child DisplayObject to retrieve.</param>
--- <returns>A DisplayObject at the given index or null if an invalid index.</returns>
function MovieClip:GetChildAt(iIndex)

	return self.m_udNativeInstance:GetChildAt(iIndex)
end

--- <summary>
--- Retrieve a child DisplayObject of the current MovieClip and cast it to the given type.
--- </summary>
--- <typeparam name="T">Coerced target type of the DisplayObject.</typeparam>
--- <param name="asParts">Path to the child to retrieve.</param>
--- <returns>The target DisplayObject or nil if no such DisplayObject.</returns>
--- <remarks>
--- The instance retrieve will be wrapped in a script class binding as needed
--- (e.g. if the return instance is of type Button, a new Button script class
--- will be instantiated and wrapped around the native instance).
--- 
--- Note that the child is expected to already be of that type and this will
--- not allow you to convert to a more derived type.
--- (IE making a MovieClip into something more specific will not work).
--- </remarks>
function MovieClip:GetChildByPath(T, ...)


	local child = self.m_udNativeInstance:GetChildByPath(...)



	return child

end

function MovieClip:GetChildByName(sName)

	return self:GetChildByPath(DisplayObject, sName)
end

function MovieClip:GetChildByNameFromSubTree(sName)

	return self.m_udNativeInstance:GetChildByNameFromSubTree(sName)
end

--- <summary>
--- Retrieve the 0-based index of the child in the current
--- parent.
--- </summary>
--- <param name="child">The child instance to search for.</param>
--- <returns>The 0-based index if found, or -1 if the child is not a child of this parent.</returns>
function MovieClip:GetChildIndex(child)

	local iCount = self:GetChildCount()
	for i = 0, (iCount) -1 do

		local d = self:GetChildAt(i)
		if d == child then

			return i
		end
	end

	return -1
end

function MovieClip:IncreaseAllChildDepthByOne()

	self.m_udNativeInstance:IncreaseAllChildDepthByOne()
end

--- <summary>
--- Alternative to AddChildBitmap that returns an unparented
--- Bitmap instance with no configuration.
--- </summary>
--- <returns>An unparented Bitmap instance.</returns>
function MovieClip:NewBitmap()

	return self.m_udNativeInterface:NewBitmap()
end

function MovieClip:NewMovieClip(sClassName, ...)

	return self.m_udNativeInterface:NewMovieClip(sClassName, ...)
end

function MovieClip:CreateMovieClip(T, ...)

	return self.m_udNativeInterface:NewMovieClip(TypeOf(T):get_Name(), ...)
end

-- Fire an 'out-of-band', manual hit test against the Falcon scene graph,
-- starting at self. Not part of normal input handling, can be used for
-- special cases.
function MovieClip:HitTest(iMask, fX, fY)

	return self.m_udNativeInstance:HitTest(
		iMask,
		fX,
		fY)
end

-- UI system BroadcastEvent calls are normally delivered only to the root
-- MovieClip. This function hooks a function into the root MovieClip that
-- will forward BroadcastEvent calls to self.
function MovieClip:RegisterBroadcastEventHandlerWithRootMovie(sKey, oFunction)

	if nil ~= sKey then

		local oRootMC = self:GetRootMovieClip()

		if nil == oRootMC then

			--[[CoreNative.Warn(
				tostring("RegisterBroadcastEventHandlerWithRootMovie failed to register key '" .. tostring(sKey) .. "'") ..tostring(
				' because no root movie clip was found for MovieClip ' .. tostring(self:GetName()) .. '. ') ..
				'Did you attempt to register a movie clip for an event before the move clip was parented?')]]
			return
		end

		local oOriginalFunction = oRootMC[sKey]
		if nil ~= oOriginalFunction then

			oRootMC[sKey] = function(...)

				oOriginalFunction(...)
				oFunction(...)
			end

		else

			oRootMC[sKey] = oFunction
		end
	end
end

-- General purpose functionality similar to RootMovieClip:RootAnimOut(),
-- except targeting an inner MovieClip.
function MovieClip:AnimOutAndDisableInput(sEventName, completionCallback, sAnimOutFrameLabel)

	-- Optional argument.
	sAnimOutFrameLabel = sAnimOutFrameLabel or 'AnimOut'

	local iHitTestSelfMask = self:GetHitTestSelfMode()
	local iHitTestChildrenMask = self:GetHitTestChildrenMode()
	self:SetHitTestSelf(false)
	self:SetHitTestChildren(false)

	-- Setup an event listener for the AnimOut event.
	local callbackContainer = {}
	callbackContainer.m_Callback = function(...)

		-- Invoke the specific completion callback.
		if nil ~= completionCallback then

			completionCallback(...)
		end

		-- If input was enabled prior to AnimOut, re-enable it now.
		self:SetHitTestSelfMode(iHitTestSelfMask)
		self:SetHitTestChildrenMode(iHitTestChildrenMask)

		-- Remove the event listener.
		self:RemoveEventListener(sEventName, callbackContainer.m_Callback)
	end

	-- Add the event listener.
	self:AddEventListener(sEventName, callbackContainer.m_Callback)

	-- Play the AnimOut frame.
	self:GotoAndPlayByLabel(sAnimOutFrameLabel)
end

function MovieClip:UnsetGameStateValue(k)

	self:SetGameStateValue(k, nil)
end

function MovieClip:GetGameStateValue(T, k)

	return DynamicGameStateData.GetValue(T, k)
end

function MovieClip:GetGameStateValue2_FD5EA049(T, k, defaultVal)

	local output = self:GetGameStateValue(T, k)
	if output == nil then

		return defaultVal
	end
	return output
end

function MovieClip:SetGameStateValue(k, v)

	local name = nil



	DynamicGameStateData.SetValue(k, v, name)
end

function MovieClip:AddFullScreenClipper()

	self.m_udNativeInstance:AddFullScreenClipper()
end

function MovieClip:GetAbsorbOtherInput()

	return self.m_udNativeInstance:GetAbsorbOtherInput()
end

function MovieClip:GetChildCount()

	return self.m_udNativeInstance:GetChildCount()
end

function MovieClip:GetCurrentFrame()

	return self.m_udNativeInstance:GetCurrentFrame()
end

function MovieClip:GetCurrentLabel()

	return self.m_udNativeInstance:GetCurrentLabel()
end

function MovieClip:GetDepth3D()

	return self.m_udNativeInstance:GetDepth3D()
end

function MovieClip:GetExactHitTest()

	return self.m_udNativeInstance:GetExactHitTest()
end

function MovieClip:GetHitTestChildren()

	return self.m_udNativeInstance:GetHitTestChildren()
end

function MovieClip:GetHitTestClickInputOnly()

	return 0 ~= bit.band(self:GetHitTestSelfMode(), 1--[[HitTestMode.m_iClickInputOnly]])
end

function MovieClip:GetHitTestChildrenClickInputOnly()

	return 0 ~= bit.band(self:GetHitTestChildrenMode(), 1--[[HitTestMode.m_iClickInputOnly]])
end

function MovieClip:GetHitTestChildrenMode()

	return self.m_udNativeInstance:GetHitTestChildrenMode()
end

function MovieClip:GetHitTestSelf()

	return self.m_udNativeInstance:GetHitTestSelf()
end

function MovieClip:GetHitTestSelfMode()

	return self.m_udNativeInstance:GetHitTestSelfMode()
end

function MovieClip:GetTotalFrames()

	return self.m_udNativeInstance:GetTotalFrames()
end

function MovieClip:GotoAndPlay(iFrame)

	return self.m_udNativeInstance:GotoAndPlay(iFrame)
end

function MovieClip:GotoAndPlayByLabel(sLabel)

	return self.m_udNativeInstance:GotoAndPlayByLabel(sLabel)
end

function MovieClip:GotoAndStop(iFrame)

	return self.m_udNativeInstance:GotoAndStop(iFrame)
end

function MovieClip:GotoAndStopByLabel(sLabel)

	return self.m_udNativeInstance:GotoAndStopByLabel(sLabel)
end

function MovieClip:HasChild(child)

	return child:GetParent() == self
end

function MovieClip:HasDescendant(descendant)

	while true do

		local parent = descendant:GetParent()
		if parent == nil then

			return false
		end
		if parent == self then

			return true
		end
		descendant = parent
	end
end

function MovieClip:IsPlaying()

	return self.m_udNativeInstance:IsPlaying()
end

function MovieClip:Play()

	self.m_udNativeInstance:Play()
end

function MovieClip:RemoveAllChildren()

	self.m_udNativeInstance:RemoveAllChildren()
end

function MovieClip:RemoveChildAt(iIndex)

	return self.m_udNativeInstance:RemoveChildAt(iIndex)
end

function MovieClip:RemoveChildByName(sName)

	return self.m_udNativeInstance:RemoveChildByName(sName)
end

function MovieClip:RestoreTypicalDefault()

	self.m_udNativeInstance:RestoreTypicalDefault()
end

function MovieClip:SetAbsorbOtherInput(bAbsorbOtherInput)

	self.m_udNativeInstance:SetAbsorbOtherInput(bAbsorbOtherInput)
end

function MovieClip:SetDepth3D(fDepth3D)

	self.m_udNativeInstance:SetDepth3D(fDepth3D)
end

function MovieClip:SetDepth3DFromYLoc(fYLoc)

	self.m_udNativeInstance:SetDepth3DFromYLoc(fYLoc)
end

function MovieClip:SetInputActionDisabled(bInputActionDisabled)

	self.m_udNativeInstance:SetInputActionDisabled(bInputActionDisabled)
end

function MovieClip:SetEnterFrame(bEnableEnterFrame)

	self.m_udNativeInstance:SetEnterFrame(bEnableEnterFrame)
end

function MovieClip:SetAutoCulling(bEnableAutoCulling)

	self.m_udNativeInstance:SetAutoCulling(bEnableAutoCulling)
end

function MovieClip:SetAutoDepth3D(bEnableAutoDepth3D)

	self.m_udNativeInstance:SetAutoDepth3D(bEnableAutoDepth3D)
end

function MovieClip:SetCastPlanarShadows(bCastPlanarShadows)

	self.m_udNativeInstance:SetCastPlanarShadows(bCastPlanarShadows)
end

function MovieClip:SetDeferDrawing(bDeferDrawing)

	self.m_udNativeInstance:SetDeferDrawing(bDeferDrawing)
end

function MovieClip:SetChildrenVisible(bVisible)

	self.m_udNativeInstance:SetChildrenVisible(bVisible)
end

function MovieClip:SetTickEvent(bEnableTickEvent)

	self.m_udNativeInstance:SetTickEvent(bEnableTickEvent)
end

function MovieClip:SetExactHitTest(bExactHitTest)

	self.m_udNativeInstance:SetExactHitTest(bExactHitTest)
end

function MovieClip:SetHitTestChildren(bHitTestChildren)

	self.m_udNativeInstance:SetHitTestChildren(bHitTestChildren)
end

function MovieClip:SetHitTestChildrenMode(iMask)

	self.m_udNativeInstance:SetHitTestChildrenMode(iMask)
end

function MovieClip:SetHitTestClickInputOnly(value)

	local mode = self:GetHitTestSelfMode()
	if value then

		mode = bit.bor(mode, 1--[[HitTestMode.m_iClickInputOnly]])

	else

		mode = bit.band(mode, -2--[[bit.bnot(HitTestMode.m_iClickInputOnly)]])
	end

	self:SetHitTestSelfMode(mode)
end

function MovieClip:SetHitTestChildrenClickInputOnly(value)

	local mode = self:GetHitTestChildrenMode()
	if value then

		mode = bit.bor(mode, 1--[[HitTestMode.m_iClickInputOnly]])

	else

		mode = bit.band(mode, -2--[[bit.bnot(HitTestMode.m_iClickInputOnly)]])
	end

	self:SetHitTestChildrenMode(mode)
end

function MovieClip:SetHitTestSelf(bHitTestSelf)

	self.m_udNativeInstance:SetHitTestSelf(bHitTestSelf)
end

function MovieClip:SetHitTestSelfMode(iMask)

	self.m_udNativeInstance:SetHitTestSelfMode(iMask)
end

function MovieClip:SetReorderChildrenFromDepth3D(bReorder)

	self.m_udNativeInstance:SetReorderChildrenFromDepth3D(bReorder)
end

function MovieClip:Stop()

	self.m_udNativeInstance:Stop()
end






























--DEBUG
return MovieClip
