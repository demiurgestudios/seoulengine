/**
 * \file EditorUIViewSceneViewport.cpp
 * \brief EditorUI view that renders a 3D viewport
 * of a scene hierarchy.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include <imgui.h>
#include <imgui_internal.h> // TODO: Eliminate.

#include "Camera.h"
#include "DevUIImGui.h"
#include "DevUIImGuiRenderer.h"
#include "DevUIController.h"
#include "EditorSceneContainer.h"
#include "EditorSceneState.h"
#include "EditorUIDragSourceFilePath.h"
#include "EditorUIIControllerSceneRoot.h"
#include "EditorUIIcons.h"
#include "EditorUITransformGizmo.h"
#include "EditorUIRoot.h"
#include "EditorUIUtil.h"
#include "EditorUIViewSceneViewport.h"
#include "EditorUISceneRenderer.h"
#include "Engine.h"
#include "InputKeys.h"
#include "ReflectionType.h"
#include "ReflectionUtil.h"
#include "RenderDevice.h"
#include "SceneFxComponent.h"
#include "SceneFreeTransformComponent.h"
#include "SceneMeshDrawComponent.h"
#include "SceneObject.h"
#include "ScenePrefabComponent.h"
#include "Viewport.h"

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

static const Float32 kfDesiredGizmoScaleInPixels = 40.0f;

// TODO: Break out into a utility file.
static const HString kDefaultFxCategory("Fx");
static const HString kDefaultMeshCategory("Meshes");
static const HString kDefaultPrefabCategory("Prefabs");

ViewSceneViewport::ViewSceneViewport(const Settings& settings)
	: m_pRenderer(SEOUL_NEW(MemoryBudgets::Editor) SceneRenderer(settings))
	, m_ContextMenuPick()
	, m_CameraMovement()
	, m_Camera()
	, m_bCapturedRightMouse(false)
	, m_bDragginRightMouse(false)
{
}

ViewSceneViewport::~ViewSceneViewport()
{
}

void ViewSceneViewport::DoPrePose(DevUI::Controller& rController, RenderPass& rPass)
{
	static const Float32 kfCameraNormalMovementSpeed = 10.0f; // TODO: Expose as configuration.
	static const Float32 kfCameraFastMovementSpeed = 40.0f; // TODO: Expose as configuration.

	using namespace ImGui;

	IControllerSceneRoot* pRoot = DynamicCast<IControllerSceneRoot*>(&rController);

	// Early out if no state.
	if (!pRoot->GetScene().GetState().IsValid())
	{
		return;
	}

	auto& rCameraState = pRoot->GetScene().GetState()->GetEditState().m_CameraState;
	m_CameraMovement = CameraMovement();
	m_CameraMovement.m_fDeltaTimeInSeconds = Engine::Get()->GetSecondsInTick(); // TODO: Eliminate this.

	Bool bShowContextMenu = false;
	Bool bRenderScene = false;
	if (BeginChild(
		"ViewportArea",
		ImVec2(0, 0),
		true,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoCollapse) &&
		ImGui::GetCurrentWindowRead()->ClipRect.GetWidth() >= 1.0f &&
		ImGui::GetCurrentWindowRead()->ClipRect.GetHeight() >= 1.0f)
	{
		// TODO: Need to use the actual rendering viewport here,
		// not the back buffer.
		ImRect const clipRect = ImGui::GetCurrentWindowRead()->ClipRect;
		Viewport const fullViewport(RenderDevice::Get()->GetBackBufferViewport());
		Viewport const viewport(Viewport::Create(
			fullViewport.m_iTargetWidth,
			fullViewport.m_iTargetHeight,
			fullViewport.m_iViewportX + Min((Int32)clipRect.Min.x, fullViewport.m_iViewportWidth),
			fullViewport.m_iViewportY + Min((Int32)clipRect.Min.y, fullViewport.m_iViewportHeight),
			Min((Int32)(clipRect.Max.x - clipRect.Min.x), fullViewport.m_iViewportWidth),
			Min((Int32)(clipRect.Max.y - clipRect.Min.y), fullViewport.m_iViewportWidth)));
		Point2DInt const current((Int32)GetIO().MousePos.x, (Int32)GetIO().MousePos.y);

		// Drag and drop handling.
		if (Root::Get()->IsWindowDragAndDropTarget() &&
			Root::Get()->GetDragData().m_Data.IsOfType<DragSourceFilePath>())
		{
			auto const& filePath = Root::Get()->GetDragData().m_Data.Cast<DragSourceFilePath>().m_FilePath;
			auto const bCanPlace = InternalCanPlaceObject(filePath);

			// If we can handle the FilePath, process it now.
			if (bCanPlace)
			{
				// On release, mark that we
				// have a category update to apply.
				if (IsMouseReleased(0))
				{
					InternalPlaceObject(viewport, *pRoot, filePath);
				}
				// Otherwise, just mark that
				// we're a valid drop target.
				else
				{
					Root::Get()->MarkCanDrop();
				}
			}
		}

		// TODO: Figure out when/if we need to be restoring the aspect ratio like this.
		Float const fFullAspectRatio = m_pRenderer->GetCamera().GetAspectRatio();
		m_pRenderer->GetCamera().SetAspectRatio(viewport.GetViewportAspectRatio());

		// Left mouse button handling.
		{
			if (IsWindowClicked(0))
			{
				const SceneRenderer::CurrentPick& pick = m_pRenderer->GetCurrentPick();
				switch (pick.m_eType)
				{
				case SceneRenderer::CurrentPick::kObject:
					if (nullptr != pRoot)
					{
						Bool const bSelected = pRoot->GetSelectedObjects().HasKey(pick.m_pObject);
						auto const& io = GetIO();

						// On control click, update the selection set - either
						// remove or add the clicked object.
						if (io.KeyCtrl)
						{
							auto tNewSelection(pRoot->GetSelectedObjects());
							if (bSelected)
							{
								SEOUL_VERIFY(tNewSelection.Erase(pick.m_pObject));
							}
							else
							{
								SEOUL_VERIFY(tNewSelection.Insert(pick.m_pObject).Second);
							}

							// On control click, last object is the picked object.
							pRoot->SetSelectedObjects(pick.m_pObject, tNewSelection);
						}
						// Single click and shift click only have an effect if the object is not already
						// selected.
						else
						{
							// Shift click in the viewport is similar to Control click,
							// except we don't set the last selected and we take
							// no action if already selected.
							if (io.KeyShift)
							{
								if (!bSelected)
								{
									auto tNewSelection(pRoot->GetSelectedObjects());
									SEOUL_VERIFY(tNewSelection.Insert(pick.m_pObject).Second);
									pRoot->SetSelectedObjects(pRoot->GetLastSelection(), tNewSelection);
								}
							}
							// Click with no modifiers either requires no selection, or
							// a multiple selection - selection size must be != 1.
							else if (!bSelected || pRoot->GetSelectedObjects().GetSize() != 1)
							{
								pRoot->UniqueSetObjectSelected(pick.m_pObject, true);
							}
						}
					}
					break;
				case SceneRenderer::CurrentPick::kHandle:
					if (m_pRenderer->GetGizmo().GetEnabled())
					{
						pRoot->BeginSelectedObjectsTransform();
						m_pRenderer->GetGizmo().SetCapturedHandle(pick.m_eHandle, current);
					}
					break;
				case SceneRenderer::CurrentPick::kNone:
					if (nullptr != pRoot)
					{
						pRoot->UniqueSetObjectSelected(SharedPtr<Scene::Object>(), false);
					}
					break;
				default:
					SEOUL_FAIL("Out-of-sync enum.");
					break;
				};
			}

			// If the left mouse button is up, the gizmo has no captured handle.
			if (!IsMouseDown(0))
			{
				m_pRenderer->GetGizmo().SetCapturedHandle(TransformGizmoHandle::kNone);
				pRoot->EndSelectedObjectsTransform();
			}

			if (m_pRenderer->GetGizmo().GetCapturedHandle() != TransformGizmoHandle::kNone)
			{
				TransformGizmo::MouseState state(
					m_pRenderer->GetCamera(),
					viewport,
					current);

				// Feed mouse deltas to the gizmo. It will behave correctly
				// depending on its current capture mode.
				Transform const t0(m_pRenderer->GetGizmo().GetTransform());
				m_pRenderer->GetGizmo().OnMouseDelta(state);
				Transform const t1(m_pRenderer->GetGizmo().GetTransform());

				if (t0 != t1)
				{
					pRoot->SelectedObjectsApplyTransform(
						m_pRenderer->GetGizmo().GetCapturedTransform(),
						m_pRenderer->GetGizmo().GetTransform());
				}
			}
		}

		// Right mouse button handling.
		{
			// If not already captured, check if we should capture
			// the right mouse.
			if (!m_bCapturedRightMouse)
			{
				m_bCapturedRightMouse = IsWindowClicked(1);
			}

			// Unset capture if the right-mouse is up.
			if (!IsMouseDown(1))
			{
				// Check if we should show the context menu.
				if (m_bCapturedRightMouse && !m_bDragginRightMouse)
				{
					bShowContextMenu = true;
					m_ContextMenuPick = m_pRenderer->GetCurrentPick();

					// TODO: Only necessary in some circumstances
					// (e.g. if already have a selection set and
					// m_ContextMenuPick object is already in that set,
					// don't want to change the set).
					switch (m_ContextMenuPick.m_eType)
					{
					case SceneRenderer::CurrentPick::kObject:
						if (nullptr != pRoot)
						{
							pRoot->UniqueSetObjectSelected(m_ContextMenuPick.m_pObject, true);
						}
						break;
					case SceneRenderer::CurrentPick::kHandle: // fall-through
					case SceneRenderer::CurrentPick::kNone:
						if (nullptr != pRoot)
						{
							pRoot->UniqueSetObjectSelected(SharedPtr<Scene::Object>(), false);
						}
						break;
					default:
						SEOUL_FAIL("Out-of-sync enum.");
						break;
					};

				}

				m_bCapturedRightMouse = false;
				m_bDragginRightMouse = false;
			}

			// Prior to submission, update and apply camera motion.
			if (m_bCapturedRightMouse)
			{
				ImVec2 const delta = GetIO().MouseDelta;
				m_bDragginRightMouse = m_bDragginRightMouse || Abs(delta.x) > 0 || Abs(delta.y) > 0;
				m_CameraMovement.m_vMouseDelta = Vector2D(delta.x, delta.y);
				m_CameraMovement.m_fDeltaYawInRadians -= (delta.x / (Float32)viewport.m_iViewportWidth) * fPi;
				m_CameraMovement.m_fDeltaPitchInRadians -= (delta.y / (Float32)viewport.m_iViewportHeight) * fPi;
				m_CameraMovement.m_bBackward = ImGui::IsKeyDown(ImGuiKey_S);
				m_CameraMovement.m_bForward = ImGui::IsKeyDown(ImGuiKey_W);
				m_CameraMovement.m_bLeft = ImGui::IsKeyDown(ImGuiKey_A);
				m_CameraMovement.m_bRight = ImGui::IsKeyDown(ImGuiKey_D);
				m_CameraMovement.m_bUp = ImGui::IsKeyDown(ImGuiKey_E);
				m_CameraMovement.m_bDown = ImGui::IsKeyDown(ImGuiKey_Q);
				m_bDragginRightMouse = m_bDragginRightMouse ||
					m_CameraMovement.m_bBackward ||
					m_CameraMovement.m_bForward ||
					m_CameraMovement.m_bLeft ||
					m_CameraMovement.m_bRight ||
					m_CameraMovement.m_bUp ||
					m_CameraMovement.m_bDown;

				rCameraState.SetUnitsPerSecond(ImGui::GetIO().KeyShift
					? kfCameraFastMovementSpeed
					: kfCameraNormalMovementSpeed);
			}
		}

		// Special command handling.
		if (IsShortcutPressed("F")) { InternalFocusCamera(*pRoot); }

		// Mouse wheel handling.
		if (IsWindowHovered())
		{
			m_CameraMovement.m_fMouseWheelDelta += GetIO().MouseWheel;
		}

		// Apply camera motion to the scene's camera.
		m_Camera.Apply(
			*pRoot,
			m_CameraMovement,
			viewport,
			m_pRenderer->GetCamera());

		// Dispatch actual rendering responsibilities to the renderer.
		{
			// Cache the mouse position.
			ImVec2 const mousePosition = GetMousePos();

			// Configure picking behavior.
			m_pRenderer->ConfigurePicking(
				IsWindowHovered(),
				Point2DInt((Int32)mousePosition.x, (Int32)mousePosition.y));

			// Dispatch actual rendering responsibilities to the renderer.
			m_pRenderer->PrePose(rController);
		}

		// Pose the axis orienter.
		InternalPrePoseAxisGizmo(viewport);

		// Pose the viewport menu.
		InternalPrePoseToolBar(rCameraState);

		////////////////////////////////////////////
		// TODO: Context menu here is a WIP.
		////////////////////////////////////////////
		// Context menu handling, if triggered.
		if (bShowContextMenu)
		{
			OpenPopupEx("ViewportAreaContextMenu", false);
		}

		if (BeginPopup("ViewportAreaContextMenu"))
		{
			switch (m_ContextMenuPick.m_eType)
			{
			case SceneRenderer::CurrentPick::kObject:
				if (MenuItem("Focus Camera")) { InternalFocusCamera(*pRoot); }
				break;
			case SceneRenderer::CurrentPick::kHandle:
				// TODO:
				break;
			case SceneRenderer::CurrentPick::kNone:
				// TODO:
				break;
			default:
				SEOUL_FAIL("Out-of-sync enum.");
				break;
			};
			EndPopup();
		}
		////////////////////////////////////////////
		// /TODO:
		////////////////////////////////////////////

		// TODO: Figure out when/if we need to be restoring the aspect ratio like this.
		m_pRenderer->GetCamera().SetAspectRatio(fFullAspectRatio);
	}

	// Finally, EndChild() - always done with ImGui.
	ImGui::EndChild();
}

Bool ViewSceneViewport::InternalCanPlaceObject(FilePath filePath) const
{
	switch (filePath.GetType())
	{
	case FileType::kFxBank: return true;
	case FileType::kSceneAsset: return true;
	case FileType::kScenePrefab: return true;
	default:
		return false;
	};
}

void ViewSceneViewport::InternalFocusCamera(IControllerSceneRoot& r)
{
	Vector3D vPosition{};
	Float fZoom{};
	if (r.ComputeCameraFocus(m_pRenderer->GetCamera(), vPosition, fZoom))
	{
		m_Camera.SetTargetPosition(r, vPosition);
		if (!m_pRenderer->GetCamera().GetProjectionMatrix().IsPerspective())
		{
			m_Camera.SetTargetZoom(r, fZoom);
		}
	}
}

void ViewSceneViewport::InternalPlaceObject(
	const Viewport& viewport,
	IControllerSceneRoot& r,
	FilePath filePath) const
{
	// TODO: More basic object templates?
	// TODO: Don't copy and paste this code.
	String const sId(Path::GetFileNameWithoutExtension(filePath.GetRelativeFilename()));
	SharedPtr<Scene::Object> pObject(SEOUL_NEW(MemoryBudgets::SceneObject) Scene::Object(sId));
	pObject->AddComponent(SharedPtr<Scene::Component>(SEOUL_NEW(MemoryBudgets::SceneComponent) Scene::FreeTransformComponent));

	// TODO: Smarter, refactor into function so it can be shared.
	auto const mousePos = ImGui::GetMousePos();
	Ray3D const ray(m_pRenderer->GetCamera().GetWorldRayFromScreenSpace(
		viewport,
		Point2DInt((Int)mousePos.x, (Int)mousePos.y)));
	Vector3D const vPosition(ray.Derive(/* TODO: */5.0f));
	pObject->SetPosition(vPosition);

	switch (filePath.GetType())
	{
	case FileType::kFxBank:
	{
		// Add an FxComponent to the object.
		SharedPtr<Scene::FxComponent> pComponent(SEOUL_NEW(MemoryBudgets::SceneComponent) Scene::FxComponent);
		pComponent->SetFxFilePath(filePath);
		pObject->AddComponent(pComponent);

		// Set the category and commit the object.
		pObject->SetEditorCategory(kDefaultFxCategory);
		r.AddObject(pObject);
	}
	break;
	case FileType::kSceneAsset:
	{
		// Add a Mesh to the object.
		SharedPtr<Scene::MeshDrawComponent> pComponent(SEOUL_NEW(MemoryBudgets::SceneComponent) Scene::MeshDrawComponent);
		pComponent->SetMeshFilePath(filePath);
		pObject->AddComponent(pComponent);

		// Set the category and commit the object.
		pObject->SetEditorCategory(kDefaultMeshCategory);
		r.AddObject(pObject);
	}
	break;
	case FileType::kScenePrefab:
	{
		// Add a Prefab to the object.
		SharedPtr<Scene::PrefabComponent> pComponent(SEOUL_NEW(MemoryBudgets::SceneComponent) Scene::PrefabComponent);
		pComponent->SetFilePath(filePath);
		pObject->AddComponent(pComponent);

		// Set the category and commit the object.
		pObject->SetEditorCategory(kDefaultPrefabCategory);
		r.AddObject(pObject);
	}
	break;
	default:
		break;
	};
}

