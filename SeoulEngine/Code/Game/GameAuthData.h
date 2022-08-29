/**
 * \file GameAuthData.h
 * \brief Data requested from the server that is required for
 * server authentication and client configuration.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_AUTH_DATA_H
#define GAME_AUTH_DATA_H

#include "FilePath.h"
#include "GameClientVersionData.h"
#include "HashTable.h"
#include "HTTPStats.h"
#include "SeoulString.h"
#include "Vector.h"

namespace Seoul::Game
{

/**
 * Defines data that can be refreshed midsession (vs. auth
 * data that is sticky and never changes after the initial
 * request).
 */
struct AuthDataRefresh SEOUL_SEALED
{
	typedef HashTable<String, Int32, MemoryBudgets::Analytics> ABTests;
	typedef Vector<FilePath, MemoryBudgets::Network> RemapConfigs;

	AuthDataRefresh()
		: m_sVariationString()
		, m_sConfigUpdateUrl()
		, m_sContentUpdateUrl()
		, m_tABTests()
		, m_vRemapConfigs()
		, m_VersionRecommended()
		, m_VersionRequired()
		, m_bAnalyticsSandboxed(false)
	{
	}

	Bool operator==(const AuthDataRefresh& b) const
	{
		return (
			m_sVariationString == b.m_sVariationString &&
			m_sConfigUpdateUrl == b.m_sConfigUpdateUrl &&
			m_sContentUpdateUrl == b.m_sContentUpdateUrl &&
			m_tABTests == b.m_tABTests &&
			m_vRemapConfigs == b.m_vRemapConfigs &&
			m_VersionRecommended == b.m_VersionRecommended &&
			m_VersionRequired == b.m_VersionRequired &&
			m_bAnalyticsSandboxed == b.m_bAnalyticsSandboxed);
	}

	Bool operator!=(const AuthDataRefresh& b) const
	{
		return !(*this == b);
	}

	// Not reflected, filled in by the client.
	HTTP::Stats m_RequestStats;
	String m_sVariationString;
	String m_sConfigUpdateUrl;
	String m_sContentUpdateUrl;
	ABTests m_tABTests;
	RemapConfigs m_vRemapConfigs;
	ClientVersionData m_VersionRecommended;
	ClientVersionData m_VersionRequired;
	Bool m_bAnalyticsSandboxed;
}; // struct Game::AuthDataRefresh

/**
 * Defines all data that we must know from the server before
 * continuing with startup (and for which a change requires
 * a soft reboot and patch).
 */
struct AuthData SEOUL_SEALED
{
	AuthData()
		: m_sAnalyticsGuid()
		, m_sAuthToken()
		, m_RefreshData()
	{
	}

	Bool operator==(const AuthData& b) const
	{
		return (
			m_sAnalyticsGuid == b.m_sAnalyticsGuid &&
			m_sAuthToken == b.m_sAuthToken &&
			m_RefreshData == b.m_RefreshData);
	}

	Bool operator!=(const AuthData& b) const
	{
		return !(*this == b);
	}

	// Not reflected, filled in by the client.
	HTTP::Stats m_RequestStats;

	String m_sAnalyticsGuid;
	String m_sAuthToken;
	AuthDataRefresh m_RefreshData;
}; // struct Game::AuthData

} // namespace Seoul::Game

#endif // include guard
