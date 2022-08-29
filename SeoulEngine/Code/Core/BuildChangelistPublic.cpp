/**
 * \file BuildChangelistPublic.cpp
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

#include "BuildChangelist.h"
#include "BuildChangelistPublic.h"

namespace Seoul
{

int g_iBuildChangelist = BUILD_CHANGELIST;
const char* g_sBuildChangelistStr = BUILD_CHANGELIST_STR;
extern const int g_kBuildChangelistFixed = BUILD_CHANGELIST;
extern const char* const g_ksBuildChangelistStrFixed = BUILD_CHANGELIST_STR;

} // namespace Seoul
