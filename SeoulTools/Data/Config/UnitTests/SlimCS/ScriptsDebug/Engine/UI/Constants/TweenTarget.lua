-- Controls the target parameter of a tween.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.

--- <summary>
--- When calling AddTween(), this is the target property
--- to manipulate with the tween, or m_iTimer, to use
--- a tween simply for timed waits/delays.
--- </summary>
TweenTarget = {

--- <summary>Color alpha on [0, 1]</summary>
m_iAlpha = 0,

--- <summary>Node depth 3D, relative to its parent.</summary>
m_iDepth3D = 1,

--- <summary>Node position component X, relative to its parent.</summary>
m_iPositionX = 2,

--- <summary>Node position component Y, relative to its parent.</summary>
m_iPositionY = 3,

--- <summary>Node rotation in degrees.</summary>
m_iRotation = 4,

--- <summary>Node scale component X, relative to its parent.</summary>
m_iScaleX = 5,

--- <summary>Node scale component Y, relative to its parent.</summary>
m_iScaleY = 6,

--- <summary>
--- Special tween, affects no property. Used for
--- timing/delayed callbacks.
--- </summary>
m_iTimer = 7
}
