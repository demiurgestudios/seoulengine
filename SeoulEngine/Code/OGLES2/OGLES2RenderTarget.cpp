/**
 * \file OGLES2RenderTarget.cpp
 * \brief Specialization of RenderTarget for the OGLES2 platform. RenderTarget
 * encapsulates a renderable color surface.
 *
 * In OGLES2, RenderTarget can potentially encapsulate
 * 2 renderable color surfaces, to allow a surface to be sampled while it
 * is the render target.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "OGLES2RenderDevice.h"
#include "OGLES2RenderTarget.h"
#include "ThreadId.h"

namespace Seoul
{

/**
 * Construct this OGLES2RenderTarget from a DataStore
 * that describes it. If an error occurs, OGLES2RenderTarget will be left
 * in its default (invalid) state and will not be usable as a render target.
 */
OGLES2RenderTarget::OGLES2RenderTarget(const DataStoreTableUtil& configSettings)
	: RenderTarget(configSettings)
	, m_TextureA(0)
	, m_TextureB(0)
{
}

OGLES2RenderTarget::~OGLES2RenderTarget()
{
	SEOUL_ASSERT(IsRenderThread());

	// Unless a target needs to change with changes to
	// the back buffer, its resources are not destroyed
	// until the object is. Do so now.
	if (!IsProportional())
	{
		InternalDestroy();
	}
}

/**
 * Called by the device when this OGLES2RenderTarget needs to
 * be set as the active render target.
 */
void OGLES2RenderTarget::Select()
{
	SEOUL_ASSERT(IsRenderThread());

	// Avoid redundant sets of the same render target, however, if this
	// is an input-output target, we must re-select the target, since the
	// actual surface may have changed.
	if (SupportsSimultaneousInputOutput() || s_pActiveRenderTarget != this)
	{
		auto& rd = GetOGLES2RenderDevice();
		rd.SetRenderTarget(this);
		s_pActiveRenderTarget = this;
	}
}

/**
 * Resolves this RenderTarget to its texture.
 *
 * In OGLES2, this simply swaps the active surface and texture.
 */
void OGLES2RenderTarget::Resolve()
{
	SEOUL_ASSERT(IsRenderThread());
	if (SupportsSimultaneousInputOutput())
	{
		Swap(m_TextureA, m_TextureB);
	}
}

/**
 * Reset the render target to its default state,
 * if this render target is the currently active target.
 */
void OGLES2RenderTarget::Unselect()
{
	SEOUL_ASSERT(IsRenderThread());

	if (s_pActiveRenderTarget != this)
	{
		return;
	}

	// Reset.
	auto& rd = GetOGLES2RenderDevice();
	rd.SetRenderTarget(nullptr);

	// Clear.
	s_pActiveRenderTarget = nullptr;
}

/**
 * On create, if not a proportional target, create
 * resources.
 */
Bool OGLES2RenderTarget::OnCreate()
{
	SEOUL_ASSERT(IsRenderThread());

	// Unless a render target's resources are proportional
	// to the back buffer, we create its resources in OnCreate().
	if (!IsProportional())
	{
		// Creation failure is an OnCreate() failure.
		if (!InternalCreate())
		{
			return false;
		}
	}

	SEOUL_VERIFY(RenderTarget::OnCreate());
	return true;
}

/**
 * On lost, resets all memory pointers and places this
 * OGLES2RenderTarget into the lost state.
 */
void OGLES2RenderTarget::OnLost()
{
	SEOUL_ASSERT(IsRenderThread());

	Unselect();

	RenderTarget::OnLost();

	// We must cleanup proportional resources in OnLost().
	if (IsProportional())
	{
		InternalDestroy();
	}
}

/** Shared generation of a texture instance for branches of InternalCreate(). */
static GLuint InternalStaticGenerateTexture(
	PixelFormat eFormat,
	UInt32 uWidth,
	UInt32 uHeight)
{
	SEOUL_ASSERT(IsRenderThread());

	GLuint ret = 0;
	SEOUL_OGLES2_VERIFY(glGenTextures(1, &ret));
	if (0 != ret)
	{
		SEOUL_OGLES2_VERIFY(glBindTexture(GL_TEXTURE_2D, ret));

		// Wrap mode
		SEOUL_OGLES2_VERIFY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		SEOUL_OGLES2_VERIFY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

		// Sampling
		SEOUL_OGLES2_VERIFY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		SEOUL_OGLES2_VERIFY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));

		SEOUL_OGLES2_VERIFY(GetOGLES2RenderDevice().TexImage2D(
			GL_TEXTURE_2D,
			0,
			PixelFormatToOpenGlInternalFormat(eFormat),
			uWidth,
			uHeight,
			0,
			PixelFormatToOpenGlFormat(eFormat),
			PixelFormatToOpenGlElementType(eFormat),
			nullptr));

		SEOUL_OGLES2_VERIFY(glBindTexture(GL_TEXTURE_2D, 0));

		// Make sure the state manager's view of things is in sync once we're done.
		GetOGLES2RenderDevice().GetStateManager().RestoreActiveTextureIfSet(GL_TEXTURE_2D);
	}

	return ret;
}

/**
 * Actually initializes the renderable state of this
 * RenderTarget.
 */
void OGLES2RenderTarget::OnReset()
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
		RenderTarget::OnReset();
	}
}

/** Shared creation of target resources. */
Bool OGLES2RenderTarget::InternalCreate()
{
	SEOUL_ASSERT(IsRenderThread());

	// Sanity check
	SEOUL_ASSERT(0 == m_TextureA);
	SEOUL_ASSERT(0 == m_TextureB);

	// Refresh the width and height, in case they are dependent on
	// the back buffer.
	InternalRefreshWidthAndHeight();

	GLuint textureA = InternalStaticGenerateTexture(m_eFormat, GetWidth(), GetHeight());
	GLuint textureB = textureA;
	if (0 != textureA)
	{
		if (SupportsSimultaneousInputOutput())
		{
			textureB = InternalStaticGenerateTexture(m_eFormat, GetWidth(), GetHeight());
		}
	}

	// Success, populate and return result.
	if (0 != textureA && 0 != textureB)
	{
		m_TextureA = textureA;
		m_TextureB = textureB;

		return true;
	}
	else
	{
		if (0 != textureA)
		{
			glDeleteTextures(1, &textureA);
		}

		if (0 != textureB)
		{
			glDeleteTextures(1, &textureB);
		}
	}

	return false;
}

/** Shared destruction of target resources. */
void OGLES2RenderTarget::InternalDestroy()
{
	SEOUL_ASSERT(IsRenderThread());

	if (m_TextureB != m_TextureA)
	{
		glDeleteTextures(1, &m_TextureB);
	}
	m_TextureB = 0;

	glDeleteTextures(1, &m_TextureA);
	m_TextureA = 0;
}

} // namespace Seoul
