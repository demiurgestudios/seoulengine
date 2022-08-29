/**
 * \file Material.cpp
 * \brief A Material is a collection of parameters that control runtime uniform
 * constants in an Effect. For example, an Effect can be defined to apply
 * a diffuse texture, and a Material can be used per-geometry to specify the
 * specific texture that is applied.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Effect.h"
#include "Material.h"
#include "ReflectionDefine.h"
#include "RenderCommandStreamBuilder.h"
#include "SeoulFileReaders.h"
#include "TextureManager.h"

namespace Seoul
{

/**
 * Specialization of IMaterialParameter for TextureContentHandle
 */
class TextureMaterialParameter SEOUL_SEALED : public IMaterialParameter
{
public:
	TextureMaterialParameter(const TextureContentHandle& hTexture)
		: m_hValue(hTexture)
	{
	}

	~TextureMaterialParameter()
	{
	}

	virtual MaterialParameterType GetType() const SEOUL_OVERRIDE
	{
		return MaterialParameterType::kTexture;
	}

	virtual void Commit(
		RenderCommandStreamBuilder& rBuilder,
		const SharedPtr<Effect>& pEffect,
		HString parameterSemantic) const SEOUL_OVERRIDE
	{
		rBuilder.SetTextureParameter(pEffect, parameterSemantic, m_hValue);
	}

	virtual void Uncommit(
		RenderCommandStreamBuilder& rBuilder,
		const SharedPtr<Effect>& pEffect,
		HString parameterSemantic) const SEOUL_OVERRIDE
	{
		rBuilder.SetTextureParameter(pEffect, parameterSemantic, TextureContentHandle());
	}

	virtual IMaterialParameter* Clone() const SEOUL_OVERRIDE
	{
		return SEOUL_NEW(MemoryBudgets::Rendering) TextureMaterialParameter(m_hValue);
	}

	virtual UInt32 ComputeHash() const SEOUL_OVERRIDE
	{
		return m_hValue.GetKey().GetHash();
	}

protected:
	TextureContentHandle m_hValue;
}; // class TextureMaterialParameter

   /**
   * Specialization of IMaterialParameter for Vector4D
   */
class TextureDimensionsMaterialParameter SEOUL_SEALED : public IMaterialParameter
{
public:
	TextureDimensionsMaterialParameter(const TextureContentHandle& hTexture)
		: m_hValue(hTexture)
	{
	}

	~TextureDimensionsMaterialParameter()
	{
	}

	virtual MaterialParameterType GetType() const SEOUL_OVERRIDE
	{
		return MaterialParameterType::kTextureDimensions;
	}

	virtual void Commit(
		RenderCommandStreamBuilder& rBuilder,
		const SharedPtr<Effect>& pEffect,
		HString parameterSemantic) const SEOUL_OVERRIDE
	{
		auto pTexture(m_hValue.GetPtr());
		if (pTexture.IsValid())
		{
			rBuilder.SetVector4DParameter(
				pEffect,
				parameterSemantic,
				Vector4D((Float)pTexture->GetWidth(), (Float)pTexture->GetHeight(), 0.0f, 0.0f));
		}
	}

	virtual IMaterialParameter* Clone() const SEOUL_OVERRIDE
	{
		return SEOUL_NEW(MemoryBudgets::Rendering) TextureDimensionsMaterialParameter(m_hValue);
	}

	virtual UInt32 ComputeHash() const SEOUL_OVERRIDE
	{
		return m_hValue.GetKey().GetHash();
	}

protected:
	TextureContentHandle m_hValue;
}; // class TextureDimensionsMaterialParameter

   /**
   * Specialization of IMaterialParameter for Float
   */
class FloatMaterialParameter SEOUL_SEALED : public IMaterialParameter
{
public:
	FloatMaterialParameter(Float f)
		: m_f(f)
	{
	}

	~FloatMaterialParameter()
	{
	}

	virtual MaterialParameterType GetType() const SEOUL_OVERRIDE
	{
		return MaterialParameterType::kFloat;
	}