void ViewSceneViewport::InternalPrePoseAxisGizmo(const Viewport& viewport)
{
	// Compute our tolerance as 3% of the viewport width and height.
	ImVec2 const vTolerance(
		(Float)viewport.m_iViewportWidth * 0.01f,
		(Float)viewport.m_iViewportHeight * 0.01f);

	auto const& camera = m_pRenderer->GetCamera();

	Float fNear, fUnusedFar;
	Matrix4D::ExtractNearFar(camera.GetProjectionMatrix(), fNear, fUnusedFar);
	Vector3D const vW0 = camera.GetPosition() + (fNear + 1.0f) * camera.GetViewAxis();
	Vector2D const v0 = camera.ConvertWorldToScreenSpace(viewport, vW0).GetXY();

	Float const fScale = ComputeGizmoScale(kfDesiredGizmoScaleInPixels, camera, viewport, vW0);
	Vector2D const vX = camera.ConvertWorldToScreenSpace(viewport, vW0 + Vector3D::UnitX() * fScale).GetXY();
	Vector2D const vY = camera.ConvertWorldToScreenSpace(viewport, vW0 + Vector3D::UnitY() * fScale).GetXY();
	Vector2D const vZ = camera.ConvertWorldToScreenSpace(viewport, vW0 + Vector3D::UnitZ() * fScale).GetXY();

	Float const fOffsetX = ImGui::GetFontSize() + (camera.ConvertWorldToScreenSpace(viewport, vW0 + camera.GetRightAxis() * fScale).GetXY() - v0).Length();
	Float const fOffsetY = ImGui::GetFontSize() + (camera.ConvertWorldToScreenSpace(viewport, vW0 + camera.GetUpAxis() * fScale).GetXY() - v0).Length();

	ImGuiWindow* pWindow = ImGui::GetCurrentWindow();

	ImVec2 const p0(pWindow->ClipRect.GetBL() + ImVec2(fOffsetX, -fOffsetY));
	ImVec2 const pX(ToImVec2(vX - v0) + p0);
	ImVec2 const pY(ToImVec2(vY - v0) + p0);
	ImVec2 const pZ(ToImVec2(vZ - v0) + p0);
	ImU32 const cX(ImGui::GetColorU32(ImVec4(1, 0, 0, 1)));
	ImU32 const cY(ImGui::GetColorU32(ImVec4(0, 1, 0, 1)));
	ImU32 const cZ(ImGui::GetColorU32(ImVec4(0, 0, 1, 1)));

	if (!Equals(p0, pX, vTolerance))
	{
		pWindow->DrawList->AddLine(p0, pX, cX);
		pWindow->DrawList->AddText(pX, cX, "X");
	}

	if (!Equals(p0, pY, vTolerance))
	{
		pWindow->DrawList->AddLine(p0, pY, cY);
		pWindow->DrawList->AddText(pY, cY, "Y");
	}

	if (!Equals(p0, pZ, vTolerance))
	{
		pWindow->DrawList->AddLine(p0, pZ, cZ);
		pWindow->DrawList->AddText(pZ, cZ, "Z");
	}
}

