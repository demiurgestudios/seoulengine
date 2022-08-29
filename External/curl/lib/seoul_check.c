/* Copyright (c) 2017-2022 Demiurge Studios Inc. All rights reserved. */

#include "curl_setup.h"

#define SEOUL_STATIC_ASSERT(v) static_assert((v), #v)

SEOUL_STATIC_ASSERT(sizeof(int) == SIZEOF_INT);
SEOUL_STATIC_ASSERT(sizeof(long) == SIZEOF_LONG);
SEOUL_STATIC_ASSERT(sizeof(off_t) == SIZEOF_OFF_T);
SEOUL_STATIC_ASSERT(sizeof(short) == SIZEOF_SHORT);
SEOUL_STATIC_ASSERT(sizeof(size_t) == SIZEOF_SIZE_T);
SEOUL_STATIC_ASSERT(sizeof(curl_off_t) == SIZEOF_CURL_OFF_T);
SEOUL_STATIC_ASSERT(sizeof(time_t) == SIZEOF_TIME_T);
#ifdef SIZEOF_VOIDP
SEOUL_STATIC_ASSERT(sizeof(void*) == SIZEOF_VOIDP);
#endif
