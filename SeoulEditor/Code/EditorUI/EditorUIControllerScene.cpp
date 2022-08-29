/**
 * \file EditorUIControllerScene.cpp
 * \brief A controller implementation that encapsulates
 * editing state when manipulating a scene model.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Camera.h"
#include "EditorSceneContainer.h"
#include "EditorSceneState.h"
#include "EditorUICommandAddObject.h"
#include "EditorUICommandDeleteObjects.h"
#include "EditorUICommandPasteObjects.h"
#include "EditorUICommandPropertyEdit.h"
#include "EditorUICommandSelectObjects.h"
#include "EditorUICommandSetComponent.h"
#include "EditorUICommandSetEditorVisibility.h"
#include "EditorUICommandUniqueDeselectObject.h"
#include "EditorUICommandUniqueSelectObject.h"
#include "EditorUIControllerScene.h"
#include "EditorUIModelScene.h"
#include "EditorUIPropertyUtil.h"
#include "Engine.h"
#include "Mesh.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "SceneMeshDrawComponent.h"
#include "SceneObject.h"
#include "ScenePrefabComponent.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_TYPE(EditorUI::IControllerSceneRoot, TypeFlags::kDisableNew)

SEOUL_BEGIN_TYPE(EditorUI::ControllerScene, TypeFlags::kDisableNew)
	SEOUL_PARENT(EditorUI::ControllerBase)
	SEOUL_PARENT(EditorUI::IControllerPropertyEditor)
	SEOUL_PARENT(EditorUI::IControllerSceneRoot)
SEOUL_END_TYPE()

SEOUL_TYPE(EditorUI::DragSourceSelectedSceneObjects);

namespace EditorUI
{

static const HString kPropertyCategory("Category"); // TODO: Move into a utility header.
static const HString kPropertyId("Id"); // TODO: Move into a utility header.

ControllerScene::ControllerScene(
	const Settings& settings,
	FilePath rootScenePrefabFilePath)
	: m_pLastSelection()
	, m_tSelectedObjects()
	, m_pModel(SEOUL_NEW(MemoryBudgets::Editor) ModelScene(settings, rootScenePrefabFilePath))
	, m_Settings(settings)
	, m_RootScenePrefabFilePath(rootScenePrefabFilePath)
{
}

ControllerScene::~ControllerScene()
{
}

void ControllerScene::Tick(Float fDeltaTimeInSeconds)
{
	m_pModel->Tick(fDeltaTimeInSeconds);
}

FilePath ControllerScene::GetSaveFilePath() const
{
	return m_RootScenePrefabFilePath;
}

Bool ControllerScene::HasSaveFilePath() const
{
	return m_RootScenePrefabFilePath.IsValid();
}

Bool ControllerScene::IsOutOfDate() const
{
	return m_pModel->IsOutOfDate();
}

void ControllerScene::MarkUpToDate()
{
	m_pModel->MarkUpToDate();
}

Bool ControllerScene::NeedsSave() const
{
	// Need to save if the command history has entries
	// that are not marked or inherit markable between
	// the mark command the and head of the command list.
	return !CanReachMarkedCommand();
}

Bool ControllerScene::Save()
{
	// Don't save if no path.
	if (!m_RootScenePrefabFilePath.IsValid())
	{
		return false;
	}

	// Attempt to save.
	if (!m_pModel->GetScene().Save(m_RootScenePrefabFilePath))
	{
		return false;
	}

	// On successful save, mark the current command history
	// head as the marked node.
	MarkHeadCommand();
	return true;
}

void ControllerScene::SetSaveFilePath(FilePath filePath)
{
	m_RootScenePrefabFilePath = filePath;
}

Bool ControllerScene::CanCopy() const
{
	return !m_tSelectedObjects.IsEmpty();
}

Bool ControllerScene::CanCut() const
{
	return !m_tSelectedObjects.IsEmpty();
}

Bool ControllerScene::CanDelete() const
{
	return !m_tSelectedObjects.IsEmpty();
}

Bool ControllerScene::CanPaste() const
{
	// TODO: Ideally, we'd check the contents
	// of the system clipboard for a string with the
	// correct format (e.g. "{..."Objects"...}"), but
	// that's really expensive to be doing every frame.
	return true;
}

Bool ControllerScene::DoCopy() const
{
	String s;
	if (!Reflection::SerializeToString(
		&m_tSelectedObjects,
		s,
		true,
		0,
		true))
	{
		return false;
	}

	return Engine::Get()->WriteToClipboard(s);
}

void ControllerScene::Copy()
{
	(void)DoCopy();
}

void ControllerScene::Cut()
{
	// TODO: An alternative (and probably better) implementation of this is
	// to perform the copy, and then mark the copy as a cut, so that
	// on the next paste, the delete occurs, instead of immediately.

	// Early out if the copy fails, without inserting the delete.
	if (!DoCopy())
	{
		return;
	}

	// Insert a delete action.
	ExecuteCommand(SEOUL_NEW(MemoryBudgets::Editor) CommandDeleteObjects(
		m_pModel->GetScene(),
		m_pLastSelection,
		m_tSelectedObjects,
		m_tSelectedObjects,
		true));
}

void ControllerScene::Delete()
{
	ExecuteCommand(SEOUL_NEW(MemoryBudgets::Editor) CommandDeleteObjects(
		m_pModel->GetScene(),
		m_pLastSelection,
		m_tSelectedObjects,
		m_tSelectedObjects,
		false));
}

// TODO: Fixup reflection access, property editor,
// etc. so this boilerplate is not necessary.
struct ObjectPlaceholder
{
	String m_sId;
	HString m_EditorCategory;
	Scene::Object::Components m_vComponents;
};
typedef Vector<ObjectPlaceholder, MemoryBudgets::Editor> PlaceholderObjects;

void ControllerScene::Paste()
{
	String sData;
	if (!Engine::Get()->ReadFromClipboard(sData))
	{
		return;
	}

	// TODO: Fixup reflection access, property editor,
	// etc. so this boilerplate is not necessary.
	PlaceholderObjects vPlaceholderObjects;
	if (!Reflection::DeserializeFromString(
		sData,
		&vPlaceholderObjects))
	{
		return;
	}

	// TODO: Fixup reflection access, property editor,
	// etc. so this boilerplate is not necessary.
	SelectedObjects tNewObjects;
	for (auto const& source : vPlaceholderObjects)
	{
		SharedPtr<Scene::Object> pObject(SEOUL_NEW(MemoryBudgets::SceneObject) Scene::Object(
			source.m_sId));
		pObject->SetEditorCategory(source.m_EditorCategory);
		for (auto const& pComponent : source.m_vComponents)
		{
			pObject->AddComponent(pComponent);
		}
		pObject->EditorOnlySortComponents();
		SEOUL_VERIFY(tNewObjects.Insert(pObject).Second);
	}

	ExecuteCommand(SEOUL_NEW(MemoryBudgets::Editor) CommandPasteObjects(
		m_pModel->GetScene(),
		m_pLastSelection,
		m_tSelectedObjects,
		tNewObjects));
}

class SceneObjectPropertyEditBinding SEOUL_SEALED : public IPropertyChangeBinder
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(Scene::ObjectPropertyEditBinding);

	SceneObjectPropertyEditBinding(
		EditorScene::Container& rScene,
		const ControllerScene::SelectedObjects& tSelectedObjects)
		: m_rScene(rScene)
		, m_tSelectedObjects(tSelectedObjects)
	{
	}

	virtual Bool Equals(IPropertyChangeBinder const* pB) const SEOUL_OVERRIDE
	{
		SceneObjectPropertyEditBinding const* p = DynamicCast<SceneObjectPropertyEditBinding const*>(pB);
		if (nullptr == p)
		{
			return false;
		}

		if (m_tSelectedObjects.GetSize() != p->m_tSelectedObjects.GetSize())
		{
			return false;
		}

		{
			typedef ControllerScene::SelectedObjects SelectedObjects;
			SelectedObjects::ConstIterator iB = p->m_tSelectedObjects.Begin();
			SelectedObjects::ConstIterator const iBegin = m_tSelectedObjects.Begin();
			SelectedObjects::ConstIterator const iEnd = m_tSelectedObjects.End();
			for (SelectedObjects::ConstIterator iA = iBegin; iEnd != iA; ++iA, ++iB)
			{
				if (!p->m_tSelectedObjects.HasKey(*iA))
				{
					return false;
				}

				if (!m_tSelectedObjects.HasKey(*iB))
				{
					return false;
				}
			}
		}

		return true;
	}

	virtual String GetDescription() const SEOUL_OVERRIDE
	{
		if (m_tSelectedObjects.GetSize() == 1)
		{
			return (*m_tSelectedObjects.Begin())->GetId();
		}
		else
		{
			return String("Multiple Objects");
		}
	}

	virtual UInt32 GetSizeInBytes() const SEOUL_OVERRIDE
	{
		return m_tSelectedObjects.GetMemoryUsageInBytes() + sizeof(*this);
	}

	virtual void SetValue(
		const PropertyUtil::Path& vPath,
		const Reflection::Any& anyValue) SEOUL_OVERRIDE
	{
		for (auto const& pObject : m_tSelectedObjects)
		{
			(void)SetValue(pObject.GetPtr(), vPath.Begin(), vPath.End(), anyValue);
		}

		// TODO: Better pattern for this?

		// Special case handling - if the Id or Category
		// property were changed, need to resort objects in the scene.
		if (!vPath.IsEmpty() && (
			vPath.Front().m_Id == kPropertyCategory ||
			vPath.Front().m_Id == kPropertyId))
		{
			m_rScene.SortObjects();
		}
	}

	virtual void SetValues(
		const PropertyUtil::Path& vPath,
		const Values& vValues) SEOUL_OVERRIDE
	{
		if (vValues.GetSize() != m_tSelectedObjects.GetSize())
		{
			return;
		}

		UInt32 u = 0u;
		for (auto const& pObject : m_tSelectedObjects)
		{
			(void)SetValue(pObject.GetPtr(), vPath.Begin(), vPath.End(), vValues[u++]);
		}

		// TODO: Better pattern for this?

		// Special case handling - if the Id or Category
		// property were changed, need to resort objects in the scene.
		if (!vPath.IsEmpty() && (
			vPath.Front().m_Id == kPropertyCategory ||
			vPath.Front().m_Id == kPropertyId))
		{
			m_rScene.SortObjects();
		}
	}

private:
	Bool SetValue(
		Reflection::WeakAny target,
		PropertyUtil::Path::ConstIterator i,
		PropertyUtil::Path::ConstIterator const iEnd,
		const Reflection::Any& anyValue)
	{
		using namespace Reflection;

		for (; iEnd != i; ++i)
		{
			// Special handling for PointerLike complex objects.
			{
				Attributes::PointerLike const* pPointerLike = target.GetType().GetAttribute<Attributes::PointerLike>();
				if (nullptr != pPointerLike)
				{
					WeakAny proxyObjectThis = pPointerLike->m_GetPtrDelegate(target);
					if (proxyObjectThis.IsValid())
					{
						target = proxyObjectThis;
					}
				}
			}

			PropertyUtil::NumberOrHString const e = *i;
			if (e.m_Id.IsEmpty())
			{
				Array const* pArray = target.GetType().TryGetArray();
				if (nullptr == pArray)
				{
					return false;
				}

				if (i + 1 == iEnd)
				{
					return pArray->TrySet(target, e.m_uId, anyValue);
				}
				else
				{
					// Simple case, just update the inner pointer.
					if (!pArray->TryGetElementPtr(target, e.m_uId, target))
					{
						// Complex case - get the value at this level,
						// go recursive, then update the value.
						Any innerValue;
						if (!pArray->TryGet(target, e.m_uId, innerValue))
						{
							return false;
						}

						// Now handle the rest recursively.
						if (!SetValue(
							innerValue.GetPointerToObject(),
							i + 1,
							iEnd,
							anyValue))
						{
							return false;
						}

						// Update the inner value.
						if (!pArray->TrySet(target, e.m_uId, innerValue))
						{
							return false;
						}

						// Done completely - recursion handled
						// the rest of processing.
						return true;
					}
				}
			}
			else
			{
				Property const* pProperty = target.GetType().GetProperty(e.m_Id);
				if (nullptr == pProperty)
				{
					return false;
				}

				if (i + 1 == iEnd)
				{
					return pProperty->TrySet(target, anyValue);
				}
				else
				{
					// Simple case, just update the inner pointer.
					if (!pProperty->TryGetPtr(target, target))
					{
						// Complex case - get the value at this level,
						// go recursive, then update the value.
						Any innerValue;
						if (!pProperty->TryGet(target, innerValue))
						{
							return false;
						}

						// Now handle the rest recursively.
						if (!SetValue(
							innerValue.GetPointerToObject(),
							i + 1,
							iEnd,
							anyValue))
						{
							return false;
						}

						// Update the inner value.
						if (!pProperty->TrySet(target, innerValue))
						{
							return false;
						}

						// Done completely - recursion handled
						// the rest of processing.
						return true;
					}
				}
			}
		}

		return false;
	}

	EditorScene::Container& m_rScene;
	ControllerScene::SelectedObjects m_tSelectedObjects;

	SEOUL_DISABLE_COPY(SceneObjectPropertyEditBinding);
}; // class Scene::ObjectPropertyEditBinding

void ControllerScene::CommitPropertyEdit(
	const PropertyUtil::Path& vPath,
	const PropertyValues& vValues,
	const PropertyValues& vNewValues)
{
	if (m_tSelectedObjects.IsEmpty())
	{
		return;
	}

	ExecuteCommand(SEOUL_NEW(MemoryBudgets::Editor) CommandPropertyEdit(
		SEOUL_NEW(MemoryBudgets::Editor) SceneObjectPropertyEditBinding(m_pModel->GetScene(), m_tSelectedObjects),
		vPath,
		vValues,
		vNewValues));
}

Bool ControllerScene::GetPropertyButtonContext(Reflection::Any& rContext) const
{
	// TODO: Eliminate need for this cast. Limitation of the Reflection system.
	Scene::Interface* pInterface = m_pModel->GetScene().GetState().Get();
	if (nullptr == pInterface)
	{
		return false;
	}

	rContext = pInterface;
	return true;
}

Bool ControllerScene::GetPropertyTargets(
	Instances& rvInstances) const
{
	if (m_tSelectedObjects.IsEmpty())
	{
		return false;
	}

	rvInstances.Reserve(rvInstances.GetSize() + m_tSelectedObjects.GetSize());
	for (auto const& pObject : m_tSelectedObjects)
	{
		rvInstances.PushBack(&pObject);
	}
	return true;
}

void ControllerScene::AddObject(const SharedPtr<Scene::Object>& pObject)
{
	ExecuteCommand(SEOUL_NEW(MemoryBudgets::Editor) CommandAddObject(
		m_pModel->GetScene(),
		m_pLastSelection,
		m_tSelectedObjects,
		pObject));
}

const EditorScene::Container& ControllerScene::GetScene() const
{
	return m_pModel->GetScene();
}

void ControllerScene::SetObjectVisibility(
	const SelectedObjects& tObjects,
	Bool bTargetVisibility)
{
	ExecuteCommand(SEOUL_NEW(MemoryBudgets::Editor) CommandSetEditorVisibility(
		tObjects,
		bTargetVisibility));
}

void ControllerScene::SetSelectedObjects(
	const SharedPtr<Scene::Object>& pLastSelection,
	const SelectedObjects& tSelected)
{
	ExecuteCommand(SEOUL_NEW(MemoryBudgets::Editor) CommandSelectObjects(
		m_pLastSelection,
		m_tSelectedObjects,
		m_tSelectedObjects,
		pLastSelection,
		tSelected));
}

void ControllerScene::UniqueSetObjectSelected(
	const SharedPtr<Scene::Object>& pObject,
	Bool bSelected)
{
	if (!bSelected)
	{
		if (!m_tSelectedObjects.IsEmpty())
		{
			ExecuteCommand(
				SEOUL_NEW(MemoryBudgets::Editor) CommandUniqueDeselectObject(
					m_pLastSelection,
					m_tSelectedObjects));
		}
	}
	else
	{
		if (!m_tSelectedObjects.HasKey(pObject) ||
			m_tSelectedObjects.GetSize() > 1u)
		{
			ExecuteCommand(
				SEOUL_NEW(MemoryBudgets::Editor) CommandUniqueSelectObject(
					m_pLastSelection,
					m_tSelectedObjects,
					pObject));
		}
	}
}

void ControllerScene::BeginSelectedObjectsTransform()
{
	m_vSelectedObjectTransforms.Clear();
	m_vSelectedObjectTransforms.Reserve(m_tSelectedObjects.GetSize());

	SelectedObjects::Iterator const iBegin = m_tSelectedObjects.Begin();
	SelectedObjects::Iterator const iEnd = m_tSelectedObjects.End();
	for (SelectedObjects::Iterator i = iBegin; iEnd != i; ++i)
	{
		auto pMesh((*i)->GetComponent<Scene::MeshDrawComponent>());

		CommandTransformObjects::Entry entry;
		entry.m_pObject = (*i);
		entry.m_mTransform.m_vScale = (pMesh.IsValid() ? pMesh->GetScale() : Vector3D::One());
		entry.m_mTransform.m_qRotation = entry.m_pObject->GetRotation();
		entry.m_mTransform.m_vTranslation = entry.m_pObject->GetPosition();
		m_vSelectedObjectTransforms.PushBack(entry);
	}
}

void ControllerScene::SelectedObjectsApplyTransform(
	const Transform& mReferenceTransform,
	const Transform& mTargetTransform)
{
	if (!m_vSelectedObjectTransforms.IsEmpty())
	{
		ExecuteCommand(SEOUL_NEW(MemoryBudgets::Editor) CommandTransformObjects(
			m_vSelectedObjectTransforms,
			mReferenceTransform,
			mTargetTransform));
	}
}

void ControllerScene::EndSelectedObjectsTransform()
{
	if (!m_vSelectedObjectTransforms.IsEmpty())
	{
		m_vSelectedObjectTransforms.Clear();
		if (GetHeadCommand() != nullptr && GetHeadCommand()->GetReflectionThis().IsOfType<CommandTransformObjects const*>())
		{
			LockHeadCommand();
		}
	}
}

Bool ControllerScene::CanModifyComponents() const
{
	// TODO: Support component manipulation during multiselect?
	if (m_tSelectedObjects.GetSize() != 1u)
	{
		return false;
	}

	// Objects with an PrefabComponent are treated specially,
	// since they are "flattened" at runtime (the object ceases to exist,
	// the nested objects are instantiated with qualified names into
	// the root scene).
	//
	// As a result, we don't want an PrefabComponent object to
	// ever change its Components (it is always a FreeTransformComponent
	// + an PrefabComponent).
	if ((*m_tSelectedObjects.Begin())->GetComponent<Scene::PrefabComponent>().IsValid())
	{
		return false;
	}

	return true;
}

/** Utility used for computing camera framing attributes. */
template <typename OBJECTS>
static void Traverse(const Matrix4D& mParent, const OBJECTS& v, AABB& r)
{
	for (auto const& p : v)
	{
		// Nested prefabs.
		{
			auto pComp = p->GetComponent<Scene::PrefabComponent>();
			if (pComp.IsValid())
			{
				Traverse(p->ComputeNormalTransform(), pComp->GetObjects(), r);
			}
		}

		// Mesh.
		{
			// No mesh component, skip.
			auto pComp = p->GetComponent<Scene::MeshDrawComponent>();
			if (!pComp.IsValid())
			{
				continue;
			}

			// Skip meshes with any special modes (currently, all
			// special modes are infinite projections).
			if (0u != pComp->GetMeshDrawFlags())
			{
				continue;
			}

			// Skip components with invalid or still loading mesh
			// data.
			auto pMesh(GetMeshPtr(pComp->GetMesh()));
			if (!pMesh.IsValid())
			{
				continue;
			}

			// Compute the merged AABB data.
			r = AABB::CalculateMerged(
				r,
				AABB::Transform(
					mParent * p->ComputeNormalTransform() * Matrix4D::CreateScale(pComp->GetScale()),
					pMesh->GetBoundingBox()));
		}
	}
}

