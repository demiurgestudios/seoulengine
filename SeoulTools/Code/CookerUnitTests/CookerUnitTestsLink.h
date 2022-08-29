/**
 * \file CookerUnitTests.h
 * \brief Root header to include Cooker unit tests in a project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef COOKER_UNIT_TESTS_LINK_H
#define COOKER_UNIT_TESTS_LINK_H

#include "ReflectionType.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_LINK_ME(class, JsonMergeTest);
SEOUL_LINK_ME(class, SlimCSTest);

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
