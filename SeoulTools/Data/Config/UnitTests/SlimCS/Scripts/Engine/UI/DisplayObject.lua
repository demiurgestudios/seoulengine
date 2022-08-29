-- Contains DisplayObject, the base class of any script wrapper
-- around a native Falcon::Instance.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.





--- <summary>
--- Utility class, wraps a Native.ScriptUIMovie with a handle style API.
--- </summary>
--- <remarks>
--- Native.ScriptUIMovie is "special". Most native user data that is bound
--- into SlimCS is the native object itself, and the object has garbage collected
--- lifespan.
---
--- In the case of Native.ScriptUIMovie, the object is a pointer to
--- a subclass of Seoul::UIMovie and that pointer can be set to nullptr. Unfortunately,
--- this does *not* invalidate the Native.ScriptUIMovie object (null == m_udNative will
--- still return false, even after the underlying pointer has been set to nullptr), though any
--- attempts to call methods on the object will raise a NullReferenceException. This
--- is similar to a Unity ScriptableObject in behavior.
---
--- NativeScriptUIMovieHandle exists to resolve this issue by wrapping the underlying
--- Native.ScriptUIMovie instance and forcing calls to Target to resolve it, which
--- will only return the instance if the underlying pointer is still valid.
---
--- Note that this class is generally overkill and unneeded. As long as all users
--- of a Native.ScriptUIMovie are internal to that movie, there is no lifespan issue.
---
--- Mainly this class is useful for (e.g.) HTTP callbacks that can return after
--- the UIMovie and its graph have been destroyed (in native), since the GC
--- owned script interface can still exist until those callbacks are released.
--- </remarks>
local NativeScriptUIMovieHandle = class('NativeScriptUIMovieHandle', nil, nil, nil, 'IContextualGameStateSetter')







function NativeScriptUIMovieHandle:Constructor(udNative)

	self.m_udNative = udNative
end



function NativeScriptUIMovieHandle:get_IsAlive()

	return _G.SeoulIsNativeValid(self.m_udNative)
end


function NativeScriptUIMovieHandle:SetGameStateValue(k, v)

	local name = nil









	DynamicGameStateData.SetValue(k, v, name)
end



function NativeScriptUIMovieHandle:get_Target()

	if self:get_IsAlive() then

		return self.m_udNative

	else

		return nil
	end
end



local EventDispatcher = class 'EventDispatcher'



local ListenerCollection = class('ListenerCollection', nil, 'EventDispatcher+ListenerCollection')



function ListenerCollection:Constructor() self.m_Count = 0; end


local ListenerEntry = class('ListenerEntry', nil, 'EventDispatcher+ListenerCollection+ListenerEntry')




function ListenerCollection:Add(listener)

	if nil == self.m_Listeners then

		self.m_Listeners = {}

	else

		-- Check - only register once, nop if already in the list.
		for _, o in ipairs(self.m_Listeners) do

			if o.m_Listener == listener then

				return
			end
		end
	end

	-- Increment count.
	self.m_Count = self.m_Count + 1

	-- Add a new entry.
	table.insert(
		self.m_Listeners,
		initlist(ListenerEntry:New(),

		false, 'm_Listener', listener))

end



function ListenerCollection:get_Count()

	return self.m_Count
end


function ListenerCollection:Dispatch(target, _, ...)

	if nil == self.m_Listeners then

		return nil, false
	end

	-- Manually check i, in case there is an add mid-stream.
	-- We want to match AS semantics, which says that
	-- listeners added during dispatch are not invoked
	-- until the next dispatch.
	local listeners = self.m_Listeners
	local count = #listeners
	local i = 1
	local toReturn = nil
	local bReturn = false
	while i <= count do

		-- Removed, cleanup now.
		local e = listeners[i]
		if nil == e.m_Listener then

			table.remove(listeners, i)
			count = count - 1

		else

			-- Invoke.
			toReturn = e.m_Listener(target, ...)
			bReturn = true

			-- Advance.
			i = i + 1
		end
	end

	return toReturn, bReturn
end

function ListenerCollection:Remove(listener)

	-- Check.
	if nil == self.m_Listeners then

		return
	end

	-- Find and remove.
	for i, o in ipairs(self.m_Listeners) do

		if o.m_Listener == listener then

			-- Decrement.
			self.m_Count = self.m_Count - 1

			-- Null out, will be removed by dispatch passes.
			o.m_Listener = nil
			return
		end
	end
