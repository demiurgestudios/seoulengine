/**
 * \file SteamPrivateApi.h
 * \brief Wrapper around external steam redistributable headers.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef STEAM_PRIVATE_API_H
#define STEAM_PRIVATE_API_H

#include "Prereqs.h"

#if SEOUL_WITH_STEAM

#pragma warning(push)

// warning C4996: '{0}': This function or variable has been superseded by newer
// library or operating system functionality. Consider using {1} instead.
//See online help for details.
#pragma warning(disable:4996)  // CRT deprecation warning

#include <steam_api.h>

#pragma warning(pop)

#endif // /#if SEOUL_WITH_STEAM

#endif // include guard
