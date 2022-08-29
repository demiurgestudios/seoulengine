/**
 * \file EditorUISceneRenderer.cpp
 * \brief View that renders a 3D viewport
 * of a scene hierarchy.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Camera.h"
#include "DevUIController.h"
#include "DevUIImGui.h"
#include "EditorSceneContainer.h"
#include "EditorUIIControllerSceneRoot.h"
#include "EditorUISettings.h"
#include "EditorUITransformGizmo.h"
#include "EditorUISceneRenderer.h"
#include "EffectManager.h"
#include "EffectPass.h"
#include "IndexBuffer.h"
#include "ReflectionMethod.h"
#include "ReflectionType.h"
#include "RenderDevice.h"
#include "Renderer.h"
#include "RenderPass.h"
#include "RenderSurface.h"
#include "SceneMeshDrawComponent.h"
#include "SceneObject.h"
#include "ScenePrefab.h"
#include "ScenePrimitiveRenderer.h"
#include "SceneRenderer.h"
#include "TextureConfig.h"
#include "VertexBuffer.h"
#include "VertexFormat.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

namespace EditorUI
{

static const HString kEditorDrawPrimitives("EditorDrawPrimitives");

/** EffectParmaeter used for mip visualization mode. */
static const HString kEffectParameterMipVizTexture("seoul_MipVizTexture");

/**
 * @return true if the given viewport effect type is
 * a debug/developer mode, not a mode intended for
 * use at runtime.
 */
static inline Bool IsDiagnosticMode(ViewportEffectType eType)
{
	switch (eType)
	{
	case ViewportEffectType::kUnlit: return false;
	default:
		return true;
	};
}

static inline Scene::RendererConfig GetInitialRendererConfig(const Settings& settings)
{
	Scene::RendererConfig config;
	config.m_FxEffectFilePath = settings.m_aFxEffectFilePaths[(UInt32)ViewportEffectType::kUnlit];
	config.m_MeshEffectFilePath = settings.m_aMeshEffectFilePaths[(UInt32)ViewportEffectType::kUnlit];
	return config;
}

/** Utility used by CreateMipVizTexture(). */
static inline ColorARGBu8* CreateData(UInt32 uWidth, UInt32 uHeight, ColorARGBu8 c)
{
	ColorARGBu8* p = (ColorARGBu8*)MemoryManager::AllocateAligned(
		sizeof(ColorARGBu8) * uWidth * uHeight,
		MemoryBudgets::Rendering,
		SEOUL_ALIGN_OF(ColorARGBu8));

	for (UInt32 i = 0u; i < (uWidth * uHeight); ++i)
	{
		p[i] = c;
	}

	return p;
}
static TextureData CreateBase(UInt32 uWidth, UInt32 uHeight, ColorARGBu8 c, PixelFormat& reFormat)
{
	auto p = CreateData(uWidth, uHeight, c);
	return TextureData::CreateFromInMemoryBuffer(p, sizeof(ColorARGBu8) * uWidth * uHeight, reFormat);
}

static SharedPtr<TextureLevelData> CreateLevel(UInt32 uWidth, UInt32 uHeight, ColorARGBu8 c)
{
	auto p = CreateData(uWidth, uHeight, c);
	return SharedPtr<TextureLevelData>(SEOUL_NEW(MemoryBudgets::Rendering) TextureLevelData(p, sizeof(ColorARGBu8) * uWidth * uHeight, p, nullptr));
}

/**
 * Generates a texture for mip visualization.
 *
 * \sa http://aras-p.info/blog/2011/05/03/a-way-to-visualize-mip-levels/
 */
