/**
 * \file ConfigZipBuilder.h
 * \brief Tool for collecting all config .json files under the config root
 * and writing them in .zip format to a SyncFile.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CONFIG_ZIP_BUILDER_H
#define CONFIG_ZIP_BUILDER_H

#include "SeoulTypes.h"
#include "SeoulFile.h"
#include "ScopedPtr.h"

namespace Seoul::ConfigZipBuilder
{

// Fills a.zip file with all config.json files in the Config directory.
Bool WriteAllJson(SyncFile& zipFile);

} // namespace Seoul::ConfigZipBuilder

#endif  // include guard
