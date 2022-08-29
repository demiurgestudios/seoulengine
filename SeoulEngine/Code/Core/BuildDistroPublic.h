/**
 * \file BuildDistroPublic.h
 * \brief Public header for the current build's distribution setting.
 *
 * In most cases, you want this header file, instead of BuildDistro.h. This file
 * does not change and is not affected by changes to the distro macro embedded by the builder.
 * This reduces the number of files that need to always change on an incremental build.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef BUILD_DISTRO_PUBLIC_H
#define BUILD_DISTRO_PUBLIC_H

namespace Seoul
{

extern const bool g_kbBuildForDistribution;

} // namespace Seoul

#endif // include guard
