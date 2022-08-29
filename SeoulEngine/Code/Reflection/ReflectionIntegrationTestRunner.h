/**
 * \file ReflectionIntegrationTestRunner.h
 * \brief Defines a runner that executes integration tests using the Reflection system
 * to enumerate available unit tests.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_INTEGRATION_TEST_RUNNER_H
#define REFLECTION_INTEGRATION_TEST_RUNNER_H

#include "Prereqs.h"
namespace Seoul { class String; }

namespace Seoul::UnitTesting
{

#if SEOUL_UNIT_TESTS
Bool RunIntegrationTests(const String& sOptionalTestName);
#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul::UnitTesting

#endif // include guard