void ViewSceneViewport::InternalPrePoseCameraMode(EditorScene::CameraState& rState)
{
	Int iCurrent = (Int)rState.GetMode();
	if (ImGui::Combo(
		"##Camera Modes",
		&iCurrent,
		ImGuiEnumNameUtil<EditorScene::CameraMode>,
		nullptr,
		(Int)EditorScene::CameraMode::COUNT))
	{
		rState.SetMode((EditorScene::CameraMode)iCurrent);
	}
	SetTooltipEx("Select the viewport camera mode.");
}

void ViewSceneViewport::InternalPrePoseRenderMode()
{
	Int iCurrent = (Int)m_pRenderer->GetSceneRendererType();
	if (ImGui::Combo(
		"##Render Modes",
		&iCurrent,
		ImGuiEnumNameUtil<ViewportEffectType>,
		nullptr,
		(Int)ViewportEffectType::COUNT))
	{
		m_pRenderer->SetSceneRendererType((ViewportEffectType)iCurrent);
	}
	SetTooltipEx("Select the viewport render (visualization) mode.");
}

void ViewSceneViewport::InternalPrePoseSnapRotation()
{
	// TODO:
	static const Float s_kaSnapValues[] =
	{
		5.0f,
		10.0f,
		15.0f,
		30.0f,
		45.0f,
		60.0f,
		90.0f,
		120.0f,
	};
	static Byte const* s_kaSnapStrs[] =
	{
		"5.0\xc2\xb0",
		"10.0\xc2\xb0",
		"15.0\xc2\xb0",
		"30.0\xc2\xb0",
		"45.0\xc2\xb0",
		"60.0\xc2\xb0",
		"90.0\xc2\xb0",
		"120.0\xc2\xb0",
	};

	auto const& icons = Root::Get()->GetIcons();
	auto& renderer = Root::Get()->GetRenderer();
	auto pSnap = renderer.ResolveTexture(icons.m_SnapRotation);

	Bool bRotationSnap = m_pRenderer->GetGizmo().GetRotationSnap();
	if (ImGui::ToolbarButton(pSnap, bRotationSnap))
	{
		bRotationSnap = !bRotationSnap;
		m_pRenderer->GetGizmo().SetRotationSnap(bRotationSnap);
	}
	SetTooltipEx("Enable/disable rotation snapping.");

	ImGui::SameLine();

	Float const fRotationSnapDegrees = m_pRenderer->GetGizmo().GetRotationSnapDegrees();

	Int iCurrent = 0;
	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(s_kaSnapValues); ++i)
	{
		if (s_kaSnapValues[i] >= fRotationSnapDegrees)
		{
			iCurrent = (Int)i;
			break;
		}
	}

	if (ImGui::Combo(
		"##Rotation Snap Sizes",
		&iCurrent,
		s_kaSnapStrs,
		SEOUL_ARRAY_COUNT(s_kaSnapStrs)))
	{
		m_pRenderer->GetGizmo().SetRotationSnapDegrees(s_kaSnapValues[iCurrent]);
	}
	SetTooltipEx("Set rotation snapping angle (in degrees).");
}

