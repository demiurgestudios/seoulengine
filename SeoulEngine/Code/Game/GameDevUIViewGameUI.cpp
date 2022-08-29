/**
 * \file GameDevUIViewGameUI.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ApplicationJson.h"
#include "Camera.h"
#include "DevUIImGui.h"
#include "DevUIRoot.h"
#include "DevUIUtil.h"
#include "GameDevUIViewGameUI.h"
#include "Engine.h"
#include "FalconHitTester.h"
#include "FalconMovieClipInstance.h"
#include "FileManager.h"
#include "FxManager.h"
#include "GamePaths.h"
#include "ImageWrite.h"
#include "ReflectionDefine.h"
#include "RenderCommandStreamBuilder.h"
#include "RenderDevice.h"
#include "RenderPass.h"
#include "ScopedAction.h"
#include "UIManager.h"
#include "UIMovie.h"
#include "UIRenderer.h"

#if SEOUL_ENABLE_DEV_UI

namespace Seoul
{

static const Float kfNegativeInverseZoomFactor = 0.05f;
static const Float kfPositiveInverseZoomFactor = 0.05f;
static const Float kfMinInverseZoom = 0.05f;
static const Float kfMaxInverseZoom = 4.0f;

SEOUL_BEGIN_TYPE(Game::DevUIViewGameUI, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(DisplayName, "Game UI")
	SEOUL_PARENT(DevUI::View)
SEOUL_END_TYPE()

namespace Game
{

DevUIViewGameUI::DevUIViewGameUI()
	: m_hSelected()
	, m_pSelected(nullptr)
	, m_vPendingScreenshots()
{
}

DevUIViewGameUI::~DevUIViewGameUI()
{
}

extern Viewport g_GameUIRootViewportInDevUI;
static Viewport s_FullViewport;

static inline Viewport ClipViewport(const Vector2D& vMin, const Vector2D& vMax, Float fWindowScale, const Viewport& fullViewport)
{
	return Viewport::Create(
		fullViewport.m_iTargetWidth,
		fullViewport.m_iTargetHeight,
		fullViewport.m_iViewportX + Min((Int32)(vMin.X / fWindowScale), fullViewport.m_iViewportWidth),
		fullViewport.m_iViewportY + Min((Int32)(vMin.Y / fWindowScale), fullViewport.m_iViewportHeight),
		Min((Int32)((vMax.X - vMin.X) / fWindowScale), fullViewport.m_iViewportWidth),
		Min((Int32)((vMax.Y - vMin.Y) / fWindowScale), fullViewport.m_iViewportHeight));
}

static inline Viewport ClipViewport(const ImVec2& vMin, const ImVec2& vMax, Float fWindowScale, const Viewport& fullViewport)
{
	return ClipViewport(Convert(vMin), Convert(vMax), fWindowScale, fullViewport);
}

/**
 * @return true if the ImGui mouse position is currently over
 * the game's viewport.
 */
Bool DevUIViewGameUI::HoverGameView() const
{
	using namespace ImGui;

	auto const& io = GetIO();

	Byte const* sName = nullptr;
	Bool bInClientArea = false;
	if (WillWantCaptureMousePos(io.MousePos, &sName, &bInClientArea) &&
		bInClientArea && /* vs. overlapping window resize handle, etc. */
		nullptr != sName &&
		HString(sName) == GetId())
	{
		// Special handling for buttons, etc.
		if (IsAnyItemHovered())
		{
			return false;
		}

		// Possible - at the very least, the current mouse position would trigger
		// a capture to game UI.
		return true;
	}

	return false;
}

