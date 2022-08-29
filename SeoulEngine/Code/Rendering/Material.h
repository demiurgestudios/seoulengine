/**
 * \file Material.h
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

#pragma once
#ifndef MATERIAL_H
#define MATERIAL_H

#include "ContentHandle.h"
#include "Prereqs.h"
#include "SeoulHString.h"
#include "SharedPtr.h"
#include "Vector.h"
namespace Seoul { class BaseTexture; }
namespace Seoul { class Effect; }
namespace Seoul { class RenderCommandStreamBuilder; }
namespace Seoul { struct Vector4D; }
namespace Seoul { typedef Content::Handle<BaseTexture> TextureContentHandle; }

namespace Seoul
{

enum class MaterialParameterType : UInt32
{
	kFloat,
	kTexture,
	kTextureDimensions,
	kVector4D,
};

/**
 * Polymorphic interface of all Material parameters.
 */
class IMaterialParameter SEOUL_ABSTRACT
{
public:
	IMaterialParameter() {}
	virtual ~IMaterialParameter() {}
	virtual MaterialParameterType GetType() const = 0;
	virtual void Commit(RenderCommandStreamBuilder& rBuilder, const SharedPtr<Effect>& pEffect, HString parameterSemantic) const = 0;
	virtual UInt32 ComputeHash() const = 0;
	virtual IMaterialParameter* Clone() const = 0;
	virtual void Uncommit(RenderCommandStreamBuilder& rBuilder, const SharedPtr<Effect>& pEffect, HString parameterSemantic) const { /* Nop */ }

private:
	SEOUL_DISABLE_COPY(IMaterialParameter);
};

/**
 * Material is a collection of Effect parameters.
 * The parameters in Material can have various types, and Material
 * provides methods for deserializing from both binary and INI files,
 * as well as methods to clone and manipulate the parameters it contains.
 */
class Material SEOUL_SEALED
{
public:
	Material();
	~Material();

	UInt32 ComputeHash() const;

	SharedPtr<Material> Clone() const;

	Bool Load(SyncFile& rFile);

	HString GetTechnique() const
	{
		return m_Technique;
	}

	void Commit(RenderCommandStreamBuilder& rBuilder, const SharedPtr<Effect>& pEffect) const;
	void Uncommit(RenderCommandStreamBuilder& rBuilder, const SharedPtr<Effect>& pEffect) const;

	Bool operator==(const Material& b) const;
	Bool operator!=(const Material& b) const
	{
		return !(*this == b);
	}

	void SetValue(HString name, Float fValue);
	void SetValue(HString name, const Vector4D& vValue);
	void SetValue(HString name, const TextureContentHandle& hValue, HString dimensionsParameterName = HString());

	struct ParameterEntry
	{
		static ParameterEntry Create(
			HString name,
			IMaterialParameter* pParameter)
		{
			ParameterEntry ret;
			ret.m_Name = name;
			ret.m_pParameter = pParameter;
			return ret;
		}

		HString m_Name;
		IMaterialParameter* m_pParameter;
	};
	typedef Vector<ParameterEntry, MemoryBudgets::Rendering> Parameters;

	/**
	 * Constant iterator for iterating over this Material's
	 * parameter table.
	 */
	Parameters::ConstIterator BeginParameters() const
	{
		return m_vParameters.Begin();
	}

	/**
	 * Constant iterator for iterating over this Material's
	 * parameter table.
	 */
	Parameters::ConstIterator EndParameters() const
	{
		return m_vParameters.End();
	}

private:
	SEOUL_REFERENCE_COUNTED(Material);

	Parameters m_vParameters;
	HString m_Technique;

	Bool InternalCompareParameter(
		IMaterialParameter const* a,
		IMaterialParameter const* b) const;

	/**
	 * Helper method, finds the parameter with name name by
	 * name.
	 *
	 * @return True if a parameter with name name is in this
	 * Material, false otherwise.
	 *
	 * Although this method is O(n), Materials tend to have a
	 * very small number of parameters and the cost of walking a non-compact
	 * key-value table of parameters within Material::Commit() is far
	 * more expensive than the cost of this linear search in general.
	 */
	Bool InternalGetParameter(HString name, IMaterialParameter const*& rpParam) const
	{
		UInt32 nParameterCount = m_vParameters.GetSize();
		for (UInt32 i = 0u; i < nParameterCount; ++i)
		{
			const ParameterEntry& paramEntry = m_vParameters[i];
			if (name == paramEntry.m_Name)
			{
				rpParam = paramEntry.m_pParameter;
				return true;
			}
		}

		return false;
	}

	/**
	 * Helper method, finds the parameter with name name by
	 * name.
	 *
	 * @return True if a parameter with name name is in this
	 * Material, false otherwise.
	 *
	 * Although this method is O(n), Materials tend to have a
	 * very small number of parameters and the cost of walking a non-compact
	 * key-value table of parameters within Material::Commit() is far
	 * more expensive than the cost of this linear search in general.
	 */
	Bool InternalGetParameter(HString name, IMaterialParameter*& rpParam)
	{
		UInt32 nParameterCount = m_vParameters.GetSize();
		for (UInt32 i = 0u; i < nParameterCount; ++i)
		{
			const ParameterEntry& paramEntry = m_vParameters[i];
			if (name == paramEntry.m_Name)
			{
				rpParam = paramEntry.m_pParameter;
				return true;
			}
		}

		return false;
	}

	void InternalRemoveParameter(HString name)
	{
		UInt32 nParameterCount = m_vParameters.GetSize();
		for (UInt32 i = 0u; i < nParameterCount; ++i)
		{
			const ParameterEntry& paramEntry = m_vParameters[i];
			if (name == paramEntry.m_Name)
			{
				SafeDelete(m_vParameters[i].m_pParameter);
				m_vParameters.Erase(m_vParameters.Begin() + i);
				return;
			}
		}
	}

	/**
	 * Helper function, returns a material parameter entry
	 * for the given HString name or nullptr if this Material does not
	 * contain a parameter with the given name.
	 */
	IMaterialParameter const* InternalGetParameter(HString name) const
	{
		IMaterialParameter const* pParam;
		if (!InternalGetParameter(name, pParam))
		{
			pParam = nullptr;
		}

		return pParam;
	}

	/**
	 * Helper function, returns a material parameter entry
	 * for the given HString name or nullptr if this Material does not
	 * contain a parameter with the given name.
	 */
	IMaterialParameter* InternalGetParameter(HString name)
	{
		IMaterialParameter* pParam;
		if (!InternalGetParameter(name, pParam))
		{
			pParam = nullptr;
		}

		return pParam;
	}

	/**
	 * Helper function, inserts a material parameter with the
	 * given HString name identifier into this Material's parameter
	 * table.
	 */
	void InternalInsertParameter(HString name, IMaterialParameter* pParam)
	{
		m_vParameters.PushBack(ParameterEntry::Create(name, pParam));
	}

	void InternalClear();
	void InternalClearParameters();

	SEOUL_DISABLE_COPY(Material);
}; // class Material

} // namespace Seoul

#endif // include guard