void ViewSceneViewport::InternalPrePoseSnapScale()
{
	// TODO:
	static const Float s_kaSnapValues[] =
	{
		10.0f,
		1.0f,
		0.5f,
		0.25f,
		0.125f,
		0.0625f,
		0.03125f,
	};
	static Byte const* s_kaSnapStrs[] =
	{
		"10.0x",
		"1.0x",
		"0.5x",
		"0.25x",
		"0.125x",
		"0.0625x",
		"0.03125x",
	};

	auto const& icons = Root::Get()->GetIcons();
	auto& renderer = Root::Get()->GetRenderer();
	auto pSnap = renderer.ResolveTexture(icons.m_SnapScale);

	Bool bScaleSnap = m_pRenderer->GetGizmo().GetScaleSnap();
	if (ImGui::ToolbarButton(pSnap, bScaleSnap))
	{
		bScaleSnap = !bScaleSnap;
		m_pRenderer->GetGizmo().SetScaleSnap(bScaleSnap);
	}
	SetTooltipEx("Enable/disable scale snapping.");

	ImGui::SameLine();

	Float const fScaleSnapFactor = m_pRenderer->GetGizmo().GetScaleSnapFactor();

	Int iCurrent = (Int)SEOUL_ARRAY_COUNT(s_kaSnapValues) - 1;
	for (Int i = (Int)SEOUL_ARRAY_COUNT(s_kaSnapValues) - 1; i >= 0; --i)
	{
		if (s_kaSnapValues[i] >= fScaleSnapFactor)
		{
			iCurrent = (Int)i;
			break;
		}
	}

	if (ImGui::Combo(
		"##Scale Snap Sizes",
		&iCurrent,
		s_kaSnapStrs,
		SEOUL_ARRAY_COUNT(s_kaSnapStrs)))
	{
		m_pRenderer->GetGizmo().SetScaleSnapFactor(s_kaSnapValues[iCurrent]);
	}
	SetTooltipEx("Set scale snapping multiple.");
}

