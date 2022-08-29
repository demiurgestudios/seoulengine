/**
 * \file ReflectionMethodVariations.h
 * \brief Internal header file, mainly, includes
 * SEOUL_METHOD_VARIATION_IMPL_FILENAME over and over to generate
 * many variations of a tempalted type used to generate a concrete
 * implementation of a Reflection::Method subclass.

 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef SEOUL_METHOD_VARIATION_IMPL_FILENAME
#error ReflectionPropertyVariations.h should only be by files that define SEOUL_METHOD_VARIATION_IMPL_FILENAME.
#endif

	// R()
#   define SEOUL_METHOD_VARIATION_ARGC  0
#   define SEOUL_METHOD_VARIATION_T
#   define SEOUL_METHOD_VARIATION_T_ARGS
#   define SEOUL_METHOD_VARIATION_ARGS
#   define SEOUL_METHOD_VARIATION_PARAMS
#	define SEOUL_METHOD_VARIATION_PREFIX Arg0
#	define SEOUL_METHOD_VARIATION_TYPE_IDS
#   include SEOUL_METHOD_VARIATION_IMPL_FILENAME

	// R(A0)
#   define SEOUL_METHOD_VARIATION_ARGC  1
#   define SEOUL_METHOD_VARIATION_T      typename A0
#   define SEOUL_METHOD_VARIATION_T_ARGS A0
#   define SEOUL_METHOD_VARIATION_ARGS   A0 a0
#   define SEOUL_METHOD_VARIATION_PARAMS a0
#	define SEOUL_METHOD_VARIATION_PREFIX Arg1
#	define SEOUL_METHOD_VARIATION_TYPE_IDS      TypeInfoDetail::TypeInfoImpl<A0>::Get()
#   include SEOUL_METHOD_VARIATION_IMPL_FILENAME

	// R(A0, A1)
#   define SEOUL_METHOD_VARIATION_ARGC  2
#   define SEOUL_METHOD_VARIATION_T      typename A0, typename A1
#   define SEOUL_METHOD_VARIATION_T_ARGS A0, A1
#   define SEOUL_METHOD_VARIATION_ARGS   A0 a0, A1 a1
#   define SEOUL_METHOD_VARIATION_PARAMS a0, a1
#	define SEOUL_METHOD_VARIATION_PREFIX Arg2
#	define SEOUL_METHOD_VARIATION_TYPE_IDS      TypeInfoDetail::TypeInfoImpl<A0>::Get(), TypeInfoDetail::TypeInfoImpl<A1>::Get()
#   include SEOUL_METHOD_VARIATION_IMPL_FILENAME

	// R(A0, A1, A2)
#   define SEOUL_METHOD_VARIATION_ARGC  3
#   define SEOUL_METHOD_VARIATION_T      typename A0, typename A1, typename A2
#   define SEOUL_METHOD_VARIATION_T_ARGS A0, A1, A2
#   define SEOUL_METHOD_VARIATION_ARGS   A0 a0, A1 a1, A2 a2
#   define SEOUL_METHOD_VARIATION_PARAMS a0, a1, a2
#	define SEOUL_METHOD_VARIATION_PREFIX Arg3
#	define SEOUL_METHOD_VARIATION_TYPE_IDS      TypeInfoDetail::TypeInfoImpl<A0>::Get(), TypeInfoDetail::TypeInfoImpl<A1>::Get(), TypeInfoDetail::TypeInfoImpl<A2>::Get()
#   include SEOUL_METHOD_VARIATION_IMPL_FILENAME

	// R(A0, A1, A2, A3)
#   define SEOUL_METHOD_VARIATION_ARGC  4
#   define SEOUL_METHOD_VARIATION_T      typename A0, typename A1, typename A2, typename A3
#   define SEOUL_METHOD_VARIATION_T_ARGS A0, A1, A2, A3
#   define SEOUL_METHOD_VARIATION_ARGS   A0 a0, A1 a1, A2 a2, A3 a3
#   define SEOUL_METHOD_VARIATION_PARAMS a0, a1, a2, a3
#	define SEOUL_METHOD_VARIATION_PREFIX Arg4
#	define SEOUL_METHOD_VARIATION_TYPE_IDS      TypeInfoDetail::TypeInfoImpl<A0>::Get(), TypeInfoDetail::TypeInfoImpl<A1>::Get(), TypeInfoDetail::TypeInfoImpl<A2>::Get(), TypeInfoDetail::TypeInfoImpl<A3>::Get()
#   include SEOUL_METHOD_VARIATION_IMPL_FILENAME

	// R(A0, A1, A2, A3, A4)
#   define SEOUL_METHOD_VARIATION_ARGC  5
#   define SEOUL_METHOD_VARIATION_T      typename A0, typename A1, typename A2, typename A3, typename A4
#   define SEOUL_METHOD_VARIATION_T_ARGS A0, A1, A2, A3, A4
#   define SEOUL_METHOD_VARIATION_ARGS   A0 a0, A1 a1, A2 a2, A3 a3, A4 a4
#   define SEOUL_METHOD_VARIATION_PARAMS a0, a1, a2, a3, a4
#	define SEOUL_METHOD_VARIATION_PREFIX Arg5
#	define SEOUL_METHOD_VARIATION_TYPE_IDS      TypeInfoDetail::TypeInfoImpl<A0>::Get(), TypeInfoDetail::TypeInfoImpl<A1>::Get(), TypeInfoDetail::TypeInfoImpl<A2>::Get(), TypeInfoDetail::TypeInfoImpl<A3>::Get(), TypeInfoDetail::TypeInfoImpl<A4>::Get()
#   include SEOUL_METHOD_VARIATION_IMPL_FILENAME

	// R(A0, A1, A2, A3, A4, A5)
#   define SEOUL_METHOD_VARIATION_ARGC  6
#   define SEOUL_METHOD_VARIATION_T      typename A0, typename A1, typename A2, typename A3, typename A4, typename A5
#   define SEOUL_METHOD_VARIATION_T_ARGS A0, A1, A2, A3, A4, A5
#   define SEOUL_METHOD_VARIATION_ARGS   A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5
#   define SEOUL_METHOD_VARIATION_PARAMS a0, a1, a2, a3, a4, a5
#	define SEOUL_METHOD_VARIATION_PREFIX Arg6
#	define SEOUL_METHOD_VARIATION_TYPE_IDS      TypeInfoDetail::TypeInfoImpl<A0>::Get(), TypeInfoDetail::TypeInfoImpl<A1>::Get(), TypeInfoDetail::TypeInfoImpl<A2>::Get(), TypeInfoDetail::TypeInfoImpl<A3>::Get(), TypeInfoDetail::TypeInfoImpl<A4>::Get(), TypeInfoDetail::TypeInfoImpl<A5>::Get()
#   include SEOUL_METHOD_VARIATION_IMPL_FILENAME

	// R(A0, A1, A2, A3, A4, A5, A6)
#   define SEOUL_METHOD_VARIATION_ARGC  7
#   define SEOUL_METHOD_VARIATION_T      typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6
#   define SEOUL_METHOD_VARIATION_T_ARGS A0, A1, A2, A3, A4, A5, A6
#   define SEOUL_METHOD_VARIATION_ARGS   A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6
#   define SEOUL_METHOD_VARIATION_PARAMS a0, a1, a2, a3, a4, a5, a6
#	define SEOUL_METHOD_VARIATION_PREFIX Arg7
#	define SEOUL_METHOD_VARIATION_TYPE_IDS      TypeInfoDetail::TypeInfoImpl<A0>::Get(), TypeInfoDetail::TypeInfoImpl<A1>::Get(), TypeInfoDetail::TypeInfoImpl<A2>::Get(), TypeInfoDetail::TypeInfoImpl<A3>::Get(), TypeInfoDetail::TypeInfoImpl<A4>::Get(), TypeInfoDetail::TypeInfoImpl<A5>::Get(), TypeInfoDetail::TypeInfoImpl<A6>::Get()
#   include SEOUL_METHOD_VARIATION_IMPL_FILENAME

	// R(A0, A1, A2, A3, A4, A5, A6, A7)
#   define SEOUL_METHOD_VARIATION_ARGC  8
#   define SEOUL_METHOD_VARIATION_T      typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7
#   define SEOUL_METHOD_VARIATION_T_ARGS A0, A1, A2, A3, A4, A5, A6, A7
#   define SEOUL_METHOD_VARIATION_ARGS   A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7
#   define SEOUL_METHOD_VARIATION_PARAMS a0, a1, a2, a3, a4, a5, a6, a7
#	define SEOUL_METHOD_VARIATION_PREFIX Arg8
#	define SEOUL_METHOD_VARIATION_TYPE_IDS      TypeInfoDetail::TypeInfoImpl<A0>::Get(), TypeInfoDetail::TypeInfoImpl<A1>::Get(), TypeInfoDetail::TypeInfoImpl<A2>::Get(), TypeInfoDetail::TypeInfoImpl<A3>::Get(), TypeInfoDetail::TypeInfoImpl<A4>::Get(), TypeInfoDetail::TypeInfoImpl<A5>::Get(), TypeInfoDetail::TypeInfoImpl<A6>::Get(), TypeInfoDetail::TypeInfoImpl<A7>::Get()
#   include SEOUL_METHOD_VARIATION_IMPL_FILENAME

	// R(A0, A1, A2, A3, A4, A5, A6, A7, A8)
#   define SEOUL_METHOD_VARIATION_ARGC  9
#   define SEOUL_METHOD_VARIATION_T      typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8
#   define SEOUL_METHOD_VARIATION_T_ARGS A0, A1, A2, A3, A4, A5, A6, A7, A8
#   define SEOUL_METHOD_VARIATION_ARGS   A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8
#   define SEOUL_METHOD_VARIATION_PARAMS a0, a1, a2, a3, a4, a5, a6, a7, a8
#	define SEOUL_METHOD_VARIATION_PREFIX Arg9
#	define SEOUL_METHOD_VARIATION_TYPE_IDS      TypeInfoDetail::TypeInfoImpl<A0>::Get(), TypeInfoDetail::TypeInfoImpl<A1>::Get(), TypeInfoDetail::TypeInfoImpl<A2>::Get(), TypeInfoDetail::TypeInfoImpl<A3>::Get(), TypeInfoDetail::TypeInfoImpl<A4>::Get(), TypeInfoDetail::TypeInfoImpl<A5>::Get(), TypeInfoDetail::TypeInfoImpl<A6>::Get(), TypeInfoDetail::TypeInfoImpl<A7>::Get(), TypeInfoDetail::TypeInfoImpl<A8>::Get()
#   include SEOUL_METHOD_VARIATION_IMPL_FILENAME

	// R(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9)
#   define SEOUL_METHOD_VARIATION_ARGC  10
#   define SEOUL_METHOD_VARIATION_T      typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9
#   define SEOUL_METHOD_VARIATION_T_ARGS A0, A1, A2, A3, A4, A5, A6, A7, A8, A9
#   define SEOUL_METHOD_VARIATION_ARGS   A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9
#   define SEOUL_METHOD_VARIATION_PARAMS a0, a1, a2, a3, a4, a5, a6, a7, a8, a9
#	define SEOUL_METHOD_VARIATION_PREFIX Arg10
#	define SEOUL_METHOD_VARIATION_TYPE_IDS      TypeInfoDetail::TypeInfoImpl<A0>::Get(), TypeInfoDetail::TypeInfoImpl<A1>::Get(), TypeInfoDetail::TypeInfoImpl<A2>::Get(), TypeInfoDetail::TypeInfoImpl<A3>::Get(), TypeInfoDetail::TypeInfoImpl<A4>::Get(), TypeInfoDetail::TypeInfoImpl<A5>::Get(), TypeInfoDetail::TypeInfoImpl<A6>::Get(), TypeInfoDetail::TypeInfoImpl<A7>::Get(), TypeInfoDetail::TypeInfoImpl<A8>::Get(), TypeInfoDetail::TypeInfoImpl<A9>::Get()
#   include SEOUL_METHOD_VARIATION_IMPL_FILENAME

	// R(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10)
#   define SEOUL_METHOD_VARIATION_ARGC  11
#   define SEOUL_METHOD_VARIATION_T      typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10
#   define SEOUL_METHOD_VARIATION_T_ARGS A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10
#   define SEOUL_METHOD_VARIATION_ARGS   A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10
#   define SEOUL_METHOD_VARIATION_PARAMS a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10
#	define SEOUL_METHOD_VARIATION_PREFIX Arg11
#	define SEOUL_METHOD_VARIATION_TYPE_IDS      TypeInfoDetail::TypeInfoImpl<A0>::Get(), TypeInfoDetail::TypeInfoImpl<A1>::Get(), TypeInfoDetail::TypeInfoImpl<A2>::Get(), TypeInfoDetail::TypeInfoImpl<A3>::Get(), TypeInfoDetail::TypeInfoImpl<A4>::Get(), TypeInfoDetail::TypeInfoImpl<A5>::Get(), TypeInfoDetail::TypeInfoImpl<A6>::Get(), TypeInfoDetail::TypeInfoImpl<A7>::Get(), TypeInfoDetail::TypeInfoImpl<A8>::Get(), TypeInfoDetail::TypeInfoImpl<A9>::Get(), TypeInfoDetail::TypeInfoImpl<A10>::Get()
#   include SEOUL_METHOD_VARIATION_IMPL_FILENAME

	// R(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11)
#   define SEOUL_METHOD_VARIATION_ARGC  12
#   define SEOUL_METHOD_VARIATION_T      typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11
#   define SEOUL_METHOD_VARIATION_T_ARGS A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11
#   define SEOUL_METHOD_VARIATION_ARGS   A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11
#   define SEOUL_METHOD_VARIATION_PARAMS a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11
#	define SEOUL_METHOD_VARIATION_PREFIX Arg12
#	define SEOUL_METHOD_VARIATION_TYPE_IDS      TypeInfoDetail::TypeInfoImpl<A0>::Get(), TypeInfoDetail::TypeInfoImpl<A1>::Get(), TypeInfoDetail::TypeInfoImpl<A2>::Get(), TypeInfoDetail::TypeInfoImpl<A3>::Get(), TypeInfoDetail::TypeInfoImpl<A4>::Get(), TypeInfoDetail::TypeInfoImpl<A5>::Get(), TypeInfoDetail::TypeInfoImpl<A6>::Get(), TypeInfoDetail::TypeInfoImpl<A7>::Get(), TypeInfoDetail::TypeInfoImpl<A8>::Get(), TypeInfoDetail::TypeInfoImpl<A9>::Get(), TypeInfoDetail::TypeInfoImpl<A10>::Get(), TypeInfoDetail::TypeInfoImpl<A11>::Get()
#   include SEOUL_METHOD_VARIATION_IMPL_FILENAME

	// R(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12)
#   define SEOUL_METHOD_VARIATION_ARGC  13
#   define SEOUL_METHOD_VARIATION_T      typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12
#   define SEOUL_METHOD_VARIATION_T_ARGS A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12
#   define SEOUL_METHOD_VARIATION_ARGS   A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12
#   define SEOUL_METHOD_VARIATION_PARAMS a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12
#	define SEOUL_METHOD_VARIATION_PREFIX Arg13
#	define SEOUL_METHOD_VARIATION_TYPE_IDS      TypeInfoDetail::TypeInfoImpl<A0>::Get(), TypeInfoDetail::TypeInfoImpl<A1>::Get(), TypeInfoDetail::TypeInfoImpl<A2>::Get(), TypeInfoDetail::TypeInfoImpl<A3>::Get(), TypeInfoDetail::TypeInfoImpl<A4>::Get(), TypeInfoDetail::TypeInfoImpl<A5>::Get(), TypeInfoDetail::TypeInfoImpl<A6>::Get(), TypeInfoDetail::TypeInfoImpl<A7>::Get(), TypeInfoDetail::TypeInfoImpl<A8>::Get(), TypeInfoDetail::TypeInfoImpl<A9>::Get(), TypeInfoDetail::TypeInfoImpl<A10>::Get(), TypeInfoDetail::TypeInfoImpl<A11>::Get(), TypeInfoDetail::TypeInfoImpl<A12>::Get()
#   include SEOUL_METHOD_VARIATION_IMPL_FILENAME

	// R(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13)
#   define SEOUL_METHOD_VARIATION_ARGC  14
#   define SEOUL_METHOD_VARIATION_T      typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13
#   define SEOUL_METHOD_VARIATION_T_ARGS A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13
#   define SEOUL_METHOD_VARIATION_ARGS   A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13
#   define SEOUL_METHOD_VARIATION_PARAMS a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13
#	define SEOUL_METHOD_VARIATION_PREFIX Arg14
#	define SEOUL_METHOD_VARIATION_TYPE_IDS      TypeInfoDetail::TypeInfoImpl<A0>::Get(), TypeInfoDetail::TypeInfoImpl<A1>::Get(), TypeInfoDetail::TypeInfoImpl<A2>::Get(), TypeInfoDetail::TypeInfoImpl<A3>::Get(), TypeInfoDetail::TypeInfoImpl<A4>::Get(), TypeInfoDetail::TypeInfoImpl<A5>::Get(), TypeInfoDetail::TypeInfoImpl<A6>::Get(), TypeInfoDetail::TypeInfoImpl<A7>::Get(), TypeInfoDetail::TypeInfoImpl<A8>::Get(), TypeInfoDetail::TypeInfoImpl<A9>::Get(), TypeInfoDetail::TypeInfoImpl<A10>::Get(), TypeInfoDetail::TypeInfoImpl<A11>::Get(), TypeInfoDetail::TypeInfoImpl<A12>::Get(), TypeInfoDetail::TypeInfoImpl<A13>::Get()
#   include SEOUL_METHOD_VARIATION_IMPL_FILENAME

	// R(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14)
#   define SEOUL_METHOD_VARIATION_ARGC  15
#   define SEOUL_METHOD_VARIATION_T      typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13, typename A14
#   define SEOUL_METHOD_VARIATION_T_ARGS A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14
#   define SEOUL_METHOD_VARIATION_ARGS   A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13, A14 a14
#   define SEOUL_METHOD_VARIATION_PARAMS a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14
#	define SEOUL_METHOD_VARIATION_PREFIX Arg15
#	define SEOUL_METHOD_VARIATION_TYPE_IDS      TypeInfoDetail::TypeInfoImpl<A0>::Get(), TypeInfoDetail::TypeInfoImpl<A1>::Get(), TypeInfoDetail::TypeInfoImpl<A2>::Get(), TypeInfoDetail::TypeInfoImpl<A3>::Get(), TypeInfoDetail::TypeInfoImpl<A4>::Get(), TypeInfoDetail::TypeInfoImpl<A5>::Get(), TypeInfoDetail::TypeInfoImpl<A6>::Get(), TypeInfoDetail::TypeInfoImpl<A7>::Get(), TypeInfoDetail::TypeInfoImpl<A8>::Get(), TypeInfoDetail::TypeInfoImpl<A9>::Get(), TypeInfoDetail::TypeInfoImpl<A10>::Get(), TypeInfoDetail::TypeInfoImpl<A11>::Get(), TypeInfoDetail::TypeInfoImpl<A12>::Get(), TypeInfoDetail::TypeInfoImpl<A13>::Get(), TypeInfoDetail::TypeInfoImpl<A14>::Get()
#   include SEOUL_METHOD_VARIATION_IMPL_FILENAME

// The purpose of this assert is to remind you to add another variation above
// when you increase the size of the MethodArguments array.  Once you have done
// so, you can update this assert accordingly.
SEOUL_STATIC_ASSERT(MethodArguments::StaticSize == 15);

#undef SEOUL_METHOD_VARIATION_IMPL_FILENAME
