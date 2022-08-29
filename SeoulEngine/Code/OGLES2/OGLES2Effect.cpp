/**
 * \file OGLES2Effect.cpp
 * \brief Implementation of SeoulEngine Effect for the OGLES2. Uses a custom
 * shader Effect system, GLSLFXLite, to handle the low-level tasks
 * of management effect samplers, render states, and shader parameters.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "GLSLFXLiteInternal.h"
#include "OGLES2Effect.h"
#include "UnsafeHandle.h"

namespace Seoul
{

OGLES2Effect::OGLES2Effect(
	FilePath filePath,
	void* pRawEffectFileData,
	UInt32 zFileSizeInBytes)
	: Effect(filePath, pRawEffectFileData, zFileSizeInBytes)
{
}

OGLES2Effect::~OGLES2Effect()
{
	SEOUL_ASSERT(IsRenderThread());

	SafeDelete<GLSLFXLite>(m_hHandle);
}

/**
 * When called, sets all texture parameters to nullptr. This
 * should be called before any textures are unloaded to prevent
 * dangling references on some platforms.
 */
void OGLES2Effect::UnsetAllTextures()
{
	SEOUL_ASSERT(IsRenderThread());

	GLSLFXLite* e = StaticCast<GLSLFXLite*>(m_hHandle);
	if (e)
	{
		GLSLFXLiteEffectDescription effectDescription;
		memset(&effectDescription, 0, sizeof(effectDescription));
		e->GetEffectDescription(&effectDescription);

		for (UInt i = 0u; i < effectDescription.m_uParameters; ++i)
		{
			UnsafeHandle h = e->GetParameterHandleFromIndex(i);

			GLSLFXLiteParameterDescription parameterDescription;
			memset(&parameterDescription, 0, sizeof(parameterDescription));
			e->GetParameterDescription(h, &parameterDescription);

			if (GLSLFX_PARAMETERCLASS_SAMPLER == parameterDescription.m_Class)
			{
				e->SetSampler(h, nullptr);
			}
		}
	}
}

/**
 * Called by the render device when the device is lost, to allow
 * the Effect to do any necessary bookkeeping.
 */
void OGLES2Effect::OnLost()
{
	SEOUL_ASSERT(IsRenderThread());

	BaseGraphicsObject::OnLost();

	if (m_hHandle.IsValid())
	{
		StaticCast<GLSLFXLite*>(m_hHandle)->OnLostDevice();
	}
}

/**
 * Called by the render device when the device is reset after being lost,
 * to allow the Effect to do any necessary bookkeeping.
 */
void OGLES2Effect::OnReset()
{
	SEOUL_ASSERT(IsRenderThread());

	if (m_hHandle.IsValid())
	{
		StaticCast<GLSLFXLite*>(m_hHandle)->OnResetDevice();
	}

	BaseGraphicsObject::OnReset();
}

/**
 * Get the EffectParameterType of the parameter described by hHandle.
 */
EffectParameterType OGLES2Effect::InternalGetParameterType(UnsafeHandle hHandle) const
{
	SEOUL_ASSERT(IsRenderThread());

	GLSLFXLite* e = StaticCast<GLSLFXLite*>(m_hHandle);
	if (e)
	{
		GLSLFXLiteParameterDescription parameterDescription;
		memset(&parameterDescription, 0, sizeof(parameterDescription));
		e->GetParameterDescription(hHandle, &parameterDescription);

		// Any array type is type EffectParameterType::kArray, independent
		// from its per-element type.
		if (GLSLFX_PARAMETERCLASS_ARRAY == parameterDescription.m_Class)
		{
			return EffectParameterType::kArray;
		}

		switch (parameterDescription.m_Class)
		{
		case GLSLFX_PARAMETERCLASS_SCALAR:
			switch (parameterDescription.m_Type)
			{
			case GLSLFX_FLOAT:
				return EffectParameterType::kFloat;
			case GLSLFX_INT:
				return EffectParameterType::kInt;
			case GLSLFX_BOOL:
				return EffectParameterType::kBool;
			default:
				break;
			};
			break;
		case GLSLFX_PARAMETERCLASS_VECTOR:
			switch (parameterDescription.m_Type)
			{
			case GLSLFX_FLOAT1: return EffectParameterType::kFloat;
			case GLSLFX_FLOAT2: return EffectParameterType::kVector2D;
			case GLSLFX_FLOAT3: return EffectParameterType::kVector3D;
			case GLSLFX_FLOAT4: return EffectParameterType::kVector4D;
			default:
				break;
			};
			break;
		case GLSLFX_PARAMETERCLASS_MATRIX:
			if (GLSLFX_FLOAT4x4 == parameterDescription.m_Type)
			{
				return EffectParameterType::kMatrix4D;
			}
			break;
		case GLSLFX_PARAMETERCLASS_SAMPLER:
			return EffectParameterType::kTexture;
		default:
			break;
		};
	}

	return EffectParameterType::kUnknown;
}

