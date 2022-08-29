-- Equivalent to Seoul::HTTP::Result.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.

local HTTPResults = class_static 'HTTPResults'

-- Valid HTTP result codes - equivalent to HTTP::Result
-- enum in HTTPCommon.h
HTTPResults.m_iResultSuccess = 0
HTTPResults.m_iResultFailure = 1
HTTPResults.m_iResultCanceled = 2
HTTPResults.m_iResultConnectFailure = 3
return HTTPResults
