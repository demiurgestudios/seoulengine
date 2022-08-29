/**
 * \file PathAndroid.cpp
 * \brief Android specific implementations for Path functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Mutex.h"
#include "Path.h"
#include "SeoulString.h"

namespace Seoul::Path
{

static Mutex s_Mutex;
static String s_sCacheDir;

String AndroidGetCacheDir()
{
	Lock lock(s_Mutex);
	return s_sCacheDir;
}

void AndroidSetCacheDir(const String& s)
{
	Lock lock(s_Mutex);
	s_sCacheDir = s;
}

} // namespace Seoul::Path
