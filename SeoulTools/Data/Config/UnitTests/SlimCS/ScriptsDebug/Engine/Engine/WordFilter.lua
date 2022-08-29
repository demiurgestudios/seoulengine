-- Access to native WordFilter objects.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local WordFilter = class_static 'WordFilter'

function WordFilter.Load(configPath)

	local udWordFilter = CoreUtilities.NewNativeUserData(ScriptEngineWordFilter, configPath)
	if nil == udWordFilter then

		error 'Failed instantiating ScriptEngineWordFilter.'
	end

	return udWordFilter
end
return WordFilter
