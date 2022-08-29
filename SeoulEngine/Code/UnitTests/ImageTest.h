/**
 * \file ImageTest.h
 * \brief Unit tests for the Seoul Engine wrapper
 * around image reading functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef IMAGE_TEST_H
#define IMAGE_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class ImageTest SEOUL_SEALED
{
public:
	void TestPngSuite();
}; // class ImageTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif  // include guard
