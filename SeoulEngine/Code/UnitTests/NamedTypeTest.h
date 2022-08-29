/**
* \file NamedTypeTest.h
* \brief Unit test header file for the Seoul NamedType class
*
* Copyright (c) Demiurge Studios, Inc.
* 
* This source code is licensed under the MIT license.
* Full license details can be found in the LICENSE file
* in the root directory of this source tree.
*/

#pragma once
#ifndef NAMED_TYPE_TEST_H
#define NAMED_TYPE_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class NamedTypeTest SEOUL_SEALED
{
public:
	void TestOperators();
	// reflection is tested in ReflectionTest.h
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
