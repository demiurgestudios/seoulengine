-- Equivalent to Seoul::HTTP::Status.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.

-- Valid HTTP result codes - equivalent to HTTP::Status
-- enum in HTTPCommon.h
local HTTPStatusCodes = class_static 'HTTPStatusCodes'

HTTPStatusCodes.m_iStatusOK = 200
HTTPStatusCodes.m_iStatusPartialContent = 206
HTTPStatusCodes.m_iStatusBadRequest = 400
HTTPStatusCodes.m_iStatusUnauthorized = 401
HTTPStatusCodes.m_iStatusForbidden = 403
HTTPStatusCodes.m_iStatusNotFound = 404
HTTPStatusCodes.m_iStatusInternalServerError = 500
HTTPStatusCodes.m_iStatusNotImplemented = 501
HTTPStatusCodes.m_iStatusBadGateway = 502
HTTPStatusCodes.m_iStatusServiceUnavailable = 503
return HTTPStatusCodes
