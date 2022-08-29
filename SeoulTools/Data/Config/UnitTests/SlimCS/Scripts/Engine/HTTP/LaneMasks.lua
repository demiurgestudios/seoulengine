-- Contains lane mask bits for HTTP requests
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.

-- Valid HTTP result codes - equivalent to HTTP::Status
-- enum in HTTPCommon.h
local HTTPLaneMasks = class_static 'HTTPLaneMasks'

-- All script HTTP requests get this lane mask by default
HTTPLaneMasks.m_iScriptDefault = 1--[[bit.lshift(1, 0)]]
return HTTPLaneMasks
