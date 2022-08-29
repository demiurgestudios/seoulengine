/**
 * \file EffectReceiverGLSLES2GlslangWrapper.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EFFECT_RECEIVER_GLSLES2_GLSLANG_WRAPPER_H
#define EFFECT_RECEIVER_GLSLES2_GLSLANG_WRAPPER_H

#include "Prereqs.h"
namespace Seoul { class String; }

namespace Seoul
{

class EffectReceiverGLSLES2GlslangWrapper SEOUL_SEALED
{
public:
	EffectReceiverGLSLES2GlslangWrapper();
	~EffectReceiverGLSLES2GlslangWrapper();

	Bool Validate(
		const String& sPixelShaderSource,
		const String& sVertexShaderSource) const;

private:
	SEOUL_DISABLE_COPY(EffectReceiverGLSLES2GlslangWrapper);
}; // class EffectReceiverGLSLES2GlslangWrapper

} // namespace Seoul

#endif // include guard
