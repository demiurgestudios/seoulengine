/**
 * \file OGLES2RenderDevice.cpp
 * \brief Specialization of RenderDevice for OGLES2.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnalyticsManager.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "JobsFunction.h"
#include "GLSLFXLite.h"
#include "HashSet.h"
#include "JobsManager.h"
#include "Logger.h"
#include "Platform.h"
#include "OGLES2DepthStencilSurface.h"
#include "OGLES2Effect.h"
#include "OGLES2IndexBuffer.h"
#include "OGLES2RenderCommandStreamBuilder.h"
#include "OGLES2RenderDevice.h"
#include "OGLES2RenderTarget.h"
#include "OGLES2Texture.h"
#include "OGLES2Util.h"
#include "OGLES2VertexBuffer.h"
#include "OGLES2VertexFormat.h"
#include "PlatformData.h"
#include "ReflectionDataStoreTableUtil.h"
#include "ReflectionUtil.h"
#include "ScopedAction.h"
#include "SettingsManager.h"
#include "SeoulFile.h"
#include "StringUtil.h"
#include "ToString.h"

#if SEOUL_PLATFORM_ANDROID
#include <android/native_window.h>
#include <EGL/egl.h>
#endif

namespace Seoul
{

#if SEOUL_PLATFORM_ANDROID
/** Android constants for the hardware scalar. */
static const HString ksAndroidMaxBackBufferHeight("AndroidMaxBackBufferHeight");
static const HString ksApplication("Application");

/** Tracking counter for system vertical syncs. */
static Atomic32 s_AndroidNativeVsyncCounter(0);
static Atomic32 s_AndroidNativeVsyncInterval(0);
static Signal s_AndroidNativeVsync;

/** Window inset handling. */
static Atomic32 s_AndroidWindowInsetTop(0);
static Atomic32 s_AndroidWindowInsetBottom(0);

void AndroidNativeOnVsync()
{
	/** Native vsync callback hook. */
	using namespace Seoul;

	// Increment the counter.
	auto const vsyncCounter = ++s_AndroidNativeVsyncCounter;

	// Apply interval.
	auto const vsyncInterval = s_AndroidNativeVsyncInterval;

	// 0 interval, just reset the counter immediately.
	// > 0, reset if we've reached the interval.
	if (0 == vsyncInterval || vsyncCounter >= vsyncInterval)
	{
		s_AndroidNativeVsyncCounter.Reset();
		s_AndroidNativeVsync.Activate();
	}
}

/** Reporting of window inset changes. */
void AndroidNativeOnWindowInsets(Int iTop, Int iBottom)
{
	s_AndroidWindowInsetTop.Set((Atomic32Type)iTop);
	s_AndroidWindowInsetBottom.Set((Atomic32Type)iBottom);
}

OGLES2WindowHandlePtr::OGLES2WindowHandlePtr()
	: m_p(nullptr)
{
}

OGLES2WindowHandlePtr::OGLES2WindowHandlePtr(ANativeWindow* p)
	: m_p(p)
{
	if (nullptr != m_p)
	{
		SEOUL_ASSERT(IsRenderThread());
		ANativeWindow_acquire(m_p);
	}
}

OGLES2WindowHandlePtr::OGLES2WindowHandlePtr(const OGLES2WindowHandlePtr& b)
	: m_p(b.m_p)
{
	if (nullptr != m_p)
	{
		SEOUL_ASSERT(IsRenderThread());
		ANativeWindow_acquire(m_p);
	}
}

OGLES2WindowHandlePtr& OGLES2WindowHandlePtr::operator=(const OGLES2WindowHandlePtr& b)
{
	Reset(b.GetPtr());
	return *this;
}

OGLES2WindowHandlePtr::~OGLES2WindowHandlePtr()
{
	auto p = m_p;
	m_p = nullptr;

	if (nullptr != p)
	{
		SEOUL_ASSERT(IsRenderThread());
		ANativeWindow_release(p);
	}
}

Bool OGLES2WindowHandlePtr::IsValid() const
{
	// Simple case, no pointer.
	if (nullptr == m_p)
	{
		return false;
	}

	// Extra checking, also appears in some official
	// sample implementations (e.g. https://github.com/NVIDIAGameWorks/GraphicsSamples/blob/master/extensions/src/NvAppBase/android/NvAndroidNativeAppGlue.c),
	// the window can be invalided on a different thread,
	// so we query dimensions to sanity check on the render thread.
	SEOUL_ASSERT(IsRenderThread());
	return ANativeWindow_getWidth(m_p) > 0 && ANativeWindow_getHeight(m_p) > 0;
}

void OGLES2WindowHandlePtr::Reset(ANativeWindow* pIn /* = nullptr*/)
{
	// Easy case, nop.
	if (pIn == m_p)
	{
		return;
	}

	// Cache current locally.
	auto p = m_p;
	m_p = nullptr;

	// Decrement the existing pointer.
	if (p)
	{
		SEOUL_ASSERT(IsRenderThread());
		ANativeWindow_release(p);
	}

	// Update.
	p = pIn;

	// Increment the new pointer.
	if (p)
	{
		SEOUL_ASSERT(IsRenderThread());
		ANativeWindow_acquire(p);
	}

	// Assign the new pointer.
	m_p = p;
}

#if SEOUL_LOGGING_ENABLED
static inline Byte const* EGLGetErrorString(EGLint eError)
{
	switch (eError)
	{
#	define CASE(val) case EGL_##val: return #val
	CASE(SUCCESS);
	CASE(NOT_INITIALIZED);
	CASE(BAD_ACCESS);
	CASE(BAD_ALLOC);
	CASE(BAD_ATTRIBUTE);
	CASE(BAD_CONTEXT);
	CASE(BAD_CONFIG);
	CASE(BAD_CURRENT_SURFACE);
	CASE(BAD_DISPLAY);
	CASE(BAD_SURFACE);
	CASE(BAD_MATCH);
	CASE(BAD_PARAMETER);
	CASE(BAD_NATIVE_PIXMAP);
	CASE(BAD_NATIVE_WINDOW);
	CASE(CONTEXT_LOST);
#	undef CASE
	default:
		return "UNKNOWN";
	};
}
#endif // /SEOUL_LOGGING_ENABLED

#endif // /#if SEOUL_PLATFORM_ANDROID

// Object-C implemented functions that are used to setup the render context and other
// operations on iOS.
#if SEOUL_PLATFORM_IOS

Bool InitializeEAGLContext(Bool& rbSupportsES3, Bool &rbSupportsAsyncTextureCreate);
void DeinitializeEAGLContext();
Bool EAGLSwapBuffers(GLint colorBuffer);
void InitializeEAGLBackBufferColorBuffer(void* pLayer);
Bool MakeAsyncContextActive();
void MakeAsyncContextInactive();
Bool RestoreEAGLContext();

// Equivalent functions implemented on ANdroid.
#elif SEOUL_PLATFORM_ANDROID

Bool InitializeEAGLContext(ANativeWindow* pMainWindow, EGLDisplay display, EGLConfig config, Bool& rbSupportsES3, Bool &rbSupportsAsyncTextureCreate);
void DeinitializeEAGLContext(EGLDisplay display);
Bool EAGLSwapBuffers(EGLDisplay display, EGLSurface surface);
Bool MakeAsyncContextActive(EGLDisplay display, EGLConfig config);
void MakeAsyncContextInactive(EGLDisplay display);
Bool MakeEAGLContextActive(EGLDisplay display, EGLSurface surface);
void MakeEAGLContextInactive(EGLDisplay display);

#endif

/** Handle platform specific differences W.R.T. acquiring extension proc addresses. */
static void* GetExtensionProcAddress(Byte const* sName)
{
#if SEOUL_PLATFORM_ANDROID
	return (void*)eglGetProcAddress(sName);
#elif SEOUL_PLATFORM_IOS
	// Extension addresses are just included.
	     if (0 == strcmp(sName, "glPopGroupMarkerEXT")) { return (void*)glPopGroupMarkerEXT; }
	else if (0 == strcmp(sName, "glPushGroupMarkerEXT")) { return (void*)glPushGroupMarkerEXT; }
	else
	{
		return nullptr;
	}
#else
#	error "Define for this platform."
#endif
}

#if SEOUL_PLATFORM_ANDROID
OGLES2RenderDevice::OGLES2RenderDevice(ANativeWindow* pMainWindow, const RefreshRate& refreshRate, Bool bDesireBGRA)
#elif SEOUL_PLATFORM_IOS
OGLES2RenderDevice::OGLES2RenderDevice(void* pLayer)
#else
OGLES2RenderDevice::OGLES2RenderDevice()
#endif
	: m_ReportOnce()
	, m_glPopGroupMarkerEXT(nullptr)
	, m_glPushGroupMarkerEXT(nullptr)
	, m_OGLES2StateManager()
	, m_vpVertexFormats()
#if SEOUL_PLATFORM_ANDROID
	, m_HardwareScalarState()
	, m_pMainWindow(pMainWindow)
	, m_pPendingMainWindow(pMainWindow)
	, m_Display(EGL_NO_DISPLAY)
	, m_Config(0)
	, m_Surface(EGL_NO_SURFACE)
	, m_iNativeVisualID(0)
	, m_iNativeVisualType(0)
#endif // /#if SEOUL_PLATFORM_ANDROID
#if SEOUL_PLATFORM_IOS
	, m_BackBufferColorBuffer(0)
	, m_pLayer(pLayer)
#endif
	, m_BackBufferViewport()
#if SEOUL_PLATFORM_ANDROID
	, m_RefreshRate(refreshRate)
#else
	, m_RefreshRate()
#endif
	, m_OnePixelWhiteTexture(0u)
	, m_bHasFrameToPresent(false)
	, m_bInScene(false)
	, m_bRecalculateBackBufferViewport(false)
	, m_bSupportsES3(false)
	, m_bSupportsAsyncTextureCreate(false)
	, m_bHasContext(false)
	, m_bInBackground(false)
{
	SEOUL_ASSERT(IsRenderThread());

	// Defaults - may be overriden by platform specific logic.
	m_eBackBufferDepthStencilFormat = DepthStencilFormat::kD24S8;
	m_eBackBufferPixelFormat = PixelFormat::kA8B8G8R8;

#if SEOUL_PLATFORM_IOS
	m_eCompatible32bit4colorRenderTargetFormat = PixelFormat::kA8R8G8B8;

	// Value is true - all iOS devices support BGRA.
	m_Caps.m_bBGRA = true;
#else
	m_eCompatible32bit4colorRenderTargetFormat = PixelFormat::kA8B8G8R8;

	// Initial value is based on bDesireBGRA - this is used to override
	// BGRA support on some devices that lie about supporting it.
	m_Caps.m_bBGRA = bDesireBGRA;
#endif

	InternalInitializeOpenGl();

	m_Caps.m_bBackBufferWithAlpha = PixelFormatHasAlpha(m_eBackBufferPixelFormat);
}

OGLES2RenderDevice::~OGLES2RenderDevice()
{
	SEOUL_ASSERT(IsRenderThread());
	SEOUL_ASSERT(!m_bInScene);

	m_vpVertexFormats.Clear(); SEOUL_TEARDOWN_TRACE();

	SEOUL_VERIFY(InternalDestructorMaintenance()); SEOUL_TEARDOWN_TRACE();

	InternalShutdownOpenGl(); SEOUL_TEARDOWN_TRACE();

	// Cleanup on shutdown.
#if SEOUL_PLATFORM_ANDROID
	s_AndroidWindowInsetBottom.Reset(); SEOUL_TEARDOWN_TRACE();
	s_AndroidWindowInsetTop.Reset(); SEOUL_TEARDOWN_TRACE();
#endif
}

