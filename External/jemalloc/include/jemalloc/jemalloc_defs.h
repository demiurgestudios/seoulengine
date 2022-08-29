/* include/jemalloc/jemalloc_defs.h.  Generated from jemalloc_defs.h.in by configure.  */
/* Defined if __attribute__((...)) syntax is supported. */
#ifndef _MSC_VER /* EXT_DEMIURGE: */
#define JEMALLOC_HAVE_ATTR 
#endif /* EXT_DEMIURGE: */

/* Defined if alloc_size attribute is supported. */
#ifndef _MSC_VER /* EXT_DEMIURGE: */
#define JEMALLOC_HAVE_ATTR_ALLOC_SIZE 
#endif /* EXT_DEMIURGE: */

/* Defined if format(gnu_printf, ...) attribute is supported. */
#ifndef _MSC_VER /* EXT_DEMIURGE: */
#define JEMALLOC_HAVE_ATTR_FORMAT_GNU_PRINTF 
#endif /* EXT_DEMIURGE: */

/* Defined if format(printf, ...) attribute is supported. */
#ifndef _MSC_VER /* EXT_DEMIURGE: */
#define JEMALLOC_HAVE_ATTR_FORMAT_PRINTF 
#endif /* EXT_DEMIURGE: */

/*
 * Define overrides for non-standard allocator-related functions if they are
 * present on the system.
 */
#ifndef _MSC_VER /* EXT_DEMIURGE: */
#define JEMALLOC_OVERRIDE_MEMALIGN 
#ifndef __LP64__ /* EXT_DEMIURGE: */
#define JEMALLOC_OVERRIDE_VALLOC 
#endif /* EXT_DEMIURGE: */
#endif /* EXT_DEMIURGE: */

/*
 * At least Linux omits the "const" in:
 *
 *   size_t malloc_usable_size(const void *ptr);
 *
 * Match the operating system's prototype.
 */
#if !defined(_MSC_VER) && !defined(__ANDROID__) /* EXT_DEMIURGE: */
#define JEMALLOC_USABLE_SIZE_CONST 
#else /* EXT_DEMIURGE: */
#define JEMALLOC_USABLE_SIZE_CONST const
#endif /* EXT_DEMIURGE: */

/*
 * If defined, specify throw() for the public function prototypes when compiling
 * with C++.  The only justification for this is to match the prototypes that
 * glibc defines.
 */
#if defined(__GLIBC__) /* EXT_DEMIURGE: */
#define	JEMALLOC_USE_CXX_THROW
#endif /* EXT_DEMIURGE: */

#ifdef _MSC_VER
#  ifdef _WIN64
#    define LG_SIZEOF_PTR_WIN 3
#  else
#    define LG_SIZEOF_PTR_WIN 2
#  endif
#endif

/* sizeof(void *) == 2^LG_SIZEOF_PTR. */
#ifndef _MSC_VER /* EXT_DEMIURGE: */
#ifdef __LP64__ /* EXT_DEMIURGE: */
#define	LG_SIZEOF_PTR 3
#else /* EXT_DEMIURGE: */
#define	LG_SIZEOF_PTR 2
#endif /* EXT_DEMIURGE: */
#else /* EXT_DEMIURGE: */
#define	LG_SIZEOF_PTR LG_SIZEOF_PTR_WIN
#endif /* EXT_DEMIURGE: */