/**
 * Utility which computes near, far, and position data
 * ideal for a fitted orthographic camera.
 *
 * \note rfNear, rfFar, and rvPosition are in-out parameters
 * that may be fully or partially unmodified based
 * on \a eMode.
 */
void ControllerScene::ApplyFittingCameraProperties(
	EditorScene::CameraMode eMode,
	Float& rfNear,
	Float& rfFar,
	Vector3D& rvPosition) const
{
	// Fixed distances.
	static const Float32 kfOrthographicNear = 1.0f;
	static const Float32 kfPositionOversize = 2.0f;
	static const Float32 kfFarOversize = 4.0f;

	// Start with AABB of zero, not inverse max,
	// as we always want the world origin to be in
	// the space.
	AABB aabb;
	Traverse(Matrix4D::Identity(), m_pModel->GetScene().GetObjects(), aabb);

	// Set the near to a fixed value for orthographic modes.
	if (EditorScene::CameraMode::kPerspective != eMode)
	{
		rfNear = kfOrthographicNear;
	}

	// Compute the full axis dimensions of the total AABB.
	// and then select from it.
	auto const vDimensions = aabb.GetDimensions();
	switch (eMode)
	{
	case EditorScene::CameraMode::kPerspective:
		break;
	case EditorScene::CameraMode::kTop:
		rfFar = vDimensions.Y + kfFarOversize;
		rvPosition.Y = aabb.m_vMax.Y + kfPositionOversize;
		break;
	case EditorScene::CameraMode::kBottom:
		rfFar = vDimensions.Y + kfFarOversize;
		rvPosition.Y = aabb.m_vMin.Y - kfPositionOversize;
		break;
	case EditorScene::CameraMode::kLeft:
		rfFar = vDimensions.X + kfFarOversize;
		rvPosition.X = aabb.m_vMin.X - kfPositionOversize;
		break;
	case EditorScene::CameraMode::kRight:
		rfFar = vDimensions.X + kfFarOversize;
		rvPosition.X = aabb.m_vMax.X + kfPositionOversize;
		break;
	case EditorScene::CameraMode::kFront:
		rfFar = vDimensions.Z + kfFarOversize;
		rvPosition.Z = aabb.m_vMax.Z + kfPositionOversize;
		break;
	case EditorScene::CameraMode::kBack:
		rfFar = vDimensions.Z + kfFarOversize;
		rvPosition.Z = aabb.m_vMin.Z - kfPositionOversize;
		break;
	default:
		break;
	};
}