RenderCommandStreamBuilder* OGLES2RenderDevice::CreateRenderCommandStreamBuilder(UInt32 zInitialCapacity /* = 0u */) const
{
	return SEOUL_NEW(MemoryBudgets::Rendering) OGLES2RenderCommandStreamBuilder(zInitialCapacity);
}

Bool OGLES2RenderDevice::BeginScene()
{
	SEOUL_ASSERT(IsRenderThread());
	SEOUL_ASSERT(!m_bInScene);

	// Immediately exit if in the background.
	//
	// Also, BeginScene() may be encountered from within
	// the present context if we are within
	// a background redraw operation. Fail the BeginScene()
	// immediately in this case.
	if (m_bInBackground || m_bInPresent)
	{
		return false;
	}

	// On iOS, make sure the EAGL context is active for the next scene.
#if SEOUL_PLATFORM_IOS
	if (!RestoreEAGLContext())
	{
		return false;
	}
#endif // /#if SEOUL_PLATFORM_IOS

#if SEOUL_PLATFORM_ANDROID
	// Can't render if we're suspended and don't have a surface
	// to render to.
	if (!IsReset())
	{
		InternalDoReset();

		if (!IsReset())
		{
			return false;
		}
	}
#endif // /#if SEOUL_PLATFORM_ANDROID

	if (!InternalPerFrameMaintenance())
	{
		return false;
	}

	// Restore the active viewport to the default.
	SEOUL_OGLES2_VERIFY(glDepthRangef(0.0f, 1.0f));

	// Set the default scissor and viewport values.
	m_OGLES2StateManager.SetScissor(
		m_BackBufferViewport.m_iViewportX,
		m_BackBufferViewport.m_iViewportY,
		m_BackBufferViewport.m_iViewportWidth,
		m_BackBufferViewport.m_iViewportHeight);
	m_OGLES2StateManager.SetViewport(
		m_BackBufferViewport.m_iViewportX,
		m_BackBufferViewport.m_iViewportY,
		m_BackBufferViewport.m_iViewportWidth,
		m_BackBufferViewport.m_iViewportHeight);

	// Make scissor and viewport dirty so they must commit the next
	// time a commit is required.
	m_OGLES2StateManager.MarkScissorRectangleDirty();
	m_OGLES2StateManager.MarkViewportRectangleDirty();

	m_bInScene = true;
	return true;
}

void OGLES2RenderDevice::EndScene()
{
	SEOUL_ASSERT(IsRenderThread());
	SEOUL_ASSERT(m_bInScene);

	// No longer in the scene.
	m_bInScene = false;

	// Check before calling any more gl functions -
	// if the device context has already been lost,
	// or if we've entered the background, stop
	// calling gl functions:
	// - on iOS, calling a gl function in this state
	//   can trigger an assertion: https://developer.apple.com/library/content/documentation/3DDrawing/Conceptual/OpenGLES_ProgrammingGuide/ImplementingaMultitasking-awareOpenGLESApplication/ImplementingaMultitasking-awareOpenGLESApplication.html#//apple_ref/doc/uid/TP40008793-CH5-SW5
	// - unpredictable behavior on Android - sometimes
	//   a crash, sometimes handled gracefully by the driver,
	//   but (almost) never valid.
	if (!IsReset() || m_bInBackground)
	{
		return;
	}

	// Perform the flush.
	glFlush();

	// Present here on iOS and Android.
	if (m_bHasFrameToPresent)
	{
		auto const eResult = InternalPresent();

		// If failed or interrupted, handle.
		if (PresentResult::kSuccess != eResult)
		{
			// Android specific handling around a failed present
			// (vs. an interrupted present).
#if SEOUL_PLATFORM_ANDROID
			// A failure is unexpected and we want to warn about it.
			if (PresentResult::kFailure == eResult)
			{
				auto const eError = eglGetError();
				(void)eError;

				// Report for vetting later.
				SEOUL_LOG_RENDER("[OGLES2RenderDevice]: EndScene failed to present: %s", EGLGetErrorString(eError));

				// Device lost on present failure.
				InternalDoLost();
			}
#endif // /#if SEOUL_PLATFORM_ANDROID

			// Return on both platforms in this case.
			return;
		}
	}

	// Update the viewport if requested.
	InternalRecalculateBackBufferViewport();

#if SEOUL_PLATFORM_ANDROID
	// According to NVidia docs, we need to do this, because on some devices, the normal triggers
	// for a changed window may not report the correct size at the time of the trigger.
	// Update the viewport from what Android reports.
	{
		Int iWidth = 0;
		Int iHeight = 0;
		if (EGL_FALSE != eglQuerySurface(m_Display, m_Surface, EGL_WIDTH, &iWidth) &&
			EGL_FALSE != eglQuerySurface(m_Display, m_Surface, EGL_HEIGHT, &iHeight) &&
			iWidth > 0 &&
			iHeight > 0)
		{
			// Update.
			if (iWidth != m_GraphicsParameters.m_iWindowViewportWidth ||
				iHeight != m_GraphicsParameters.m_iWindowViewportHeight)
			{
				SEOUL_LOG_RENDER("Viewport dimensions changed from (%d x %d) to (%d x %d)",
					m_GraphicsParameters.m_iWindowViewportWidth,
					m_GraphicsParameters.m_iWindowViewportHeight,
					iWidth,
					iHeight);

				m_GraphicsParameters.m_iWindowViewportWidth = iWidth;
				m_GraphicsParameters.m_iWindowViewportHeight = iHeight;
			}
		}
	}

	// Handle situation mentioned above, or a pending main window change.
	if (m_pPendingMainWindow != m_pMainWindow ||
		m_GraphicsParameters.m_iWindowViewportWidth != m_BackBufferViewport.m_iTargetWidth ||
		m_GraphicsParameters.m_iWindowViewportHeight != m_BackBufferViewport.m_iTargetHeight)
	{
		UpdateWindow(m_pPendingMainWindow.GetPtr());
	}
#endif
}

/**
 * @return The dimensions and settings of the back buffer. On OGLES2,
 * this does not change once the game has started.
 */
const Viewport& OGLES2RenderDevice::GetBackBufferViewport() const
{
	return m_BackBufferViewport;
}

/**
 * @return The screen refresh rate in hertz. On some platforms,
 * this value may change with changes to the display device or window.
 */
RefreshRate OGLES2RenderDevice::GetDisplayRefreshRate() const
{
	return m_RefreshRate;
}

/**
 * Return a VertexFormat object that is described by pElements.
 *
 * Unlike all other graphics objects, OGLES2RenderDevice owns the returned
 * object. Do not call SEOUL_DELETE on the returned object.
 */
SharedPtr<VertexFormat> OGLES2RenderDevice::CreateVertexFormat(
	VertexElement const* pElements)
{
	Lock lock(m_VertexFormatsMutex);

	// First calculate the size of the vertex format - this size does
	// not include the null terminator.
	UInt32 zSize = 0u;
	while (pElements[zSize] != VertexElementEnd)
	{
		++zSize;
	}

	UInt32 const nVertexFormatCount = m_vpVertexFormats.GetSize();

	// Linearly search through the current array of vertex formats -
	// if an existing format matches the format described by pElements,
	// just use the existing format instead of creating a new one.
	for (UInt32 i = 0u; i < nVertexFormatCount; ++i)
	{
		SEOUL_ASSERT(m_vpVertexFormats[i].IsValid());
		const VertexFormat::VertexElements& vElements =
			m_vpVertexFormats[i]->GetVertexElements();

		// If sizes match, then the formats can potentially be equal.
		if (vElements.GetSize() == zSize)
		{
			// Walk the list of elements and check if each element is equal.
			Bool bResult = true;
			for (UInt32 j = 0; j < zSize; ++j)
			{
				if (vElements[j] != pElements[j])
				{
					bResult = false;
					break;
				}
			}

			// If all elements were equal, use the vertex format at ith.
			if (bResult)
			{
				return m_vpVertexFormats[i];
			}
		}
	}

	// No existing format fulfills the definition of pElements, so we need
	// to create a new VertexFormat.
	SharedPtr<OGLES2VertexFormat> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) OGLES2VertexFormat(pElements));
	InternalAddObject(pReturn);
	m_vpVertexFormats.PushBack(pReturn);

	// Return the newly created vertex format.
	return m_vpVertexFormats.Back();
}

/**
 * Instantiate a new OGLES2DepthStencilSurface. The caller
 * takes ownership of the returned object and must call SEOUL_DELETE
 * on the returned object when done with it.
 */
SharedPtr<DepthStencilSurface> OGLES2RenderDevice::CreateDepthStencilSurface(
	const DataStoreTableUtil& configSettings)
{
	SharedPtr<DepthStencilSurface> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) OGLES2DepthStencilSurface(configSettings));
	InternalAddObject(pReturn);
	return pReturn;
}

/**
 * Instantiate a new OGLES2RenderTarget. The caller
 * takes ownership of the returned object and must call SEOUL_DELETE
 * on the returned object when done with it.
 */
SharedPtr<RenderTarget> OGLES2RenderDevice::CreateRenderTarget(
	const DataStoreTableUtil& configSettings)
{
	SharedPtr<RenderTarget> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) OGLES2RenderTarget(configSettings));
	InternalAddObject(pReturn);
	return pReturn;
}

/**
 * Set pSurface as the depth-stencil surface that will be used
 * for rendering. Must only be called by OGLES2DepthStencilSurface.
 */
void OGLES2RenderDevice::SetDepthStencilSurface(OGLES2DepthStencilSurface const* pSurface)
{
	SEOUL_ASSERT(IsRenderThread());

	m_bCurrentRenderSurfaceIsDirty = true;

	// If setting a nullptr surface, disable the depth-stencil surface.
	if (!pSurface)
	{
		m_CurrentRenderSurface.m_Depth = 0;
		m_CurrentRenderSurface.m_Stencil = 0;
	}
	else
	{
		m_CurrentRenderSurface.m_Depth = pSurface->m_DepthSurface;
		m_CurrentRenderSurface.m_Stencil = pSurface->m_StencilSurface;
	}
}

/**
 * Set pTarget as the color surface that will be used
 * for rendering. Must only be called by OGLES2RenderTarget.
 */
void OGLES2RenderDevice::SetRenderTarget(OGLES2RenderTarget const* pRenderTarget)
{
	SEOUL_ASSERT(IsRenderThread());

	m_bCurrentRenderSurfaceIsDirty = true;

	if (pRenderTarget)
	{
		m_CurrentRenderSurface.m_RenderTarget = pRenderTarget->m_TextureA;
	}
	else
	{
		m_CurrentRenderSurface.m_RenderTarget = 0;
	}
}

/**
 * Commits any changes to the depth-stencil targets
 * and color targets to the OpenGL API.
 */