	virtual void Commit(
		RenderCommandStreamBuilder& rBuilder,
		const SharedPtr<Effect>& pEffect,
		HString parameterSemantic) const SEOUL_OVERRIDE
	{
		rBuilder.SetFloatParameter(pEffect, parameterSemantic, m_f);
	}

	virtual void Uncommit(
		RenderCommandStreamBuilder& rBuilder,
		const SharedPtr<Effect>& pEffect,
		HString parameterSemantic) const SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual IMaterialParameter* Clone() const SEOUL_OVERRIDE
	{
		return SEOUL_NEW(MemoryBudgets::Rendering) FloatMaterialParameter(m_f);
	}

	virtual UInt32 ComputeHash() const SEOUL_OVERRIDE
	{
		return Seoul::GetHash(m_f);
	}

	Float GetValue() const
	{
		return m_f;
	}

	void SetValue(Float f)
	{
		m_f = f;
	}

protected:
	Float m_f;
}; // class FloatMaterialParameter

   /**
   * Specialization of IMaterialParameter for Vector4D
   */
class Vector4DMaterialParameter SEOUL_SEALED : public IMaterialParameter
{
public:
	Vector4DMaterialParameter(const Vector4D& v)
		: m_v(v)
	{
	}

	~Vector4DMaterialParameter()
	{
	}

	virtual MaterialParameterType GetType() const SEOUL_OVERRIDE
	{
		return MaterialParameterType::kVector4D;
	}

	virtual void Commit(
		RenderCommandStreamBuilder& rBuilder,
		const SharedPtr<Effect>& pEffect,
		HString parameterSemantic) const SEOUL_OVERRIDE
	{
		rBuilder.SetVector4DParameter(pEffect, parameterSemantic, m_v);
	}

	virtual void Uncommit(
		RenderCommandStreamBuilder& rBuilder,
		const SharedPtr<Effect>& pEffect,
		HString parameterSemantic) const SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual IMaterialParameter* Clone() const SEOUL_OVERRIDE
	{
		return SEOUL_NEW(MemoryBudgets::Rendering) Vector4DMaterialParameter(m_v);
	}

	virtual UInt32 ComputeHash() const SEOUL_OVERRIDE
	{
		return Seoul::GetHash((Byte const*)&m_v, sizeof(m_v));
	}

	const Vector4D& GetValue() const
	{
		return m_v;
	}

	void SetValue(const Vector4D& v)
	{
		m_v = v;
	}

protected:
	Vector4D m_v;
}; // class Vector4DMaterialParameter

Material::Material()
	: m_vParameters()
	, m_Technique()
{
}

Material::~Material()
{
	InternalClearParameters();
}

UInt32 Material::ComputeHash() const
{
	UInt32 uHash = 0u;
	UInt32 const uParameters = m_vParameters.GetSize();
	for (UInt32 i = 0u; i < uParameters; ++i)
	{
		IncrementalHash(uHash, m_vParameters[i].m_pParameter->ComputeHash());
	}
	return uHash;
}

/**
 * Given an HString parameter semantic and a value,
 * adds or updates a material parameter entry for that Effect parameter
 * semantic and value.
 */
void Material::SetValue(HString name, Float fValue)
{
	// Delete the old parameter and insert a new one.
	InternalRemoveParameter(name);

	auto p = SEOUL_NEW(MemoryBudgets::Rendering) FloatMaterialParameter(fValue);
	InternalInsertParameter(name, p);
}

/**
 * Given an HString parameter semantic and a value,
 * adds or updates a material parameter entry for that Effect parameter
 * semantic and value.
 */
void Material::SetValue(HString name, const Vector4D& vValue)
{
	// Delete the old parameter and insert a new one.
	InternalRemoveParameter(name);

	auto p = SEOUL_NEW(MemoryBudgets::Rendering) Vector4DMaterialParameter(vValue);
	InternalInsertParameter(name, p);
}

/**
 * SetValue override for TextureContentHandle parameters.
 */
