/* Hint to the compiler that a function never returns */
#define NGHTTP2_NORETURN 

/* Define to `int' if <sys/types.h> does not define. */
#ifdef __ANDROID__
#elif (defined(__linux__) || defined(__MINGW32__))
#else
#define ssize_t int
#endif

/* Define to 1 if you have the `std::map::emplace`. */
#define HAVE_STD_MAP_EMPLACE 1

/* Define to 1 if you have `libjansson` library. */
/* #undef HAVE_JANSSON */

/* Define to 1 if you have `libxml2` library. */
/* #undef HAVE_LIBXML2 */

/* Define to 1 if you have `spdylay` library. */
/* #undef HAVE_SPDYLAY */

/* Define to 1 if you have `mruby` library. */
/* #undef HAVE_MRUBY */

/* Define to 1 if you have `neverbleed` library. */
/* #undef HAVE_NEVERBLEED */

/* sizeof(int *) */
#if defined(_WIN64)
#  define SIZEOF_INT_P 8
#else
#  define SIZEOF_INT_P 4
#endif

/* VS2005 and later dafault size for time_t is 64-bit, unless
_USE_32BIT_TIME_T has been defined to get a 32-bit time_t. */
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#  ifndef _USE_32BIT_TIME_T
#    define SIZEOF_TIME_T 8
#  else
#    define SIZEOF_TIME_T 4
#  endif
#endif

/* Define to 1 if you have the `_Exit` function. */
#define HAVE__EXIT 1

/* Define to 1 if you have the `accept4` function. */
/* #undef HAVE_ACCEPT4 */

/* Define to 1 if you have the `mkostemp` function. */
/* #undef HAVE_MKOSTEMP */

/* Define to 1 if you have the `initgroups` function. */
#define HAVE_DECL_INITGROUPS 0

/* Define to 1 to enable debug output. */
/* #undef DEBUGBUILD */

/* Define to 1 if you want to disable threads. */
/* #undef NOTHREADS */

/* Define to 1 if you have the <arpa/inet.h> header file. */
/* #undef HAVE_ARPA_INET_H */

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the <netdb.h> header file. */
/* #undef HAVE_NETDB_H */

/* Define to 1 if you have the <netinet/in.h> header file. */
#ifdef __ANDROID__
#define HAVE_NETINET_IN_H 1
#elif (defined(__linux__) || defined(__MINGW32__))
#define HAVE_NETINET_IN_H 1
#else
#undef HAVE_NETINET_IN_H
#endif

/* Define to 1 if you have the <pwd.h> header file. */
/* #undef HAVE_PWD_H */

/* Define to 1 if you have the <sys/socket.h> header file. */
/* #undef HAVE_SYS_SOCKET_H */

/* Define to 1 if you have the <sys/time.h> header file. */
/* #undef HAVE_SYS_TIME_H */

/* Define to 1 if you have the <syslog.h> header file. */
/* #undef HAVE_SYSLOG_H */

/* Define to 1 if you have the <time.h> header file. */
#define HAVE_TIME_H 1

/* Define to 1 if you have the <unistd.h> header file. */
/* #undef HAVE_UNISTD_H */