void OGLES2RenderDevice::CommitRenderSurface()
{
	SEOUL_ASSERT(IsRenderThread());

	// If the render surface has changes since the last
	// commit, update those changes.
	if (m_bCurrentRenderSurfaceIsDirty)
	{
		if (m_CurrentRenderSurface.m_RenderTarget == 0)
		{
			// iOS - the back buffer is just the framebuffer with a special
			// renderbuffer color buffer.
#if SEOUL_PLATFORM_IOS
			SEOUL_OGLES2_VERIFY(glBindFramebuffer(GL_FRAMEBUFFER, m_CurrentRenderSurface.m_Framebuffer));
			SEOUL_OGLES2_VERIFY(glFramebufferRenderbuffer(
				GL_FRAMEBUFFER,
				GL_DEPTH_ATTACHMENT,
				GL_RENDERBUFFER,
				m_CurrentRenderSurface.m_Depth));
			SEOUL_OGLES2_VERIFY(glFramebufferRenderbuffer(
				GL_FRAMEBUFFER,
				GL_STENCIL_ATTACHMENT,
				GL_RENDERBUFFER,
				m_CurrentRenderSurface.m_Stencil));

			SEOUL_OGLES2_VERIFY(glFramebufferRenderbuffer(
				GL_FRAMEBUFFER,
				GL_COLOR_ATTACHMENT0,
				GL_RENDERBUFFER,
				m_BackBufferColorBuffer));
			// On Android, we use "no framebuffer" to target the back buffer.
#else
			// IMPORTANT: In the glDeleteRenderbuffers spec: "If a renderbuffer object
			// that is currently bound is deleted, the binding reverts to 0 (the absence
			// of any renderbuffer object). Additionally, special care must be taken when
			// deleting a renderbuffer object if the image of the renderbuffer is attached
			// to a framebuffer object. In this case, if the deleted renderbuffer object is
			// attached to the currently bound framebuffer object, it is automatically
			// detached.  However, attachments to any other framebuffer objects are the
			// responsibility of the application."
			//
			// Basically, the results are undefined if glDeleteRenderbuffers is called on
			// a renderbuffer that is associated with a framebuffer object which is not the currently
			// bound framebuffer object. On Adreno hardware, this undefined behavior is a memory
			// leak (the render buffer storage associated with deleted render buffers is
			// apparently never released).
			//
			// So, we NEED to make sure that if we are unbinding our framebuffer object, that we first
			// unbind any render buffer storage from it. We do the same with color render textures,
			// just to be thorough.
			SEOUL_OGLES2_VERIFY(glBindFramebuffer(GL_FRAMEBUFFER, m_CurrentRenderSurface.m_Framebuffer));
			SEOUL_OGLES2_VERIFY(glFramebufferRenderbuffer(
				GL_FRAMEBUFFER,
				GL_DEPTH_ATTACHMENT,
				GL_RENDERBUFFER,
				0));
			SEOUL_OGLES2_VERIFY(glFramebufferRenderbuffer(
				GL_FRAMEBUFFER,
				GL_STENCIL_ATTACHMENT,
				GL_RENDERBUFFER,
				0));
			SEOUL_OGLES2_VERIFY(glFramebufferTexture2D(
				GL_FRAMEBUFFER,
				GL_COLOR_ATTACHMENT0,
				GL_TEXTURE_2D,
				0,
				0));

			// Unbind the framebuffer to use the back buffer.
			SEOUL_OGLES2_VERIFY(glBindFramebuffer(GL_FRAMEBUFFER, 0));
#endif
		}
		// On all OGLES2 platforms, use the framebuffer with desired render buffer storage
		// when we have an explicit depth-stencil surface.
		else
		{
			SEOUL_OGLES2_VERIFY(glBindFramebuffer(GL_FRAMEBUFFER, m_CurrentRenderSurface.m_Framebuffer));
			SEOUL_OGLES2_VERIFY(glFramebufferRenderbuffer(
				GL_FRAMEBUFFER,
				GL_DEPTH_ATTACHMENT,
				GL_RENDERBUFFER,
				m_CurrentRenderSurface.m_Depth));
			SEOUL_OGLES2_VERIFY(glFramebufferRenderbuffer(
				GL_FRAMEBUFFER,
				GL_STENCIL_ATTACHMENT,
				GL_RENDERBUFFER,
				m_CurrentRenderSurface.m_Stencil));
			SEOUL_OGLES2_VERIFY(glFramebufferTexture2D(
				GL_FRAMEBUFFER,
				GL_COLOR_ATTACHMENT0,
				GL_TEXTURE_2D,
				m_CurrentRenderSurface.m_RenderTarget,
				0));
		}

		// Sanity check that the frame-buffer was correctly configured.
		SEOUL_ASSERT(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));

		m_bCurrentRenderSurfaceIsDirty = false;
	}
}

#if SEOUL_PLATFORM_ANDROID
/**
 * On Android only, handle a redraw request from
 * the main tick loop
 */
Bool OGLES2RenderDevice::RedrawBegin()
{
	SEOUL_ASSERT(IsRenderThread());
	SEOUL_ASSERT(!m_bInScene);

	// Cannot perform a redraw if we don't have a device,
	// we're in the background, or if we're in the middle
	// of a present.
	if (!IsReset() || m_bInBackground || m_bInPresent)
	{
		return false;
	}

	m_OGLES2StateManager.ApplyDefaultRenderStates();
	m_OGLES2StateManager.SetScissor(0, 0, m_BackBufferViewport.m_iTargetWidth, m_BackBufferViewport.m_iTargetHeight);
	m_OGLES2StateManager.SetViewport(0, 0, m_BackBufferViewport.m_iTargetWidth, m_BackBufferViewport.m_iTargetHeight);
	return true;
}

void OGLES2RenderDevice::RedrawBlack()
{
	SEOUL_ASSERT(IsRenderThread());
	SEOUL_ASSERT(!m_bInScene);

	SEOUL_OGLES2_VERIFY(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
	SEOUL_OGLES2_VERIFY(glClearDepthf(1.0f));
	SEOUL_OGLES2_VERIFY(glClearStencil(0u));
	m_OGLES2StateManager.CommitPendingStates();
	SEOUL_OGLES2_VERIFY(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
	m_bHasFrameToPresent = true;
	(void)InternalPresent();
}

void OGLES2RenderDevice::RedrawEnd(Bool bFinishGl /*= true*/)
{
	SEOUL_ASSERT(IsRenderThread());
	SEOUL_ASSERT(!m_bInScene);

	if (bFinishGl)
	{
		glFinish();
	}
}

/**
 * Android only method used to update the window
 * surface on a suspend/resume event.
 */
void OGLES2RenderDevice::UpdateWindow(ANativeWindow* pMainWindow)
{
	SEOUL_ASSERT(IsRenderThread());
	SEOUL_ASSERT(!m_bInScene);

	// Pending is now always up-to-date.
	m_pPendingMainWindow.Reset(pMainWindow);

	// Always perform the reset unless pMainWindow is nullptr and
	// the m_pMainWindow member is also nullptr.
	if (pMainWindow != m_pMainWindow || pMainWindow != nullptr)
	{
		// If in the background (some devices don't kill the window
		// until after stop), wake up temporarily.
		auto const bInBackground = m_bInBackground;
		if (bInBackground) { InternalRenderThreadLeaveBackground(); }

		InternalDoLost();
		m_pMainWindow.Reset(pMainWindow);
		InternalDoReset();

		if (bInBackground) { InternalRenderThreadEnterBackground(); }
	}
}
#endif // /#if SEOUL_PLATFORM_ANDROID

/**
 * Instantiate a new OGLES2IndexBuffer. The caller
 * takes ownership of the returned object and must call SEOUL_DELETE
 * on the returned object when done with it.
 */
SharedPtr<IndexBuffer> OGLES2RenderDevice::CreateIndexBuffer(
	void const* pInitialData,
	UInt32 zInitialDataSizeInBytes,
	UInt32 zTotalSizeInBytes,
	IndexBufferDataFormat eFormat)
{
	SharedPtr<IndexBuffer> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) OGLES2IndexBuffer(
		pInitialData,
		zInitialDataSizeInBytes,
		zTotalSizeInBytes,
		eFormat,
		false));
	InternalAddObject(pReturn);
	return pReturn;
}


SharedPtr<IndexBuffer> OGLES2RenderDevice::CreateDynamicIndexBuffer(
	UInt32 zTotalSizeInBytes,
	IndexBufferDataFormat eFormat)
{
	SharedPtr<IndexBuffer> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) OGLES2IndexBuffer(
		nullptr,
		0u,
		zTotalSizeInBytes,
		eFormat,
		true));
	InternalAddObject(pReturn);
	return pReturn;
}

/**
 * Instantiate a new OGLES2VertexBuffer. The caller
 * takes ownership of the returned object and must call SEOUL_DELETE
 * on the returned object when done with it.
 */
SharedPtr<VertexBuffer> OGLES2RenderDevice::CreateVertexBuffer(
	void const* pInitialData,
	UInt32 zInitialDataSizeInBytes,
	UInt32 zTotalSizeInBytes,
	UInt32 zStrideInBytes)
{
	SharedPtr<VertexBuffer> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) OGLES2VertexBuffer(
		pInitialData,
		zInitialDataSizeInBytes,
		zTotalSizeInBytes,
		zStrideInBytes,
		false));
	InternalAddObject(pReturn);
	return pReturn;
}

SharedPtr<VertexBuffer> OGLES2RenderDevice::CreateDynamicVertexBuffer(
	UInt32 zTotalSizeInBytes,
	UInt32 zStrideInBytes)
{
	SharedPtr<VertexBuffer> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) OGLES2VertexBuffer(
		nullptr,
		0u,
		zTotalSizeInBytes,
		zStrideInBytes,
		true));
	InternalAddObject(pReturn);
	return pReturn;
}

/** Compute the estimated total memory size of the data. */
static UInt32 GetGpuMemorySizeInBytes(const TextureData& data, UInt32 uWidth, UInt32 uHeight, PixelFormat eFormat)
{
	auto uSize = 0u;
	for (UInt32 i = 0u; i < data.GetSize(); ++i)
	{
		auto uLevelSize = GetDataSizeForPixelFormat(uWidth, uHeight, eFormat);
		if (data.HasSecondary())
		{
			uLevelSize *= 2u;
		}

		uSize += uLevelSize;
		uWidth >>= 1u;
		uHeight >>= 1u;
	}

	return uSize;
}

SharedPtr<BaseTexture> OGLES2RenderDevice::AsyncCreateTexture(
	const TextureConfig& config,
	const TextureData& data,
	UInt uWidth,
	UInt uHeight,
	PixelFormat eFormat)
{
	// This becomes a normal CreateTexture() if on the render thread.
	if (IsRenderThread())
	{
		return CreateTexture(
			config,
			data,
			uWidth,
			uHeight,
			eFormat);
	}

	// Early out if not supported.
	if (!m_bSupportsAsyncTextureCreate)
	{
		return SharedPtr<BaseTexture>();
	}

	// Must make the async context for this thread active.
#if SEOUL_PLATFORM_IOS
	if (!MakeAsyncContextActive())
#elif SEOUL_PLATFORM_ANDROID
	if (!MakeAsyncContextActive(m_Display, m_Config))
#else
#	error "Implement for this platform."
#endif
	{
		return SharedPtr<BaseTexture>();
	}

	// Instantiate the texture.
	SharedPtr<BaseTexture> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) OGLES2Texture(
		config,
		data,
		uWidth,
		uHeight,
		eFormat,
		GetGpuMemorySizeInBytes(data, uWidth, uHeight, eFormat),
		false,
		true));

	// Block and wait (glFinish) for the command queue every time, we're a worker thread for creation and we want
	// to ensure proper synchronization with the render thread at the CPU level.
	glFinish();

	// Release the context.
#if SEOUL_PLATFORM_IOS
	MakeAsyncContextInactive();
#elif SEOUL_PLATFORM_ANDROID
	MakeAsyncContextInactive(m_Display);
#else
#	error "Implement for this platform."
#endif

	InternalAddObject(pReturn);
	return pReturn;
}

/**
 * Instantiate a new OGLES2Texture. The caller
 * takes ownership of the returned object and must call SEOUL_DELETE
 * on the returned object when done with it.
 */