void ViewSceneViewport::InternalPrePoseSnapTranslation()
{
	// TODO:
	static const Float s_kaSnapValues[] =
	{
		0.01f,
		0.05f,
		0.1f,
		0.5f,
		1.0f,
		5.0f,
		10.0f,
	};
	static Byte const* s_kaSnapStrs[] =
	{
		"0.01 m",
		"0.05 m",
		"0.1 m",
		"0.5 m",
		"1.0 m",
		"5.0 m",
		"10.0 m",
	};

	auto const& icons = Root::Get()->GetIcons();
	auto& renderer = Root::Get()->GetRenderer();
	auto pSnap = renderer.ResolveTexture(icons.m_SnapTranslation);

	Bool bTranslationSnap = m_pRenderer->GetGizmo().GetTranslationSnap();
	if (ImGui::ToolbarButton(pSnap, bTranslationSnap))
	{
		bTranslationSnap = !bTranslationSnap;
		m_pRenderer->GetGizmo().SetTranslationSnap(bTranslationSnap);
	}
	SetTooltipEx("Enable/disable translation snapping.");

	ImGui::SameLine();

	Float const fTranslationSnapFactor = m_pRenderer->GetGizmo().GetTranslationSnapFactor();

	Int iCurrent = 0;
	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(s_kaSnapValues); ++i)
	{
		if (s_kaSnapValues[i] >= fTranslationSnapFactor)
		{
			iCurrent = (Int)i;
			break;
		}
	}

	if (ImGui::Combo(
		"##Translation Snap Sizes",
		&iCurrent,
		s_kaSnapStrs,
		SEOUL_ARRAY_COUNT(s_kaSnapStrs)))
	{
		m_pRenderer->GetGizmo().SetTranslationSnapFactor(s_kaSnapValues[iCurrent]);
	}
	SetTooltipEx("Set translation snapping (in meters).");
}