static inline TextureContentHandle CreateMipVizTexture()
{
	// Colors for each mip level.
	static const ColorARGBu8 kacColors[] =
	{
		ColorARGBu8::Create(  0,   0, 255, 204),
		ColorARGBu8::Create(  0, 127, 255, 102),
		ColorARGBu8::Create(255, 255, 255,   0),
		ColorARGBu8::Create(255, 178,   0,  51),
		ColorARGBu8::Create(255,  76,   0, 153),
		ColorARGBu8::Create(255,   0,   0, 204),
	};

	// 32 x 32 texture with a full mip chain.
	UInt32 uWidth = 32u;
	UInt32 uHeight = 32u;

	// Create the base level.
	PixelFormat eFormat = PixelFormat::kA8R8G8B8;
	auto data = CreateBase(uWidth, uHeight, kacColors[0], eFormat);

	// Now create the mip levels.
	for (size_t i = 1u; i < SEOUL_ARRAY_COUNT(kacColors); ++i)
	{
		uWidth >>= 1u;
		uHeight >>= 1u;
		data = TextureData::PushBackLevel(data, CreateLevel(uWidth, uHeight, kacColors[i]));
	}

	// Sanity check.
	SEOUL_ASSERT(1u == uWidth);
	SEOUL_ASSERT(1u == uHeight);

	// Create the texture itself.
	TextureConfig config;
	config.m_bMipped = true;
	config.m_bWrapAddressU = true;
	config.m_bWrapAddressV = true;
	return TextureContentHandle(RenderDevice::Get()->CreateTexture(
		config,
		data,
		32u,
		32u,
		eFormat).GetPtr());

}

class SceneReadPixel SEOUL_SEALED : public IReadPixel
{
public:
	SEOUL_DELEGATE_TARGET(SceneReadPixel);

	SceneReadPixel()
		: m_hRenderer()
		, m_tPickTable()
		, m_Pick(0, 0)
	{
	}

	SceneRendererHandle m_hRenderer;
	Scene::Renderer::PickTable m_tPickTable;
	Point2DInt m_Pick;

