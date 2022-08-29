-- Main entry point of the App UI's Lua code. Loaded into a VM
-- specific to UI.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.












--- <summary>
--- Main entry point of the application.
--- </summary>
--- <remarks>
--- This is the script invoked by the game to populate the game's
--- script VM. The TopLevelChunk attribute indicates that
--- this class is handled specially by SlimCS. The body
--- of Chunk will be emitted verbatim as top-level Lua code.
--- </remarks>


-- Absolutely first - if it exists, instantiate the native
-- instance that manages any native "script lifespan tied"
-- code. This is code that is implemented globally in native
-- but is a pre-requisite of any and all script implementation.
if _G.SeoulHasNativeUserData 'AppSpecificNativeScriptDependencies' then

	local ud = _G.SeoulNativeNewNativeUserData 'AppSpecificNativeScriptDependencies'
	ud.Construct(ud)
	_G['AppSpecificNativeScriptDependenciesGlobalObject__5dd3aa5836981818 '] = ud

	-- Assign a closure to SeoulIsFullyInitialized so that the patcher can query
	-- whether the script VM has fully initialized, based on game logic.
	local isFullyInitialized = ud.IsFullyInitialized
	if nil ~= isFullyInitialized then

		_G.SeoulIsFullyInitialized = function()

			return isFullyInitialized(ud)
		end
	end
end

-- Kick off SlimCS initialization (this registers and static constructs
-- all types defined in SlimCS).
require 'SlimCSMain'
require 'SlimCSFiles'