void DevUIViewGameUI::RenderUIManager(const ImDrawList* pParentList, const ImDrawCmd* pCommand, void* /*pUnusedContext*/)
{
	// Acquire.
	auto& rPass = *((RenderPass*)pCommand->UserCallbackData);
	auto& rBuilder = *rPass.GetRenderCommandStreamBuilder();
	auto const& vImGuiInnerRectMin = DevUIViewGameUI::Get()->m_vImGuiInnerRectMin;
	auto const& vImGuiInnerRectMax = DevUIViewGameUI::Get()->m_vImGuiInnerRectMax;

	// Compute viewport.
	Viewport const fullViewport(rBuilder.GetCurrentViewport());

	if (!DevUI::Root::Get()->IsVirtualizedDesktop())
	{
		g_GameUIRootViewportInDevUI = fullViewport;
		s_FullViewport = fullViewport;
	}
	else
	{
		auto const fWindowScale = DevUI::Root::Get()->GetWindowScale();

		auto const viewport(ClipViewport(vImGuiInnerRectMin, vImGuiInnerRectMax, fWindowScale, fullViewport));

		// Protect against zero sized rendering.
		if (!(viewport.m_iTargetHeight > 0 &&
			viewport.m_iTargetWidth > 0 &&
			viewport.m_iViewportHeight > 0 &&
			viewport.m_iViewportWidth > 0))
		{
			UI::Manager::Get()->SkipPose(Engine::Get()->GetSecondsInTick());
			return;
		}

		// Update viewport.
		g_GameUIRootViewportInDevUI = viewport;
		s_FullViewport = fullViewport;
	}

	// Pass-through to UIManager.
	UI::Manager::Get()->PassThroughPose(Engine::Get()->GetSecondsInTick(), rPass);

	// If a screenshot is pending, perform it now before we grab the UI.
	if (DevUIViewGameUI::Get())
	{
		if (!DevUIViewGameUI::Get()->m_vPendingScreenshots.IsEmpty())
		{
			auto const config = DevUIViewGameUI::Get()->m_vPendingScreenshots.Front();
			DevUIViewGameUI::Get()->m_vPendingScreenshots.Erase(DevUIViewGameUI::Get()->m_vPendingScreenshots.Begin());

			DevUIViewGameUI::Get()->InternalTakeScreenshot(rBuilder, config);
		}
	}

	// Restore viewport after drawing UIManager.
	rBuilder.SetCurrentViewport(fullViewport);
	rBuilder.SetScissor(true, fullViewport);
}

void DevUIViewGameUI::PreBegin()
{
	// Full window.
	if (!DevUI::Root::Get()->IsVirtualizedDesktop())
	{
		using namespace ImGui;

		// Cache rescale factor.
		auto const fWindowScale = DevUI::Root::Get()->GetWindowScale();

		auto const viewport = RenderDevice::Get()->GetBackBufferViewport();
		Float32 const fWidth = viewport.m_iViewportWidth * fWindowScale;
		Float32 const fHeight = viewport.m_iViewportHeight * fWindowScale;

		// Force to the entire viewport.
		SetNextWindowContentSize(ImVec2(fWidth, fHeight));
		SetNextWindowPos(ImVec2(0, 0));
		SetNextWindowSize(ImVec2(fWidth, fHeight));
		SetNextWindowBgAlpha(0.0f);

		// Also make sure we're on the bottom of the draw stack
		// when operating as a full screen window.
		SetNextWindowBringToDisplayBack();

		PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	}
}

void DevUIViewGameUI::PostEnd()
{
	// Full window.
	if (!DevUI::Root::Get()->IsVirtualizedDesktop())
	{
		using namespace ImGui;

		PopStyleVar();
	}
}

Bool DevUIViewGameUI::OnMouseButton(InputDevice* pInputDevice, InputButton eButtonID, ButtonEventType eEventType)
{
	// TODO: More coupling than I'd like between UI explorer and
	// GameDevUIViewGameUI. This supports right-click to select (picking).
	if (InputButton::MouseRightButton == eButtonID)
	{
		return false;
	}

	// On mouse down, check if this will be a hit into the game
	// view. If so, return pass through to UI::Manager and report
	// true.
	//
	// In all other cases, just pass through and report true.
	if (ButtonEventType::ButtonReleased != eEventType)
	{
		if (HoverGameView())
		{
			(void)UI::Manager::Get()->PassThroughButtonEvent(pInputDevice, eButtonID, eEventType);
			return true;
		}
	}
	else
	{
		(void)UI::Manager::Get()->PassThroughButtonEvent(pInputDevice, eButtonID, eEventType);
		return true;
	}

	return false;
}

