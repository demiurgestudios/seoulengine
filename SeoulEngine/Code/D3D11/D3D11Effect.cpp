/**
 * \file D3D11Effect.cpp
 * \brief PC DX11 implementation of Effect interface.
 *
 * Effect does not use polymorphism. There is one common interface
 * that is implemented for different graphics APIs.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "D3D11Effect.h"
#include "D3D11Device.h"
#include "D3D11Util.h"
#include "D3DCommonEffect.h"
#include "HashFunctions.h"
#include "ThreadId.h"

namespace Seoul
{

D3D11Effect::D3D11Effect(
	FilePath filePath,
	void* pRawEffectFileData,
	UInt32 uFileSizeInBytes)
	: Effect(filePath, pRawEffectFileData, uFileSizeInBytes)
{
}

D3D11Effect::~D3D11Effect()
{
	SEOUL_ASSERT(IsRenderThread());

	SafeRelease<ID3DX11Effect>(m_hHandle);
}

/**
 * When called, sets all texture parameters to nullptr. This
 * should be called before any textures are unloaded to prevent
 * dangling references on some platforms.
 */
void D3D11Effect::UnsetAllTextures()
{
	SEOUL_ASSERT(IsRenderThread());

	ID3DX11Effect* e = StaticCast<ID3DX11Effect*>(m_hHandle);
	if (e)
	{
		D3DX11_EFFECT_DESC effectDescription;
		memset(&effectDescription, 0, sizeof(effectDescription));
		e->GetDesc(&effectDescription);

		for (UInt i = 0u; i < effectDescription.GlobalVariables; ++i)
		{
			ID3DX11EffectVariable* p = e->GetVariableByIndex(i);

			D3DX11_EFFECT_TYPE_DESC type;
			memset(&type, 0, sizeof(type));
			SEOUL_D3D11_VERIFY(p->GetType()->GetDesc(&type));

			switch (type.Type)
			{
			case D3D10_SVT_TEXTURE: // fall-through
			case D3D10_SVT_TEXTURE1D: // fall-through
			case D3D10_SVT_TEXTURE2D: // fall-through
			case D3D10_SVT_TEXTURE3D: // fall-through
			case D3D10_SVT_TEXTURECUBE:
				SEOUL_D3D11_VERIFY(p->AsShaderResource()->SetResource(nullptr));
				break;
			default:
				break;
			};
		}
	}
}

/**
 * Called by the render device when the device is lost, to allow
 * the Effect to do any necessary bookkeeping.
 */
void D3D11Effect::OnLost()
{
	SEOUL_ASSERT(IsRenderThread());

	BaseGraphicsObject::OnLost();
}

/**
 * Called by the render device when the device is reset after being lost,
 * to allow the Effect to do any necessary bookkeeping.
 */
void D3D11Effect::OnReset()
{
	SEOUL_ASSERT(IsRenderThread());

	BaseGraphicsObject::OnReset();
}

/**
 * Get the EffectParameterType of the parameter described by hHandle.
 */
