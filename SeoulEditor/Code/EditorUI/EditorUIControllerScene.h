/**
 * \file EditorUIControllerScene.h
 * \brief A controller implementation that encapsulates
 * editing state when manipulating a scene model.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDTIOR_UI_CONTROLLER_SCENE_H
#define EDTIOR_UI_CONTROLLER_SCENE_H

#include "EditorUICommandTransformObjects.h"
#include "EditorUIControllerBase.h"
#include "EditorUIIControllerPropertyEditor.h"
#include "EditorUIIControllerSceneRoot.h"
#include "EditorUISettings.h"
#include "FilePath.h"
#include "HashSet.h"
namespace Seoul { namespace EditorUI { class ModelScene; } }

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

class ControllerScene SEOUL_SEALED : public ControllerBase
	, public IControllerPropertyEditor
	, public IControllerSceneRoot
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(ControllerScene);

	ControllerScene(
		const Settings& settings,
		FilePath rootScenePrefabFilePath);
	~ControllerScene();

	// DevUI::Controller overrides
	virtual void Tick(Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;
	virtual FilePath GetSaveFilePath() const SEOUL_OVERRIDE;
	virtual Bool HasSaveFilePath() const SEOUL_OVERRIDE;
	virtual Bool IsOutOfDate() const SEOUL_OVERRIDE;
	virtual void MarkUpToDate() SEOUL_OVERRIDE;
	virtual Bool NeedsSave() const SEOUL_OVERRIDE;
	virtual Bool Save() SEOUL_OVERRIDE;
	virtual void SetSaveFilePath(FilePath filePath) SEOUL_OVERRIDE;

	// TODO: Move this into a more specific interface?

	// Edit interface of a controller.
	virtual Bool CanCopy() const SEOUL_OVERRIDE;
	virtual Bool CanCut() const SEOUL_OVERRIDE;
	virtual Bool CanDelete() const SEOUL_OVERRIDE;
	virtual Bool CanPaste() const SEOUL_OVERRIDE;
	virtual void Copy() SEOUL_OVERRIDE;
	virtual void Cut() SEOUL_OVERRIDE;
	virtual void Delete() SEOUL_OVERRIDE;
	virtual void Paste() SEOUL_OVERRIDE;
	// /DevUI::Controller overrides

	// IControllerPropertyEditor overrides.
	virtual void CommitPropertyEdit(
		const PropertyUtil::Path& vPath,
		const PropertyValues& vValues,
		const PropertyValues& vNewValues) SEOUL_OVERRIDE;
	virtual Bool GetPropertyButtonContext(Reflection::Any& rContext) const SEOUL_OVERRIDE;
	virtual Bool GetPropertyTargets(Instances& rvInstances) const SEOUL_OVERRIDE;
	// /IControllerPropertyEditor overrides.

	// IControllerSceneRoot overrides.
	virtual void AddObject(const SharedPtr<Scene::Object>& pObject) SEOUL_OVERRIDE;
	virtual const EditorScene::Container& GetScene() const SEOUL_OVERRIDE;
	virtual const SharedPtr<Scene::Object>& GetLastSelection() const { return m_pLastSelection; }
	virtual const SelectedObjects& GetSelectedObjects() const SEOUL_OVERRIDE
	{
		return m_tSelectedObjects;
	}
	virtual void SetObjectVisibility(
		const SelectedObjects& tObjects,
		Bool bTargetVisibility) SEOUL_OVERRIDE;
	virtual void SetSelectedObjects(
		const SharedPtr<Scene::Object>& pLastSelection,
		const SelectedObjects& tSelected) SEOUL_OVERRIDE;
	virtual void UniqueSetObjectSelected(
		const SharedPtr<Scene::Object>& pObject,
		Bool bSelected) SEOUL_OVERRIDE;
	virtual void BeginSelectedObjectsTransform() SEOUL_OVERRIDE;
	virtual void SelectedObjectsApplyTransform(
		const Transform& mReferenceTransform,
		const Transform& mTargetTransform) SEOUL_OVERRIDE;
	virtual void EndSelectedObjectsTransform() SEOUL_OVERRIDE;
	virtual Bool CanModifyComponents() const SEOUL_OVERRIDE;
	virtual void SelectedObjectAddComponent(HString typeName) SEOUL_OVERRIDE;
	virtual void SelectedObjectRemoveComponent(HString typeName) SEOUL_OVERRIDE;
	virtual void ApplyFittingCameraProperties(
		EditorScene::CameraMode eMode,
		Float& rfNear,
		Float& rfFar,
		Vector3D& rvPosition) const SEOUL_OVERRIDE;
	virtual Bool ComputeCameraFocus(
		const Camera& camera,
		Vector3D& rvPosition,
		Float& rfZoom) const SEOUL_OVERRIDE;
	// /IControllerSceneRoot overrides.

	/** @return settings structure used to construct \a this ControllerScene. */
	const Settings& GetSettings() const
	{
		return m_Settings;
	}

	/** @return root Prefab used to construct \a this ControllerScene. */
	FilePath GetRootScenePrefabFilePath() const
	{
		return m_RootScenePrefabFilePath;
	}

	/**
	 * Update the root prefab for this controller. Used to set an initial filename
	 * and to perform a save as.
	 */
	void SetRootScenePrefabFilePath(FilePath filePath)
	{
		m_RootScenePrefabFilePath = filePath;
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(ControllerScene);

	Bool DoCopy() const;

	SharedPtr<Scene::Object> m_pLastSelection;
	SelectedObjects m_tSelectedObjects;
	CommandTransformObjects::Entries m_vSelectedObjectTransforms;
	ScopedPtr<ModelScene> const m_pModel;
	Settings const m_Settings;
	FilePath m_RootScenePrefabFilePath;

	SEOUL_DISABLE_COPY(ControllerScene);
}; // class ControllerScene

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
