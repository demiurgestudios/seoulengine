-- Flags that can be applied to hit testing to control what is hit.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.

-- Hit test options to SetHitTestSelfMode() and
-- SetHitTestChildrenMode().
local HitTestMode = class_static 'HitTestMode'

-- These correlate with constants in FalconConstants.h
HitTestMode.m_iOff = 0 -- (no bits)
HitTestMode.m_iClickInputOnly = 1--[[bit.lshift(1, 0)]]
HitTestMode.m_iHorizontalDragInputOnly = 2--[[bit.lshift(1, 1)]]
HitTestMode.m_iVerticalDragInputOnly = 4--[[bit.lshift(1, 2)]]
HitTestMode.m_iDragInputOnly = 6--[[bit.bor(bit.lshift(1, 1), bit.lshift(1, 2))]]
HitTestMode.m_iClickAndDragInput = 7--[[bit.bor(bit.bor(bit.lshift(1, 0), bit.lshift(1, 1)), bit.lshift(1, 2))]]
HitTestMode.m_iMaskAll = 255 -- (0xFF)
return HitTestMode