/**
 * Compute position and (optionally, only for orthographic camera)
 * zoom for framing the currently selected set of objects.
 */
Bool ControllerScene::ComputeCameraFocus(
	const Camera& camera,
	Vector3D& rvPosition,
	Float& rfZoom) const
{
	static const Float kfMin = 0.1f; // TODO: Configure.
	static const Float kfMax = 1000.0f; // TODO: Configure.
	static const Float kfZoomFactor = 0.75f; // TODO: Derive?

	AABB aabb = AABB::InverseMaxAABB();
	Traverse(Matrix4D::Identity(), m_tSelectedObjects, aabb);

	// No objects, no framing.
	if (aabb == AABB::InverseMaxAABB())
	{
		return false;
	}

	// Basic values.
	auto const vCenter = aabb.GetCenter();
	auto const fRadius = Clamp(aabb.GetExtents().Length(), kfMin, kfMax);

	// Adjust distance based on aspect ratio.
	auto const& mProjection = camera.GetProjectionMatrix();
	auto const fAspectRatio = Matrix4D::ExtractAspectRatio(mProjection);
	auto fDistance = Max(fAspectRatio, 1.0f) * fRadius;

	// Final computations depend on projection type.
	if (mProjection.IsPerspective())
	{
		// Use the ratio of the frustum to project the distance
		// into a perspective correct distance to shift
		// the center by.
		fDistance = fDistance / Tan(Matrix4D::ExtractFovInRadians(mProjection) * 0.5f);

		// Done - position is center offset by the distance toward the camera.
		rvPosition = vCenter - camera.GetViewAxis() * fDistance;

		// Zoom left unmodified in perspective.
	}
	else
	{
		// In orthographic, position is just the object center, but
		// we need to also compute a zoom.
		rvPosition = vCenter;
		rfZoom = fDistance * kfZoomFactor;
	}

	return true;
}