end




EventDispatcher.CHANGE = 'change'

function EventDispatcher:GetParent()

	return nil
end

function EventDispatcher:AddEventListener(sName, listener)

	if nil == sName then

		error('called AddEventListener with a nil sName value.', 2)
	end
	if nil == listener then

		error('called AddEventListener with a nil listener value.', 2)
	end

	local tEventListeners = self.m_tEventListeners
	if nil == tEventListeners then

		tEventListeners = {}
		self.m_tEventListeners = tEventListeners
	end

	local listeners = tEventListeners[sName]
	if nil == listeners then

		listeners = ListenerCollection:New()
		tEventListeners[sName] = listeners
	end

	-- Convenience handling for Event.ENTER_FRAME and Event.TICK
	local udNativeInstance = rawget(self, 'm_udNativeInstance')
	if nil ~= udNativeInstance and 0 == listeners:get_Count() then

		if 'enterFrame'--[[Event.ENTER_FRAME]] == sName then

			udNativeInstance:SetEnterFrame(true)

		elseif 'tick'--[[Event.TICK]] == sName then

			udNativeInstance:SetTickEvent(true)

		elseif 'tickScaled'--[[Event.TICK_SCALED]] == sName then

			udNativeInstance:SetTickScaledEvent(true)
		end
	end

	listeners:Add(listener)
end

function EventDispatcher:RemoveEventListener(sName, listener)

	if nil == sName then

		error('called RemoveEventListener with a nil sName value.', 2)
	end

	local tEventListeners = self.m_tEventListeners
	if nil == tEventListeners then

		return
	end

	local listeners = tEventListeners[sName]
	if nil == listeners then

		return
	end

	listeners:Remove(listener)

	-- Convenience handling for Event.ENTER_FRAME and Event.TICK
	local udNativeInstance = rawget(self, 'm_udNativeInstance')
	if nil ~= udNativeInstance and 0 == listeners:get_Count() then

		if 'enterFrame'--[[Event.ENTER_FRAME]] == sName then

			udNativeInstance:SetEnterFrame(false)

		elseif 'tick'--[[Event.TICK]] == sName then

			udNativeInstance:SetTickEvent(false)

		elseif 'tickScaled'--[[Event.TICK_SCALED]] == sName then

			udNativeInstance:SetTickScaledEvent(false)
		end
	end
end

function EventDispatcher:RemoveAllEventListenersOfType(sName)

	if nil == sName then

		error('called RemoveEventListener with a nil sName value.', 2)
	end

	local tEventListeners = self.m_tEventListeners
	if nil == tEventListeners then

		return
	end

	local tListeners = tEventListeners[sName]
	if nil == tListeners then

		return
	end

	rawset(tEventListeners, sName, nil)

	-- Convenience handling fo Event.ENTER_FRAME and Event.TICK
	local udNativeInstance = rawget(self, 'm_udNativeInstance')
	if nil ~= udNativeInstance then

		if 'enterFrame'--[[Event.ENTER_FRAME]] == sName then

			udNativeInstance:SetEnterFrame(false)

		elseif 'tick'--[[Event.TICK]] == sName then

			udNativeInstance:SetTickEvent(false)
		end
	end
end

--- <summary>
--- Fire any listeners registered with oTarget.
--- </summary>
--- <param name="oTarget">EventDispatcher that contains listeners.</param>
--- <param name="sEventName">Target event to dispatch.</param>
--- <param name="vararg0">Args to pass to the listeners.</param>
--- <returns>Return value from the last invoked listener, and true/false if any listeners were invoked.</returns>
function EventDispatcher:DispatchEvent(target, sEventName, ...)

	local toReturn = nil
	local bReturn = false
	local tEventListeners = target.m_tEventListeners
	if nil ~= tEventListeners then

		local listeners = tEventListeners[sEventName]
		if nil ~= listeners then

			toReturn, bReturn = listeners:Dispatch(target, sEventName, ...)
		end
	end

	return toReturn, bReturn
end

