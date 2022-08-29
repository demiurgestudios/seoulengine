/**
 * \file EditorUIViewSceneObjects.h
 * \brief An editor view that displays
 * the list of objects in the view's root object
 * group.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_VIEW_SCENE_OBJECTS_H
#define EDITOR_UI_VIEW_SCENE_OBJECTS_H

#include "DevUIView.h"
#include "EditorUISceneComponentUtil.h"
#include "SharedPtr.h"
namespace Seoul { namespace EditorUI { class ControllerScene; } }
namespace Seoul { namespace Scene { class Object; } }

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

enum class ViewSceneObjectsFilterMode
{
	kId,
	kType,
	COUNT,
};

class ViewSceneObjects SEOUL_SEALED : public DevUI::View
{
public:
	ViewSceneObjects();
	~ViewSceneObjects();

	// DevUI::View overrides
	virtual HString GetId() const SEOUL_OVERRIDE
	{
		static const HString kId("Objects");
		return kId;
	}
	// /DevUI::View overrides

private:
	typedef Vector<SharedPtr<Scene::Object>, MemoryBudgets::SceneObject> Objects;

	// DevUI::View overrides
	virtual void DoPrePose(DevUI::Controller& rController, RenderPass& rPass) SEOUL_OVERRIDE;
	// /DevUI::View overrides

	SceneComponentUtil::ComponentTypes const m_vComponentTypes;
	Bool m_bMouseLeftLock;
	ViewSceneObjectsFilterMode m_eFilterMode;
	String m_sFilterId;
	Int32 m_iFilterType;
	Objects m_vFilteredObjects;
	SharedPtr<Scene::Object> m_pRenaming;
	Vector<Byte, MemoryBudgets::Editor> m_vRenameBuffer;
	Bool m_bRenameFirst;

	void InternalAddObject(ControllerScene& rController);
	void InternalAddPrefab(ControllerScene& rController);
	static Bool InternalGetComponentName(void* pData, Int iIndex, Byte const** psOut);
	void InternalPoseContextMenu(ControllerScene& rController);
	void InternalPoseFilter(ControllerScene& rController);
	void InternalPoseObjects(ControllerScene& rController);
	void InternalHandleRenamObject(ControllerScene& rController, void* pTexture);
	const Objects& InternalResolveObjects(ControllerScene& rController);
	void InternalStartRenamObject(const SharedPtr<Scene::Object>& pObject);
	Bool IsFiltering() const
	{
		switch (m_eFilterMode)
		{
		case ViewSceneObjectsFilterMode::kId:
			return (!m_sFilterId.IsEmpty());
		case ViewSceneObjectsFilterMode::kType:
			return (m_iFilterType >= 0);
		default:
			return false;
		};
	}
	Bool IsRenaming(const SharedPtr<Scene::Object>& p) const { return (m_pRenaming == p); }

	SEOUL_DISABLE_COPY(ViewSceneObjects);
}; // class ViewSceneObjects

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