/**
 * Constructs the effect - if successful, the effect will be in the kCreated state
 * and can be used on non-render threads. Render operations will not be valid until the
 * effect is reset.
 */
Bool OGLES2Effect::OnCreate()
{
	SEOUL_ASSERT(IsRenderThread());

	m_hHandle = SEOUL_NEW(MemoryBudgets::Rendering) GLSLFXLite(
		m_FilePath,
		m_pRawEffectFileData,
		m_zFileSizeInBytes);

	InternalPopulateParameterTable();
	InternalPopulateTechniqueTable();

	InternalFreeFileData();
	SEOUL_VERIFY(BaseGraphicsObject::OnCreate());
	return true;
}


/**
 * Fills a hash table owned by Effect with HString to
 * parameter handles.
 *
 * This exists so that parameters can be looked up in constant time
 * given an HString name. HStrings are cheap keys, since they are
 * only a 16-bit ID once the HString has been instantiated.
 */
void OGLES2Effect::InternalPopulateParameterTable()
{
	SEOUL_ASSERT(IsRenderThread());

	m_tParametersBySemantic.Clear();

	GLSLFXLite* e = StaticCast<GLSLFXLite*>(m_hHandle);
	if (e)
	{
		GLSLFXLiteEffectDescription effectDescription;
		memset(&effectDescription, 0, sizeof(effectDescription));
		e->GetEffectDescription(&effectDescription);

		for (UInt i = 0u; i < effectDescription.m_uParameters; ++i)
		{
			UnsafeHandle h = e->GetParameterHandleFromIndex(i);

			GLSLFXLiteParameterDescription parameterDescription;
			memset(&parameterDescription, 0, sizeof(parameterDescription));
			e->GetParameterDescription(h, &parameterDescription);

			// Parameters can lack a semantic. We take this as an indication
			// that the parameter is not supposed to be set by the runtime
			// code.
			if (IsValid(parameterDescription.m_hName))
			{
				ParameterEntry entry;
				entry.m_hHandle = h;
				entry.m_eType = InternalGetParameterType(h);
				m_tParametersBySemantic.Insert(HString(
					e->GetString(parameterDescription.m_hName)), entry);
			}
		}
	}
}

/**
 * Fills a hash table owned by Effect with HString to
 * technique handles.
 *
 * This exists so that techniques can be looked up in constant time
 * given an HString name. HStrings are cheap keys, since they are
 * only a 16-bit ID once the HString has been instantiated.
 */
void OGLES2Effect::InternalPopulateTechniqueTable()
{
	SEOUL_ASSERT(IsRenderThread());

	m_tTechniquesByName.Clear();

	GLSLFXLite* e = StaticCast<GLSLFXLite*>(m_hHandle);
	if (e)
	{
		GLSLFXLiteEffectDescription effectDescription;
		memset(&effectDescription, 0, sizeof(effectDescription));
		e->GetEffectDescription(&effectDescription);

		for (UInt i = 0u; i < effectDescription.m_uTechniques; ++i)
		{
			UnsafeHandle h = e->GetTechniqueHandleFromIndex(i);

			GLSLFXLiteTechniqueDescription techniqueDescription;
			memset(&techniqueDescription, 0, sizeof(techniqueDescription));
			e->GetTechniqueDescription(h, &techniqueDescription);

			// Techniques can lack a name. We let this go in case
			// the Effect has in-development techniques that are
			// not supposed to be available at runtime yet.
			if (IsValid(techniqueDescription.m_hName))
			{
				TechniqueEntry entry;
				entry.m_hHandle = h;
				entry.m_uPassCount = techniqueDescription.m_uPasses;
				m_tTechniquesByName.Insert(HString(
					e->GetString(techniqueDescription.m_hName)), entry);
			}
		}
	}
}

} // namespace Seoul
