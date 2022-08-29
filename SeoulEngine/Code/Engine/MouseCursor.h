/**
 * \file MouseCursor.h
 * \brief Enum of mouse cursor states. Conditionally supported
 * by the current platform (not all platforms support a mouse
 * cursor).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef MOUSE_CURSOR_H
#define MOUSE_CURSOR_H

#include "Prereqs.h"

namespace Seoul
{

enum class MouseCursor
{
	kArrow,
	kArrowLeftBottomRightTop,
	kArrowLeftRight,
	kArrowLeftTopRightBottom,
	kArrowUpDown,
	kIbeam,
	kMove,
	COUNT,
};

} // namespace Seoul

#endif // include guard
