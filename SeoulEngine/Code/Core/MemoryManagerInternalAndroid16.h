/**
 * \file MemoryManagerInternalAndroid16.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef MEMORY_MANAGER_INTERNAL_ANDROID_16_H
#define MEMORY_MANAGER_INTERNAL_ANDROID_16_H

#include "Prereqs.h"

#if __ANDROID_API__ < 17
// Implements dlmalloc_usable_size for Android API-16.
extern "C" { size_t DlmallocAndroid16UsableSize(void const* ptr); }
#endif // /(__ANDROID_API__ < 17)

#endif // include guard
