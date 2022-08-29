/**
 * \file Axis.h
 * \brief Enum used to identify up to 4 spatial axes.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef AXIS_H
#define AXIS_H

#include "Prereqs.h"

namespace Seoul
{

/**
 * Enum used to identify axes for up to 4 dimensions.
 */
enum class Axis
{
	kX = 0,
	kY,
	kZ,
	kW
};

} // namespace Seoul

#endif // include guard