void DevUIViewGameUI::OnMouseMove(Int iX, Int iY, Bool bWillCapture)
{
	if (bWillCapture)
	{
		// Pass through.
		UI::Manager::Get()->PassThroughMouseMoveEvent(iX, iY);
	}
}

Bool DevUIViewGameUI::OnMouseWheel(InputDevice* pInputDevice, InputDevice::Axis* pAxis)
{
	{
		// Check for and handle mouse wheel.
		FxPreviewModeState state;
		if (FxManager::Get()->GetFxPreviewModeState(state) && state.m_bActive &&
			HoverGameView())
		{
			auto fZoomAxis = (pAxis->GetRawState() > 0.0f ? 1.0f : -1.0f);
			if (fZoomAxis != 0.0f)
			{
				Float fInverseZoom = UI::Manager::Get()->GetRenderer().GetFxCameraInverseZoom();
				if (fInverseZoom < 1.0f || (fInverseZoom == 1.0f && fZoomAxis < 0.0f))
				{
					fZoomAxis *= kfNegativeInverseZoomFactor;
				}
				else if (fInverseZoom > 1.0f || (fInverseZoom == 1.0f && fZoomAxis > 0.0f))
				{
					fZoomAxis *= kfPositiveInverseZoomFactor;
				}

				fInverseZoom -= fZoomAxis;
				fInverseZoom = Clamp(fInverseZoom, kfMinInverseZoom, kfMaxInverseZoom);

				// Sanitizing so we get back to 1.0f.
				if (Equals(fInverseZoom, 1.0f, 1e-2f))
				{
					fInverseZoom = 1.0f;
				}

				UI::Manager::Get()->GetRenderer().SetFxCameraInverseZoom(fInverseZoom);
			}

			return true;
		}
	}

	// Check if this will be a hit into the game view and if so,
	// pass through and return true.
	if (HoverGameView())
	{
		UI::Manager::Get()->PassThroughAxisEvent(pInputDevice, pAxis);
		return true;
	}

	return false;
}

void DevUIViewGameUI::DoPrePose(DevUI::Controller& rController, RenderPass& rPass)
{
	// Cache before draw.
	m_vImGuiInnerRectMin = Convert(ImGui::GetWindowInnerRectMin());
	m_vImGuiInnerRectMax = Convert(ImGui::GetWindowInnerRectMax());

	// Custom draw callback.
	ImDrawList* pDrawList = ImGui::GetWindowDrawList();
	pDrawList->AddCallback(&RenderUIManager, &rPass);
	DrawSelection();

	// Fx preview.
	InternalPrePoseFxState();
}

void DevUIViewGameUI::DoSkipPose(DevUI::Controller& rController, RenderPass& rPass)
{
	UI::Manager::Get()->SkipPose(Engine::Get()->GetSecondsInTick());
}

