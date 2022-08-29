-- RootMovieClip is a convenience class, meant to be subclassed
-- by classes which match the root MovieClip of a Falcon/Flash
-- scene graph.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local RootMovieClip = class('RootMovieClip', 'MovieClip')

-- Hooks called form native on reload events.
















































local function RunTask(self, task, fDeltaTimeInSeconds)

	-- Early out if no task.
	if nil == task then

		return
	end

	-- Execute and report.
	local bStatus, msg = coroutine.resume(task, fDeltaTimeInSeconds)
	if not bStatus then

		error(msg)
	end
end

function RootMovieClip:Constructor() self.m_aRootEveryTickTasks = {}; self.m_aRootTasks = {}; self.m_tPendingRequests = {}; self.m_bRootIsInAnimOut = false;




end

function RootMovieClip:AddPendingRequest(req)

	if nil ~= req then

		self.m_tPendingRequests[req] = true
	end
end

function RootMovieClip:RemovePendingRequest(req)

	if nil ~= req then

		self.m_tPendingRequests[req] = nil
	end
end

function RootMovieClip:Destructor()

	for req, _ in pairs(self.m_tPendingRequests) do

		req:Cancel()
	end

	self.m_tPendingRequests = {}




end

-- Get if this root is animating out, typically to perform
-- a state transition to a new UI state.
function RootMovieClip:IsInAnimOut()

	return self.m_bRootIsInAnimOut
end

-- Utility for handling the common pattern of animating out a root MovieClip.
-- Disables input on the entire Falcon scene graph until the animation completes.
function RootMovieClip:RootAnimOut(sEventName, oMovieClip, completionCallback, sOptionalAnimOutFrameLabel)

	-- Optional argument.
	sOptionalAnimOutFrameLabel = sOptionalAnimOutFrameLabel or 'AnimOut'

	local udNative = self:GetNative()

	-- Disable input while animating out.
	local bAcceptingInput = udNative:AcceptingInput()
	udNative:SetAcceptInput(false)

	-- Setup an event listener for the AnimOut event.
	local callbackContainer = {}
	callbackContainer.m_Callback = function()

		-- Invoke the specific completion callback.
		if nil ~= completionCallback then

			completionCallback()
		end

		-- If input was enabled prior to AnimOut, re-enable it now.
		if bAcceptingInput then

			udNative:SetAcceptInput(true)
		end

		-- Remove the event listener.
		oMovieClip:RemoveEventListener(sEventName, callbackContainer.m_Callback)

		-- No longer in AnimOut.
		self.m_bRootIsInAnimOut = false
	end

	-- Add the event listener.
	oMovieClip:AddEventListener(sEventName, callbackContainer.m_Callback)

	-- Play the AnimOut frame.
	self.m_bRootIsInAnimOut = true
	oMovieClip:GotoAndPlayByLabel(sOptionalAnimOutFrameLabel)
end

--- <summary>
--- Hook to override for root movie animation out events. Standard
--- setup - can be customized, see RootStandardAnimOut().
--- </summary>
function RootMovieClip:AnimOutComplete()

	-- Nop by default.
end

--- <summary>
--- Standard configuration for RootAnimOut. Usually,
--- you can call this if your timeline and instance names
--- match our common naming convention.
--- </summary>
function RootMovieClip:RootStandardAnimOut()

	self:RootAnimOut(
		'AnimOutComplete',
		self.m_mcTop,
		function()

			self:AnimOutComplete()
		end)
end

--- <summary>
--- Simple cooperative multi-tasking task functionality.
--- </summary>
--- <param name="func">The task to run.</param>
--- <param name="everyTickeveryTick">
--- If true, task is run once every frame. By default,
--- tasks are run in sequence and given a single time
--- slice - meaning, if task 0 and task 1 are scheduled,
--- only task 0 will run until it completes.
--- </param>
--- <remarks>
--- Add a task, and the native UI code will invoke YieldToTasks()
--- once per frame. This allows the implementation of func to
--- use coroutine.yield() to timeslice the body of the task.
--- </remarks>
function RootMovieClip:AddTask(func, everyTick)

	local task = coroutine.create(func)
	if everyTick then

		table.insert(self.m_aRootEveryTickTasks, task)

	else

		table.insert(self.m_aRootTasks, task)
	end

	return task
end

function RootMovieClip:RemoveTask(task)

	for index, taskObj in ipairs(self.m_aRootEveryTickTasks) do

		if taskObj == task then

			table.remove(self.m_aRootEveryTickTasks, index)
			return
		end
	end

	for index, taskObj in ipairs(self.m_aRootTasks) do

		if taskObj == task then

			table.remove(self.m_aRootTasks, index)
			return
		end
	end
end

--- <summary>
--- Return true if this root has any pending tasks to process.
--- </summary>


function RootMovieClip:get_HasRootTasks()

	return nil ~= self.m_aRootTasks[1]
end


