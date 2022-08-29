/**
 * \file StaticAssert.h
 * \brief SeoulEngine wrapper of a compile time assertion.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef STATIC_ASSERT_H
#define STATIC_ASSERT_H

// Single argument static_assert is supported as of C++17.
#if defined(__cplusplus) && (__cplusplus) >= 201703L
#	define SEOUL_STATIC_ASSERT(expression)               static_assert(expression)
#else
#	define SEOUL_STATIC_ASSERT(expression)               static_assert((expression), #expression)
#endif
#define SEOUL_STATIC_ASSERT_MESSAGE(expression, message) static_assert((expression), message)

#endif // include guard