UInt32 DevUIViewGameUI::GetFlags() const
{
	// Full screen.
	if (!DevUI::Root::Get()->IsVirtualizedDesktop())
	{
		return (
			ImGuiWindowFlags_NoNavFocus |
			ImGuiWindowFlags_NoBackground |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings);
	}
	else
	{
		return (ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	}
}

Bool DevUIViewGameUI::GetInitialPosition(Vector2D& rv) const
{
	// Full screen.
	if (!DevUI::Root::Get()->IsVirtualizedDesktop())
	{
		rv = Vector2D::Zero();
		return true;
	}
	else
	{
		return DevUI::View::GetInitialPosition(rv);
	}
}

Vector2D DevUIViewGameUI::GetInitialSize() const
{
	// Full screen.
	if (!DevUI::Root::Get()->IsVirtualizedDesktop())
	{
		auto const viewport = RenderDevice::Get()->GetBackBufferViewport();
		return Vector2D((Float)viewport.m_iViewportWidth, (Float)viewport.m_iViewportHeight);
	}
	else
	{
		return DevUI::View::GetInitialSize();
	}
}

/** Derive the full 3D depth for the given movie clip. */
static inline Pair<Float, Int> ComputeDepth3D(const Falcon::Instance& instance)
{
	Float fReturn = 0.0;
	Int iReturn = 0;
	auto p = &instance;
	while (nullptr != p)
	{
		fReturn += p->GetDepth3D();
		iReturn += p->GetIgnoreDepthProjection() ? 1 : 0;
		p = p->GetParent();
	}
	return MakePair(fReturn, iReturn);
}

static inline Vector2D Rescale(const Vector4D& vRescale, const ImVec2& innerMin, const Vector2D& v)
{
	return Vector2D(
		(v.X + vRescale.X) * vRescale.Z + innerMin.x,
		(v.Y + vRescale.Y) * vRescale.W + innerMin.y);
}

/** Our WindowPrep function handles draw of the selection box, if an item is selected. */
void DevUIViewGameUI::DrawSelection()
{
	// Check selection, clear if unique.
	if (m_pSelected.IsUnique())
	{
		m_pSelected.Reset();
		m_hSelected.Reset();
	}

	// Nothing to do if no selection.
	if (!m_pSelected.IsValid())
	{
		return;
	}

	// Selected movie.
	auto pSelectedMovie = GetPtr(m_hSelected);
	if (nullptr == pSelectedMovie)
	{
		return;
	}

	// Compute bounds - if this fails, early out.
	Falcon::Rectangle bounds;
	if (!m_pSelected->ComputeLocalBounds(bounds))
	{
		return;
	}

	// Viewport world bounds.
	auto const fWindowScale = DevUI::Root::Get()->GetWindowScale();
	auto const innerMin = ImGui::GetWindowInnerRectMin() / fWindowScale;
	auto const innerMax = ImGui::GetWindowInnerRectMax() / fWindowScale;
	auto const lastMovieViewport = pSelectedMovie->GetLastViewport();
	auto const clipViewport = Viewport::Create(
		lastMovieViewport.m_iTargetWidth,
		lastMovieViewport.m_iTargetHeight,
		(Int32)innerMin.x,
		(Int32)innerMin.y,
		(Int32)(innerMax.x - innerMin.x),
		(Int32)(innerMax.y - innerMin.y));
	auto const clipViewportWorldBounds = pSelectedMovie->ViewportToWorldBounds(clipViewport);

	// Render a quad on the selection.
	auto tester = pSelectedMovie->GetHitTester();
	auto const computed = ComputeDepth3D(*m_pSelected);
	tester.PushDepth3D(computed.First, (computed.Second != 0));
	auto const mWorld = m_pSelected->ComputeWorldTransform();

	// Compute rescale factors.
	Vector4D const vRescale(
		-clipViewportWorldBounds.m_fLeft,
		-clipViewportWorldBounds.m_fTop,
		(Float)clipViewport.m_iViewportWidth / clipViewportWorldBounds.GetWidth(),
		(Float)clipViewport.m_iViewportHeight / clipViewportWorldBounds.GetHeight());

	// Compute world rect to rescale positions.
	auto const a = fWindowScale * Rescale(vRescale, innerMin, tester.DepthProject(Matrix2x3::TransformPosition(mWorld, Vector2D(bounds.m_fLeft, bounds.m_fTop))));
	auto const b = fWindowScale * Rescale(vRescale, innerMin, tester.DepthProject(Matrix2x3::TransformPosition(mWorld, Vector2D(bounds.m_fRight, bounds.m_fTop))));
	auto const c = fWindowScale * Rescale(vRescale, innerMin, tester.DepthProject(Matrix2x3::TransformPosition(mWorld, Vector2D(bounds.m_fRight, bounds.m_fBottom))));
	auto const d = fWindowScale * Rescale(vRescale, innerMin, tester.DepthProject(Matrix2x3::TransformPosition(mWorld, Vector2D(bounds.m_fLeft, bounds.m_fBottom))));

	ImGui::GetWindowDrawList()->AddQuadFilled(
		ImVec2(a.X, a.Y),
		ImVec2(b.X, b.Y),
		ImVec2(c.X, c.Y),
		ImVec2(d.X, d.Y),
		ImGui::GetColorU32(ImGuiCol_TextSelectedBg, 0.25f));
	ImGui::GetWindowDrawList()->AddQuad(
		ImVec2(a.X, a.Y),
		ImVec2(b.X, b.Y),
		ImVec2(c.X, c.Y),
		ImVec2(d.X, d.Y),
		ImGui::GetColorU32(ImGuiCol_Text),
		2.0f);
}

namespace
{

static Atomic32 s_ScreenshotGrabCount;

class ScreenshotGrab SEOUL_SEALED : public IGrabFrame
{
public:
	ScreenshotGrab(const DevUI::Config::ScreenshotConfig& config, Float fAspect)
		: m_Config(config)
		, m_fAspect(fAspect)
	{
	}

	virtual void OnGrabFrame(
		UInt32 uFrame,
		const SharedPtr<IFrameData>& pFrameData,
		Bool bSuccess) SEOUL_OVERRIDE
	{
		if (!bSuccess)
		{
			return;
		}

		String sDirectory(GamePaths::Get()->GetVideosDir());
		if (sDirectory.IsEmpty())
		{
			return;
		}
		sDirectory = Path::Combine(Path::GetDirectoryName(sDirectory, 2), "Screenshots");

		{
			static const HString ksApplicationName("ApplicationName");

			String sBaseName("Screenshot");
			(void)GetApplicationJsonValue(ksApplicationName, sBaseName);

			(void)FileManager::Get()->CreateDirPath(sDirectory);

			auto sPath(Path::Combine(sDirectory, sBaseName + ".png"));
			if (m_Config.m_bDedup)
			{
				sPath = Path::Combine(sDirectory, String::Printf("%s - %03u.png", sBaseName.CStr(), uFrame));
				while (FileManager::Get()->Exists(sPath))
				{
					sPath = Path::Combine(sDirectory, String::Printf("%s - %03u.png", sBaseName.CStr(), uFrame++));
				}
			}

			ScopedPtr<SyncFile> pFile;
			if (!FileManager::Get()->OpenFile(sPath, File::kWriteTruncate, pFile))
			{
				return;
			}

			auto const iHeight = (Int32)pFrameData->GetFrameHeight();
			auto const iPitch = (Int32)pFrameData->GetPitch();
			if (iHeight < 1)
			{
				return;
			}
			auto const iWidth = (Int32)pFrameData->GetFrameWidth();
			auto const iComponents = 4;
			// Must be tight to the capture rectangle, may not have iPitch worth padded for the last row.
			auto const iSize = (iHeight - 1) * iPitch + (iComponents * iWidth);
			if (iSize <= 0)
			{
				return;
			}
			void* p = nullptr;

			auto const scope(MakeScopedAction(
				[&]()
			{
				p = MemoryManager::Allocate(iSize, MemoryBudgets::Rendering);
				memcpy(p, pFrameData->GetData(), iSize);
			},
				[&]()
			{
				MemoryManager::Deallocate(p);
			}));

			auto const bSwapRB = !PixelFormatIsRGB(pFrameData->GetPixelFormat());

			// Fill the alpha channel, and swap RB if necessary.
			{
				for (Int y = 0; y < iHeight; ++y)
				{
					for (Int x = 0; x < iWidth; ++x)
					{
						auto pE = (UInt8*)p + (y * iPitch) + (x * iComponents);
						pE[3] = 255;
						if (bSwapRB) { Swap(pE[0], pE[2]); }
					}
				}
			}

			Int32 iOutHeight = m_Config.m_iTargetHeight;
			if (iOutHeight <= 0)
			{
				if (!ImageWritePng(
					iWidth,
					iHeight,
					4,
					p,
					iPitch,
					*pFile))
				{
					return;
				}
			}
			else
			{
				Int32 const iOutWidth = (Int32)(iOutHeight * m_fAspect);
				if (!ImageResizeAndWritePng(
					iWidth,
					iHeight,
					4,
					p,
					iPitch,
					iOutWidth,
					iOutHeight,
					*pFile))
				{
					return;
				}
			}
		}
	}

private:
	SEOUL_DISABLE_COPY(ScreenshotGrab);
	SEOUL_REFERENCE_COUNTED_SUBCLASS(ScreenshotGrab);

	DevUI::Config::ScreenshotConfig const m_Config;
	Float const m_fAspect;
}; // class ScreenshotGrab

} // namespace anonymous

static Vector2D FxWorldToWindow(const Vector3D& vFxWorld)
{
	// Values.
	auto const fZoom = UI::Manager::Get()->GetRenderer().GetFxCameraInverseZoom();
	auto const vOffset = UI::Manager::Get()->GetRenderer().GetFxCameraOffset();

	// Cache half the world height.
	auto const fWindowScale = DevUI::Root::Get()->GetWindowScale();
	auto const viewport = (DevUI::GetDevUIConfig().m_FxPreviewConfig.m_bShowGame
		? UI::Manager::Get()->ComputeViewport()
		: g_GameUIRootViewportInDevUI);
	Float const fHalfHeight = 0.5f * fZoom * UI::Manager::Get()->ComputeUIRendererFxCameraWorldHeight(viewport);
	Float const fHalfWidth = fHalfHeight * viewport.GetViewportAspectRatio();

	// Base camera position.
	auto const vBasePosition = vOffset + Vector3D(0, 0, UI::kfUIRendererFxCameraWorldDistance);

	// Projection.
	auto const vPos = (vFxWorld - vBasePosition);
	auto const innerViewport(ClipViewport(ImGui::GetWindowInnerRectMin(), ImGui::GetWindowInnerRectMax(), fWindowScale, s_FullViewport));
	auto const innerMin = ImVec2((Float)innerViewport.m_iViewportX, (Float)innerViewport.m_iViewportY);
	auto const innerMax = ImVec2(innerMin.x + (Float)innerViewport.m_iViewportWidth, innerMin.y + (Float)innerViewport.m_iViewportHeight);
	auto fAlphaX = (vPos.X / fHalfWidth) * 0.5f + 0.5f;
	auto fAlphaY = (vPos.Y / fHalfHeight) * -0.5f + 0.5f;

	// When Fx rendering is showing the game, we need to match the aspect
	// ratio of the game, which means the Fx area is potentially smaller
	// than the full window clip area.
	if (DevUI::GetDevUIConfig().m_FxPreviewConfig.m_bShowGame)
	{
		auto const rootViewport = g_GameUIRootViewportInDevUI;

		// Scale and shift to account for the subset of the subset.
		fAlphaX *= ((Float)viewport.m_iViewportWidth / (Float)rootViewport.m_iViewportWidth);
		fAlphaX += ((Float)(viewport.m_iViewportX - rootViewport.m_iViewportX) / (Float)rootViewport.m_iViewportWidth);
		fAlphaY *= ((Float)viewport.m_iViewportHeight / (Float)rootViewport.m_iViewportHeight);
		fAlphaY += ((Float)(viewport.m_iViewportY - rootViewport.m_iViewportY) / (Float)rootViewport.m_iViewportHeight);
	}

	// Value - this may need to be adjusted depending on whether the viewport
	// be used for Fx renderer matches the full viewport or not.
	return Vector2D(
		(innerMin.x + fAlphaX * (innerMax.x - innerMin.x)) * fWindowScale,
		(innerMin.y + fAlphaY * (innerMax.y - innerMin.y)) * fWindowScale);
}

static Vector3D ScreenToFxWorld(const Vector2D& vScreen)
{
	// Values.
	auto const fZoom = UI::Manager::Get()->GetRenderer().GetFxCameraInverseZoom();
	auto const vOffset = UI::Manager::Get()->GetRenderer().GetFxCameraOffset();

	// Cache half the world height.
	auto const viewport = (DevUI::GetDevUIConfig().m_FxPreviewConfig.m_bShowGame
		? UI::Manager::Get()->ComputeViewport()
		: g_GameUIRootViewportInDevUI);
	Float const fHalfHeight = 0.5f * fZoom * UI::Manager::Get()->ComputeUIRendererFxCameraWorldHeight(viewport);
	Float const fHalfWidth = fHalfHeight * viewport.GetViewportAspectRatio();

	// Window boundary.
	auto const fWindowScale = DevUI::Root::Get()->GetWindowScale();
	auto const innerViewport(ClipViewport(ImGui::GetWindowInnerRectMin(), ImGui::GetWindowInnerRectMax(), fWindowScale, s_FullViewport));
	auto const innerMin = ImVec2((Float)innerViewport.m_iViewportX, (Float)innerViewport.m_iViewportY);
	auto const innerMax = ImVec2(innerMin.x + (Float)innerViewport.m_iViewportWidth, innerMin.y + (Float)innerViewport.m_iViewportHeight);

	// When Fx rendering is showing the game, we need to match the aspect
	// ratio of the game, which means the Fx area is potentially smaller
	// than the full window clip area.
	auto fOffsetX = innerMin.x;
	auto fOffsetY = innerMin.y;
	auto fScaleX = (innerMax.x - innerMin.x);
	auto fScaleY = (innerMax.y - innerMin.y);
	if (DevUI::GetDevUIConfig().m_FxPreviewConfig.m_bShowGame)
	{
		fOffsetX = (Float)viewport.m_iViewportX;
		fOffsetY = (Float)viewport.m_iViewportY;
		fScaleX = (Float)viewport.m_iViewportWidth;
		fScaleY = (Float)viewport.m_iViewportHeight;
	}

	// Projection.
	auto const vWorldX = Vector2D(
		((((vScreen.X - fOffsetX) / fScaleX) - 0.5f) * 2.0f) * fHalfWidth,
		((((vScreen.Y - fOffsetY) / fScaleY) - 0.5f) * -2.0f) * fHalfHeight);

	return Vector3D(vWorldX, 0.0f);
}

static Vector3D MouseToFxWorld(const Vector2D& vWindow)
{
	return ScreenToFxWorld(vWindow / DevUI::Root::Get()->GetMouseScale());
}

void DevUIViewGameUI::InternalPrePoseFxState()
{
	using namespace ImGui;

	static const Float kfRadius = 40.0f;
	static const Float kfWidth = 2.0f;
	static const ImColor kvRed(1.0f, 0.0f, 0.0f, 1.0f);
	static const ImColor kvGreen(0.0f, 1.0f, 0.0f, 1.0f);
	static const ImColor kvBlue(0.0f, 0.0f, 1.0f, 1.0f);

	FxPreviewModeState state;
	if (FxManager::Get()->GetFxPreviewModeState(state) && state.m_bActive)
	{
		// Check for and handle mouse movement.
		if (IsMouseClicked(1))
		{
			m_vStartFxOffset = UI::Manager::Get()->GetRenderer().GetFxCameraOffset();
		}
		else if (IsMouseDragging(1))
		{
			auto const vStart = MouseToFxWorld(Vector2D(
				GetIO().MouseClickedPos[1].x,
				GetIO().MouseClickedPos[1].y));
			auto const vEnd = MouseToFxWorld(Vector2D(
				GetIO().MousePos.x,
				GetIO().MousePos.y));

			UI::Manager::Get()->GetRenderer().SetFxCameraOffset(m_vStartFxOffset - (vEnd - vStart));
		}

		// Draw the preview position.
		{
			// Access.
			auto const fWindowScale = DevUI::Root::Get()->GetWindowScale();
			auto drawList = ImGui::GetWindowDrawList();

			auto const vWindow = FxWorldToWindow(state.m_vPosition);
			auto const fRadius = (kfRadius * fWindowScale);
			auto const vCenter = ImVec2(vWindow.X, vWindow.Y);
			auto const vCenterCorner = ImVec2(vWindow.X + kfWidth, vWindow.Y + kfWidth);
			auto const vX0(vWindow + Vector2D(kfWidth, 0));
			auto const vX1(vWindow + Vector2D(fRadius, 0));
			auto const vY0(vWindow - Vector2D(0, kfWidth)); // Y is up in FX space.
			auto const vY1(vWindow - Vector2D(0, fRadius)); // Y is up in FX space.
			drawList->AddLine(ImVec2(vX0.X, vX0.Y), ImVec2(vX1.X, vX1.Y), kvRed, kfWidth);
			drawList->AddLine(ImVec2(vY0.X, vY0.Y), ImVec2(vY1.X, vY1.Y), kvGreen, kfWidth);
			drawList->AddRectFilled(vCenter, vCenterCorner, kvBlue, ImDrawFlags_None);
		}

		// Avoid the main menu bar.
		if (!DevUI::Root::Get()->IsVirtualizedDesktop() &&
			DevUI::Root::Get()->IsMainMenuVisible())
		{
			NewLineEx(GetMainMenuBarHeight());
		}

		// Controls.
		if (Button("Reset Preview Camera"))
		{
			UI::Manager::Get()->GetRenderer().SetFxCameraInverseZoom(1.0f);
			UI::Manager::Get()->GetRenderer().SetFxCameraOffset(Vector3D::Zero());
		}
		SameLine();
		if (Checkbox("Show Game", &DevUI::GetDevUIConfig().m_FxPreviewConfig.m_bShowGame))
		{
			(void)DevUI::SaveDevUIConfig();
		}

		// Draw the zoom state.
		auto const fInverseZoom = UI::Manager::Get()->GetRenderer().GetFxCameraInverseZoom();
		auto const fZoomPct = Round((1.0f / fInverseZoom) * 100.0f);
		if (fZoomPct != 100.0f)
		{
			SameLine();
			Text(" Zoom: %.2f%%", fZoomPct);
		}
	}
}

void DevUIViewGameUI::InternalTakeScreenshot(RenderCommandStreamBuilder& rBuilder, const DevUI::Config::ScreenshotConfig& config)
{
	auto const viewport = UI::Manager::Get()->ComputeViewport();
	auto const rect = Rectangle2DInt(
		viewport.m_iViewportX,
		viewport.m_iViewportY,
		viewport.m_iViewportX + viewport.m_iViewportWidth,
		viewport.m_iViewportY + viewport.m_iViewportHeight);

	auto const vFixed = UI::Manager::Get()->GetFixedAspectRatio();
	auto const fAspect = (vFixed.IsZero()
		? viewport.GetViewportAspectRatio()
		: (vFixed.X / vFixed.Y));

	auto const uCount = (UInt32)(++s_ScreenshotGrabCount);
	SharedPtr<ScreenshotGrab> pCallback(SEOUL_NEW(MemoryBudgets::DevUI) ScreenshotGrab(config, fAspect));
	rBuilder.GrabBackBufferFrame(uCount, rect, pCallback);
}

} // namespace Game

} // namespace Seoul

#endif // /SEOUL_ENABLE_DEV_UI