--- <summary>
--- Called once per-frame by native UI code. Used to advance
--- the queue of tasks.
--- </summary>
function RootMovieClip:YieldToTasks(fDeltaTimeInSeconds)

	-- Once a yield tasks.
	do
		local tasks = self.m_aRootTasks
		local task = tasks[1]
		while nil ~= task and coroutine.status(task) == 'dead' do

			table.remove(tasks, 1)
			task = tasks[1]
		end

		RunTask(self, task, fDeltaTimeInSeconds)
	end

	-- Every yield tasks.
	do
		local tasks = self.m_aRootEveryTickTasks
		local i = 1
		local task = tasks[i]
		while nil ~= task do

			-- Remove the task - don't advance, tasks[i] is a new task.
			if coroutine.status(task) == 'dead' then

				table.remove(tasks, i)


			-- Otherwise, execute and advance.
			else

				RunTask(self, task, fDeltaTimeInSeconds)
				i = i + 1
			end

			-- Acquire the next task.
			task = tasks[i]
		end
	end
end

function RootMovieClip:HANDLER_OnViewportChanged(fAspectRatio)

	local scrolledMovie = self:GetChildByPath(MovieScroller, 'UIMovieScroller')
	if scrolledMovie then scrolledMovie:OnViewportChanged(fAspectRatio) end
end



--- <summary>
--- Utility invoked by native to invoke a method on a nested member field.
--- </summary>
--- <param name="sMember">Name of the field to target the invoke.</param>
--- <param name="sMethod">Name of the method to invoke.</param>
--- <param name="varargs">Arguments to the method.</param>
function RootMovieClip:CallMemberMethod(sMember, sMethod, ...)

	local member = self[sMember]
	member[sMethod](member, ...)
end

--- <summary>
--- Base handling for enter state. Configures movie scrollers if defined.
--- </summary>
--- <param name="sPreviousState">Name of the state we're leaving - may be empty.</param>
--- <param name="sNextState">Name of the state we're entering - may be empty.</param>
--- <param name="bWasInPreviousState">True if we were in the previous state, false otherwise.</param>
local SetupScrolledMovie;function RootMovieClip:OnEnterState(_, _, _)

	-- Cleanup any existing scroller first.
	local scrolledMovie = self:GetChildByPath(MovieScroller, 'UIMovieScroller')
	if scrolledMovie then scrolledMovie:Deinit() end
	if scrolledMovie then scrolledMovie:RemoveFromParent() end

	-- Check if the state we're entering has screen scrollers defined.
	local native = self:GetNative()
	local aScrolledMovies = native:GetStateConfigValue('ScrolledMovies')

	-- We have a proper set of screen scrolls, check if we're
	-- the primary (we're the middle element).
	if nil ~= aScrolledMovies and #aScrolledMovies == 3 then

		-- We're the primary if our class name matches
		-- the primary name.
		local sName = aScrolledMovies[2]
		local sClassName = getmetatable(self).m_sClassName
		if sName == sClassName then

			-- Grab triggers now as well.
			local aScrolledTriggers = native:GetStateConfigValue('ScrolledTriggers')

			-- We're the primary, setup a scroller.
			SetupScrolledMovie(self, aScrolledMovies, aScrolledTriggers)
		end
	end
end

function RootMovieClip:OnExitState(_, _, _)

end

function RootMovieClip:OnLoad()

end

--- <summary>
--- Add a MovieScroller MovieClip as the last child of this root.
--- </summary>
--- <param name="aScrolledMovies">List of scrollers defined.</param>
--- <param name="aScrolledTriggers">List of scroller triggers defined.</param>
function SetupScrolledMovie(self,
	aScrolledMovies,
	aScrolledTriggers)

	-- Create and initialize.
	local screenScroller = self:NewMovieClip('MovieScroller')
	self:AddChild(screenScroller, 'UIMovieScroller', -1)
	screenScroller:Init(self, aScrolledMovies, aScrolledTriggers)
end

--- <summary>
--- The height of the Flash stage, in pixels.
--- </summary>
function RootMovieClip:GetMovieHeight()

	return self:GetNative():GetMovieHeight()
end

--- <summary>
--- The width of the Flash stage, in pixels.
--- </summary>
function RootMovieClip:GetMovieWidth()

	return self:GetNative():GetMovieWidth()
end

--- <summary>
--- Look through all sibling movies and return the root
--- of the first movie found with the given type name.
--- </summary>
--- <param name="sSiblingName">Name of the movie to get a root from.</param>
--- <returns></returns>
function RootMovieClip:GetSiblingRootMovieClip(sSiblingName)

	return self.m_udNativeInterface:GetSiblingRootMovieClip(sSiblingName)
end

function RootMovieClip:GetUIStateConfigString(key)

	return self:GetNative():GetStateConfigValue(key)
end

function RootMovieClip:AttachVFXToMovie(parent, vfxName)

	local flags = -2147483648--[[FxFlags.m_iFollowBone]]
	local fx = parent:AddChildFx(vfxName, 0, 0, flags)
	return fx
end


local NativeScriptUIMovieExtensions = class_static 'NativeScriptUIMovieExtensions'



function NativeScriptUIMovieExtensions.DynamicCall(movie, methodName, ...)

	movie[methodName](movie, ...)
end
return NativeScriptUIMovieExtensions
