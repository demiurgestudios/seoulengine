/**
 * \file BuildConfig.h
 * \brief Header file that defines the major config types (i.e. DEBUG, DEVELOPER, SHIP, etc.)
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef BUILD_CONFIG_H
#define BUILD_CONFIG_H

#ifdef _DEBUG
#define SEOUL_DEBUG 1
#define SEOUL_DEVELOPER 0
#define SEOUL_SHIP 0

#define SEOUL_BUILD_CONFIG_STR "Debug"
#elif defined(_SHIP)
#define SEOUL_DEBUG 0
#define SEOUL_DEVELOPER 0
#define SEOUL_SHIP 1

#if !defined(SEOUL_PROFILING_BUILD)
#define SEOUL_BUILD_CONFIG_STR "Ship"
#else
#define SEOUL_BUILD_CONFIG_STR "Profiling"
#endif
#else
#define SEOUL_DEBUG 0
#define SEOUL_DEVELOPER 1
#define SEOUL_SHIP 0

#define SEOUL_BUILD_CONFIG_STR "Developer"
#endif

#endif // include guard
