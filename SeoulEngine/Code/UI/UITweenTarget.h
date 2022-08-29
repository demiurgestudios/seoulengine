/**
 * \file UITweenTarget.h
 * \brief Enum describes the property that will be manipulated
 * by a UITween.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_TWEEN_TARGET_H
#define UI_TWEEN_TARGET_H

#include "Prereqs.h"

namespace Seoul::UI
{

enum class TweenTarget
{
	/** Apply tween transformation to the color multiply alpha. */
	kAlpha,

	/** Apply tween transformation to the object space depth 3D value. */
	kDepth3D,

	/** Apply tween transformation to the object space position X. */
	kPositionX,

	/** Apply tween transformation to the object space position Y. */
	kPositionY,

	/** Apply tween transformation to the object space rotation. */
	kRotation,

	/** Apply tween transformation to the object space scale X. */
	kScaleX,

	/** Apply tween transformation to the object space scale Y. */
	kScaleY,

	/** Apply no transformation, tween only waits for a time and then invokes its completion callback. */
	kTimer,
};

} // namespace Seoul::UI

#endif // include guard
