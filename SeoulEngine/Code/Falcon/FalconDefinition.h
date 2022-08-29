/**
 * \file FalconDefinition.h
 * \brief A Definition is the base class of all shared
 * data in a Falcon scene graph.
 *
 * Definitions are instantiated/exist in a Falcon scene
 * graph via instances. All definitions have a corresponding
 * instance (e.g. BitmapDefinition and BitmapInstance).
 *
 * The Definition represents the shared, immutable data while
 * the instance holds any per-node, mutable data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_DEFINITION_H
#define FALCON_DEFINITION_H

#include "FalconInstance.h"
#include "SharedPtr.h"
namespace Seoul { struct Matrix2x3; }

namespace Seoul::Falcon
{

// forward declarations
class FCNFile;
class Instance;

enum class DefinitionType
{
	kUnknown,
	kBinaryData,
	kBitmap,
	kEditText,
	kFont,
	kMovieClip,
	kShape,
};

class Definition SEOUL_ABSTRACT
{
public:
	virtual ~Definition()
	{
	}

	UInt16 GetDefinitionID() const
	{
		return m_uDefinitionID;
	}

	DefinitionType GetType() const
	{
		return m_eType;
	}

	template <typename T>
	void CreateInstance(SharedPtr<T>& rp) const
	{
		SharedPtr<Instance> p;
		DoCreateInstance(p);
		SEOUL_ASSERT(!p.IsValid() || InstanceTypeOf<T>::Value == p->GetType());

		SharedPtr<T> pt((T*)p.GetPtr());
		rp.Swap(pt);
	}

	void CreateInstance(SharedPtr<Instance>& rp) const
	{
		DoCreateInstance(rp);
	}

protected:
	Definition(DefinitionType eType, UInt16 uDefinitionID)
		: m_eType(eType)
		, m_uDefinitionID(uDefinitionID)
	{
	}

	virtual void DoCreateInstance(SharedPtr<Instance>& rp) const;

	SEOUL_REFERENCE_COUNTED(Definition);

private:
	DefinitionType const m_eType;
	UInt16 const m_uDefinitionID;

	SEOUL_DISABLE_COPY(Definition);
}; // class Definition
template <typename T> struct DefinitionTypeOf;

} // namespace Seoul::Falcon

#endif // include guard
