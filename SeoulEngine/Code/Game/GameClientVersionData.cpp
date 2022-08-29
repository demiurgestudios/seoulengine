/**
 * \file GameClientVersionData.cpp
 * \brief Encapsulates game version data. Combined into
 * Game::ClientVersionRequestData to fully define version
 * requirements for the current build (recommended and
 * required versions).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BuildChangelistPublic.h"      // for g_iBuildChangelist
#include "BuildVersion.h"               // for BUILD_VERSION
#include "GameClientVersionData.h"      // for Game::ClientVersionData
#include "ReflectionDefine.h"           // for SEOUL_BEGIN_TYPE

namespace Seoul
{

SEOUL_BEGIN_TYPE(Game::ClientVersionData)
	SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("VersionMajor", m_iVersionMajor)
	SEOUL_PROPERTY_N("Changelist", m_iChangelist)
SEOUL_END_TYPE()

namespace Game
{

ClientVersionData::ClientVersionData()
	: m_iVersionMajor(-1)
	, m_iChangelist(-1)
{
}

Bool ClientVersionData::CheckCurrentBuild() const
{
	// Version is > current build, build is too old.
	if (m_iVersionMajor > BUILD_VERSION_MAJOR) { return false; }

	// Changelist is > current build, build is too old.
	if (m_iChangelist > g_iBuildChangelist) { return false; }

	// Build is good to go.
	return true;
}

Bool ClientVersionData::operator==(const ClientVersionData& b) const
{
	return (
		m_iVersionMajor == b.m_iVersionMajor &&
		m_iChangelist == b.m_iChangelist);
}

Bool ClientVersionData::operator!=(const ClientVersionData& b) const
{
	return !(*this == b);
}

} // namespace Game

} // namespace Seoul