SharedPtr<BaseTexture> OGLES2RenderDevice::CreateTexture(
	const TextureConfig& config,
	const TextureData& data,
	UInt uWidth,
	UInt uHeight,
	PixelFormat eFormat)
{
	SharedPtr<BaseTexture> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) OGLES2Texture(
		config,
		data,
		uWidth,
		uHeight,
		eFormat,
		GetGpuMemorySizeInBytes(data, uWidth, uHeight, eFormat),
		false,
		false));
	InternalAddObject(pReturn);
	return pReturn;
}

/**
 * Instantiate a new Effect instance from raw effect data.
 */
SharedPtr<Effect> OGLES2RenderDevice::CreateEffectFromFileInMemory(
	FilePath filePath,
	void* pRawEffectFileData,
	UInt32 zFileSizeInBytes)
{
	SharedPtr<Effect> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) OGLES2Effect(filePath, pRawEffectFileData, zFileSizeInBytes));
	InternalAddObject(pReturn);
	return pReturn;
}

/**
 * For some platforms, implements specific handling in the render system
 * on enter/exit background (on mobile devices, when the app becomes inactive,
 * it has entered the background).
 */
void OGLES2RenderDevice::OnEnterBackground()
{
	// Log for testing and debug tracking.
	SEOUL_LOG("OGLES2RenderDevice::OnEnterBackground()");

	// Set this immediately, since there an be a race
	// if a frame present has already been enqueued on the render thread.
	m_bPresentInterrupt = true;

	// This is synchronous for two reasons:
	// - the Jobs::Manager will be put to sleep immediately after the call
	//   to OnEnterBackground() and as such, this dispatch to the render thread
	//   will not be delivered until after we *return* from the background unless
	//   we call it synchronously here.
	// - on iOS, a crash will occur (in gpus_ReturnNotPermittedKillClient) if
	//   we interact with any OpenGL functions after background enter. As such,
	//   we must make sure that the render device has entered the background state
	//   before returning.
	Jobs::AwaitFunction(
		GetRenderThreadId(),
		SEOUL_BIND_DELEGATE(&OGLES2RenderDevice::InternalRenderThreadEnterBackground, this));
}

void OGLES2RenderDevice::OnLeaveBackground()
{
	// Log for testing and debug tracking.
	SEOUL_LOG("OGLES2RenderDevice::OnLeaveBackground()");

	// Perform synchronously so that we don't risk loss of
	// expected sequencing.
	Jobs::AwaitFunction(
		GetRenderThreadId(),
		SEOUL_BIND_DELEGATE(&OGLES2RenderDevice::InternalRenderThreadLeaveBackground, this));
}

/**
 * Must be called once per frame - performs that actual flip
 * of the back buffer to the video hardware.
 *
 * If configured as such, this method may block to match the
 * vertical refresh.
 *
 * @return True if the present succeeds, false otherwise.
 */
OGLES2RenderDevice::PresentResult OGLES2RenderDevice::InternalPresent()
{
	SEOUL_ASSERT(IsRenderThread());
	SEOUL_ASSERT(!m_bInScene);
	SEOUL_ASSERT(!m_bInPresent);

	// Scope present to this body.
	auto const scoped(MakeScopedAction(
	[&]()
	{
		m_bInPresent = true;
	},
	[&]()
	{
		m_bInPresent = false;
	}));

	m_bHasFrameToPresent = false;
	m_bPresentInterrupt = false;

#if SEOUL_PLATFORM_ANDROID
	// Commit the vsync interval.
	if (m_iDesiredVsyncInterval != m_GraphicsParameters.m_iVsyncInterval)
	{
		m_GraphicsParameters.m_iVsyncInterval = m_iDesiredVsyncInterval;
		s_AndroidNativeVsyncInterval.Set((Atomic32Type)m_GraphicsParameters.m_iVsyncInterval);
	}
#endif // /#if SEOUL_PLATFORM_ANDROID

	InternalPrePresent();

	// Sanity check that any yielded to jobs from pre-present
	// did not start a new scene.
	SEOUL_ASSERT(!m_bInScene);

	// If a present interruption occurred in pre present,
	// return now.
	if (m_bPresentInterrupt)
	{
		m_bPresentInterrupt = false;
		return PresentResult::kInterrupted;
	}

#if SEOUL_PLATFORM_ANDROID
	// Sanitizing value - this is the max threshold we will use
	// to wait for vsync (7 FPS, in short).
	static const Double kfMaxInvervalMs = (8.0 / 60.0) * 1000.0;

	Bool const bReturn = (EAGLSwapBuffers(m_Display, m_Surface));

	// Wait for vsync if enabled. We set a timeout
	// to handle cases where a vsync may be mismatched with game
	// state, to avoid deadlock (e.g. background/foreground).
	if (m_GraphicsParameters.m_iVsyncInterval > 0)
	{
		auto const displayRefresh = GetDisplayRefreshRate();
		if (!displayRefresh.IsZero())
		{
			// Compute target interval.
			auto const fDisplayHz = displayRefresh.ToHz();
			auto const fTargetHz = (fDisplayHz / (Double)m_GraphicsParameters.m_iVsyncInterval);
			auto const fIntervalMs = (1000.0 / fTargetHz);

			// We expand the interval to avoid prematurely
			// breaking out of the signal. The timeout is
			// purely to avoid deadlock in special
			// cases, we otherwise want to wait for vsync.
			auto const fToleranceIntervalMs = Clamp(
				(fIntervalMs + fIntervalMs * 0.5),
				0.0,
				kfMaxInvervalMs);

			// Compute remaining time.
			auto const fCurrentMs = SeoulTime::ConvertTicksToMilliseconds(SeoulTime::GetGameTimeInTicks() - GetPresentMarkerInTicks());

			// Apply - sanitize - if we somehow
			// have a negative current, just continue on.
			if (fCurrentMs >= 0.0 && fCurrentMs < fToleranceIntervalMs)
			{
				auto const uTimeout = (UInt32)((Int32)(fToleranceIntervalMs - fCurrentMs));

				// Last sanity check after truncation - don't want to wait for a very long time,
				// as that can effectively deadlock the game.
				if (uTimeout > 0u &&
					(Double)uTimeout < fToleranceIntervalMs)
				{
					(void)s_AndroidNativeVsync.Wait(uTimeout);
				}
			}
		}
	}
#elif SEOUL_PLATFORM_IOS
	Bool const bReturn = (EAGLSwapBuffers(m_BackBufferColorBuffer));
#else
#	error "Define for this platform."
#endif

	InternalPostPresent();

	return (bReturn ? PresentResult::kSuccess : PresentResult::kFailure);
}

#if SEOUL_PLATFORM_ANDROID
namespace
{

// Local structure used to lookup current config values that we
// care about.
struct Attributes SEOUL_SEALED
{
	Attributes()
		: m_iRed(-1)
		, m_iGreen(-1)
		, m_iBlue(-1)
		, m_iAlpha(-1)
		, m_iDepth(-1)
		, m_iStencil(-1)
		, m_iSample(-1)
		, m_iSurfaceType(-1)
		, m_iRenderableType(-1)
	{
	}

	/** Check for minimum requirements - attribute sets that do not meet these cannot be used in any circumstances. */
	Bool MeetsMinimumRequirements() const
	{
		// Reject a config based on base requirements.
		if ((m_iAlpha < 5) ||
			(m_iRed < 5) ||
			(m_iGreen < 6) ||
			(m_iBlue < 5) ||
			(m_iDepth < 16) ||
			(m_iSample != 0) ||
			(m_iSurfaceType & EGL_WINDOW_BIT) == 0 ||
			(m_iRenderableType & EGL_OPENGL_ES2_BIT) == 0)
		{
			return false;
		}

		return true;
	}

	/** Populate these attributes from an EGL config. */
	Bool PopulateFrom(EGLDisplay display, EGLConfig config)
	{
		if (EGL_FALSE == eglGetConfigAttrib(display, config, EGL_RED_SIZE, &m_iRed) ||
			EGL_FALSE == eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &m_iGreen) ||
			EGL_FALSE == eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &m_iBlue) ||
			EGL_FALSE == eglGetConfigAttrib(display, config, EGL_ALPHA_SIZE, &m_iAlpha) ||
			EGL_FALSE == eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &m_iDepth) ||
			EGL_FALSE == eglGetConfigAttrib(display, config, EGL_STENCIL_SIZE, &m_iStencil) ||
			EGL_FALSE == eglGetConfigAttrib(display, config, EGL_SAMPLE_BUFFERS, &m_iSample) ||
			EGL_FALSE == eglGetConfigAttrib(display, config, EGL_SURFACE_TYPE, &m_iSurfaceType) ||
			EGL_FALSE == eglGetConfigAttrib(display, config, EGL_RENDERABLE_TYPE, &m_iRenderableType))
		{
			return false;
		}

		return true;
	}

	EGLint m_iRed;
	EGLint m_iGreen;
	EGLint m_iBlue;
	EGLint m_iAlpha;
	EGLint m_iDepth;
	EGLint m_iStencil;
	EGLint m_iSample;
	EGLint m_iSurfaceType;
	EGLint m_iRenderableType;
};

} // namespace anonymous

/**
 * @return The EGLconfig to use for the current context.
 */