void Material::SetValue(HString name, const TextureContentHandle& hValue, HString dimensionsParameterName /*= HString()*/)
{
	// Delete the old parameter and insert a new one.
	InternalRemoveParameter(name);

	// Main texture parameter.
	{
		auto p = SEOUL_NEW(MemoryBudgets::Rendering) TextureMaterialParameter(hValue);
		InternalInsertParameter(name, p);
	}

	if (!dimensionsParameterName.IsEmpty())
	{
		auto p = SEOUL_NEW(MemoryBudgets::Rendering) TextureDimensionsMaterialParameter(hValue);
		InternalInsertParameter(dimensionsParameterName, p);
	}
}

/**
 * Populate this Material from a binary material definition.
 *
 * @return True on success, false otherwise.
 */
Bool Material::Load(SyncFile& file)
{
	UInt32 uMaterialParameters = 0u;
	HString parameterName;
	MaterialParameterType eType = MaterialParameterType::kFloat;

	// Verify the material delimiter
	if (!VerifyDelimiter(DataTypeMaterial, file))
	{
		goto error;
	}

	// Read the material technique.
	if (!ReadHString(file, m_Technique))
	{
		goto error;
	}

	// Read the number of parameters
	if (!ReadUInt32(file, uMaterialParameters))
	{
		goto error;
	}

	// Read the parameters
	for (UInt32 i = 0u; i < uMaterialParameters; ++i)
	{
		// Verify the parameter delimiter.
		if (!VerifyDelimiter(DataTypeMaterialParameter, file))
		{
			goto error;
		}

		// Read parameter name
		if (!ReadHString(file, parameterName))
		{
			goto error;
		}

		// Read parameter type
		if (!ReadEnum(file, eType))
		{
			goto error;
		}

		switch (eType)
		{
		case MaterialParameterType::kTexture:
			{
				FilePath textureFilePath;
				if (!ReadFilePath(file, GameDirectory::kContent, textureFilePath))
				{
					goto error;
				}

				// TODO: This is temporary until I can sort out
				// how textures for 3D world assets work vs. 2D assets.
				textureFilePath.SetType(FileType::kTexture0);

				// TODO: Probably, the cooker needs a texture metadata
				// file that defines this and other things. For now, all world Material
				// textures juse use wrapping on U and V.
				{
					TextureConfig config;
					config.m_bWrapAddressU = true;
					config.m_bWrapAddressV = true;
					config.m_bMipped = true;
					TextureManager::Get()->UpdateTextureConfig(textureFilePath, config);
				}

				auto hTexture = TextureManager::Get()->GetTexture(textureFilePath);

				// TODO: We'll probably want this in-game/at runtime/outside developer
				// tools in some cases.
#if SEOUL_EDITOR_AND_TOOLS
				SetValue(parameterName, hTexture, HString(String(parameterName) + String("Dimensions")));
#else
				SetValue(parameterName, hTexture);
#endif
			}
			break;
		case MaterialParameterType::kFloat:
			{
				Float f;
				if (!ReadSingle(file, f))
				{
					goto error;
				}
				SetValue(parameterName, f);
			}
			break;
		case MaterialParameterType::kVector4D:
			{
				Vector4D vVector4D;
				if (!ReadVector4D(file, vVector4D))
				{
					goto error;
				}
				SetValue(parameterName, vVector4D);
			}
			break;
		default:
			SEOUL_FAIL("Material::LoadFromFile: bad enum, this is a bug, bug a programmer.");
			break;
		};
	}

	return true;

error:
	InternalClear();
	return false;
}

/**
 * Commits the material to the effect specified by argument pEffect.
 *
 * Materials no longer store parameters (which are attached to a
 * specific effect). They only store HString ids for their parameter,
 * and then the paramter is acquired from the Effect that the Material
 * is being committed to. This is more flexible and should be reasonably
 * quick, since the lookup operation is a constant time lookup in a HashTable.
 * If it is not fast enough in the long run, we shoudl revisit this.
 */
