/**
 * \file Ternary.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef TERNARY_H
#define TERNARY_H

#include "Prereqs.h"

namespace Seoul
{

/**
 * a type for reflection which can be true, false, or unset
 */
enum class Ternary
{
	Unset = -1,
	TernaryFalse = 0,
	TernaryTrue = 1
};

} // namespace Seoul

#endif // include guard
