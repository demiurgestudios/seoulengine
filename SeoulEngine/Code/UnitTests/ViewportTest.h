/**
 * \file ViewportTest.h
 * \brief Unit tests for the Viewport struct. A Viewport
 * represents an area of a RenderTarget.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef VIEWPORT_TEST_H
#define VIEWPORT_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class ViewportTest SEOUL_SEALED
{
public:
	void TestBasic();
	void TestMethods();
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