EffectParameterType D3D11Effect::InternalGetParameterType(UnsafeHandle hHandle) const
{
	SEOUL_ASSERT(IsRenderThread());

	ID3DX11Effect* e = StaticCast<ID3DX11Effect*>(m_hHandle);
	if (e)
	{
		ID3DX11EffectVariable* p = StaticCast<ID3DX11EffectVariable*>(hHandle);
		D3DX11_EFFECT_TYPE_DESC desc;
		memset(&desc, 0, sizeof(desc));
		SEOUL_D3D11_VERIFY(p->GetType()->GetDesc(&desc));

		// Use the general Array type if the
		// description has an element count. It will
		// be 0u if it's a single value and not an array.
		if (desc.Elements > 0u)
		{
			return EffectParameterType::kArray;
		}

		switch (desc.Class)
		{
		case D3D10_SVC_SCALAR:
			switch (desc.Type)
			{
			case D3D10_SVT_FLOAT:
				return EffectParameterType::kFloat;
			case D3D10_SVT_INT:
				return EffectParameterType::kInt;
			case D3D10_SVT_BOOL:
				return EffectParameterType::kBool;
			default:
				break;
			};
			break;
		case D3D10_SVC_VECTOR:
			if (D3D10_SVT_FLOAT == desc.Type)
			{
				if (1u == desc.Rows)
				{
					switch (desc.Columns)
					{
					case 2u: return EffectParameterType::kVector2D;
					case 3u: return EffectParameterType::kVector3D;
					case 4u: return EffectParameterType::kVector4D;
					};
				}
			}
			break;
		case D3D10_SVC_MATRIX_ROWS: // fall-through
		case D3D10_SVC_MATRIX_COLUMNS:
			if (D3D10_SVT_FLOAT == desc.Type)
			{
				return EffectParameterType::kMatrix4D;
			}
			break;
		case D3D10_SVC_OBJECT:
			switch (desc.Type)
			{
			case D3D10_SVT_TEXTURE: // fall-through
			case D3D10_SVT_TEXTURE1D: // fall-through
			case D3D10_SVT_TEXTURE2D: // fall-through
			case D3D10_SVT_TEXTURE3D: // fall-through
			case D3D10_SVT_TEXTURECUBE:
				return EffectParameterType::kTexture;
			default:
				break;
			};
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
Bool D3D11Effect::OnCreate()
{
	SEOUL_ASSERT(IsRenderThread());

	// Must succeed, should have been validated by the device.
	Byte const* p = nullptr;
	UInt32 u = 0u;
	SEOUL_VERIFY(GetEffectData(true, (Byte const*)m_pRawEffectFileData, m_zFileSizeInBytes, p, u));

	ID3DX11Effect* pD3D11Effect = nullptr;
	if (FAILED(D3DX11CreateEffectFromMemory(
		p,
		u,
		0u,
		GetD3D11Device().GetD3DDevice(),
		&pD3D11Effect)))
	{
		SafeRelease(pD3D11Effect);
	}

	if (nullptr != pD3D11Effect)
	{
		m_hHandle = pD3D11Effect;

		InternalPopulateParameterTable();
		InternalPopulateTechniqueTable();

		SEOUL_VERIFY(BaseGraphicsObject::OnCreate());

		return true;
	}

	return false;
}

/**
 * Fills a hash table owned by Effect with HString to
 * parameter handles.
 *
 * This exists so that parameters can be looked up in constant time
 * given an HString name. HStrings are cheap keys, since they are
 * only a 16-bit ID once the HString has been instantiated.
 */
void D3D11Effect::InternalPopulateParameterTable()
{
	SEOUL_ASSERT(IsRenderThread());

	m_tParametersBySemantic.Clear();

	ID3DX11Effect* e = StaticCast<ID3DX11Effect*>(m_hHandle);
	if (e)
	{
		D3DX11_EFFECT_DESC effectDescription;
		memset(&effectDescription, 0, sizeof(effectDescription));
		e->GetDesc(&effectDescription);

		for (UInt i = 0u; i < effectDescription.GlobalVariables; ++i)
		{
			ID3DX11EffectVariable* p = e->GetVariableByIndex(i);

			D3DX11_EFFECT_VARIABLE_DESC variableDescription;
			memset(&variableDescription, 0, sizeof(variableDescription));
			SEOUL_D3D11_VERIFY(p->GetDesc(&variableDescription));

			// Parameters can lack a semantic. We take this as an indication
			// that the parameter is not supposed to be set by the runtime
			// code.
			if (variableDescription.Semantic)
			{
				ParameterEntry entry;
				entry.m_hHandle = UnsafeHandle(p);
				entry.m_eType = InternalGetParameterType(entry.m_hHandle);
				m_tParametersBySemantic.Insert(HString(variableDescription.Semantic), entry);
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
void D3D11Effect::InternalPopulateTechniqueTable()
{
	SEOUL_ASSERT(IsRenderThread());

	m_tTechniquesByName.Clear();

	ID3DX11Effect* e = StaticCast<ID3DX11Effect*>(m_hHandle);
	if (e)
	{
		D3DX11_EFFECT_DESC effectDescription;
		memset(&effectDescription, 0, sizeof(effectDescription));
		e->GetDesc(&effectDescription);

		for (UInt i = 0u; i < effectDescription.Techniques; ++i)
		{
			ID3DX11EffectTechnique* t = e->GetTechniqueByIndex(i);
			if (t->IsValid())
			{
				D3DX11_TECHNIQUE_DESC techniqueDescription;
				memset(&techniqueDescription, 0, sizeof(techniqueDescription));
				SEOUL_D3D11_VERIFY(t->GetDesc(&techniqueDescription));

				// Techniques can lack a name. We let this go in case
				// the Effect has in-development techniques that are
				// not supposed to be available at runtime yet.
				if (techniqueDescription.Name)
				{
					TechniqueEntry entry;
					entry.m_hHandle = UnsafeHandle(t);
					entry.m_uPassCount = techniqueDescription.Passes;
					m_tTechniquesByName.Insert(HString(techniqueDescription.Name), entry);
				}
			}
		}
	}
}

} // namespace Seoul