void ViewSceneViewport::InternalPrePoseToolBar(EditorScene::CameraState& rState)
{
	using namespace ImGui;

	static const Float kfToolbarHeight = 37.0f; // TODO: Figure out how to get imgui to auto size height.
	static const Float kfBackgroundAlpha = 0.5f; // TODO: Don't hardcode.
	auto const eFlags = (
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar);

	ImVec4 childBackground = GetStyle().Colors[ImGuiCol_ChildBg];
	childBackground.w = kfBackgroundAlpha;
	PushStyleColor(ImGuiCol_ChildBg, childBackground);
	if (BeginChild("ViewportToolBar", ImVec2(0, kfToolbarHeight), true, eFlags))
	{
		PushItemWidth(120.0f); // TODO:
		InternalPrePoseCameraMode(rState);
		SameLine();
		InternalPrePoseRenderMode();
		SameLine();
		PopItemWidth();
		InternalPrePoseTransformGizmoMode();
		SameLine();
		PushItemWidth(80.0f); // TODO:
		InternalPrePoseSnapTranslation();
		SameLine();
		InternalPrePoseSnapRotation();
		SameLine();
		PopItemWidth();
		PushItemWidth(100.0f); // TODO:
		InternalPrePoseSnapScale();
		PopItemWidth();
	}
	EndChild();
	PopStyleColor();
}