--- <summary>
--- Fire any listeners registered with oTarget. Also bubbles to parents unless the event is handled.
--- </summary>
--- <param name="oTarget">EventDispatcher that contains listeners.</param>
--- <param name="sEventName">Target event to dispatch.</param>
--- <param name="vararg0">Args to pass to the listeners.</param>
--- <returns>Return value from the last invoked listener, and true/false if any listeners were invoked.</returns>
function EventDispatcher:DispatchEventAndBubble(target, sEventName, ...)

	local toReturn = nil
	local bReturn = false
	local tEventListeners = target.m_tEventListeners
	if nil ~= tEventListeners then

		local listeners = tEventListeners[sEventName]
		if nil ~= listeners then

			toReturn, bReturn = listeners:Dispatch(target, sEventName, ...)
		end
	end

	if bReturn then

		return toReturn, bReturn
	end

	local parent = target:GetParent()
	if nil ~= parent then

		return parent:DispatchEventAndBubble(parent, sEventName, ...)
	end

	return toReturn, bReturn
end


interface 'IVisible'





local DisplayObject = class('DisplayObject', 'EventDispatcher', nil, nil, 'IVisible')

DisplayObject.kfFadeInTimeSecs = 0.2







-- Hooks called form native on hot load events.


















-- Setup via some low-level mechanics of DisplayObject
-- creation so otherwise never explicitly assigned
-- from the C# compiler's view.






--- <summary>
--- Handle wrapper variation of GetNative().
--- </summary>
--- <returns>A newly instantiated handle wrapper around GetNative().</returns>
--- <remarks>
--- The handle wrapper ensures that the native API is valid before it is returned.
--- If the native API is not valid, calls to Target will return null instead.
--- </remarks>
function DisplayObject:BindNativeHandle()

	return NativeScriptUIMovieHandle:New(self.m_udNativeInterface)
end

--- <summary>
--- Retrieve a reference to the native movie API.
--- </summary>
--- <returns>Native movie API</returns>
--- <remarks>
--- The native movie API allows access to native functionality
--- available via Seoul::UIMovie, Seoul::ScriptUIMovie, etc.
---
--- NOTE: All method calls will fail if IsNativeValid() returns
--- true for the returned instance. This only occurs if the underlying
--- native movie has been destroyed. This is not typical - it can occur
--- if "dangling" references are maintained (e.g. in an HTTP callback).
--- </remarks>
function DisplayObject:GetNative()

	return self.m_udNativeInterface
end

function DisplayObject:GetNative0_B46431A3(T)


	return self.m_udNativeInterface
end



function DisplayObject:IsNativeValid()

	return _G.SeoulIsNativeValid(self:GetNative())
end

function DisplayObject:GetParent()

	return self.m_udNativeInstance:GetParent()
end

-- Return the root MovieClip for this DisplayObject.
function DisplayObject:GetRootMovieClip()

	if self:IsNativeValid() then

		return self.m_udNativeInterface:GetRootMovieClip()

	else

		return nil
	end
end

--- <summary>
--- Convenience alias.
--- </summary>
--- <param name="sType">Motion type to instantiate.</param>
--- <param name="aArgs">Arguments to pass to the instantiated motion.</param>
--- <returns>Unique id of the instantiated motion util.</returns>
function DisplayObject:AddMotionNoCallback(sType, ...)

	return self.m_udNativeInstance:AddMotion(sType, nil, ...)
end

function DisplayObject:AddMotion(sType, callback, ...)

	return self.m_udNativeInstance:AddMotion(sType, callback, ...)
end

function DisplayObject:AddTween(eTarget, ...)

	return self.m_udNativeInstance:AddTween(eTarget, ...)
end

function DisplayObject:AddTweenCurve(eType, eTarget, ...)

	return self.m_udNativeInstance:AddTweenCurve(eType, eTarget, ...)
end

function DisplayObject:CancelMotion(iIdentifier)

	self.m_udNativeInstance:CancelMotion(iIdentifier)
end

function DisplayObject:CancelTween(iIdentifier)

	self.m_udNativeInstance:CancelTween(iIdentifier)
end

function DisplayObject:Clone()

	return self.m_udNativeInstance:Clone()
end

function DisplayObject:GetAlpha()

	return self.m_udNativeInstance:GetAlpha()
end

function DisplayObject:GetBounds()

	return self.m_udNativeInstance:GetBounds()
end

