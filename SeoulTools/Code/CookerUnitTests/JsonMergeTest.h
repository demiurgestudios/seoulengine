/**
 * \file JsonMergeTest.h
 * \brief Unit tests of the SlimCS compiler and toolchain.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef JSON_MERGE_TEST_H
#define JSON_MERGE_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class JsonMergeTest SEOUL_SEALED
{
public:
	void TestDirected();
}; // class JsonMergeTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
