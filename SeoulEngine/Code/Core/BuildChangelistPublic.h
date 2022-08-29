/**
 * \file BuildChangelistPublic.h
 * \brief Public header for the current build's changelist info.
 *
 * In most cases, you want this header file, instead of BuildChangelist.h. This file
 * does not change and is not affected by changes to the CL embedded by the builder.
 * This reduces the number of files that need to always change on an incremental build.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef BUILD_CHANGELIST_PUBLIC_H
#define BUILD_CHANGELIST_PUBLIC_H

namespace Seoul
{

// Build changelist as number and string. These are the variables
// to use unless you know that you're doing something special
// and need instead the variables below.
extern int g_iBuildChangelist;
extern const char* g_sBuildChangelistStr;

// Use in low-level scenarios where you know what you want is
// the changelist applied by the builder at compile time. For
// game builds, this will always be the same as g_iBuildChangelist.
// For tool builds, this can differ.
extern const int g_kBuildChangelistFixed;
extern const char* const g_ksBuildChangelistStrFixed;

} // namespace Seoul

#endif // include guard