void Material::Commit(RenderCommandStreamBuilder& rBuilder, const SharedPtr<Effect>& pEffect) const
{
	if (pEffect.IsValid())
	{
		UInt32 const uParameterCount = m_vParameters.GetSize();
		for (UInt32 i = 0u; i < uParameterCount; ++i)
		{
			const ParameterEntry& parameterEntry = m_vParameters[i];
			HString const name = parameterEntry.m_Name;
			IMaterialParameter const* const pParameter = parameterEntry.m_pParameter;
			SEOUL_ASSERT(nullptr != pParameter);

			pParameter->Commit(rBuilder, pEffect, name);
		}
	}
}

void Material::Uncommit(RenderCommandStreamBuilder& rBuilder, const SharedPtr<Effect>& pEffect) const
{
	if (pEffect.IsValid())
	{
		UInt32 const uParameterCount = m_vParameters.GetSize();
		for (UInt32 i = 0u; i < uParameterCount; ++i)
		{
			const ParameterEntry& parameterEntry = m_vParameters[i];
			HString const name = parameterEntry.m_Name;
			IMaterialParameter const* const pParameter = parameterEntry.m_pParameter;
			SEOUL_ASSERT(nullptr != pParameter);

			pParameter->Uncommit(rBuilder, pEffect, name);
		}
	}
}

/**
 * Comparison function, returns true if this Material is exactly
 * equal to Material b.
 */
Bool Material::operator==(const Material& b) const
{
	if (m_vParameters.GetSize() != b.m_vParameters.GetSize())
	{
		return false;
	}

	UInt32 const uParameterCount = m_vParameters.GetSize();
	for (UInt32 i = 0u; i < uParameterCount; ++i)
	{
		IMaterialParameter const* pA = m_vParameters[i].m_pParameter;
		IMaterialParameter const* pB = b.m_vParameters[i].m_pParameter;

		if (!InternalCompareParameter(pA, pB))
		{
			return false;
		}
	}

	return true;
}

/**
 * Creates a clone of this Material.
 *
 * The clone will be an exact copy of this Material. A comparison
 * between this Material and its clone will return true if neither
 * this Material nor the clone are modified.
 */
SharedPtr<Material> Material::Clone() const
{
	SharedPtr<Material> pClone(SEOUL_NEW(MemoryBudgets::Rendering) Material());

	UInt32 const uParameters = m_vParameters.GetSize();
	pClone->m_vParameters.Reserve(uParameters);
	for (UInt32 i = 0u; i < uParameters; ++i)
	{
		const ParameterEntry& entry = m_vParameters[i];
		pClone->m_vParameters.PushBack(
			ParameterEntry::Create(entry.m_Name, entry.m_pParameter->Clone()));
	}

	return pClone;
}

/**
 * Helper function, compares MaterialParameter a to
 * MaterialParameter b and returns true if they are exactly equal.
 *
 * This method does not apply any tolerance to floating point
 * comparisons - two Float, Vector*D, or Matrix*D objects will
 * only be equal if their components are exactly equal.
 */
Bool Material::InternalCompareParameter(
	IMaterialParameter const* a,
	IMaterialParameter const* b) const
{
	SEOUL_ASSERT(nullptr != a && nullptr != b);

	if (a->GetType() != b->GetType())
	{
		return false;
	}

	// TODO: Can fail, possibly good enough for this use case though.
	return (a->ComputeHash() == b->ComputeHash());
}

/**
 * Clears this material, restoring it to its default state.
 */
void Material::InternalClear()
{
	InternalClearParameters();
}

/**
 * Deletes heap allocated parameters and clears the parameter vector.
 */
void Material::InternalClearParameters()
{
	Int32 const iParameterCount = (Int32)m_vParameters.GetSize();
	for (Int32 i = (iParameterCount - 1); i >= 0; --i)
	{
		SafeDelete(m_vParameters[i].m_pParameter);
	}
	m_vParameters.Clear();
}

} // namespace Seoul
