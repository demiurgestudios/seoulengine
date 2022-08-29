/**
 * \file UIStackFilter.h
 * \brief Enum used to filter stack loads
 * in various configurations.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_STACK_FILTER_H
#define UI_STACK_FILTER_H

#include "Prereqs.h"

namespace Seoul::UI
{

enum class StackFilter
{
	/** Stacks with this filter are always loaded. */
	kAlways,

	/** Stacks with this filter are loaded for developers and automated tests. */
	kDevAndAutomation,

	/** Stacks with this filter are only loaded for developers. */
	kDevOnly,
};

} // namespace Seoul::UI

#endif // include guard
