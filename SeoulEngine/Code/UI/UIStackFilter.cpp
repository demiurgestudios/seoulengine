/**
 * \file UIStackFilter.cpp
 * \brief Enum used to filter stack loads
 * in various configurations.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "UIStackFilter.h"

namespace Seoul
{

SEOUL_BEGIN_ENUM(UI::StackFilter)
	SEOUL_ENUM_N("Always", UI::StackFilter::kAlways)
	SEOUL_ENUM_N("DevAndAutomation", UI::StackFilter::kDevAndAutomation)
	SEOUL_ENUM_N("DevOnly", UI::StackFilter::kDevOnly)
SEOUL_END_ENUM()

} // namespace Seoul
