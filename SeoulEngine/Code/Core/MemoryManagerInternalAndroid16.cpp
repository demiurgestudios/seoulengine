/**
 * \file MemoryManagerInternalAndroid16.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "MemoryManagerInternalAndroid16.h"

#if __ANDROID_API__ < 17
extern "C"
{

/*
 * Minimum bits of Doug Lea's malloc:
 * "
 * This is a version (aka dlmalloc) of malloc/free/realloc written by
 * Doug Lea and released to the public domain, as explained at
 * http://creativecommons.org/publicdomain/zero/1.0/ Send questions,
 * comments, complaints, performance data, etc to dl@cs.oswego.edu
 * Version 2.8.6 Wed Aug 29 06:57:58 2012  Doug Lea
 * Note: There may be an updated version of this malloc obtainable at
 * ftp://gee.cs.oswego.edu/pub/misc/malloc.c
 * Check before installing!
 */
struct malloc_chunk {
  size_t               prev_foot;  /* Size of previous chunk (if free).  */
  size_t               head;       /* Size and inuse bits. */
  struct malloc_chunk* fd;         /* double links -- used only if free. */
  struct malloc_chunk* bk;
};
typedef struct malloc_chunk  mchunk;
typedef struct malloc_chunk* mchunkptr;

#define SIZE_T_SIZE         (sizeof(size_t))
#define SIZE_T_ONE          ((size_t)1)
#define SIZE_T_TWO          ((size_t)2)
#define SIZE_T_FOUR         ((size_t)4)
#define TWO_SIZE_T_SIZES    (SIZE_T_SIZE<<1)
#define PINUSE_BIT          (SIZE_T_ONE)
#define CINUSE_BIT          (SIZE_T_TWO)
#define FLAG4_BIT           (SIZE_T_FOUR)
#define INUSE_BITS          (PINUSE_BIT|CINUSE_BIT)
#define FLAG_BITS           (PINUSE_BIT|CINUSE_BIT|FLAG4_BIT)

#define CHUNK_OVERHEAD      (SIZE_T_SIZE)
#define MMAP_CHUNK_OVERHEAD (TWO_SIZE_T_SIZES)

#define is_inuse(p)         (((p)->head & INUSE_BITS) != PINUSE_BIT)
#define is_mmapped(p)       (((p)->head & INUSE_BITS) == 0)
#define chunksize(p)        ((p)->head & ~(FLAG_BITS))
#define mem2chunk(mem)      ((mchunkptr)((char*)(mem) - TWO_SIZE_T_SIZES))
#define overhead_for(p)     (is_mmapped(p) ? MMAP_CHUNK_OVERHEAD : CHUNK_OVERHEAD)

/**
 * Implements dlmalloc_usable_size manually because the function is
 * not exported from API-16 of Android.
 *
 * WARNING: We're making the assumption that all libc.so files on all Android devices
 * at API-16 are unmodified from stock and wrap an implementation
 * of dlmalloc (see also: https://android.googlesource.com/platform/bionic/+/jb-mr2-release/libc/upstream-dlmalloc/malloc.c)
 *
 * This is relatively safe but no guaranteed. Fortunately, we *only* need this
 * until we can drop API-16 as our min and in fact, all our current projects
 * do not actually call API that will hit dlmalloc_usable_size in Shipping
 * builds (thought this is not enforced), so we've determined this risk
 * to be worth the alternative (which is to prematurely drop API-16 support).
 *
 * - see also: https://android.googlesource.com/platform/bionic/+/master/libc/bionic/ndk_cruft.cpp#343
 * - see also: https://android.googlesource.com/platform/bionic/+/jb-mr2-release/libc/upstream-dlmalloc/malloc.c#1246
 */
size_t DlmallocAndroid16UsableSize(void const* mem)
{
	if (nullptr != mem)
	{
		mchunkptr p = mem2chunk(mem);
		if (is_inuse(p))
		{
			return chunksize(p) - overhead_for(p);
		}
	}
	return 0u;
}

} // extern "C"
#endif // /(__ANDROID_API__ < 17)