static EGLConfig InternalStaticGetBestConfig(
	EGLDisplay display,
	DepthStencilFormat& reDepthStencilFormat,
	PixelFormat& rePixelFormat)
{
	// Get the total number of configs currently available.
	EGLint nNumberOfConfigs = 0;
	SEOUL_VERIFY(EGL_FALSE != eglGetConfigs(display, nullptr, 0, &nNumberOfConfigs));

	// Get all of the configs reported.
	Vector<EGLConfig> vConfigs(nNumberOfConfigs);
	if (!vConfigs.IsEmpty())
	{
		SEOUL_VERIFY(EGL_FALSE != eglGetConfigs(display, vConfigs.Get(0u), vConfigs.GetSize(), &nNumberOfConfigs));
		SEOUL_ASSERT(nNumberOfConfigs == (Int)vConfigs.GetSize());
	}

	// We do config selection in two passes. In the first pass, we're looking
	// for the best config that has 8888 color+alpha channels. This is important -
	// we've had devices (Huawei running version 9 of their OS) that lie about
	// their capabilities - advertising support for 16-bit per channel but then
	// failing silently (to our software - the user experience is what appears to
	// be a hung phone).
	//
	// As such, in the first pass, we look for 8888 configs and then only fall back
	// to a wider search if we can't find a single acceptable 8888 config.
	EGLConfig bestConfig;
	memset(&bestConfig, 0, sizeof(bestConfig));
	Attributes bestAttributes;
	Bool bHasBest = false;

	// Search for 8888 configs only.
	{
		Int iBestConfigScore = 0;
		for (Int i = 0; i < nNumberOfConfigs; ++i)
		{
			EGLConfig config = vConfigs[i];

			// Get all necessary values from the config.
			Attributes attributes;
			if (!attributes.PopulateFrom(display, config))
			{
				// Read of some values failed, skip.
				continue;
			}

			// Reject a config based on base requirements.
			if (!attributes.MeetsMinimumRequirements())
			{
				// Not ever acceptable, skip.
				continue;
			}

			// For this first pass, we only consider options with 8888.
			if ((attributes.m_iAlpha != 8) ||
				(attributes.m_iRed != 8) ||
				(attributes.m_iGreen != 8) ||
				(attributes.m_iBlue != 8))
			{
				// Not a config for consideration in this first pass.
				continue;
			}

			// Now score and potentially merge.
			Int iConfigScore = 0;
			iConfigScore += ((attributes.m_iDepth - 16) / 8);
			iConfigScore += (attributes.m_iStencil);

			// Choose this config if we don't have one, of if it is
			// deemed better than the existing config.
			if (iConfigScore > iBestConfigScore)
			{
				bestConfig = config;
				bestAttributes = attributes;
				iBestConfigScore = iConfigScore;
				bHasBest = true;
			}
		}
	}

	// Fallback - use a wider search that allows color configs of (e.g.) 5551 or 16-16-16-16.
	if (!bHasBest)
	{
		Int iBestConfigScore = 0;
		for (Int i = 0; i < nNumberOfConfigs; ++i)
		{
			EGLConfig config = vConfigs[i];

			// Get all necessary values from the config.
			Attributes attributes;
			if (!attributes.PopulateFrom(display, config))
			{
				// Read of some values failed, skip.
				continue;
			}

			// Reject a config based on base requirements.
			if (!attributes.MeetsMinimumRequirements())
			{
				// Not ever acceptable, skip.
				continue;
			}

			// The best config has higher RGB precision, stencil, alpha, and higher (weighted) depth precision.
			Int iConfigScore = 0;
			iConfigScore += (attributes.m_iRed - 5);
			iConfigScore += (attributes.m_iGreen - 6);
			iConfigScore += (attributes.m_iBlue - 5);
			iConfigScore += ((attributes.m_iDepth - 16) / 8);
			iConfigScore += (attributes.m_iStencil);
			iConfigScore += (attributes.m_iAlpha / 8);

			// Choose this config if we don't have one, of if it is
			// deemed better than the existing config.
			if (iConfigScore > iBestConfigScore)
			{
				bestConfig = config;
				bestAttributes = attributes;
				iBestConfigScore = iConfigScore;
				bHasBest = true;
			}
		}
	}

	// TODO: Probably should call into the default selector in this case.
	SEOUL_ASSERT(bHasBest);

	// TODO: Doing this (perhaps temporarily) to catch config logic errors on our
	// device farm. In general we should support configs other than this.
	if (8 != bestAttributes.m_iRed ||
		8 != bestAttributes.m_iGreen ||
		8 != bestAttributes.m_iBlue ||
		8 != bestAttributes.m_iAlpha ||
		bestAttributes.m_iStencil == 0 ||
		bestAttributes.m_iDepth == 0)
	{
		SEOUL_WARN("[OGLES2RenderDevice]: INVALID CONFIG SELECTED: (%d, %d, %d, %d, %d, %d, %d, %d, %d)",
			bestAttributes.m_iRed,
			bestAttributes.m_iGreen,
			bestAttributes.m_iBlue,
			bestAttributes.m_iAlpha,
			bestAttributes.m_iDepth,
			bestAttributes.m_iStencil,
			bestAttributes.m_iSample,
			bestAttributes.m_iSurfaceType,
			bestAttributes.m_iRenderableType);
	}

	SEOUL_LOG_RENDER("[OGLES2RenderDevice]: BEST CONFIG: (%d, %d, %d, %d, %d, %d, %d, %d, %d)",
		bestAttributes.m_iRed,
		bestAttributes.m_iGreen,
		bestAttributes.m_iBlue,
		bestAttributes.m_iAlpha,
		bestAttributes.m_iDepth,
		bestAttributes.m_iStencil,
		bestAttributes.m_iSample,
		bestAttributes.m_iSurfaceType,
		bestAttributes.m_iRenderableType);

	// Derive the back buffer stencil format.
	switch (bestAttributes.m_iDepth)
	{
	case 24:
		reDepthStencilFormat = (bestAttributes.m_iStencil > 0 ? DepthStencilFormat::kD24S8 : DepthStencilFormat::kD24X8);
		break;
	case 16: // fall-through
		// Any other depth, we fall back to the safe configuration of 16 bit depth.
	default:
		reDepthStencilFormat = (bestAttributes.m_iStencil > 0 ? DepthStencilFormat::kD16S8 : DepthStencilFormat::kD16);
		break;
	};

	// Derive the back buffer pixel format.
	if (bestAttributes.m_iRed < 8 ||
		bestAttributes.m_iGreen < 8 ||
		bestAttributes.m_iBlue < 8)
	{
		rePixelFormat = PixelFormat::kR5G6B5;
	}
	else
	{
		rePixelFormat = (bestAttributes.m_iAlpha > 0 ? PixelFormat::kA8B8G8R8 : PixelFormat::kX8B8G8R8);
	}

	// Warn if the best config is not at least A8B8G8R8 (we must accept it so we
	// have *something* but it will almost certainly produce graphical artifacts).
	if (bestAttributes.m_iRed < 8 ||
		bestAttributes.m_iGreen < 8 ||
		bestAttributes.m_iBlue < 8 ||
		bestAttributes.m_iAlpha < 8)
	{
		SEOUL_WARN("[OGLES2RenderDevice]: Best config is below recommended minimums, "
			"too low precision in the back buffer: (%d, %d, %d, %d)",
			bestAttributes.m_iRed,
			bestAttributes.m_iGreen,
			bestAttributes.m_iBlue,
			bestAttributes.m_iAlpha);
	}

	return bestConfig;
}
#endif // /#if SEOUL_PLATFORM_ANDROID

/** Shared utility to expand a string of the form "a b c d" to [a b c d]. */
static void InternalExpandStringDelimArray(Byte const* sArray, Vector<String, MemoryBudgets::Rendering>& rvs)
{
	// Empty.
	rvs.Clear();

	// Early out, no values.
	if (nullptr == sArray) { return; }

	// Initial split.
	SplitString(String(sArray), ' ', rvs);

	// Now clean.
	Int32 iCount = (Int32)rvs.GetSize();
	for (Int32 i = 0; i < iCount; ++i)
	{
		rvs[i] = TrimWhiteSpace(rvs[i]);
		if (rvs[i].IsEmpty())
		{
			Swap(rvs[i], rvs[iCount - 1]);
			--iCount;
			--i;
		}
	}

	// Trim.
	rvs.Resize((UInt32)iCount);
}

void OGLES2RenderDevice::InternalGetDeviceSpecificBugs()
{
	// Settings.
	auto const sRenderer = SafeGlGetString(GL_RENDERER);

	// Bug on Mali-GXX, introduced in Android 10.
	// Workaround is relatively low impact, so we're
	// applying the workaround to all Mali-G devices
	// until if and when we get more specific information
	// from ARM that might change the workaround or
	// its scope.
	if (sRenderer == strstr(sRenderer, "Mali-G"))
	{
		m_bMaliGxxTextureCorruptionBug = true;
	}
	else
	{
		m_bMaliGxxTextureCorruptionBug = false;
	}
}

/**
 * Acquire the extension functions that we support.
 */
void OGLES2RenderDevice::InternalGetExtensions()
{
	HashSet<String, MemoryBudgets::Rendering> hs;
	{
		auto const sExtensions = SafeGlGetString(GL_EXTENSIONS);

		Vector<String, MemoryBudgets::Rendering> vs;
		InternalExpandStringDelimArray(sExtensions, vs);

		for (auto const& s : vs)
		{
			(void)hs.Insert(s);
		}
	}

	// See the documentation for eglGetProcAddress - we can't just call eglGetProcAddress(),
	// we need to check for this extension string as well.
	if (hs.HasKey("GL_EXT_debug_marker"))
	{
		m_glPopGroupMarkerEXT = (PopGroupMarkerEXT)GetExtensionProcAddress("glPopGroupMarkerEXT");
		m_glPushGroupMarkerEXT = (PushGroupMarkerEXT)GetExtensionProcAddress("glPushGroupMarkerEXT");
		if (nullptr != m_glPopGroupMarkerEXT && nullptr != m_glPushGroupMarkerEXT)
		{
			SEOUL_LOG_RENDER("[OGLES2RenderDevice]: GL_EXT_debug_marker enabled.");
		}
	}

	// ES3 always supports min/max blend modes.
	if (m_bSupportsES3 || hs.HasKey("GL_EXT_blend_minmax"))
	{
		m_Caps.m_bBlendMinMax = true;
	}
	else
	{
		m_Caps.m_bBlendMinMax = false;
	}

	// ES3 always supports ETC1. Otherwise, we need to check for the extension.
	if (m_bSupportsES3 || hs.HasKey("GL_OES_compressed_ETC1_RGB8_texture"))
	{
		m_Caps.m_bETC1 = true;
	}
	else
	{
		m_Caps.m_bETC1 = false;
	}

	// Track whether we support BGRA or not - we know we always do on iOS.
#if SEOUL_PLATFORM_IOS
	m_Caps.m_bBGRA = true;
#else
	// On Android, we must query for the extension. Check existing value
	// so it can be overriden by platform/device specific considerations.
	if (m_Caps.m_bBGRA)
	{
		m_Caps.m_bBGRA = hs.HasKey("GL_EXT_texture_format_BGRA8888");
	}
#endif

	// ES3 supports max level.
	if (m_bSupportsES3 ||
		hs.HasKey("GL_APPLE_texture_max_level"))
	{
		m_Caps.m_bIncompleteMipChain = true;
	}
	else
	{
		m_Caps.m_bIncompleteMipChain = false;
	}
}

/**
 * Does initial setup of the OpenGL device interface.
 */
void OGLES2RenderDevice::InternalInitializeOpenGl()
{
	SEOUL_ASSERT(IsRenderThread());
	SEOUL_ASSERT(!m_bInScene);

#if SEOUL_PLATFORM_ANDROID
	m_Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	// Verify and initialize the display.
	SEOUL_ASSERT(EGL_NO_DISPLAY != m_Display);
	SEOUL_VERIFY(EGL_FALSE != eglInitialize(m_Display, nullptr, nullptr));

	// Get the config that will be used.
	m_Config = InternalStaticGetBestConfig(m_Display, m_eBackBufferDepthStencilFormat, m_eBackBufferPixelFormat);

	SEOUL_VERIFY(EGL_FALSE != eglGetConfigAttrib(m_Display, m_Config, EGL_NATIVE_VISUAL_ID, &m_iNativeVisualID));
	SEOUL_VERIFY(EGL_FALSE != eglGetConfigAttrib(m_Display, m_Config, EGL_NATIVE_VISUAL_TYPE, &m_iNativeVisualType));
#endif // /#if SEOUL_PLATFORM_ANDROID

	InternalDoReset();
}

/**
 * Perform final teardown of the OpenGL API.
 */
void OGLES2RenderDevice::InternalShutdownOpenGl()
{
	SEOUL_ASSERT(IsRenderThread());
	SEOUL_ASSERT(!m_bInScene);

	InternalDoLost();

#if SEOUL_PLATFORM_ANDROID
	DeinitializeEAGLContext(m_Display);
	m_iNativeVisualType = 0;
	m_iNativeVisualID = 0;
	m_Config = 0;
	SEOUL_VERIFY(EGL_FALSE != eglTerminate(m_Display));
	m_Display = EGL_NO_DISPLAY;
	m_eBackBufferPixelFormat = PixelFormat::kX8B8G8R8;
	m_eBackBufferDepthStencilFormat = DepthStencilFormat::kD24S8;
#endif // /#if SEOUL_PLATFORM_ANDROID
}

/**
 * Called to either initialize the window for the first
 * time or to restore the surface on Android after a suspend event.
 */
