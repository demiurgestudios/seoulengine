/**
 * \file GamePathsTest.h
 * \brief Unit test header file for game paths class
 *
 * This file contains the unit test declarations of GamePaths, the game dir class
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_PATHS_TEST_H
#define GAME_PATHS_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class GamePathsTest SEOUL_SEALED
{
public:
	void TestMethods(void);
	void TestValues(void);
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