--- <summary>
--- Return the bounds of this DisplayObject in the coordinate space of targetCoordinateSpace.
--- </summary>
--- <param name="targetCoordinateSpace">DisplayObject that defines the coordinate space of the returned bounds.</param>
--- <returns>Bounds as (left, top, right, bottom)</returns>
--- <example>
--- var (l, t, r, b) = obj.GetBoundsIn(do);                    // Bounds in local space of the object - equivalent to GetLocalBounds().
--- (l, t, r, b) = obj.GetBoundsIn(do.GetParent());        // Bounds in the space of the parent - equivalent to GetBounds().
--- (l, t, r, b) = obj.GetBoundsIn(do.GetRootMovieClip()); // Bounds in world space - equivalent to GetWorldBounds().
--- </example>
function DisplayObject:GetBoundsIn(targetCoordinateSpace)

	return self.m_udNativeInstance:GetBoundsIn(targetCoordinateSpace.m_udNativeInstance)
end

function DisplayObject:GetClipDepth()

	return self.m_udNativeInstance:GetClipDepth()
end

function DisplayObject:GetDepthInParent()

	return self.m_udNativeInstance:GetDepthInParent()
end

function DisplayObject:GetColorTransform()

	return self.m_udNativeInstance:GetColorTransform()
end

function DisplayObject:GetDebugName()




	return ''--[[String.Empty]]

end

--- <summary>
--- Return the object space (*not* local space) height of the DisplayObject.
--- </summary>
--- <returns>Height in pixels.</returns>
--- <remarks>
--- This is equivalent to:
--- var (_, t, _, b) = GetBounds();
--- return (b - t);
--- </remarks>
function DisplayObject:GetHeight()

	local _, t, _, b = self:GetBounds()
	return b - t
end

function DisplayObject:GetIgnoreDepthProjection()

	return self.m_udNativeInstance:GetIgnoreDepthProjection()
end

function DisplayObject:GetLocalBounds()

	return self.m_udNativeInstance:GetLocalBounds()
end

function DisplayObject:GetLocalMouse()

	return self.m_udNativeInstance:GetLocalMousePosition()
end

function DisplayObject:GetName()

	return self.m_udNativeInstance:GetName()
end

function DisplayObject:GetFullName()

	return self.m_udNativeInstance:GetFullName()
end



function DisplayObject:get_NativeInstance()

	return self.m_udNativeInstance
end


function DisplayObject:GetPosition()

	return self.m_udNativeInstance:GetPosition()
end

function DisplayObject:GetPositionX()

	return self.m_udNativeInstance:GetPositionX()
end

function DisplayObject:GetPositionY()

	return self.m_udNativeInstance:GetPositionY()
end

function DisplayObject:GetRotation()

	return self.m_udNativeInstance:GetRotation()
end

function DisplayObject:GetScale()

	return self.m_udNativeInstance:GetScale()
end

function DisplayObject:GetScaleX()

	return self.m_udNativeInstance:GetScaleX()
end

function DisplayObject:GetScaleY()

	return self.m_udNativeInstance:GetScaleY()
end

function DisplayObject:GetScissorClip()

	return self.m_udNativeInstance:GetScissorClip()
end

function DisplayObject:GetVisible()

	return self.m_udNativeInstance:GetVisible()
end

function DisplayObject:GetVisibleToRoot()

	return self.m_udNativeInstance:GetVisibleToRoot()
end

--- <summary>
--- Return the object space (*not* local space) width of the DisplayObject.
--- </summary>
--- <returns>Width in pixels.</returns>
--- <remarks>
--- This is equivalent to:
--- var (l, _, r, _) = GetBounds();
--- return (r - l);
--- </remarks>
function DisplayObject:GetWidth()

	local l, _, r, _ = self:GetBounds()
	return r - l
end

function DisplayObject:GetWorldBounds()

	return self.m_udNativeInstance:GetWorldBounds()
end

function DisplayObject:GetWorldDepth3D()

	return self.m_udNativeInstance:GetWorldDepth3D()
end

function DisplayObject:GetWorldPosition()

	return self.m_udNativeInstance:GetWorldPosition()
end

function DisplayObject:LocalToWorld(fX, fY)

	return self.m_udNativeInstance:LocalToWorld(fX, fY)
end

function DisplayObject:HasParent()

	return self.m_udNativeInstance:HasParent()
end

--- <summary>
--- Given a world space point, check if it intersects this DisplayObject.
--- </summary>
--- <param name="fWorldX">X position in world space.</param>
--- <param name="fWorldY">Y position in world space.</param>
--- <param name="bExactHitTest">
--- If true, exactly triangles of the Shape
--- are used (when applicable). Otherwise, only tests against the bounds.
--- </param>
--- <returns>true on intersection, false otherwise.</returns>
--- <remarks></remarks>
function DisplayObject:Intersects(fWorldX, fWorldY, bExactHitTest)

	return self.m_udNativeInstance:Intersects(fWorldX, fWorldY, bExactHitTest)