void OGLES2RenderDevice::InternalDoReset()
{
	SEOUL_ASSERT(IsRenderThread());
	SEOUL_ASSERT(!m_bInScene);

#if SEOUL_PLATFORM_ANDROID
	// Early out if no window.
	if (!m_pMainWindow.IsValid())
	{
		return;
	}

	// Nothing to do if we've already reset the surface.
	if (IsReset())
	{
		return;
	}

	// Context create only happen once - must be deferred until we have
	// a valid Window on Android.
	if (!m_bHasContext)
	{
		// Always true.
		m_bHasContext = true;

		SEOUL_VERIFY(InitializeEAGLContext(m_pMainWindow.GetPtr(), m_Display, m_Config, m_bSupportsES3, m_bSupportsAsyncTextureCreate));
	}

	// Hardware scalar configuration.
	{
		// Get the dimensions of the window area (this is the actual screen resolution).
		Int32 const iWindowWidth = ANativeWindow_getWidth(m_pMainWindow.GetPtr());
		Int32 const iWindowHeight = ANativeWindow_getHeight(m_pMainWindow.GetPtr());

		// Load the max height setting, default to the window height.
		Int32 iMaxHeight = iWindowHeight;

		// Get the max height setting.
		{
			SharedPtr<DataStore> pDataStore(SettingsManager::Get()->WaitForSettings(
				GamePaths::Get()->GetApplicationJsonFilePath()));
			if (pDataStore.IsValid())
			{
				DataStoreTableUtil applicationSection(*pDataStore, ksApplication);
				(void)applicationSection.GetValue(ksAndroidMaxBackBufferHeight, iMaxHeight);

				// Invalid or undefined, set to the window height.
				if (iMaxHeight <= 0)
				{
					iMaxHeight = iWindowHeight;
				}
			}
		}

		// Desired height is the window height, unless it's beyond the
		// specified max height for Android.
		Int32 const iDesiredHeight = (iWindowHeight > iMaxHeight ? iMaxHeight : iWindowHeight);

		// Desired width is the window width, unless the height is beyond
		// the specified max height for Android, in which case it is derived
		// based on the aspect ratio of the window.
		Int32 const iDesiredWidth = (iWindowHeight > iMaxHeight
			? (Int32)Round((Double)iMaxHeight * ((Double)iWindowWidth / (Double)iWindowHeight))
			: iWindowWidth);

		// Cache scalar settings.
		m_HardwareScalarState.m_iWindowHeight = iWindowHeight;
		m_HardwareScalarState.m_iBufferHeight = iDesiredHeight;
		Bool const bScaling = m_HardwareScalarState.IsScaling();

		// Attempt to configure the scalar - if this fails, retry
		// with 0s to use defaults.
		if (0 != ANativeWindow_setBuffersGeometry(
			m_pMainWindow.GetPtr(),
			(bScaling ? iDesiredWidth : 0),
			(bScaling ? iDesiredHeight : 0),
			m_iNativeVisualID))
		{
			// Reconfigure on failure.
			m_HardwareScalarState.m_iWindowHeight = iWindowHeight;
			m_HardwareScalarState.m_iBufferHeight = iWindowHeight;

			// Must succeed here, we have no fallback.
			SEOUL_VERIFY(0 == ANativeWindow_setBuffersGeometry(
				m_pMainWindow.GetPtr(),
				0,
				0,
				m_iNativeVisualID));
		}
	}

	EGLint surfaceAttributeList[] = { EGL_NONE, EGL_NONE };
	m_Surface = eglCreateWindowSurface(m_Display, m_Config, m_pMainWindow.GetPtr(), surfaceAttributeList);
	SEOUL_ASSERT(EGL_NO_SURFACE != m_Surface);
	SEOUL_VERIFY(MakeEAGLContextActive(m_Display, m_Surface));
#endif // /#if SEOUL_PLATFORM_ANDROID

#if SEOUL_PLATFORM_IOS
	SEOUL_VERIFY(InitializeEAGLContext(m_bSupportsES3, m_bSupportsAsyncTextureCreate));
#endif // /#if SEOUL_PLATFORM_IOS

	// Check for device specific bugs.
	InternalGetDeviceSpecificBugs();

	// Check for extensions.
	InternalGetExtensions();

	// Initialize the frame buffer used for rendering.
	SEOUL_OGLES2_VERIFY(glGenFramebuffers(1, &m_CurrentRenderSurface.m_Framebuffer));

	// Create the one pixel white texture
	m_OnePixelWhiteTexture = InternalCreateOnePixelWhiteTexture(m_Caps.m_bBGRA);

	// Make sure the state manager's view of things is in sync once we're done.
	m_OGLES2StateManager.RestoreActiveTextureIfSet(GL_TEXTURE_2D);

	// iOS needs to explicitly create the back buffer render surface.
#if SEOUL_PLATFORM_IOS
	SEOUL_OGLES2_VERIFY(glGenRenderbuffers(1, &m_BackBufferColorBuffer));
	SEOUL_OGLES2_VERIFY(glBindRenderbuffer(GL_RENDERBUFFER, m_BackBufferColorBuffer));

	// This function will initialize the render buffer storage.
	InitializeEAGLBackBufferColorBuffer(m_pLayer);

	// Override the window viewport based on the size of the render buffer.
	SEOUL_OGLES2_VERIFY(glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &m_GraphicsParameters.m_iWindowViewportWidth));
	SEOUL_OGLES2_VERIFY(glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &m_GraphicsParameters.m_iWindowViewportHeight));

	// Clear the render buffer bind.
	SEOUL_OGLES2_VERIFY(glBindRenderbuffer(GL_RENDERBUFFER, 0));
#endif // /#if SEOUL_PLATFORM_IOS

	// Update the viewport from what Android reports.
#if SEOUL_PLATFORM_ANDROID
	SEOUL_VERIFY(EGL_FALSE != eglQuerySurface(m_Display, m_Surface, EGL_WIDTH, &m_GraphicsParameters.m_iWindowViewportWidth));
	SEOUL_VERIFY(EGL_FALSE != eglQuerySurface(m_Display, m_Surface, EGL_HEIGHT, &m_GraphicsParameters.m_iWindowViewportHeight));
	SEOUL_ASSERT(m_GraphicsParameters.m_iWindowViewportWidth > 0 && m_GraphicsParameters.m_iWindowViewportHeight > 0);

	SEOUL_LOG_RENDER("[OGLES2RenderDevice]: eglQuerySurface (%d x %d)\n",
		m_GraphicsParameters.m_iWindowViewportWidth,
		m_GraphicsParameters.m_iWindowViewportHeight);
	SEOUL_LOG_RENDER("[OGLES2RenderDevice]: ANativeWindow_get (%d x %d)\n",
		ANativeWindow_getWidth(m_pMainWindow.GetPtr()),
		ANativeWindow_getHeight(m_pMainWindow.GetPtr()));
#endif // /#if SEOUL_PLATFORM_ANDROID

	// Create the default back buffer viewport.
	m_BackBufferViewport = InternalCreateDefaultViewport();

	// Clear the recompute flag.
	m_bRecalculateBackBufferViewport = false;

	// Reset graphics objects.
	UInt32 const nCount = m_vGraphicsObjects.GetSize();
	for (UInt32 i = 0u; i < nCount; ++i)
	{
		if (BaseGraphicsObject::kDestroyed == m_vGraphicsObjects[i]->GetState())
		{
			if (!m_vGraphicsObjects[i]->OnCreate())
			{
				continue;
			}
		}

		if (BaseGraphicsObject::kCreated == m_vGraphicsObjects[i]->GetState())
		{
			m_vGraphicsObjects[i]->OnReset();
		}
	}

	// Stat reporting - log reporting in developer, analytics reporting
	// in all builds.
	InternalReportDeviceData();

	// No longer have a frame to present after a reset.
	m_bHasFrameToPresent = false;
}

/**
 * Called either prior to destroying the device or
 * to initiate a suspend event on Android.
 */
void OGLES2RenderDevice::InternalDoLost()
{
	SEOUL_ASSERT(IsRenderThread());
	SEOUL_ASSERT(!m_bInScene);

	// In all cases, on device lost, a pending present
	// is interrupted.
	m_bPresentInterrupt = true;

#if SEOUL_PLATFORM_ANDROID
	// Nothing to do if we've already lost the surface.
	if (!IsReset())
	{
		return;
	}
#endif // /#if SEOUL_PLATFORM_ANDROID

	// Unselect the depth stencil surface.
	if (DepthStencilSurface::GetActiveDepthStencilSurface())
	{
		DepthStencilSurface::GetActiveDepthStencilSurface()->Unselect();
	}

	// Unselect the render target.
	if (RenderTarget::GetActiveRenderTarget())
	{
		RenderTarget::GetActiveRenderTarget()->Unselect();
	}

	// Commit render target changes.
	CommitRenderSurface();

	// Set all objects to the lost state.
	Int const nCount = (Int)m_vGraphicsObjects.GetSize();
	for (Int i = (nCount - 1); i >= 0; --i)
	{
		if (BaseGraphicsObject::kReset == m_vGraphicsObjects[i]->GetState())
		{
			m_vGraphicsObjects[i]->OnLost();
		}
	}

	// Cleanup the back buffer and OGLES2 context on iOS.
#if SEOUL_PLATFORM_IOS
	SEOUL_OGLES2_VERIFY(glDeleteRenderbuffers(1, &m_BackBufferColorBuffer));
	m_BackBufferColorBuffer = 0;
#endif // /#if SEOUL_PLATFORM_IOS

	// Cleanup the one pixel white texture.
	SEOUL_OGLES2_VERIFY(glDeleteTextures(1, &m_OnePixelWhiteTexture));
	m_OnePixelWhiteTexture = 0;

	SEOUL_OGLES2_VERIFY(glDeleteFramebuffers(1, &m_CurrentRenderSurface.m_Framebuffer));
	m_CurrentRenderSurface.m_Framebuffer = 0;
	m_bCurrentRenderSurfaceIsDirty = true;

#if SEOUL_PLATFORM_IOS
	DeinitializeEAGLContext();
#endif // /#if SEOUL_PLATFORM_IOS

#if SEOUL_PLATFORM_ANDROID
	MakeEAGLContextInactive(m_Display);
	SEOUL_VERIFY(EGL_FALSE != eglDestroySurface(m_Display, m_Surface));
	m_Surface = EGL_NO_SURFACE;
#endif // /#if SEOUL_PLATFORM_ANDROID
}

Viewport OGLES2RenderDevice::InternalCreateDefaultViewport()
{
	Viewport viewport;
	viewport.m_iTargetWidth = m_GraphicsParameters.m_iWindowViewportWidth;
	viewport.m_iTargetHeight = m_GraphicsParameters.m_iWindowViewportHeight;
	viewport.m_iViewportX = 0;
#if SEOUL_PLATFORM_ANDROID
	viewport.m_iViewportY = Max(0, (Int)s_AndroidWindowInsetTop);
#else
	viewport.m_iViewportY = 0;
#endif
	viewport.m_iViewportWidth = viewport.m_iTargetWidth;
#if SEOUL_PLATFORM_ANDROID
	viewport.m_iViewportHeight = Max(1, viewport.m_iTargetHeight - Max(0, (Int)s_AndroidWindowInsetBottom));
#else
	viewport.m_iViewportHeight = viewport.m_iTargetHeight;
#endif
	return viewport;
}

#define KEY(name) HString(String::Printf("hwinfo_%s", String(name).ToLowerASCII().CStr()))
#define LOG_GLB(name, val) \
		props.SetBooleanValueToTable(root, KEY(name), (val)); \
		SEOUL_LOG_RENDER("[OGLES2RenderDevice]: %s: %s\n", (name), (val) ? "YES" : "NO")
#define LOG_GLN(name, val) \
		props.SetFloat32ValueToTable(root, KEY(name), (val)); \
		SEOUL_LOG_RENDER("[OGLES2RenderDevice]: %s: %g\n", (name), (val))
#define LOG_GLS(name, val) \
		props.SetStringToTable(root, KEY(name), (val)); \
		SEOUL_LOG_RENDER("[OGLES2RenderDevice]: %s: %s\n", (name), (val))

