/**
 * \file EditorUIIControllerSceneRoot.h
 * \brief Shared interface for any models in the editor
 * that can resolve a scene root query (return a
 * root EditorScene).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_ICONTROLLER_SCENE_ROOT_H
#define EDITOR_UI_ICONTROLLER_SCENE_ROOT_H

#include "EditorSceneCameraMode.h"
#include "HashSet.h"
#include "SharedPtr.h"
#include "Prereqs.h"
#include "ReflectionDeclare.h"
namespace Seoul { class Camera; }
namespace Seoul { namespace EditorScene { class Container; } }
namespace Seoul { namespace EditorUI { struct Transform; } }
namespace Seoul { namespace Scene { class Object; } }

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

class IControllerSceneRoot SEOUL_ABSTRACT
{
public:
	SEOUL_REFLECTION_POLYMORPHIC_BASE(IControllerSceneRoot);

	typedef HashSet<SharedPtr<Scene::Object>, MemoryBudgets::Editor> SelectedObjects;

	virtual ~IControllerSceneRoot()
	{
	}

	// Get the root scene instance. Can fail if
	// none defined or still loading.
	virtual const EditorScene::Container& GetScene() const = 0;

	// Object management.
	virtual void AddObject(const SharedPtr<Scene::Object>& pObject) = 0;

	// Access of selected objects.

	/**
	 * The object that was the last primary target of a selection operation.
	 * This is, for example, the start object of a multiple selection
	 * operation. Note that, the "last selection" is not necessarily
	 * still selection (don't assume it is in the GetSelectedObjects()
	 * set). For example, on a CTRL+click selection operation that toggles off
	 * a selection, GetLastSelection() will be equal to that object, even
	 * though it is no longer selected.
	 */
	virtual const SharedPtr<Scene::Object>& GetLastSelection() const = 0;
	virtual const SelectedObjects& GetSelectedObjects() const = 0;
	virtual void SetObjectVisibility(
		const SelectedObjects& tObjects,
		Bool bTargetVisibility) = 0;
	virtual void SetSelectedObjects(
		const SharedPtr<Scene::Object>& pLastSelection,
		const SelectedObjects& tSelected) = 0;
	virtual void UniqueSetObjectSelected(
		const SharedPtr<Scene::Object>& pObject,
		Bool bSelected) = 0;

	// Selected object transformation.
	virtual void BeginSelectedObjectsTransform() = 0;
	virtual void SelectedObjectsApplyTransform(
		const Transform& mReferenceTransform,
		const Transform& mTargetTransform) = 0;
	virtual void EndSelectedObjectsTransform() = 0;

	// Selecte object component manipulation.
	virtual Bool CanModifyComponents() const = 0;
	virtual void SelectedObjectAddComponent(HString typeName) = 0;
	virtual void SelectedObjectRemoveComponent(HString typeName) = 0;

	// Utility, computes appropriate camera settings for the
	// given camera mode, and apply them to the fields.
	// Fields are in-out variables - in some cases
	// (e.g. if eMode is kPerspective), they are left
	// unmodified.
	virtual void ApplyFittingCameraProperties(
		EditorScene::CameraMode eMode,
		Float& rfNear,
		Float& rfFar,
		Vector3D& rvPosition) const = 0;

	// Utility, computes a position and (optional) zoom
	// to be applied to a camera in order to focus
	// it on the current selection set.
	virtual Bool ComputeCameraFocus(
		const Camera& camera,
		Vector3D& rvPosition,
		Float& rfZoom) const = 0;

protected:
	IControllerSceneRoot()
	{
	}

private:
	SEOUL_DISABLE_COPY(IControllerSceneRoot);
}; // class IControllerSceneRoot

// Tag for a drag and drop operation that is the currently selected objects of a scene.
struct DragSourceSelectedSceneObjects {};

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
