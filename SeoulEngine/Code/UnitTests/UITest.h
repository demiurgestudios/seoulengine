/**
 * \file UITest.h
 * \brief Unit test for functionality in the UI project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_TEST_H
#define UI_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class UITest SEOUL_SEALED
{
public:
	void TestMotionCancel();
	void TestMotionCancelAll();

	void TestTweensBasic();
	void TestTweensCancel();
	void TestTweensCancelAll();
	void TestTweensValues();
}; // class UITest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
