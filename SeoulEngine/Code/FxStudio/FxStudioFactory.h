/**
 * \file FxStudioFactory.h
 * \brief Specialization of FxStudio::ComponentFactory used to spawn
 * SeoulEngine types that implement FxStudio::Component in a consistent
 * way.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FX_STUDIO_FACTORY_H
#define FX_STUDIO_FACTORY_H

#include "Prereqs.h"

#if SEOUL_WITH_FX_STUDIO
#include "FxStudioRT.h"

namespace Seoul::FxStudio
{

template <typename T>
class Factory SEOUL_SEALED : public ::FxStudio::ComponentFactory
{
public:
	Factory()
		: ::FxStudio::ComponentFactory(true)
	{
	}

	virtual ~Factory()
	{
	}

	virtual const char* GetComponentName() const SEOUL_OVERRIDE
	{
		return T::StaticTypeName();
	}

	virtual ::FxStudio::Component* CreateComponent(
		Int iComponentIndex,
		const ::FxStudio::Component::InternalDataType& internalData,
		void* pUserData) SEOUL_OVERRIDE
	{
		return SEOUL_NEW(MemoryBudgets::Fx) T(iComponentIndex, internalData, *((const FilePath*)pUserData));
	}

	virtual void DestroyComponent(::FxStudio::Component* pComponent) SEOUL_OVERRIDE
	{
		SafeDelete(pComponent);
	}

	virtual void GetAssets(
		const ::FxStudio::Component::InternalDataType& internalData,
		AssetCallback assetCallback,
		void* pUserData) SEOUL_OVERRIDE
	{
		::FxStudio::Component* pComponent = SEOUL_NEW(MemoryBudgets::Fx) ::FxStudio::Component(internalData);
		T::GetAssets(pComponent, assetCallback, pUserData);
		SafeDelete(pComponent);
	}

private:
	SEOUL_DISABLE_COPY(Factory);
}; // class FxStudio::Factory

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO

#endif // include guard
