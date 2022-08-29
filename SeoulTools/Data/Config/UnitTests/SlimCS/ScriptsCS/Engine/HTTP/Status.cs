// Equivalent to Seoul::HTTP::Status.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

// Valid HTTP result codes - equivalent to HTTP::Status
// enum in HTTPCommon.h
public static class HTTPStatusCodes
{
	public const double m_iStatusOK = 200;
	public const double m_iStatusPartialContent = 206;
	public const double m_iStatusBadRequest = 400;
	public const double m_iStatusUnauthorized = 401;
	public const double m_iStatusForbidden = 403;
	public const double m_iStatusNotFound = 404;
	public const double m_iStatusInternalServerError = 500;
	public const double m_iStatusNotImplemented = 501;
	public const double m_iStatusBadGateway = 502;
	public const double m_iStatusServiceUnavailable = 503;
}