void ViewSceneViewport::InternalPrePoseTransformGizmoMode()
{
	using namespace ImGui;

	auto const& icons = Root::Get()->GetIcons();
	auto& renderer = Root::Get()->GetRenderer();
	auto pGlobal = renderer.ResolveTexture(icons.m_Global);
	auto pRotate = renderer.ResolveTexture(icons.m_Rotate);
	auto pScale = renderer.ResolveTexture(icons.m_Scale);
	auto pTranslate = renderer.ResolveTexture(icons.m_Translate);

	TransformGizmoMode const eMode = m_pRenderer->GetGizmo().GetMode();
	if (ToolbarButton(pTranslate, (eMode == TransformGizmoMode::kTranslation)))
	{
		m_pRenderer->GetGizmo().SetMode(TransformGizmoMode::kTranslation);
	}
	SetTooltipEx("Set transform gizmo to translation mode.");

	SameLine();

	if (ToolbarButton(pRotate, (eMode == TransformGizmoMode::kRotation)))
	{
		m_pRenderer->GetGizmo().SetMode(TransformGizmoMode::kRotation);
	}
	SetTooltipEx("Set transform gizmo to rotation mode.");

	SameLine();

	if (ToolbarButton(pScale, (eMode == TransformGizmoMode::kScale)))
	{
		m_pRenderer->GetGizmo().SetMode(TransformGizmoMode::kScale);
	}
	SetTooltipEx("Set transform gizmo to scale mode.");

	SameLine();

	Bool bGlobal = m_pRenderer->GetGizmo().GetGlobalMode();
	if (ToolbarButton(pGlobal, bGlobal))
	{
		bGlobal = !bGlobal;
		m_pRenderer->GetGizmo().SetGlobalMode(bGlobal);
	}
	SetTooltipEx("Toggle transform gizmo between global and local (object) space.");
}

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE
