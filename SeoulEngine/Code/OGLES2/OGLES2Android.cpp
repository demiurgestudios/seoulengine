/**
 * \file OGLES2Android.cpp
 * \brief Context management for Android, handles creation
 * and destruction of EAGL contexts.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AtomicRingBuffer.h"
#include "Engine.h"
#include "JobsFunction.h"
#include "JobsManager.h"
#include "HeapAllocatedPerThreadStorage.h"
#include "Logger.h"
#include "OGLES2Util.h"
#include "PlatformData.h"
#include "Prereqs.h"
#include "ThreadId.h"

#if SEOUL_PLATFORM_ANDROID

#include <EGL/egl.h>

namespace Seoul
{

namespace
{

/** Render thread context, created first. */
static EGLContext s_pRenderThreadContext = EGL_NO_CONTEXT;
static Bool s_bES3 = false;
static Bool s_bAsyncSupport = false;

/** Pool of multithreaded contexts for texture creation. */
static AtomicRingBuffer<EGLContext> s_FreeContexts;
/** Pool of multithreaded surfaces for texture creation. */
static AtomicRingBuffer<EGLSurface> s_FreeSurfaces;

/**
 * Encapsulates an EAGL context for threads
 * other than the render thread. Used
 * for asynchronous graphics object creation.
 */
class AsyncContextWrapper SEOUL_SEALED
{
public:
	explicit AsyncContextWrapper(Atomic32Type /*threadIndex*/)
		: m_pContext(EGL_NO_CONTEXT)
	{
	}

	~AsyncContextWrapper()
	{
		// Make sure our context has been released.
		SEOUL_ASSERT(EGL_NO_CONTEXT == m_pContext);
	}

	/** Set this context to the active context. */
	Bool MakeCurrent(EGLDisplay display)
	{
		// Sanity check - must never be called on the render thread.
		SEOUL_ASSERT(!IsRenderThread());

		// Nop if no context.
		if (EGL_NO_CONTEXT == m_pContext)
		{
			return false;
		}

		SEOUL_VERIFY(EGL_FALSE != eglMakeCurrent(display, m_pSurface, m_pSurface, m_pContext));
		return true;
	}

	/** Populate this context - nop if already a valid context. */
	void Acquire(EGLDisplay display, EGLConfig config)
	{
		// Nothing to do if we already have a context.
		if (EGL_NO_CONTEXT != m_pContext)
		{
			return;
		}

		// Acquire a free context if possible.
		m_pContext = s_FreeContexts.Pop();

		// Create a new one if necessary.
		if (EGL_NO_CONTEXT == m_pContext)
		{
			// Cannot create if no render thread context.
			if (EGL_NO_CONTEXT == s_pRenderThreadContext)
			{
				return;
			}

			// Create - the money is that we use the render thread's share group.
			// This allows us to instantiate graphics objects on threads other than
			// the render thread.
			EGLint contextAttributes[] = {
				EGL_CONTEXT_CLIENT_VERSION, (s_bES3 ? 3 : 2),
				EGL_NONE, EGL_NONE
			};
			m_pContext = eglCreateContext(display, config, s_pRenderThreadContext, contextAttributes);
			SEOUL_ASSERT(EGL_NO_CONTEXT != m_pContext);
		}

		// Acquire a free surface if possible.
		m_pSurface = s_FreeSurfaces.Pop();

		// Create a new one if necessary.
		if (EGL_NO_SURFACE == m_pSurface)
		{
			// Cannot create if no render thread context.
			if (EGL_NO_CONTEXT == s_pRenderThreadContext)
			{
				// Put the context back in the free buffer.
				s_FreeContexts.Push(m_pContext);
				m_pContext = EGL_NO_CONTEXT;
				return;
			}

			EGLint surfaceAttributes[] = {
				EGL_WIDTH, 1,
				EGL_HEIGHT, 1,
				EGL_TEXTURE_TARGET, EGL_NO_TEXTURE,
				EGL_TEXTURE_FORMAT, EGL_NO_TEXTURE,
				EGL_NONE, EGL_NONE,
			};
			m_pSurface = eglCreatePbufferSurface(display, config, surfaceAttributes);
			SEOUL_ASSERT(EGL_NO_SURFACE != m_pSurface);
		}
	}

	/** Destroy this context - nop if already a valid context. */
	void Release(EGLDisplay display)
	{
		// Nothing to do if we don't have a context.
		if (EGL_NO_CONTEXT == m_pContext)
		{
			return;
		}

		// Sanity check - only unset if we're set to the current thread's
		// context.
		auto pContext = eglGetCurrentContext();
		if (m_pContext == pContext)
		{
			SEOUL_VERIFY(EGL_FALSE != eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
		}

		// Release the surface.
		SEOUL_VERIFY(EGL_FALSE != eglDestroySurface(display, m_pSurface));
		m_pSurface = EGL_NO_SURFACE;

		// Release the context.
		SEOUL_VERIFY(EGL_FALSE != eglDestroyContext(display, m_pContext));
		m_pContext = EGL_NO_CONTEXT;
	}

private:
	EGLContext m_pContext;
	EGLSurface m_pSurface;

	SEOUL_DISABLE_COPY(AsyncContextWrapper);
}; // class AsyncContextWrapper

} // namespace anonymous

static HeapAllocatedPerThreadStorage<AsyncContextWrapper, 64> s_AsyncContexts;

/** Get the current thread's context and activate it. */
Bool MakeAsyncContextActive(EGLDisplay display, EGLConfig config)
{
	// Sanity check.
	SEOUL_ASSERT(!IsRenderThread());
	SEOUL_ASSERT(s_bAsyncSupport);

	auto& r = s_AsyncContexts.Get();
	r.Acquire(display, config);
	return r.MakeCurrent(display);
}

/** Set the current thread's context to EGL_NO_CONTEXT. */
void MakeAsyncContextInactive(EGLDisplay display)
{
	// Sanity checks
	SEOUL_ASSERT(!IsRenderThread());
	SEOUL_ASSERT(s_bAsyncSupport);

	// Unset.
	SEOUL_VERIFY(EGL_FALSE != eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
}

/** On the render thread only, restore the global render context. */
Bool MakeEAGLContextActive(EGLDisplay display, EGLSurface surface)
{
	// Sanity check
	SEOUL_ASSERT(IsRenderThread());

	if (EGL_NO_CONTEXT != s_pRenderThreadContext)
	{
		SEOUL_VERIFY(EGL_FALSE != eglMakeCurrent(display, surface, surface, s_pRenderThreadContext));
		return true;
	}

	return false;
}

/** On the render thread only, unset the global render context. */
void MakeEAGLContextInactive(EGLDisplay display)
{
	// Sanity check
	SEOUL_ASSERT(IsRenderThread());

	// Unset.
	SEOUL_VERIFY(EGL_FALSE != eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
}

/**
 * Determine default rbSupportsAsyncTextureCreate based on context.
 * glGetString() *must* be a valid call when this function is called
 * (we must have a valid gl context.
 */
Bool DetermineInitialAsyncTextureCreateSupport()
{
	Byte const* const sRenderer = SafeGlGetString(GL_RENDERER);
	Byte const* const sVendor = SafeGlGetString(GL_VENDOR);

	// The GPU in the Samsung Galaxy Tab 3 (Vivante GC1000)
	// does not support contexts off the main thread. Unfortunately,
	// it fails at the attempt to bind, not create, so we
	// handle it specially here and just turn off worker contexts.
	//
	// See also: https://github.com/flutter/flutter/issues/6886
	if (nullptr != strstr(sRenderer, "GC1000") &&
		nullptr != strstr(sVendor, "Vivante"))
	{
		return false;
	}

	// From live crash reports and this Xamarin post:
	// https://bugzilla.xamarin.com/show_bug.cgi?id=2139
	//
	// VideoCore IV devices do not have consistent support for sharing
	// groups.
	if (nullptr != strstr(sRenderer, "VideoCore IV"))
	{
		return false;
	}

	// Tegra 2 and 3 hardware can hard lock.
	// See also: https://bugzilla.mozilla.org/show_bug.cgi?id=759225
	//
	// Unfortunately, according to the response from Nvidia, this
	// functionality should work as expected on Tegra 3 but we're
	// not seeing this.
	//
	// Further action (almost certainly not worth it given the small
	// user base of these devices):
	// - may be a change we can make to keep Tegra 3 happy.
	// - we can use EGLImage on Tegra devices as an alternative
	//   to multiple contexts and share groups.
	if (0 == strcmp(sRenderer, "NVIDIA Tegra 2") ||
		0 == strcmp(sRenderer, "NVIDIA Tegra 3"))
	{
		return false;
	}

	// Default is to return true.
	return true;
}

/** Create the render thread context, as well as a pool of async contexts. */
Bool InitializeEAGLContext(ANativeWindow* pMainWindow, EGLDisplay display, EGLConfig config, Bool& rbSupportsES3, Bool& rbSupportsAsyncTextureCreate)
{
	// Sanity check
	SEOUL_ASSERT(IsRenderThread());

	SEOUL_ASSERT(EGL_NO_CONTEXT == s_pRenderThreadContext);

	// Platform data for additional consideration and processing.
	PlatformData platformData;
	Engine::Get()->GetPlatformData(platformData);

	{
		// See: https://developer.android.com/guide/topics/graphics/opengl.html
		//
		// Unfortunately, the cleaner/more comfortable method described there
		// does not work on all devices (in particular, Nexus 7 (2012) running Android 4.x
		// will successfully create the context if passed in version 3, but it will be
		// (what appears to be a broken) OpenGL ES 1 context. So we need to use the
		// method of creating an OpenGL ES 1 context, querying the version string,
		// then destroying it. Even though I hate it (really OpenGL? "OpenGL ES 3." is
		// the way to do this? Ugh. And we need an active context to ask OpenGL
		// what its version code is?) Note that this query requires OpenGL ES 2 as well - using
		// OpenGL ES 1 will report OpenGL ES 1 for the version.
		{
			EGLint contextAttributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };
			auto context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttributes);
			if (EGL_NO_CONTEXT == context)
			{
				rbSupportsES3 = false;
				rbSupportsAsyncTextureCreate = false;
			}
			else
			{
				// OpenGL calls cannot be made until a surface is made current.
				EGLint surfaceAttributeList[] = { EGL_NONE, EGL_NONE };
				auto surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)pMainWindow, surfaceAttributeList);
				SEOUL_ASSERT(EGL_NO_SURFACE != surface);
				SEOUL_VERIFY(EGL_FALSE != eglMakeCurrent(display, surface, surface, context));

				// Logging some other basic info in case the device crashes early on device farm testing.
				{
#define LOG_GLS(val) SEOUL_LOG_RENDER("[OGLES2RenderDevice]: Startup: %s: %s\n", #val, SafeGlGetString(val))
					LOG_GLS(GL_EXTENSIONS);
					LOG_GLS(GL_RENDERER);
					LOG_GLS(GL_SHADING_LANGUAGE_VERSION);
					LOG_GLS(GL_VENDOR);
					LOG_GLS(GL_VERSION);
#undef LOG_GLS
				}

				// Check the version string for "OpenGL ES 3." - ugh.
				Byte const* sVersion = nullptr;
				SEOUL_OGLES2_VERIFY(sVersion = SafeGlGetString(GL_VERSION));
				if (nullptr == strstr(sVersion, "OpenGL ES 3.") ||
					// Workaround for a crash bug on the "Sharp Aquos Phone SBM302SH",
					// "Sony Xperia Z1 SO-01F (Honami Maki)", and others.
					//
					// Based on: Chrome driver bug workaround, id 200, cr_bug 657925: https://chromium.googlesource.com/chromium/src/gpu/+/master/config/gpu_driver_bug_list.json#1967
					//
					// Apparently, OpenGL ES3 support is unreliable prior to Android 4.4
					// (API level 19).
					platformData.m_iTargetApiOrSdkVersion < 19)
				{
					rbSupportsES3 = false;
				}
				else
				{
					rbSupportsES3 = true;
				}

				// Now also check for our initial rbSupportsAsyncTextureCreate value.
				rbSupportsAsyncTextureCreate = DetermineInitialAsyncTextureCreateSupport();

				// Cleanup the surface.
				SEOUL_VERIFY(EGL_FALSE != eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_SURFACE));
				SEOUL_VERIFY(EGL_FALSE != eglDestroySurface(display, surface));
				surface = EGL_NO_SURFACE;

				// Cleanup the context.
				SEOUL_VERIFY(EGL_FALSE != eglDestroyContext(display, context));
				context = EGL_NO_CONTEXT;
			}
		}

		// Cache the result.
		s_bES3 = rbSupportsES3;

		// Now create the real context, based on the rbSupportsES3 flag.
		EGLint contextAttributes[] = { EGL_CONTEXT_CLIENT_VERSION, (rbSupportsES3 ? 3 : 2), EGL_NONE, EGL_NONE };
		s_pRenderThreadContext = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttributes);
		SEOUL_ASSERT(EGL_NO_CONTEXT != s_pRenderThreadContext);
	}

	// Now create enough free contexts for the number of general purpose threads on the system,
	// if async create is still enabled.
	UInt32 const uThreads = Jobs::Manager::Get()->GetGeneralPurposeWorkerThreadCount();

	if (rbSupportsAsyncTextureCreate)
	{
		EGLint contextAttributes[] = { EGL_CONTEXT_CLIENT_VERSION, (rbSupportsES3 ? 3 : 2), EGL_NONE, EGL_NONE };
		for (UInt32 i = 0u; i < uThreads; ++i)
		{
			// Create - the money is that we use the render thread's share group.
			// This allows us to instantiate graphics objects on threads other than
			// the render thread.
			auto context = eglCreateContext(display, config, s_pRenderThreadContext, contextAttributes);

			// No support.
			if (EGL_NO_CONTEXT == context)
			{
				rbSupportsAsyncTextureCreate = false;
				break;
			}
			else
			{
				s_FreeContexts.Push(context);
			}
		}
	}

	// Finally, create enough free surfaces for the number of general purpose threads on the system.
	if (rbSupportsAsyncTextureCreate)
	{
		EGLint surfaceAttributes[] = {
			EGL_WIDTH, 1,
			EGL_HEIGHT, 1,
			EGL_TEXTURE_TARGET, EGL_NO_TEXTURE,
			EGL_TEXTURE_FORMAT, EGL_NO_TEXTURE,
			EGL_NONE, EGL_NONE,
		};
		for (UInt32 i = 0u; i < uThreads; ++i)
		{
			auto surface = eglCreatePbufferSurface(display, config, surfaceAttributes);

			// No support.
			if (EGL_NO_SURFACE == surface)
			{
				rbSupportsAsyncTextureCreate = false;
				break;
			}
			else
			{
				s_FreeSurfaces.Push(surface);
			}
		}
	}

	// If we get here and have no support, cleanup.
	if (!rbSupportsAsyncTextureCreate)
	{
		// Release all surfaces.
		{
			auto p = s_FreeSurfaces.Pop();
			while (nullptr != p)
			{
				SEOUL_VERIFY(EGL_FALSE != eglDestroySurface(display, p));
				p = s_FreeSurfaces.Pop();
			}
		}

		// Release all contexts.
		{
			auto p = s_FreeContexts.Pop();
			while (nullptr != p)
			{
				SEOUL_VERIFY(EGL_FALSE != eglDestroyContext(display, p));
				p = s_FreeContexts.Pop();
			}
		}
	}

	// Synchronize.
	s_bAsyncSupport = rbSupportsAsyncTextureCreate;
	return true;
}