static void ReportSpaceDelimArray(Byte const* sKey, Byte const* sArray, DataStore& props)
{
	SEOUL_LOG_RENDER("[OGLES2RenderDevice]: %s: %s\n", sKey, (nullptr == sArray ? "Unknown" : sArray));

	auto const key(KEY(sKey));
	props.SetArrayToTable(props.GetRootNode(), key);
	DataNode root;
	props.GetValueFromTable(props.GetRootNode(), key, root);

	Vector<String, MemoryBudgets::Rendering> vs;
	InternalExpandStringDelimArray(sArray, vs);
	QuickSort(vs.Begin(), vs.End());

	// Now add all to array.
	UInt32 uOut = 0u;
	for (auto const& s : vs)
	{
		props.SetStringToArray(root, uOut++, s);
	}
}

static void ReportCpuInfo(DataStore& props)
{
	// Only on Android.
#if SEOUL_PLATFORM_ANDROID
	auto const& root = props.GetRootNode();
	HashTable<String, String, MemoryBudgets::Rendering> t;

	// Read CPU info and parse it into a queryable table.
	{
		ScopedPtr<SyncFile> pFile;
		if (!FileManager::Get()->OpenFile("/proc/cpuinfo", File::kRead, pFile))
		{
			return;
		}

		// Process
		BufferedSyncFile file(pFile.Get(), false);

		// Fill out the table.
		String sLine;
		while (file.ReadLine(sLine))
		{
			auto const uSplit = sLine.Find(':');
			if (String::NPos == uSplit) { continue; }

			t.Insert(
				"PROC_" + TrimWhiteSpace(sLine.Substring(0u, uSplit)).ToUpperASCII(),
				TrimWhiteSpace(sLine.Substring(uSplit+1u)));
		}
	}

	// Clamp the total so we don't blow out our max count from a janked
	// cpu info report.
	static const UInt32 kuMaxCount = 32u;
	UInt32 uCount = 0u;
	for (auto const& pair : t)
	{
		// Special handling for CPU features array.
		if (pair.First == "PROC_FEATURES")
		{
			ReportSpaceDelimArray("PROC_FEATURES", pair.Second.CStr(), props);
		}
		else
		{
			LOG_GLS(pair.First.CStr(), pair.Second.CStr());
		}

		++uCount;
		if (uCount >= kuMaxCount)
		{
			break;
		}
	}
#endif // /#if SEOUL_PLATFORM_ANDROID
}

static void SendAnalytics(AnalyticsEvent* pEvent)
{
	if (AnalyticsManager::Get())
	{
		AnalyticsManager::Get()->TrackEvent(*pEvent);
	}

	// Release the event data.
	SafeDelete(pEvent);
}

