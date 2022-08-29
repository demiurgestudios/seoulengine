-- Equivalent to Seoul::HTTP::Method.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.

local HTTPMethods = class_static 'HTTPMethods'

-- Valid HTTP methods - equivalent to ksMethod* constants
-- in HTTPCommon.h
HTTPMethods.m_sMethodConnect = 'CONNECT'
HTTPMethods.m_sMethodDelete = 'DELETE'
HTTPMethods.m_sMethodGet = 'GET'
HTTPMethods.m_sMethodHead = 'HEAD'
HTTPMethods.m_sMethodPost = 'POST'
HTTPMethods.m_sMethodPut = 'PUT'
HTTPMethods.m_sMethodOptions = 'OPTIONS'
HTTPMethods.m_sMethodTrace = 'TRACE'
return HTTPMethods
