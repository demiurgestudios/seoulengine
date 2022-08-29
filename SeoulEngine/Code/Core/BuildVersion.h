/**
 * \file BuildVersion.h
 * \brief Header file that contains the major and minor product version numbers
 *
 * The product version number has four fields: &lt;Major Ver&gt; . &lt;Minor Ver&gt; . &lt;Changelist&gt; . 0
 * (The 4th is currently unused)
 * This file defines the first two fields. BuildChangelist.h defines the last two.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef BUILD_VERSION_H
#define BUILD_VERSION_H

#define BUILD_VERSION_MAJOR 1

// Due to the way the C preprocessor works, we need to use two layers of macros
// to convert the macro BUILD_VERSION_MAJOR into a string like "1" (instead of
// the string "BUILD_VERSION_MAJOR")
#define SEOUL_STRINGIZE_HELPER(x) #x
#define SEOUL_STRINGIZE(x) SEOUL_STRINGIZE_HELPER(x)
#define BUILD_VERSION_STR SEOUL_STRINGIZE(BUILD_VERSION_MAJOR)

#endif // include guard