static Reflection::Type const* InnerGetLeastSpecificSceneComponentSubclass(const Reflection::Type& type)
{
	UInt32 const uParents = type.GetParentCount();
	for (UInt32 i = 0u; i < uParents; ++i)
	{
		Reflection::Type const* pParent = type.GetParent(i);
		if (nullptr == pParent)
		{
			continue;
		}

		if (*pParent == TypeOf<Scene::Component>())
		{
			return &type;
		}

		Reflection::Type const* pInner = InnerGetLeastSpecificSceneComponentSubclass(*pParent);
		if (nullptr != pInner)
		{
			return pInner;
		}
	}

	return nullptr;
}

static const Reflection::Type& GetLeastSpecificSceneComponentSubclass(const Reflection::Type& type)
{
	Reflection::Type const* p = InnerGetLeastSpecificSceneComponentSubclass(type);
	if (nullptr == p)
	{
		return type;
	}

	return *p;
}

static Bool ToAdd(
	const Scene::Object& object,
	HString typeName,
	SharedPtr<Scene::Component>& rpOld,
	SharedPtr<Scene::Component>& rpNew)
{
	using namespace Reflection;

	Type const* pType = Registry::GetRegistry().GetType(typeName);
	if (nullptr == pType)
	{
		return false;
	}

	SharedPtr<Scene::Component> pComponent(pType->New<Scene::Component>(MemoryBudgets::SceneComponent));
	if (!pComponent.IsValid())
	{
		return false;
	}

	{
		// Enforce mutual exclusion of trees. Find the least specific parent
		// class of the passed in type.
		const Type& leastSpecificType = GetLeastSpecificSceneComponentSubclass(*pType);

		// Remove an existing component of the least specific type
		// before adding the new type.
		auto pExistingComponent(object.GetComponent(leastSpecificType, false));

		// Setup and return success.
		rpOld = pExistingComponent;
		rpNew = pComponent;

		return true;
	}
}

