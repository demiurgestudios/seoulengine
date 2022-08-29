// Equivalent to Seoul::HTTP::Result.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

public static class HTTPResults
{
	// Valid HTTP result codes - equivalent to HTTP::Result
	// enum in HTTPCommon.h
	public const double m_iResultSuccess = 0;
	public const double m_iResultFailure = 1;
	public const double m_iResultCanceled = 2;
	public const double m_iResultConnectFailure = 3;
}
