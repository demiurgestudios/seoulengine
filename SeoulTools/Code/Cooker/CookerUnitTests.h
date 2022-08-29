/**
 * \file CookerUnitTests.h
 * \brief Defines the main function for a build run that will execute
 * unit tests.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef COOKER_UNIT_TESTS_H
#define COOKER_UNIT_TESTS_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS
Int CookerRunUnitTests(const String& sOptionalTestName);
#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