void OGLES2RenderDevice::InternalReportDeviceData()
{
	m_ReportOnce.Call([&]()
	{
		auto pEvent(SEOUL_NEW(MemoryBudgets::Analytics) AnalyticsEvent("DeviceInfo"));

		// Configure a once token so we only report device data to analytics
		// once per device until something relevant changes (for now, OS version or
		// device data version).
		static const UInt32 kuDataVersion = 4u;
		{
			PlatformData data;
			Engine::Get()->GetPlatformData(data);

			auto const sOnceToken(String::Printf("%u_%s", kuDataVersion, data.m_sOsVersion.CStr()));
			pEvent->SetOnceToken(sOnceToken);
		}

		auto& props = pEvent->GetProperties();
		auto const& root = props.GetRootNode();

		auto const sExtensions = SafeGlGetString(GL_EXTENSIONS);
		auto const sRenderer = SafeGlGetString(GL_RENDERER);
		auto const sShadingLanguageVersion = SafeGlGetString(GL_SHADING_LANGUAGE_VERSION);
		auto const sVendor = SafeGlGetString(GL_VENDOR);
		auto const sVersion = SafeGlGetString(GL_VERSION);

		ReportCpuInfo(props);

		LOG_GLS("SEOUL_DEVICE_GUID", Engine::Get()->GetPlatformUUID().CStr());
		LOG_GLB("SEOUL_RENDER_THREAD", (GetRenderThreadId() != GetMainThreadId()));
		LOG_GLN("SEOUL_CPU_COUNT", (Float)Thread::GetProcessorCount());
		LOG_GLS("SEOUL_VIEWPORT", String::Printf("%d, %d",
			m_GraphicsParameters.m_iWindowViewportWidth,
			m_GraphicsParameters.m_iWindowViewportHeight).CStr());
#if SEOUL_PLATFORM_ANDROID
		{
			auto const iWidth = ANativeWindow_getWidth(m_pMainWindow.GetPtr());
			auto const iHeight = ANativeWindow_getHeight(m_pMainWindow.GetPtr());
			LOG_GLN("SEOUL_WINDOW_ASPECT", (Float)iWidth / (Float)iHeight);
			LOG_GLS("SEOUL_WINDOW", String::Printf("%d, %d", iWidth, iHeight).CStr());
		}
#endif // /#if SEOUL_PLATFORM_ANDROID

		LOG_GLB("SEOUL_ASYNC_TEXTURES", m_bSupportsAsyncTextureCreate);
		LOG_GLS("SEOUL_BACKBUFFER_DS", EnumToString<DepthStencilFormat>(m_eBackBufferDepthStencilFormat));
		LOG_GLS("SEOUL_BACKBUFFER_PF", EnumToString<PixelFormat>(m_eBackBufferPixelFormat));
		LOG_GLB("SEOUL_BGRA", m_Caps.m_bBGRA);
		LOG_GLS("SEOUL_COMPATIBILITY_PF", EnumToString<PixelFormat>(m_eCompatible32bit4colorRenderTargetFormat));
		LOG_GLB("SEOUL_ETC1", m_Caps.m_bETC1);
		LOG_GLB("SEOUL_GLES3", m_bSupportsES3);
		LOG_GLB("SEOUL_MALIGXXBUG", m_bMaliGxxTextureCorruptionBug);
		LOG_GLB("SEOUL_MINMAX", m_Caps.m_bBlendMinMax);
		LOG_GLN("SEOUL_REFRESH_DEN", (Float)m_RefreshRate.m_uDenominator);
		LOG_GLN("SEOUL_REFRESH_HZ", (Float)m_RefreshRate.ToHz());
		LOG_GLN("SEOUL_REFRESH_NUM", (Float)m_RefreshRate.m_uNumerator);
		ReportSpaceDelimArray("GL_EXTENSIONS", sExtensions, props);
		LOG_GLS("GL_RENDERER", (nullptr == sRenderer ? "Unknown" : sRenderer));
		LOG_GLS("GL_SHADING_LANGUAGE_VERSION", (nullptr == sShadingLanguageVersion ? "Unknown" : sShadingLanguageVersion));
		LOG_GLS("GL_VENDOR", (nullptr == sVendor ? "Unknown" : sVendor));
		LOG_GLS("GL_VERSION", (nullptr == sVersion ? "Unknown" : sVersion));

#if SEOUL_PLATFORM_ANDROID
#define LOG_EGL(id) { \
	Int iValue = -1; \
	if (EGL_FALSE != eglGetConfigAttrib(m_Display, m_Config, id, &iValue)) { \
		props.SetInt32ValueToTable(root, KEY(#id), iValue); \
		SEOUL_LOG_RENDER("[OGLES2RenderDevice]: " #id ": %d\n", iValue); \
	} else { \
		props.SetStringToTable(root, KEY(#id), "unknown"); \
		SEOUL_LOG_RENDER("[OGLES2RenderDevice]: " #id ": <unknown>\n"); \
	} }

		LOG_EGL(EGL_ALPHA_MASK_SIZE);
		LOG_EGL(EGL_ALPHA_SIZE);
		LOG_EGL(EGL_BIND_TO_TEXTURE_RGB);
		LOG_EGL(EGL_BIND_TO_TEXTURE_RGBA);
		LOG_EGL(EGL_BLUE_SIZE);
		LOG_EGL(EGL_BUFFER_SIZE);
		LOG_EGL(EGL_COLOR_BUFFER_TYPE);
		LOG_EGL(EGL_CONFIG_CAVEAT);
		LOG_EGL(EGL_CONFIG_ID);
		LOG_EGL(EGL_CONFORMANT);
		LOG_EGL(EGL_DEPTH_SIZE);
		LOG_EGL(EGL_GREEN_SIZE);
		LOG_EGL(EGL_LEVEL);
		LOG_EGL(EGL_LUMINANCE_SIZE);
		LOG_EGL(EGL_MATCH_NATIVE_PIXMAP);
		LOG_EGL(EGL_NATIVE_RENDERABLE);
		LOG_EGL(EGL_MAX_SWAP_INTERVAL);
		LOG_EGL(EGL_MIN_SWAP_INTERVAL);
		LOG_EGL(EGL_RED_SIZE);
		LOG_EGL(EGL_SAMPLE_BUFFERS);
		LOG_EGL(EGL_SAMPLES);
		LOG_EGL(EGL_STENCIL_SIZE);
		LOG_EGL(EGL_RENDERABLE_TYPE);
		LOG_EGL(EGL_SURFACE_TYPE);
		LOG_EGL(EGL_TRANSPARENT_TYPE);
		LOG_EGL(EGL_TRANSPARENT_RED_VALUE);
		LOG_EGL(EGL_TRANSPARENT_GREEN_VALUE);
		LOG_EGL(EGL_TRANSPARENT_BLUE_VALUE);

#undef LOG_EGL
#endif // /#if SEOUL_PLATFORM_ANDROID

#define LOG_GL_PREC(shader_type, prec_type) { \
	GLint aiRange[2]; GLint iPrecision; glGetShaderPrecisionFormat(shader_type, prec_type, aiRange, &iPrecision); \
	props.SetStringToTable(root, KEY(#shader_type "_" #prec_type), String::Printf("%d, %d, %d", aiRange[0], aiRange[1], iPrecision)); \
	SEOUL_LOG_RENDER("[OGLES2RenderDevice]: " #shader_type "(" #prec_type "): (%d, %d, %d)\n", aiRange[0], aiRange[1], iPrecision); }

		LOG_GL_PREC(GL_FRAGMENT_SHADER, GL_LOW_FLOAT);
		LOG_GL_PREC(GL_FRAGMENT_SHADER, GL_MEDIUM_FLOAT);
		LOG_GL_PREC(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT);
		LOG_GL_PREC(GL_FRAGMENT_SHADER, GL_LOW_INT);
		LOG_GL_PREC(GL_FRAGMENT_SHADER, GL_MEDIUM_INT);
		LOG_GL_PREC(GL_FRAGMENT_SHADER, GL_HIGH_INT);
		LOG_GL_PREC(GL_VERTEX_SHADER, GL_LOW_FLOAT);
		LOG_GL_PREC(GL_VERTEX_SHADER, GL_MEDIUM_FLOAT);
		LOG_GL_PREC(GL_VERTEX_SHADER, GL_HIGH_FLOAT);
		LOG_GL_PREC(GL_VERTEX_SHADER, GL_LOW_INT);
		LOG_GL_PREC(GL_VERTEX_SHADER, GL_MEDIUM_INT);
		LOG_GL_PREC(GL_VERTEX_SHADER, GL_HIGH_INT);

#undef LOG_GL_PREC

#	define LOG_GL_INTEGER(value) { \
		GLint iValue = 0; SEOUL_OGLES2_VERIFY(glGetIntegerv(value, &iValue)); \
		props.SetInt32ValueToTable(root, KEY(#value), iValue); \
		SEOUL_LOG_RENDER("[OGLES2RenderDevice]: %s: %d\n", #value, iValue); }
#	define LOG_GL_INTEGER2(value) { \
		GLint aiValue[2] = { 0 }; SEOUL_OGLES2_VERIFY(glGetIntegerv(value, aiValue)); \
		props.SetStringToTable(root, KEY(#value), String::Printf("%d, %d", aiValue[0], aiValue[1])); \
		SEOUL_LOG_RENDER("[OGLES2RenderDevice]: %s: (%d, %d)\n", #value, aiValue[0], aiValue[1]); }

		LOG_GL_INTEGER(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
		LOG_GL_INTEGER(GL_MAX_CUBE_MAP_TEXTURE_SIZE);
		LOG_GL_INTEGER(GL_MAX_FRAGMENT_UNIFORM_VECTORS);
		LOG_GL_INTEGER(GL_MAX_RENDERBUFFER_SIZE);
		LOG_GL_INTEGER(GL_MAX_TEXTURE_IMAGE_UNITS);
		LOG_GL_INTEGER(GL_MAX_TEXTURE_SIZE);
		LOG_GL_INTEGER(GL_MAX_VARYING_VECTORS);
		LOG_GL_INTEGER(GL_MAX_VERTEX_ATTRIBS);
		LOG_GL_INTEGER(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS);
		LOG_GL_INTEGER(GL_MAX_VERTEX_UNIFORM_VECTORS);
		LOG_GL_INTEGER2(GL_MAX_VIEWPORT_DIMS);
		LOG_GL_INTEGER(GL_NUM_COMPRESSED_TEXTURE_FORMATS);
		LOG_GL_INTEGER(GL_NUM_SHADER_BINARY_FORMATS);

#	undef LOG_GL_INTEGER2
#	undef LOG_GL_INTEGER

		Jobs::AsyncFunction(
			GetMainThreadId(),
			&SendAnalytics,
			pEvent);
	});
}

#undef LOG_GLS
#undef LOG_GLN
#undef LOG_GLB
#undef KEY

/**
 * @return A global OpenGL texture object for a texture that contains
 * a single white pixel (255, 255, 255, 255) in BGRA format.
 */
GLuint OGLES2RenderDevice::InternalCreateOnePixelWhiteTexture(Bool bSupportsBGRA)
{
	SEOUL_ASSERT(IsRenderThread());

	PixelFormat const ePixelFormat = (bSupportsBGRA
		? PixelFormat::kA8R8G8B8
		: PixelFormat::kA8B8G8R8);

	ColorARGBu8 const cWhite(ColorARGBu8::White());

	GLuint texture = 0u;
	SEOUL_OGLES2_VERIFY(glGenTextures(1u, &texture));
	SEOUL_OGLES2_VERIFY(glBindTexture(GL_TEXTURE_2D, texture));
	SEOUL_OGLES2_VERIFY(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
	SEOUL_OGLES2_VERIFY(TexImage2D(
		GL_TEXTURE_2D,
		0,
		PixelFormatToOpenGlInternalFormat(ePixelFormat),
		1,
		1,
		GL_FALSE,
		PixelFormatToOpenGlFormat(ePixelFormat),
		PixelFormatToOpenGlElementType(ePixelFormat),
		&cWhite.m_Value));
	SEOUL_OGLES2_VERIFY(glPixelStorei(GL_UNPACK_ALIGNMENT, 4));
	SEOUL_OGLES2_VERIFY(glBindTexture(GL_TEXTURE_2D, 0));

	return texture;
}

/**
 * Called on new graphics objects so they end up in the graphics list - this
 * can only be performed on the render thread, so this function may insert the
 * object into a thread-safe queue for later processing on the render thread.
 */
void OGLES2RenderDevice::InternalAddObject(const SharedPtr<BaseGraphicsObject>& pObject)
{
	if (IsRenderThread())
	{
		if (IsReset())
		{
			if (pObject->OnCreate())
			{
				pObject->OnReset();
			}
		}

		m_vGraphicsObjects.PushBack(pObject);
	}
	else
	{
		SeoulGlobalIncrementReferenceCount(pObject.GetPtr());
		m_PendingGraphicsObjects.Push(pObject.GetPtr());
	}
}

/**
 * Called once per frame to do per-frame object cleanup and maintenance operations.
 */
Bool OGLES2RenderDevice::InternalPerFrameMaintenance()
{
	SEOUL_ASSERT(IsRenderThread());
	SEOUL_ASSERT(!m_bInScene);

	// Cleanup existing objects
	Int nCount = (Int)m_vGraphicsObjects.GetSize();
	for (Int i = 0; i < nCount; ++i)
	{
		// If we have a unique reference, the object is no longer in use, so it can be destroyed.
		if (m_vGraphicsObjects[i].IsUnique())
		{
			m_vGraphicsObjects[i].Swap(m_vGraphicsObjects.Back());
			if (BaseGraphicsObject::kReset == m_vGraphicsObjects.Back()->GetState())
			{
				m_vGraphicsObjects.Back()->OnLost();
			}

			m_vGraphicsObjects.PopBack();
			--i;
			--nCount;
			continue;
		}

		// If an object is in the destroyed state, create it.
		if (BaseGraphicsObject::kDestroyed == m_vGraphicsObjects[i]->GetState())
		{
			// If we fail creating it, nothing more we can do.
			if (!m_vGraphicsObjects[i]->OnCreate())
			{
				return false;
			}
		}

		// If an object is in the lost state, reset it.
		if (BaseGraphicsObject::kCreated == m_vGraphicsObjects[i]->GetState())
		{
			m_vGraphicsObjects[i]->OnReset();
		}
	}

	// Handle pending objects in the queue.
	SharedPtr<BaseGraphicsObject> pObject(m_PendingGraphicsObjects.Pop());
	while (pObject.IsValid())
	{
		// Need to decrement the reference count once - it was incremented before inserting into the queue.
		SeoulGlobalDecrementReferenceCount(pObject.GetPtr());

		InternalAddObject(pObject);
		pObject.Reset(m_PendingGraphicsObjects.Pop());
	}

	return true;
}

/**
 * Called in the destructor, loops until the object count
 * does not change or until the graphics object count is
 * 0. Returns true if the graphics object count is 0, false
 * otherwise.
 */
Bool OGLES2RenderDevice::InternalDestructorMaintenance()
{
	SEOUL_ASSERT(IsRenderThread());
	SEOUL_ASSERT(!m_bInScene);

	// Propagate pending objects.
	{
		Atomic32Type pendingObjectCount = m_PendingGraphicsObjects.GetCount();
		while (pendingObjectCount != 0)
		{
			if (!InternalPerFrameMaintenance())
			{
				return false;
			}

			Atomic32Type newPendingObjectCount = m_PendingGraphicsObjects.GetCount();
			if (newPendingObjectCount == pendingObjectCount)
			{
				break;
			}

			pendingObjectCount = newPendingObjectCount;
		}
	}

	// Now cleanup objects.
	{
		UInt32 uObjectCount = m_vGraphicsObjects.GetSize();
		while (uObjectCount != 0u)
		{
			if (!InternalPerFrameMaintenance())
			{
				return false;
			}

			UInt32 nNewObjectCount = m_vGraphicsObjects.GetSize();
			if (uObjectCount == nNewObjectCount)
			{
				return (0u == nNewObjectCount);
			}

			uObjectCount = nNewObjectCount;
		}
	}

	return true;
}

/**
 * If needed, recompute the back buffer viewport and trigger an object reset
 * to apply those changes to dependent structures.
 */
void OGLES2RenderDevice::InternalRecalculateBackBufferViewport()
{
	SEOUL_ASSERT(IsRenderThread());
	SEOUL_ASSERT(!m_bInScene);

	if (m_bRecalculateBackBufferViewport)
	{
		// Create the default back buffer viewport.
		m_BackBufferViewport = InternalCreateDefaultViewport();

		// Clear the recompute flag.
		m_bRecalculateBackBufferViewport = false;

		// Before resetting graphics objects, release render targets - see
		// the note in CommitRenderSurface(). Not doing this can result in
		// memory leaks on some OGLES2 devices.

		// Unselect the depth stencil surface.
		if (DepthStencilSurface::GetActiveDepthStencilSurface())
		{
			DepthStencilSurface::GetActiveDepthStencilSurface()->Unselect();
		}

		// Unselect the render target.
		if (RenderTarget::GetActiveRenderTarget())
		{
			RenderTarget::GetActiveRenderTarget()->Unselect();
		}

		// Commit render target changes.
		CommitRenderSurface();

		// Reset objects to perform buffer recalculations.
		// Lose graphics objects.
		Int const nCount = (Int)m_vGraphicsObjects.GetSize();
		for (Int i = (nCount - 1); i >= 0; --i)
		{
			if (BaseGraphicsObject::kReset == m_vGraphicsObjects[i]->GetState())
			{
				m_vGraphicsObjects[i]->OnLost();
			}
		}

		// Reset graphics objects.
		for (Int i = 0; i < nCount; ++i)
		{
			if (BaseGraphicsObject::kCreated == m_vGraphicsObjects[i]->GetState())
			{
				m_vGraphicsObjects[i]->OnReset();
			}
		}
	}
}

void OGLES2RenderDevice::InternalRenderThreadEnterBackground()
{
	SEOUL_ASSERT(IsRenderThread());
	SEOUL_ASSERT(!m_bInScene);

	// In all cases, on enter background, a pending present
	// is interrupted.
	m_bPresentInterrupt = true;

	// Filter redundant calls.
	if (m_bInBackground)
	{
		return;
	}

	// Log for testing and debug tracking.
	SEOUL_LOG("OGLES2RenderDevice::InternalRenderThreadEnterBackground()");

	// Commit change.
	m_bInBackground = true;

	// See "Background Apps May Not Execute Commands on the Graphics Hardware"
	// from https://developer.apple.com/library/content/documentation/3DDrawing/Conceptual/OpenGLES_ProgrammingGuide/ImplementingaMultitasking-awareOpenGLESApplication/ImplementingaMultitasking-awareOpenGLESApplication.html#//apple_ref/doc/uid/TP40008793-CH5-SW5
	//
	// Only necessary on iOS, but "why not?" on Android.
	glFinish();
}

void OGLES2RenderDevice::InternalRenderThreadLeaveBackground()
{
	SEOUL_ASSERT(IsRenderThread());
	SEOUL_ASSERT(!m_bInScene);

	// In all cases, on leave background, a pending present
	// is interrupted.
	m_bPresentInterrupt = true;

	// Log for testing and debug tracking.
	SEOUL_LOG("OGLES2RenderDevice::InternalRenderThreadLeaveBackground()");

	// No longer in the background.
	m_bInBackground = false;
}

/** Utility for which we known the layout of the structure. */
namespace
{

SEOUL_DEFINE_PACKED_STRUCT(struct BackBufferPixel
{
	UInt8 m_R;
	UInt8 m_G;
	UInt8 m_B;
	UInt8 m_A;
});
SEOUL_STATIC_ASSERT(sizeof(BackBufferPixel) == 4);

} // namespace anonymous

Bool OGLES2RenderDevice::ReadBackBufferPixel(Int32 iX, Int32 iY, ColorARGBu8& rcColor)
{
	BackBufferPixel pixel;
	memset(&pixel, 0, sizeof(pixel));
	SEOUL_OGLES2_VERIFY(glReadPixels(iX, iY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel));

	rcColor.m_R = pixel.m_R;
	rcColor.m_G = pixel.m_G;
	rcColor.m_B = pixel.m_B;
	rcColor.m_A = pixel.m_A;

	return true;
}

} // namespace Seoul
