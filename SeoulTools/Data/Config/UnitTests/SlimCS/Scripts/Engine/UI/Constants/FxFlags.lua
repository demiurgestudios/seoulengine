-- Flags that control Fx rendering and visualization behavior.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.

-- Flags to pass when starting UI Fx playback via AddChildFx().
FxFlags = {

--- <summary>
--- Special value used to indicate that no
--- particular flags have been set.
--- </summary>
m_iNone = 0,

--- <summary>
--- When set, the initial position will be
--- based off of world position.
--- </summary>
m_iInitPositionInWorldspace = 1073741824,

--- <summary>
--- When set, the animation system will update the FX position
--- </summary>
m_iFollowBone = -2147483648
}
