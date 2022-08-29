-- Controls tween behavior.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.

-- When calling AddTweenCruve(), this is the curve type.
TweenType = {

-- Ease in-out with a cubic shape.
m_iInOutCubic = 0,

-- Ease in-out with a quadratic shape.
m_iInOutQuadratic = 1,

-- Ease in-out with a quartic shape.
m_iInOutQuartic = 2,

-- Simple linear interpolation.
m_iLine = 3,

-- Sine curve, high frequency.
m_iSinStartFast = 4,

-- Since curve, low frequency.
m_iSinStartSlow = 5
}
