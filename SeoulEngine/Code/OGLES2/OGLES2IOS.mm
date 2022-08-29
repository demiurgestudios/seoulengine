/**
 * \file OGLES2IOS.m
 * \brief Context management for iOS, handles creation
 * and destruction of EAGL contexts.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AtomicRingBuffer.h"
#include "JobsFunction.h"
#include "JobsManager.h"
#include "HeapAllocatedPerThreadStorage.h"
#include "Prereqs.h"
#include "ThreadId.h"

#if SEOUL_PLATFORM_IOS

#import <QuartzCore/QuartzCore.h>
#include <OpenGLES/EAGL.h>
#include <OpenGLES/EAGLDrawable.h>
#include <OpenGLES/ES2/gl.h>

namespace Seoul
{

namespace
{

/** Render thread context, created first. */
static EAGLContext* s_pRenderThreadContext = nil;
static Int32 s_iAPI = kEAGLRenderingAPIOpenGLES2;

/** Pool of multithreaded contexts for texture creation. */
static AtomicRingBuffer<EAGLContext*> s_FreeContexts;

/**
 * Encapsulates an EAGL context for threads
 * other than the render thread. Used
 * for asynchronous graphics object creation.
 */
class AsyncContextWrapper SEOUL_SEALED
{
public:
	AsyncContextWrapper()
		: m_pContext(nil)
	{
		Acquire();
	}

	~AsyncContextWrapper()
	{
		Release();
	}

	/** Set this context to the active context. */
	Bool MakeCurrent()
	{
		// Sanity check - must never be called on the render thread.
		SEOUL_ASSERT(!IsRenderThread());

		// Nop if no context.
		if (nil == m_pContext)
		{
			return false;
		}

		[EAGLContext setCurrentContext:m_pContext];
		return true;
	}

	/** Populate this context - nop if already a valid context. */
	void Acquire()
	{
		// Nothing to do if we already have a context.
		if (nil != m_pContext)
		{
			return;
		}

		// Acquire a free context if possible.
		m_pContext = s_FreeContexts.Pop();

		// Create a new one if necessary.
		if (nil == m_pContext)
		{
			// Cannot create if no render thread context.
			if (nil == s_pRenderThreadContext)
			{
				return;
			}

			// Create - the money is that we use the render thread's share group.
			// This allows us to instantiate graphics objects on threads other than
			// the render thread.
			m_pContext = [[EAGLContext alloc] initWithAPI:(EAGLRenderingAPI)s_iAPI sharegroup:s_pRenderThreadContext.sharegroup];
		}
	}

	/** Destroy this context - nop if already a valid context. */
	void Release()
	{
		// Nothing to do if we don't have a context.
		if (nil == m_pContext)
		{
			return;
		}

		// Sanity check - only unset if we're set to the current thread's
		// context.
		EAGLContext* pContext = [EAGLContext currentContext];
		if (m_pContext == pContext)
		{
			[EAGLContext setCurrentContext:nil];
		}

		// Release the context.
		[m_pContext release];
		m_pContext = nil;
	}

private:
	EAGLContext* m_pContext;

	SEOUL_DISABLE_COPY(AsyncContextWrapper);
}; // class AsyncContextWrapper

} // namespace anonymous

static HeapAllocatedPerThreadStorage<AsyncContextWrapper, 64> s_AsyncContexts;

/** Get the current thread's context and activate it. */
Bool MakeAsyncContextActive()
{
	// Sanity check.
	SEOUL_ASSERT(!IsRenderThread());

	auto& r = s_AsyncContexts.Get();
	r.Acquire();
	return r.MakeCurrent();
}

/** Set the current thread's context to nil. */
void MakeAsyncContextInactive()
{
	// Sanity check
	SEOUL_ASSERT(!IsRenderThread());

	[EAGLContext setCurrentContext:nil];
}

/** On the render thread only, restore the global render context. */
Bool RestoreEAGLContext()
{
	// Sanity check
	SEOUL_ASSERT(IsRenderThread());

	if (nil != s_pRenderThreadContext)
	{
		[EAGLContext setCurrentContext:s_pRenderThreadContext];
		return true;
	}

	return false;
}

/** Create the render thread context, as well as a pool of async contexts. */
Bool InitializeEAGLContext(Bool& rbSupportsES3, Bool &rbSupportsAsyncTextureCreate)
{
	// Sanity check
	SEOUL_ASSERT(IsRenderThread());

	SEOUL_ASSERT(nil == s_pRenderThreadContext);

	// Always true on iOS.
	rbSupportsAsyncTextureCreate = true;

	// Start with OpenGL ES3.
	s_pRenderThreadContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];

	// If ES3 fails, switch to ES2.
	if (nil == s_pRenderThreadContext)
	{
		s_pRenderThreadContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
		rbSupportsES3 = false;
		s_iAPI = kEAGLRenderingAPIOpenGLES2;
	}
	// Otherwise, support ES3.
	else
	{
		rbSupportsES3 = true;
		s_iAPI = kEAGLRenderingAPIOpenGLES3;
	}

	// Now create enough free contexts for the number of general purpose threads on the system.
	{
		UInt32 const uThreads = Jobs::Manager::Get()->GetGeneralPurposeWorkerThreadCount();
		for (UInt32 i = 0u; i < uThreads; ++i)
		{
			// Create - the money is that we use the render thread's share group.
			// This allows us to instantiate graphics objects on threads other than
			// the render thread.
			s_FreeContexts.Push(
				[[EAGLContext alloc] initWithAPI:(EAGLRenderingAPI)s_iAPI sharegroup:s_pRenderThreadContext.sharegroup]);
		}
	}

	return RestoreEAGLContext();
}

