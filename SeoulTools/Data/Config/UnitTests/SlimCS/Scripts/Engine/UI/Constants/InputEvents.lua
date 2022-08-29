-- Keep this in sync with UIInputEvent in UIData.h
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.

-- Flags passed for input event
local InputEvents = class_static 'InputEvents'

InputEvents.m_iUnknown = 0
InputEvents.m_iAction = 1
InputEvents.m_iBackButton = 2
InputEvents.m_iDone = 3
return InputEvents
