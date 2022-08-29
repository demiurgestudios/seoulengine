/**
 * \file AndroidGlobals.cpp
 * \brief Miscellaneous values passed from Java to Native on startup.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AndroidGlobals.h"

namespace Seoul
{

/** Buffer used to cache the current internal directory folder. */
FixedArray<Byte, 1024u> g_InternalStorageDirectoryString((Byte)0);

/** Buffer used to cache any commandline arguments passed to the app via an Intent. */
FixedArray<Byte, 1024u> g_CommandlineArguments((Byte)0);

/** Absolute path to the application's APK file on storage. */
FixedArray<Byte, 1024u> g_SourceDir((Byte)0);

/** Used to cache the current platform flavor, which on Android is used to distinguish between builds for Amazon, Google Play, etc. */
PlatformFlavor g_ePlatformFlavor(PlatformFlavor::kUnknown);

/** Slop (in pixels) to allow before a tap is classified as a drag/move action. */
Atomic32Value<Int> g_iTouchSlop(0);

} // namespace Seoul