/** Tear down the render thread and all async contexts. */
void DeinitializeEAGLContext()
{
	// Sanity check
	SEOUL_ASSERT(IsRenderThread());

	// Nothing to do if already teared down.
	if (nil == s_pRenderThreadContext)
	{
		return;
	}

	// Release any other contexts.
	{
		auto& rObjects = s_AsyncContexts.GetAllObjects();
		for (auto i = rObjects.Begin(); rObjects.End() != i; ++i)
		{
			auto p = *i;
			if (nullptr != p)
			{
				p->Release();
			}
		}
	}

	// Release any unused free contexts.
	{
		auto p = s_FreeContexts.Pop();
		while (nil != p)
		{
			[p release];
			 p = s_FreeContexts.Pop();
		}
	}

	// Destroy the render thread context.
	EAGLContext* pContext = [EAGLContext currentContext];
	if (s_pRenderThreadContext == pContext)
	{
		[EAGLContext setCurrentContext:nil];
	}

	[s_pRenderThreadContext release];
	s_pRenderThreadContext = nil;
}

// NOTE: While renderbufferStorage definitely must
// be called on the UI thread (our main thread),
// Apple has confirmed that it is safe to
// call presentRenderbuffer on threads other
// than the UI/main thread, so EAGLSwapBuffers()
// just runs this immediately:
//
// Follow-up: <redacted>
//
// Hello <redacted>,
//
// The answer to your question is yes, this is safe as per this comment
// from engineering “They can be called off the main thread, but they
// are still subject to the standard OpenGL concurrency rules: you
// cannot call into the same context from multiple threads at once,
// and you cannot modify an object on one context while it is bound to
// a different context.”
//
// You can find more detailed information on OpenGL ES and concurrency here:
// <https://developer.apple.com/library/content/documentation/3DDrawing/Conceptual/OpenGLES_ProgrammingGuide/ConcurrencyandOpenGLES/ConcurrencyandOpenGLES.html>
//
// Original question:
//
// As the title says, I want to confirm whether it is safe to call
// [EAGLContext presentRenderbuffer] on a thread other than the main/UI thread.
//
// If it is not safe, what is the recommended method to present a render
// buffer prepared off the main/UI thread?

/** Present back buffer to front on the render thread. */
Bool EAGLSwapBuffers(GLint colorBuffer)
{
	// Sanity check
	SEOUL_ASSERT(IsRenderThread());

	if (nil != s_pRenderThreadContext)
	{
		glBindRenderbuffer(GL_RENDERBUFFER, colorBuffer);
		Bool bReturn = [s_pRenderThreadContext presentRenderbuffer:GL_RENDERBUFFER];
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		return bReturn;
	}

	return false;
}

// TODO: renderbufferStorage definitely must
// be called on the UI thread (our main thread), we've
// seen it intermittently cause heap corruption otherwise.
// This is not a practical issue, since it's called once ever,
// but it would be nice to officially confirm that constraint with Apple.
static void MainThreadInitializeEAGLBackBufferColorBuffer(void* pVoidLayer)
{
	// Sanity check
	SEOUL_ASSERT(IsMainThread());

	CAEAGLLayer *pLayer = (CAEAGLLayer *)pVoidLayer;
	SEOUL_ASSERT(nil != s_pRenderThreadContext);

	// Need to manage ownership since we're "borrowing" the render
	// thread context for this call.
	EAGLContext* pOldContext = [EAGLContext currentContext];
	[EAGLContext setCurrentContext:s_pRenderThreadContext];
	[s_pRenderThreadContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:pLayer];
	[EAGLContext setCurrentContext:pOldContext];
}

/** Initialize storage for our main color back buffer. */
void InitializeEAGLBackBufferColorBuffer(void* pVoidLayer)
{
	// Sanity check
	SEOUL_ASSERT(IsRenderThread());

	// Interactions with pLayer must happen on the iOS UI
	// thread (our main thread). This is undocumented, but
	// we see intermittent heap corruption otherwise.
	Jobs::AwaitFunction(
		GetMainThreadId(),
		&MainThreadInitializeEAGLBackBufferColorBuffer,
		pVoidLayer);
}

} // namespace Seoul

#endif // /SEOUL_PLATFORM_IOS