	virtual void OnReadPixel(ColorARGBu8 cPixel, Bool bSuccess) SEOUL_OVERRIDE
	{
		CheckedPtr<SceneRenderer> p(GetPtr(m_hRenderer));
		if (p.IsValid() && bSuccess)
		{
			cPixel.m_A = 255;

			SharedPtr<Scene::Object> pObject;
			if (m_tPickTable.GetValue(cPixel, pObject))
			{
				p->OnPick(pObject);
			}
			else
			{
				TransformGizmoHandle const eGizmoHandle = TransformGizmo::PickColorToHandle(cPixel);
				if (TransformGizmoHandle::kNone != eGizmoHandle)
				{
					p->OnPick(eGizmoHandle);
				}
				else
				{
					p->OnPickNone();
				}
			}
		}
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(SceneReadPixel);

	SEOUL_DISABLE_COPY(SceneReadPixel);
}; // class SceneReadPixel

SceneRenderer::CurrentPick::CurrentPick()
	: m_eHandle(TransformGizmoHandle::kNone)
	, m_pObject()
	, m_eType(kNone)
{
}

SceneRenderer::CurrentPick::~CurrentPick()
{
}

SceneRenderer::SceneRenderer(const Settings& settings)
	: m_Settings(settings)
	, m_hThis()
	, m_pCachedController()
	, m_pTransformGizmo(SEOUL_NEW(MemoryBudgets::Editor) TransformGizmo)
	, m_pCamera(SEOUL_NEW(MemoryBudgets::Editor) Camera)
	, m_eSceneRendererType(ViewportEffectType::kUnlit)
	, m_hPrimitiveEffect(EffectManager::Get()->GetEffect(settings.m_PrimitiveEffectFilePath))
	, m_pSceneRenderer(SEOUL_NEW(MemoryBudgets::Rendering) Scene::Renderer(GetInitialRendererConfig(settings)))
	, m_pPrimitiveRenderer(SEOUL_NEW(MemoryBudgets::Scene) Scene::PrimitiveRenderer)
	, m_hMipVizTexture(CreateMipVizTexture())
	, m_PickState()
{
	// Allocate a handle for this.
	m_hThis = SceneRendererHandleTable::Allocate(this);
}

SceneRenderer::~SceneRenderer()
{
	// Free our handle.
	SceneRendererHandleTable::Free(m_hThis);
}

void SceneRenderer::ConfigurePicking(Bool bPick, const Point2DInt& mousePosition)
{
	// Sanity check.
	SEOUL_ASSERT(IsMainThread());

	if (!bPick)
	{
		// Reset all state if not picking.
		m_PickState = PickState();
	}
	// Otherwise, enable pick and set the position.
	else
	{
		m_PickState.m_bPick = bPick;
		m_PickState.m_PickMousePosition = mousePosition;
	}
}

void SceneRenderer::PrePose(DevUI::Controller& rController)
{
	// TODO: Violating my own rule of never binding
	// a SharedPtr<> to an address of a reference.

	// Cache a reference to the controller for later use.
	m_pCachedController.Reset(&rController);

	// Enqueue the operation.
	ImDrawList* pDrawList = ImGui::GetWindowDrawList();
	pDrawList->AddCallback(
		&SceneRenderer::SubmitSceneRenderingCallback,
		SceneRendererHandle::ToVoidStar(m_hThis));
}

void SceneRenderer::SetSceneRendererType(ViewportEffectType eType)
{
	// Nop if already set.
	if (eType == m_eSceneRendererType)
	{
		return;
	}

	// Update cached value.
	m_eSceneRendererType = eType;

	// Configure the renderer based on the new value.
	Scene::RendererConfig config;
	config.m_FxEffectFilePath = m_Settings.m_aFxEffectFilePaths[(UInt32)eType];
	config.m_MeshEffectFilePath = m_Settings.m_aMeshEffectFilePaths[(UInt32)eType];
	m_pSceneRenderer->Configure(config);
}

void SceneRenderer::SubmitSceneRenderingCallback(
	const ImDrawList* pParentList,
	const ImDrawCmd* pCommand,
	void* pInContext)
{
	// Get the this pointer.
	SceneRendererHandle hThis = SceneRendererHandle::ToHandle(
		pCommand->UserCallbackData);
	CheckedPtr<SceneRenderer> pThis(GetPtr(hThis));
	if (!pThis.IsValid())
	{
		return;
	}

	// Get the stream builder.
	RenderPass& rPass = *((RenderPass*)pInContext);
	RenderCommandStreamBuilder& rBuilder = *rPass.GetRenderCommandStreamBuilder();

	// Compute our render target.
	Viewport const fullViewport(rBuilder.GetCurrentViewport());
	Viewport const viewport(Viewport::Create(
		fullViewport.m_iTargetWidth,
		fullViewport.m_iTargetHeight,
		fullViewport.m_iViewportX + Min((Int32)pCommand->ClipRect.x, fullViewport.m_iViewportWidth),
		fullViewport.m_iViewportY + Min((Int32)pCommand->ClipRect.y, fullViewport.m_iViewportHeight),
		Min((Int32)(pCommand->ClipRect.z - pCommand->ClipRect.x), fullViewport.m_iViewportWidth),
		Min((Int32)(pCommand->ClipRect.w - pCommand->ClipRect.y), fullViewport.m_iViewportWidth)));

	// Issue the actual scene rendering command.
	pThis->SubmitSceneRendering(rPass, viewport);
	pThis->m_pCachedController.Reset();
}

void SceneRenderer::SubmitSceneRendering(
	RenderPass& rPass,
	const Viewport& viewport)
{
	IControllerSceneRoot* pRoot = DynamicCast<IControllerSceneRoot*>(m_pCachedController.GetPtr());

	// Nothing to do if we don't have a state.
	if (nullptr == pRoot)
	{
		return;
	}

	// TODO: Figure out when/if we need to be restoring the aspect ratio like this.

	// Apply relative viewport.
	RenderCommandStreamBuilder& rBuilder = *rPass.GetRenderCommandStreamBuilder();
	Viewport const fullViewport = rBuilder.GetCurrentViewport();
	Float32 const fullAspectRatio = m_pCamera->GetAspectRatio();
	if (viewport != fullViewport)
	{
		// Apply the new viewport to the viewport and scissor.
		rBuilder.SetCurrentViewport(viewport);
		rBuilder.SetScissor(true, viewport);

		// Update aspect ratio.
		m_pCamera->SetAspectRatio(viewport.GetViewportAspectRatio());
	}

	// Picking.
	InternalPick(viewport, pRoot, rPass);

	// Rendering.
	InternalRender(viewport, pRoot, rPass);

	// If the viewport was changed, restore it before returning.
	if (viewport != fullViewport)
	{
		// Update aspect ratio.
		m_pCamera->SetAspectRatio(fullAspectRatio);

		// Apply the original viewport to the viewport and scissor.
		rBuilder.SetCurrentViewport(fullViewport);
		rBuilder.SetScissor(true, fullViewport);
	}
}

void SceneRenderer::InternalPick(
	const Viewport& viewport,
	IControllerSceneRoot* pRoot,
	RenderPass& rPass)
{
	static const ColorARGBu8 kcClearColor(ColorARGBu8::Create(255, 255, 255, 255));

	// Early out.
	if (!m_PickState.m_bPick)
	{
		return;
	}

	RenderCommandStreamBuilder& rBuilder = *rPass.GetRenderCommandStreamBuilder();

	// Setup our callback.
	SharedPtr<SceneReadPixel> pReadPixel(SEOUL_NEW(MemoryBudgets::Editor) SceneReadPixel);
	pReadPixel->m_hRenderer = m_hThis;
	pReadPixel->m_Pick = m_PickState.m_PickMousePosition;

	// Clear.
	rBuilder.Clear(
		(UInt32)(ClearFlags::kColorTarget | ClearFlags::kDepthTarget),
		Color4(kcClearColor),
		1.0f,
		0u);

	// Pick objects in the scene.
	m_pSceneRenderer->Pick(
		m_pCamera,
		pRoot->GetScene().GetObjects(),
		rPass,
		rBuilder,
		pReadPixel->m_tPickTable);

	// Now render picked primitives and decide whether we need to read the pixel.
	Bool const bResolve =
		InternalRenderPrimitives(viewport, pRoot, true, rBuilder) ||
		!pReadPixel->m_tPickTable.IsEmpty();

	// If something was rendered, read the pixel.
	if (bResolve)
	{
		rBuilder.ReadBackBufferPixel(
			m_PickState.m_PickMousePosition.X,
			m_PickState.m_PickMousePosition.Y,
			pReadPixel,
			GetMainThreadId());
	}
}

void SceneRenderer::InternalRender(
	const Viewport& viewport,
	IControllerSceneRoot* pRoot,
	RenderPass& rPass)
{
	// TODO: Configure this.
	static const ColorARGBu8 kcClearColor(ColorARGBu8::Create(30, 30, 30, 255));

	RenderCommandStreamBuilder& rBuilder = *rPass.GetRenderCommandStreamBuilder();

	// Clear.
	rBuilder.Clear(
		(UInt32)(ClearFlags::kColorTarget | ClearFlags::kDepthTarget),
		Color4(kcClearColor),
		1.0f,
		0u);

	// TODO: Update API to eliminate usage of a Vector<> here.
	Scene::Renderer::Cameras vCameras(1u, m_pCamera);

	// Commit the mip texture.
	{
		// Fx.
		{
			auto pEffect(m_pSceneRenderer->GetFxEffect().GetPtr());
			if (pEffect.IsValid())
			{
				rBuilder.SetTextureParameter(
					pEffect,
					kEffectParameterMipVizTexture,
					m_hMipVizTexture);
			}
		}

		// Meshes.
		{
			auto pEffect(m_pSceneRenderer->GetMeshEffect().GetPtr());
			if (pEffect.IsValid())
			{
				rBuilder.SetTextureParameter(
					pEffect,
					kEffectParameterMipVizTexture,
					m_hMipVizTexture);
			}
		}
	}

	// Main scene render.
	m_pSceneRenderer->Render(
		vCameras,
		pRoot->GetScene().GetObjects(),
		rPass,
		rBuilder,
		&pRoot->GetSelectedObjects());

	// Primitives.
	InternalRenderPrimitives(viewport, pRoot, false, rBuilder);
}

Bool SceneRenderer::InternalRenderPrimitives(
	const Viewport& viewport,
	IControllerSceneRoot* pRoot,
	Bool bPicking,
	RenderCommandStreamBuilder& rBuilder)
{
	CheckedPtr<Scene::PrimitiveRenderer> pPrimitiveRenderer(m_pPrimitiveRenderer.Get());
	auto pPrimitiveEffect(m_hPrimitiveEffect.GetPtr());
	if (!pPrimitiveEffect.IsValid() || BaseGraphicsObject::kDestroyed == pPrimitiveEffect->GetState())
	{
		return false;
	}

	pPrimitiveRenderer->BeginFrame(m_pCamera, rBuilder);
	pPrimitiveRenderer->UseEffect(pPrimitiveEffect);

	// Only submit to components if not in a diagnostic mode.
	if (!bPicking && !IsDiagnosticMode(m_eSceneRendererType))
	{
		// TODO: This is currently limited to 1 because
		// PrimitiveRenderer is very expensive, and we can't render
		// many primitives at all before it makes the editor unusable.
		auto const& objects = pRoot->GetSelectedObjects();
		if (objects.GetSize() == 1u)
		{
			auto pObject(*objects.Begin());
			auto const& comps = pObject->GetComponents();
			for (auto pComp : comps)
			{
				Reflection::WeakAny const thisPointer = pComp->GetReflectionThis();
				const Reflection::Type& type = thisPointer.GetType();
				Reflection::Method const* pMethod = type.GetMethod(kEditorDrawPrimitives);
				if (nullptr != pMethod)
				{
					Reflection::MethodArguments aArguments;
					aArguments[0] = m_pPrimitiveRenderer.Get();
					(void)pMethod->TryInvoke(
						thisPointer,
						aArguments);
				}
			}
		}

		// TODO: Break this out into a function.
		Axis eGridAxis = Axis::kY;
		const Matrix4D& mProjection = m_pCamera->GetProjectionMatrix();
		if (!mProjection.IsPerspective())
		{
			Vector3D const vView = m_pCamera->GetViewAxis();
				 if (vView.X >  0.5f) { eGridAxis = Axis::kX; /* Left */ }
			else if (vView.X < -0.5f) { eGridAxis = Axis::kX; /* Right */ }
			else if (vView.Y >  0.5f) { eGridAxis = Axis::kY; /* Bottom */ }
			else if (vView.Y < -0.5f) { eGridAxis = Axis::kY; /* Top */ }
			else if (vView.Z >  0.5f) { eGridAxis = Axis::kZ; /* Back */ }
			else if (vView.Z < -0.5f) { eGridAxis = Axis::kZ; /* Front */ }
		}

		InternalRenderWorldGrid(eGridAxis);
	}
	Bool const bReturn = InternalRenderTransformGizmo(viewport, pRoot, bPicking);

	pPrimitiveRenderer->EndFrame();

	return bReturn;
}

void SceneRenderer::InternalRenderWorldGrid(Axis eAxis)
{
	static const ColorARGBu8 kcColor(ColorARGBu8::White());
	static const HString kaEffectTechniques[] =
	{
		HString("seoul_RenderGridYZ"),
		HString("seoul_RenderGridXZ"),
		HString("seoul_RenderGridXY"),
	};

	// TODO: Needs to be big enough to cover the visible area,
	// but not too big or it will introduce precision errors during
	// sampling (which exhibit as grid rendering artifacts at oblique angles).
	static const Float32 kfDimension = 500.0f;

	Vector3D const vCameraPosition = m_pCamera->GetPosition();
	Vector3D vGridCenter = Vector3D::Round(vCameraPosition);

	// Zero out the perpendicular component of grid.
	vGridCenter[(Int)eAxis] = 0.0f;

	// Compute the "x" axis of the grid - perpendicular to eAxis.
	Int32 const iAxis0 = ((Int32)eAxis + 2) % 3;

	// Initial corners.
	Vector3D v0(-kfDimension);
	Vector3D v1(kfDimension);
	Vector3D v2(-kfDimension);
	Vector3D v3(kfDimension);

	// Zero out the primary axis.
	v0[(Int)eAxis] = 0.0f;
	v1[(Int)eAxis] = 0.0f;
	v2[(Int)eAxis] = 0.0f;
	v3[(Int)eAxis] = 0.0f;

	// Negate off axes.
	v1[iAxis0] = -v1[iAxis0];
	v2[iAxis0] = -v2[iAxis0];

	// Offset all corners by the grid center.
	v0 += vGridCenter;
	v1 += vGridCenter;
	v2 += vGridCenter;
	v3 += vGridCenter;

	// Switch to the grid technique, render, then restore default.
	m_pPrimitiveRenderer->UseEffectTechnique(kaEffectTechniques[(Int)eAxis]);
	m_pPrimitiveRenderer->UseDepthBias(Scene::kfPrimitiveRendererDepthBias + Scene::kfPrimitiveRendererDepthBias);
	m_pPrimitiveRenderer->TriangleQuad(v0, v1, v2, v3, kcColor);
	m_pPrimitiveRenderer->UseDepthBias();
	m_pPrimitiveRenderer->UseEffectTechnique();
}

Bool SceneRenderer::InternalRenderTransformGizmo(
	const Viewport& viewport,
	IControllerSceneRoot* pRoot,
	Bool bPicking)
{
	// Configure the gizmo prior to pick or render.
	{
		const IControllerSceneRoot::SelectedObjects& tSelectedObjects = pRoot->GetSelectedObjects();
		if (tSelectedObjects.IsEmpty())
		{
			// Early out.
			return false;
		}

		// TODO: Use better logic for what transform we use as the gizmo target.
		// Some ideas:
		// - mean
		// - last selected, although this is not very intuitive on a "select-all"
		// - user specified.
		auto pObject(pRoot->GetLastSelection().IsValid()
			? pRoot->GetLastSelection()
			: *tSelectedObjects.Begin());
		auto pMesh(pObject->GetComponent<Scene::MeshDrawComponent>());

		auto const eActiveHandle = (m_PickState.m_CurrentPick.m_eType == CurrentPick::kHandle
			? m_PickState.m_CurrentPick.m_eHandle
			: TransformGizmoHandle::kNone);
		m_pTransformGizmo->SetHoveredHandle(eActiveHandle);
		m_pTransformGizmo->SetTransform(
			Transform(
				(pMesh.IsValid() ? pMesh->GetScale() : Vector3D::One()),
				pObject->GetRotation(),
				pObject->GetPosition()));
		m_pTransformGizmo->SetEnabled(
			(m_pTransformGizmo->GetMode() == TransformGizmoMode::kRotation && pObject->CanSetTransform()) ||
			(m_pTransformGizmo->GetMode() == TransformGizmoMode::kScale && pMesh.IsValid()) ||
			(m_pTransformGizmo->GetMode() == TransformGizmoMode::kTranslation && pObject->CanSetTransform()));
	}

	// Now pick or render the gizmo.
	if (bPicking)
	{
		m_pTransformGizmo->Pick(TransformGizmo::CameraState(*m_pCamera, viewport), *m_pPrimitiveRenderer);
	}
	else
	{
		m_pTransformGizmo->Render(TransformGizmo::CameraState(*m_pCamera, viewport), *m_pPrimitiveRenderer);
	}
	return true;
}

void SceneRenderer::OnPick(const SharedPtr<Scene::Object>& pObject)
{
	SEOUL_ASSERT(IsMainThread());

	m_PickState.m_CurrentPick.m_eHandle = TransformGizmoHandle::kNone;
	m_PickState.m_CurrentPick.m_pObject = pObject;
	m_PickState.m_CurrentPick.m_eType = CurrentPick::kObject;
}

void SceneRenderer::OnPick(TransformGizmoHandle eHandle)
{
	SEOUL_ASSERT(IsMainThread());

	m_PickState.m_CurrentPick.m_eHandle = eHandle;
	m_PickState.m_CurrentPick.m_pObject.Reset();
	m_PickState.m_CurrentPick.m_eType = CurrentPick::kHandle;
}

void SceneRenderer::OnPickNone()
{
	SEOUL_ASSERT(IsMainThread());

	m_PickState.m_CurrentPick.m_eHandle = TransformGizmoHandle::kNone;
	m_PickState.m_CurrentPick.m_pObject.Reset();
	m_PickState.m_CurrentPick.m_eType = CurrentPick::kNone;
}

} // namespace EditorUI

// NOTE: Assignment here is necessary to convince the linker in our GCC toolchain
// to include this definition. Otherwise, it strips it.
template <>
AtomicHandleTableCommon::Data EditorUI::SceneRendererHandleTable::s_Data = AtomicHandleTableCommon::Data();

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
