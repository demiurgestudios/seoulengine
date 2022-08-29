/**
 * \file CookDatabaseTest.h
 * \brief Unit test header file for the CookDatabase project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef COOK_DATABASE_TEST_H
#define COOK_DATABASE_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class CookDatabaseTest SEOUL_SEALED
{
public:
	void TestMetadata();
}; // class CookDatabaseTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
