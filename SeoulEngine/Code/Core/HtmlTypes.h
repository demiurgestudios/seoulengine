/**
 * \file HtmlTypes.h
 * \brief Types used by HtmlReader and related infrastructure.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef HTML_TYPES_H
#define HTML_TYPES_H

#include "Prereqs.h"

namespace Seoul
{

enum class HtmlAlign : UInt8
{
	// IMPORTANT: Order matters, a Max() is applied
	// to merge these modes.
	kLeft = 0,
	kRight = 1,
	kCenter = 2,
	kJustify = 3,

	COUNT,
};

enum class HtmlImageAlign
{
	kTop = 0,
	kMiddle = 1,
	kBottom = 2,
	kLeft = 3,
	kRight = 4,
};

enum class HtmlAttribute
{
	kUnknown = 0,

	kAlign,
	kColor,
	kColorBottom,
	kColorTop,
	kEffect,
	kFace,
	kHeight,
	kHoffset,
	kHspace,
	kHref,
	kLetterSpacing,
	kSize,
	kSrc,
	kType,
	kVoffset,
	kVspace,
	kWidth,
};

enum class HtmlTag
{
	kUnknown = 0,
	kB,
	kBr,
	kFont,
	kI,
	kImg,
	kLink,
	kP,
	kRoot,
	kTextChunk,
	kVerticalCentered,
};

enum class HtmlTagStyle
{
	kNone,
	kSelfTerminating,
	kTerminator,
};

} // namespace Seoul

#endif // include guard