/** Tear down the render thread and all async contexts. */
void DeinitializeEAGLContext(EGLDisplay display)
{
	// Sanity check
	SEOUL_ASSERT(IsRenderThread());

	// Nothing to do if already teared down.
	if (EGL_NO_CONTEXT == s_pRenderThreadContext)
	{
		return;
	}

	// Release any other contexts.
	{
		auto& rObjects = s_AsyncContexts.GetAllObjects();
		for (auto const& p : rObjects)
		{
			if (nullptr != p)
			{
				p->Release(display);
			}
		}
	}

	// Release any unused free surfaces.
	{
		auto p = s_FreeSurfaces.Pop();
		while (nullptr != p)
		{
			SEOUL_VERIFY(EGL_FALSE != eglDestroySurface(display, p));
			p = s_FreeSurfaces.Pop();
		}
	}

	// Release any unused free contexts.
	{
		auto p = s_FreeContexts.Pop();
		while (nullptr != p)
		{
			SEOUL_VERIFY(EGL_FALSE != eglDestroyContext(display, p));
			p = s_FreeContexts.Pop();
		}
	}

	// Destroy the render thread context.
	SEOUL_VERIFY(EGL_FALSE != eglDestroyContext(display, s_pRenderThreadContext));
	s_pRenderThreadContext = EGL_NO_CONTEXT;
}

/** Present back buffer to front on the render thread. */
Bool EAGLSwapBuffers(EGLDisplay display, EGLSurface surface)
{
	// Sanity check
	SEOUL_ASSERT(IsRenderThread());

	Bool bReturn = true;
	bReturn = bReturn && (EGL_FALSE != eglMakeCurrent(display, surface, surface, s_pRenderThreadContext));
	bReturn = bReturn && (EGL_FALSE != eglSwapBuffers(display, surface));
	return bReturn;
}

} // namespace Seoul

#endif // /SEOUL_PLATFORM_ANDROID