end

function DisplayObject:RemoveFromParent()

	self.m_udNativeInstance:RemoveFromParent()
end

function DisplayObject:GetAdditiveBlend()

	return self.m_udNativeInstance:GetAdditiveBlend()
end

function DisplayObject:SetAdditiveBlend(bAdditiveBlend)

	self.m_udNativeInstance:SetAdditiveBlend(bAdditiveBlend)
end

function DisplayObject:SetAlpha(fAlpha)

	self.m_udNativeInstance:SetAlpha(fAlpha)
end

function DisplayObject:SetClipDepth(iDepth)

	self.m_udNativeInstance:SetClipDepth(iDepth)
end

function DisplayObject:SetColorTransform(fMulR, fMulG, fMulB, iAddR, iAddG, iAddB)

	self.m_udNativeInstance:SetColorTransform(fMulR, fMulG, fMulB, iAddR, iAddG, iAddB)
end

function DisplayObject:SetDebugName(
	sName)

	self.m_udNativeInstance:SetDebugName(sName)
end

--- <summary>
--- Adjust the scale of this DisplayObject so that its
--- height (bottom - top bounds) is equalt to the specified
--- value.
--- </summary>
--- <param name="value">Desired height.</param>
--- <remarks>
--- If the local (bottom - top) of the DisplayObject is 0, this method
--- is a no-op.
--- </remarks>
function DisplayObject:SetHeight(value)

	local _, t, _, b = self:GetLocalBounds()
	local lh = b - t

	if lh < 1E-06 then

		return
	end

	local sx, sy = self:GetScale()
	sy = value / lh
	self:SetScale(sx, sy)
end

function DisplayObject:SetIgnoreDepthProjection(b)

	self.m_udNativeInstance:SetIgnoreDepthProjection(b)
end

function DisplayObject:SetName(sName)

	self.m_udNativeInstance:SetName(sName)
end

function DisplayObject:SetPosition(fX, fY)

	self.m_udNativeInstance:SetPosition(fX, fY)
end

function DisplayObject:SetPositionX(value)

	self.m_udNativeInstance:SetPositionX(value)
end

function DisplayObject:SetPositionY(value)

	self.m_udNativeInstance:SetPositionY(value)
end

function DisplayObject:SetRotation(fAngleInDegrees)

	self.m_udNativeInstance:SetRotation(fAngleInDegrees)
end

function DisplayObject:SetScale(fX, fY)

	self.m_udNativeInstance:SetScale(fX, fY)
end

function DisplayObject:SetScaleX(value)

	self.m_udNativeInstance:SetScaleX(value)
end

function DisplayObject:SetScaleY(value)

	self.m_udNativeInstance:SetScaleY(value)
end

function DisplayObject:SetScissorClip(bEnable)

	self.m_udNativeInstance:SetScissorClip(bEnable)
end

function DisplayObject:SetVisible(bVisible)

	self.m_udNativeInstance:SetVisible(bVisible)
end

--- <summary>
--- Adjust the scale of this DisplayObject so that its
--- width (right - left bounds) is equalt to the specified
--- value.
--- </summary>
--- <param name="value">Desired width.</param>
--- <remarks>
--- If the local (right - left) of the DisplayObject is 0, this method
--- is a no-op.
--- </remarks>
function DisplayObject:SetWidth(value)

	local l, _, r, _ = self:GetLocalBounds()
	local lw = r - l

	if lw < 1E-06 then

		return
	end

	local sx, sy = self:GetScale()
	sx = value / lw
	self:SetScale(sx, sy)
end

function DisplayObject:SetWorldPosition(fX, fY)

	self.m_udNativeInstance:SetWorldPosition(fX, fY)
end

function DisplayObject:WorldToLocal(fX, fY)

	return self.m_udNativeInstance:WorldToLocal(fX, fY)
end

--- <summary>
--- Convenience, adds a tween to transition this DisplayObject's
--- Alpha from 0 to 1.
--- </summary>
function DisplayObject:FadeIn()

	self:SetAlpha(0)
	self:AddTween(0--[[TweenTarget.m_iAlpha]], 0, 1, 0.2--[[DisplayObject.kfFadeInTimeSecs]])
end
return DisplayObject
