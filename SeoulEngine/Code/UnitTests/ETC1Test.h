/**
 * \file ETC1Test.h
 * \brief Unit test header file for ETC1 decompression functions.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ETC1_TEST_H
#define ETC1_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class ETC1Test SEOUL_SEALED
{
public:
	void TestDecompress();
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
