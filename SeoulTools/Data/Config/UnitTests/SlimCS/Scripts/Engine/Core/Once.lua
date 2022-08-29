-- Utility that executes a function passed to it once and once only.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local Once = class 'Once'


function Once:Constructor() self.m_bDone = false; end


function Once:Do(f)

	if self.m_bDone then

		return
	end
	self.m_bDone = true

	f()
end
return Once
