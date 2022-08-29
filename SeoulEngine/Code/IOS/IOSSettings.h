/**
 * \file IOSSettings.h
 * \brief Declares functions for interacting with the iOS Settings bundle, which
 * exposes user configurable per-app settings in the iOS Settings application.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef IOS_SETTINGS_H
#define IOS_SETTINGS_H

#include "SeoulString.h"

namespace Seoul
{

// Gets the extra command line arguments defined in the app's Settings
String IOSGetCommandLineArgumentsFromIOSSettings();

} // namespace Seoul

#endif  // Include guard
