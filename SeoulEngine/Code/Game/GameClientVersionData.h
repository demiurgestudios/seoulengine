/**
 * \file GameClientVersionData.h
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

#pragma once
#ifndef GAME_CLIENT_VERSION_DATA_H
#define GAME_CLIENT_VERSION_DATA_H

#include "Prereqs.h"                 // for SEOUL_SEALED, etc.

namespace Seoul
{

/**
 * Required and recommended version information from the server.
 */
namespace Game
{

struct ClientVersionData SEOUL_SEALED
{
	ClientVersionData();

	Int m_iVersionMajor;
	Int m_iChangelist;

	Bool CheckCurrentBuild() const;

	Bool operator==(const ClientVersionData& b) const;

	Bool operator!=(const ClientVersionData& b) const;
}; // struct Game::ClientVersionData

} // namespace Game

} // namespace Seoul

#endif // include guard
