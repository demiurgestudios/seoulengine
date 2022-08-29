/**
 * \file EffectParameterType.h
 * \brief Enum defining the different types of Effect/Material parameters.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EFFECT_PARAMETER_TYPE_H
#define EFFECT_PARAMETER_TYPE_H

#include "Prereqs.h"
namespace Seoul { class BaseTexture; }
namespace Seoul { class BaseMipTexture; }
namespace Seoul { namespace Content { template <typename T> class Handle; } }
namespace Seoul { struct Matrix3x4; }
namespace Seoul { struct Matrix4D; }
namespace Seoul {typedef Content::Handle<BaseTexture> TextureContentHandle; }
namespace Seoul { typedef Content::Handle<BaseMipTexture> MipTextureContentHandle; }
namespace Seoul { struct Vector2D; }
namespace Seoul { struct Vector3D; }
namespace Seoul { struct Vector4D; }

namespace Seoul
{

enum class EffectParameterType
{
	kBool,
	kFloat,
	kInt,
	kMatrix3x4,
	kMatrix4D,
	kTexture,
	kVector2D,
	kVector3D,
	kVector4D,
	kArray,
	kUnknown
};

// Define helper structures that allow for easy conversion between
// C++ types that can be material parameters (i.e. Bool) and an enum
// that represents that type.
template <typename T> struct TypeToParameterType {};

#define SEOUL_DEFINE_TYPE_TO_PARAMETER_TYPE_HELPER_STRUCT(name) \
	template <> \
	struct TypeToParameterType<name> \
	{ \
		static const EffectParameterType Value = EffectParameterType::k##name; \
	}

SEOUL_DEFINE_TYPE_TO_PARAMETER_TYPE_HELPER_STRUCT(Bool);
SEOUL_DEFINE_TYPE_TO_PARAMETER_TYPE_HELPER_STRUCT(Float);
SEOUL_DEFINE_TYPE_TO_PARAMETER_TYPE_HELPER_STRUCT(Int);
SEOUL_DEFINE_TYPE_TO_PARAMETER_TYPE_HELPER_STRUCT(Matrix3x4);
SEOUL_DEFINE_TYPE_TO_PARAMETER_TYPE_HELPER_STRUCT(Matrix4D);
template <> struct TypeToParameterType<MipTextureContentHandle> { static const EffectParameterType Value = EffectParameterType::kTexture; };
template <> struct TypeToParameterType<TextureContentHandle> { static const EffectParameterType Value = EffectParameterType::kTexture; };
SEOUL_DEFINE_TYPE_TO_PARAMETER_TYPE_HELPER_STRUCT(Vector2D);
SEOUL_DEFINE_TYPE_TO_PARAMETER_TYPE_HELPER_STRUCT(Vector3D);
SEOUL_DEFINE_TYPE_TO_PARAMETER_TYPE_HELPER_STRUCT(Vector4D);

#undef SEOUL_DEFINE_TYPE_TO_PARAMETER_TYPE_HELPER_STRUCT

} // namespace Seoul

#endif // include guard