static Bool ToRemove(
	const Scene::Object& object,
	HString typeName,
	SharedPtr<Scene::Component>& rpOld,
	SharedPtr<Scene::Component>& rpNew)
{
	using namespace Reflection;

	Type const* pType = Registry::GetRegistry().GetType(typeName);
	if (nullptr == pType)
	{
		return false;
	}

	{
		// Enforce mutual exclusion of trees. Find the least specific parent
		// class of the passed in type.
		const Type& leastSpecificType = GetLeastSpecificSceneComponentSubclass(*pType);

		// Remove an existing component of the least specific type
		// before adding the new type.
		auto pExistingComponent(object.GetComponent(leastSpecificType, false));

		// Setup and return.
		rpOld = pExistingComponent;
		rpNew.Reset();
		return (rpOld.IsValid());
	}
}

/**
 * If pOld and pNew both exist, we attempt to transfer any properties
 * defined in pOld to pNew.
 */
static void TransferComponentProperties(
	const SharedPtr<Scene::Component>& pOld,
	const SharedPtr<Scene::Component>& pNew)
{
	using namespace Reflection;

	// Early out if conditions are not met.
	if (!pOld.IsValid() || !pNew.IsValid())
	{
		return;
	}

	// Serialize.
	DataStore dataStore;
	if (!SerializeToDataStore(
		pOld->GetReflectionThis(),
		dataStore))
	{
		return;
	}

	// Now deserialize into new.
	DefaultNotRequiredSerializeContext context(
		ContentKey(),
		dataStore,
		dataStore.GetRootNode(),
		pNew->GetReflectionThis().GetTypeInfo());
	(void)DeserializeObject(context, dataStore, dataStore.GetRootNode(), pNew->GetReflectionThis());
}

