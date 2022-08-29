/**
 * \file SceneAnimation3DComponent.h
 * \brief Binds a 3D animation rig into a Scene object.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_ANIMATION3D_COMPONENT_H
#define SCENE_ANIMATION3D_COMPONENT_H

#include "FilePath.h"
#include "SceneComponent.h"
#include "SharedPtr.h"
namespace Seoul { namespace Animation { class EventInterface; } }
namespace Seoul { namespace Animation3D { class NetworkInstance; } }
namespace Seoul { class Effect; }
namespace Seoul { struct Matrix3x4; }
namespace Seoul { class RenderCommandStreamBuilder; }

#if SEOUL_WITH_ANIMATION_3D && SEOUL_WITH_SCENE

namespace Seoul::Scene
{

class Animation3DComponent SEOUL_SEALED : public Component
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(Animation3DComponent);

	Animation3DComponent();
	~Animation3DComponent();

	virtual SharedPtr<Component> Clone(const String& sQualifier) const SEOUL_OVERRIDE;

	Animation3D::NetworkInstance const* GetNetworkInstance() const
	{
		return m_pNetworkInstance.GetPtr();
	}

	FilePath GetNetworkFilePath() const;
	void SetNetworkFilePath(FilePath filePath);
	void Tick(Float fDeltaTimeInSeconds);

	const SharedPtr<Animation::EventInterface>& GetEventInterface() const;
	void SetEventInterface(const SharedPtr<Animation::EventInterface>& pEventInterface);

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(Animation3DComponent);
	SEOUL_REFLECTION_FRIENDSHIP(Animation3DComponent);

	FilePath m_NetworkFilePath;
	SharedPtr<Animation3D::NetworkInstance> m_pNetworkInstance;
	SharedPtr<Animation::EventInterface> m_pEventInterface;

	void Prep();

	SEOUL_DISABLE_COPY(Animation3DComponent);
}; // class Animation3DComponent

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_ANIMATION_3D && SEOUL_WITH_SCENE

#endif // include guard
