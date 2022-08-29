/**
 * \file AndroidGlobals.h
 * \brief Miscellaneous values passed from Java to Native on startup.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANDROID_GLOBALS_H
#define ANDROID_GLOBALS_H

#include "Atomic32.h"
#include "FixedArray.h"
#include "PlatformFlavor.h"
#include "Prereqs.h"

namespace Seoul
{

/** Buffer used to cache the current internal directory folder. */
extern FixedArray<Byte, 1024u> g_InternalStorageDirectoryString;

/** Buffer used to cache any commandline arguments passed to the app via an Intent. */
extern FixedArray<Byte, 1024u> g_CommandlineArguments;

/** Absolute path to the application's APK file on storage. */
extern FixedArray<Byte, 1024u> g_SourceDir;

/** Used to cache the current platform flavor, which on Android is used to distinguish between builds for Amazon, Google Play, etc. */
extern PlatformFlavor g_ePlatformFlavor;

/** Slop (in pixels) to allow before a tap is classified as a drag/move action. */
extern Atomic32Value<Int> g_iTouchSlop;

} // namespace Seoul

#endif // include guard