void ControllerScene::SelectedObjectAddComponent(HString typeName)
{
	// TODO: Support component manipulation during multiselect.
	SEOUL_ASSERT(m_tSelectedObjects.GetSize() == 1u);

	auto pObject = *(m_tSelectedObjects.Begin());

	SharedPtr<Scene::Component> pOld;
	SharedPtr<Scene::Component> pNew;
	if (ToAdd(*pObject, typeName, pOld, pNew))
	{
		TransferComponentProperties(pOld, pNew);
		ExecuteCommand(SEOUL_NEW(MemoryBudgets::Editor) CommandSetComponent(
			pObject,
			pOld,
			pNew));
	}
}

void ControllerScene::SelectedObjectRemoveComponent(HString typeName)
{
	// TODO: Support component manipulation during multiselect.
	SEOUL_ASSERT(m_tSelectedObjects.GetSize() == 1u);

	auto pObject = *(m_tSelectedObjects.Begin());

	SharedPtr<Scene::Component> pOld;
	SharedPtr<Scene::Component> pNew;
	if (ToRemove(*pObject, typeName, pOld, pNew))
	{
		ExecuteCommand(SEOUL_NEW(MemoryBudgets::Editor) CommandSetComponent(
			pObject,
			pOld,
			pNew));
	}
}

} // namespace EditorUI

SEOUL_BEGIN_TYPE(EditorUI::SceneObjectPropertyEditBinding, TypeFlags::kDisableNew)
	SEOUL_PARENT(EditorUI::IPropertyChangeBinder)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(EditorUI::ObjectPlaceholder)
	SEOUL_PROPERTY_N("Id", m_sId)
	SEOUL_PROPERTY_N("Category", m_EditorCategory)
	SEOUL_PROPERTY_N("Components", m_vComponents)
SEOUL_END_TYPE()

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
