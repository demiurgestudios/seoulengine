/**
 * \file OGLES2DepthStencilSurface.cpp
 * \brief Specialization of DepthStencilSurface for OGLES2.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "OGLES2DepthStencilSurface.h"
#include "OGLES2RenderDevice.h"

namespace Seoul
{

/**
 * Construct a OGLES2DepthStencilSurface from a DataStore that
 * fully describes it. If this fails, this OGLES2DepthStencilSurface will be
 * left in its default (invalid) state.
 */
OGLES2DepthStencilSurface::OGLES2DepthStencilSurface(const DataStoreTableUtil& configSettings)
	: DepthStencilSurface(configSettings)
	, m_DepthSurface(0)
	, m_StencilSurface(0)
{
}

OGLES2DepthStencilSurface::~OGLES2DepthStencilSurface()
{
	SEOUL_ASSERT(IsRenderThread());

	// Unless a surface needs to change with changes to
	// the back buffer, its resources are not destroyed
	// until the object is. Do so now.
	if (!IsProportional())
	{
		InternalDestroy();
	}
}

/**
 * Make this OGLES2DepthStencilSurface the active depth-stencil
 * surface for rendering.
 */
void OGLES2DepthStencilSurface::Select()
{
	SEOUL_ASSERT(IsRenderThread());

	// Avoid redundant sets of the depth buffer.
	if (this != s_pCurrentSurface)
	{
		OGLES2RenderDevice& rd = GetOGLES2RenderDevice();
		rd.SetDepthStencilSurface(this);
		s_pCurrentSurface = this;
	}
}

/**
 * If this DepthStencilSurface is the active surface, set the active
 * surface to nullptr.
 */
void OGLES2DepthStencilSurface::Unselect()
{
	SEOUL_ASSERT(IsRenderThread());

	if (s_pCurrentSurface != this)
	{
		return;
	}

	OGLES2RenderDevice& rd = GetOGLES2RenderDevice();
	rd.SetDepthStencilSurface(nullptr);
	s_pCurrentSurface = nullptr;
}

/**
 * Nothing to do when resolving a depth-stencil surface currently.
 */
void OGLES2DepthStencilSurface::Resolve()
{
	// Nop
}

/**
 * On create, if not a proportional surface, create
 * resources.
 */
Bool OGLES2DepthStencilSurface::OnCreate()
{
	SEOUL_ASSERT(IsRenderThread());

	// Unless a surface's resources are proportional
	// to the back buffer, we create its resources in OnCreate().
	if (!IsProportional())
	{
		// Creation failure is an OnCreate() failure.
		if (!InternalCreate())
		{
			return false;
		}
	}

	SEOUL_VERIFY(DepthStencilSurface::OnCreate());
	return true;
}

/**
 * On lost, resets all memory pointers and places this
 * OGLES2DepthStencilSurface into the lost state.
 */
void OGLES2DepthStencilSurface::OnLost()
{
	SEOUL_ASSERT(IsRenderThread());

	Unselect();

	DepthStencilSurface::OnLost();

	// We must cleanup proportional resources in OnLost().
	if (IsProportional())
	{
		InternalDestroy();
	}
}

/**
 * Setup this DepthStencilSurface for rendering.
 */
void OGLES2DepthStencilSurface::OnReset()
{
	SEOUL_ASSERT(IsRenderThread());

	// Proportional resources are created
	// in OnReset() and destroyed in OnLost(). Otherwise,
	// they remain alive for the life of the object
	// under OGLES2.
	Bool bReset = true;
	if (IsProportional())
	{
		bReset = InternalCreate();
	}

	// Success, call the parent implementation.
	if (bReset)
	{
		DepthStencilSurface::OnReset();
	}
}

/** Shared creation of surface resources. */
Bool OGLES2DepthStencilSurface::InternalCreate()
{
	SEOUL_ASSERT(IsRenderThread());

	// Sanity check
	SEOUL_ASSERT(0 == m_DepthSurface);
	SEOUL_ASSERT(0 == m_StencilSurface);

	// Refresh the width and height, in case they are relative to
	// the back buffer. This must happen before the call to rd.ValidateAgainstTargetGroup(),
	// since it uses the width and height of this DepthStencilSurface in the check.
	InternalRefreshWidthAndHeight();

	GLuint depth = 0;
	GLuint stencil = 0;
	SEOUL_OGLES2_VERIFY(glGenRenderbuffers(1, &depth));

	SEOUL_OGLES2_VERIFY(glBindRenderbuffer(GL_RENDERBUFFER, depth));

	// Clear the error bit.
	OGLES2ClearError();

	// Try to initialize the depth-stencil surface as a combined format.
	glRenderbufferStorage(
		GL_RENDERBUFFER,
		DepthStencilFormatToOpenGlFormat(m_eFormat),
		GetWidth(),
		GetHeight());

	// If this failed, try initializing the depth-stencil format as separate renderbuffers.
	if (GL_NO_ERROR != glGetError())
	{
		SEOUL_OGLES2_VERIFY(glRenderbufferStorage(
			GL_RENDERBUFFER,
			DepthStencilFormatToOpenGlDepthFormat(m_eFormat),
			GetWidth(),
			GetHeight()));

		if (DepthStencilFormatHasStencilBuffer(m_eFormat))
		{
			SEOUL_OGLES2_VERIFY(glBindRenderbuffer(GL_RENDERBUFFER, 0));
			SEOUL_OGLES2_VERIFY(glGenRenderbuffers(1, &stencil));
			SEOUL_OGLES2_VERIFY(glBindRenderbuffer(GL_RENDERBUFFER, stencil));
			SEOUL_OGLES2_VERIFY(glRenderbufferStorage(
				GL_RENDERBUFFER,
				DepthStencilFormatToOpenGlStencilFormat(m_eFormat),
				GetWidth(),
				GetHeight()));
		}
		else
		{
			stencil = 0;
		}
	}
	else if (DepthStencilFormatHasStencilBuffer(m_eFormat))
	{
		stencil = depth;
	}
	else
	{
		stencil = 0;
	}

	SEOUL_OGLES2_VERIFY(glBindRenderbuffer(GL_RENDERBUFFER, 0));

	m_DepthSurface = depth;
	m_StencilSurface = stencil;
	return true;
}

/** Shared destruction of surface resources. */
void OGLES2DepthStencilSurface::InternalDestroy()
{
	SEOUL_ASSERT(IsRenderThread());

	if (m_DepthSurface == m_StencilSurface)
	{
		if (0 != m_DepthSurface)
		{
			glDeleteRenderbuffers(1, &m_DepthSurface);
		}
		m_StencilSurface = 0;
		m_DepthSurface = 0;
	}
	else
	{
		if (0 != m_StencilSurface)
		{
			glDeleteRenderbuffers(1, &m_StencilSurface);
		}
		m_StencilSurface = 0;

		if (0 != m_DepthSurface)
		{
			glDeleteRenderbuffers(1, &m_DepthSurface);
		}
		m_DepthSurface = 0;
	}
}

} // namespace Seoul
