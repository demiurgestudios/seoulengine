/**
 * \file OGLES2StateManager.cpp
 * \brief Implementation of render state and sampler state management
 * and filtering. Roughly equivalent to ID3DXEffectStateManager in D3D9,
 * except more low-level and required (since there is no other mechanism
 * that can set render and sampler states from enum+value pairs).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Color.h"
#include "OGLES2StateManager.h"
#include "OGLES2Util.h"
#include "ThreadId.h"

namespace Seoul
{

/**
 * @return A bit-for-bit conversion of the Float value fValue to
 * a UInt32 value, used for sampler state setting.
 */
inline UInt32 ToUInt32(Float fValue)
{
	// Sanity check that Float is the size we expect.
	SEOUL_STATIC_ASSERT(sizeof(fValue) == sizeof(UInt32));

	union
	{
		Float in;
		UInt32 out;
	};

	in = fValue;
	return out;
}

/**
 * @return A bit-for-bit conversion of the UInt32 value uValue to
 * a Float value, used for sampler state setting.
 */
inline Float ToFloat(UInt32 uValue)
{
	union
	{
		UInt32 in;
		Float out;
	};

	in = uValue;
	return out;
}

/**
 * Populate auDefaultRenderStates with default render states.
 */
inline void GetDefaultRenderStates(FixedArray<UInt32, (UInt32)RenderState::RENDER_STATE_COUNT>& auDefaultRenderStates)
{
	auDefaultRenderStates[(UInt32)RenderState::kAlphaBlendEnable] = GL_FALSE;
	auDefaultRenderStates[(UInt32)RenderState::kAlphaFunction] = GL_ALWAYS;
	auDefaultRenderStates[(UInt32)RenderState::kAlphaReference] = 0u;
	auDefaultRenderStates[(UInt32)RenderState::kAlphaTestEnable] = GL_FALSE;

	auDefaultRenderStates[(UInt32)RenderState::kBackFacingStencilDepthFail] =  GL_KEEP;
	auDefaultRenderStates[(UInt32)RenderState::kBackFacingStencilFail] = GL_KEEP;
	auDefaultRenderStates[(UInt32)RenderState::kBackFacingStencilFunc] = GL_ALWAYS;
	auDefaultRenderStates[(UInt32)RenderState::kBackFacingStencilPass] = GL_KEEP;

	auDefaultRenderStates[(UInt32)RenderState::kBlendColor] = 0u;
	auDefaultRenderStates[(UInt32)RenderState::kBlendOp] = GL_FUNC_ADD;
	auDefaultRenderStates[(UInt32)RenderState::kBlendOpAlpha] = GL_FUNC_ADD;

	auDefaultRenderStates[(UInt32)RenderState::kColorWriteEnable] = 0xFFFFFFFF;
	auDefaultRenderStates[(UInt32)RenderState::kColorWriteEnable1] = 0u;
	auDefaultRenderStates[(UInt32)RenderState::kColorWriteEnable2] = 0u;
	auDefaultRenderStates[(UInt32)RenderState::kColorWriteEnable3] = 0u;

	auDefaultRenderStates[(UInt32)RenderState::kCull] = (UInt32)CullMode::kNone;

	auDefaultRenderStates[(UInt32)RenderState::kDepthBias] = ToUInt32(0.0f);
	auDefaultRenderStates[(UInt32)RenderState::kDepthEnable] = GL_FALSE;
	auDefaultRenderStates[(UInt32)RenderState::kDepthFunction] = GL_LESS;
	auDefaultRenderStates[(UInt32)RenderState::kDepthWriteEnable] = GL_TRUE;

	auDefaultRenderStates[(UInt32)RenderState::kDestinationBlend] = GL_ZERO;
	auDefaultRenderStates[(UInt32)RenderState::kDestinationBlendAlpha] = GL_ZERO;
	auDefaultRenderStates[(UInt32)RenderState::kFillMode] = 0; // TODO: GL_FILL;

	auDefaultRenderStates[(UInt32)RenderState::kScissor] = GL_FALSE;

	auDefaultRenderStates[(UInt32)RenderState::kSeparateAlphaBlendEnable] = GL_FALSE;
	auDefaultRenderStates[(UInt32)RenderState::kShadeMode] = 0; // TODO: GL_SMOOTH;
	auDefaultRenderStates[(UInt32)RenderState::kSlopeScaleDepthBias] = ToUInt32(0.0f);

	auDefaultRenderStates[(UInt32)RenderState::kSourceBlend] = GL_ONE;
	auDefaultRenderStates[(UInt32)RenderState::kSourceBlendAlpha] = GL_ONE;

	auDefaultRenderStates[(UInt32)RenderState::kSRGBWriteEnable] = GL_FALSE;

	auDefaultRenderStates[(UInt32)RenderState::kStencilDepthFail] = GL_KEEP;
	auDefaultRenderStates[(UInt32)RenderState::kStencilEnable] = GL_FALSE;
	auDefaultRenderStates[(UInt32)RenderState::kStencilFail] = GL_KEEP;
	auDefaultRenderStates[(UInt32)RenderState::kStencilFunction] = GL_ALWAYS;
	auDefaultRenderStates[(UInt32)RenderState::kStencilMask] = 0x000000FF;
	auDefaultRenderStates[(UInt32)RenderState::kStencilPass] = GL_KEEP;
	auDefaultRenderStates[(UInt32)RenderState::kStencilReference] = 0u;
	auDefaultRenderStates[(UInt32)RenderState::kStencilWriteMask] = 0x000000FF;
	auDefaultRenderStates[(UInt32)RenderState::kTwoSidedStencilMode] = GL_FALSE;
}

/**
 * Construct this OGLES2StateManager with default sampler and
 * renders states.
 *
 * The RSX must be in its default context when this OGLES2StateManager
 * is constructed for the RSX and this OGLES2StateManager to be in sync.
 */
OGLES2StateManager::OGLES2StateManager()
	: m_CurrentScissorState()
	, m_PendingScissorState()
	, m_CurrentViewportState()
	, m_PendingViewportState()
	, m_auCurrentRenderStates(0u)
	, m_auPendingRenderStates(0u)
	, m_auDefaultRenderStates(0u)
	, m_auActiveTexture2d(0u)
	, m_uActiveTexture2d(UIntMax)
	, m_bRenderStateDirty(false)
{
	GetDefaultRenderStates(m_auCurrentRenderStates);
	GetDefaultRenderStates(m_auPendingRenderStates);
	GetDefaultRenderStates(m_auDefaultRenderStates);
}

OGLES2StateManager::~OGLES2StateManager()
{
}

/**
 * Submit all changes necessary to put the OpenGl ES API
 * into its default render and sampler states.
 */
void OGLES2StateManager::ApplyDefaultRenderStates()
{
	for (UInt32 i = 0u; i <= 31u; ++i)
	{
		SetActiveTexture(GL_TEXTURE_2D, i, 0u);
	}

	for (Int i = 0; i < (Int)RenderState::RENDER_STATE_COUNT; ++i)
	{
		SetRenderState((RenderState)i, m_auDefaultRenderStates[i]);
	}

	CommitPendingStates();

	// Make scissor and viewport dirty so they must commit the next
	// time a commit is required.
	MarkScissorRectangleDirty();
	MarkViewportRectangleDirty();
}

/**
 * Set the render state eState to the value uValue.
 *
 * This method filters redundant states. No commit will occur
 * if eState is already set to value uValue.
 */
void OGLES2StateManager::SetRenderState(RenderState eState, UInt32 uValue)
{
	if (uValue != m_auPendingRenderStates[(UInt32)eState])
	{
		m_auPendingRenderStates[(UInt32)eState] = uValue;
		m_bRenderStateDirty = true;
	}
}

void OGLES2StateManager::SetActiveTexture(
	GLenum eTextureType,
	UInt32 uSamplerIndex,
	UInt32 uTextureId)
{
	switch (eTextureType)
	{
	case GL_TEXTURE_2D:
		if (m_auActiveTexture2d[uSamplerIndex] != uTextureId)
		{
			if (uSamplerIndex != m_uActiveTexture2d)
			{
				SEOUL_OGLES2_VERIFY(glActiveTexture(GL_TEXTURE0 + uSamplerIndex));
				m_uActiveTexture2d = uSamplerIndex;
			}

			SEOUL_OGLES2_VERIFY(glBindTexture(GL_TEXTURE_2D, uTextureId));
			m_auActiveTexture2d[uSamplerIndex] = uTextureId;
		}
		break;
	default:
		break;
	};
}

/**
 * Call to check and restore the texture bound to the currently
 * active texture if it is non-zero.
 */
void OGLES2StateManager::RestoreActiveTextureIfSet(GLenum eTextureType)
{
	switch (eTextureType)
	{
	case GL_TEXTURE_2D:
		if (m_uActiveTexture2d < m_auActiveTexture2d.GetSize() &&
			m_auActiveTexture2d[m_uActiveTexture2d] != 0u)
		{
			SEOUL_OGLES2_VERIFY(glBindTexture(GL_TEXTURE_2D, m_auActiveTexture2d[m_uActiveTexture2d]));
		}
		break;
	default:
		break;
	};
}

/**
 * Make a change to the scissor rectangle.
 */
void OGLES2StateManager::SetScissor(GLint iX, GLint iY, GLsizei iWidth, GLsizei iHeight)
{
	m_PendingScissorState.m_iX = iX;
	m_PendingScissorState.m_iY = iY;
	m_PendingScissorState.m_iWidth = iWidth;
	m_PendingScissorState.m_iHeight = iHeight;
}

/**
 * Make a change to the viewport rectangle.
 */
void OGLES2StateManager::SetViewport(GLint iX, GLint iY, GLsizei iWidth, GLsizei iHeight)
{
	m_PendingViewportState.m_iX = iX;
	m_PendingViewportState.m_iY = iY;
	m_PendingViewportState.m_iWidth = iWidth;
	m_PendingViewportState.m_iHeight = iHeight;
}

/**
 * Used by SetRenderState() to actually commit render states to
 * the OpenGL ES API.
 */
void OGLES2StateManager::InternalCommitPendingStates()
{
	SEOUL_ASSERT(IsRenderThread());

	// Commit scissor changes if pending.
	if (m_PendingScissorState != m_CurrentScissorState)
	{
		m_CurrentScissorState = m_PendingScissorState;

		SEOUL_OGLES2_VERIFY(glScissor(
			m_CurrentScissorState.m_iX,
			m_CurrentScissorState.m_iY,
			m_CurrentScissorState.m_iWidth,
			m_CurrentScissorState.m_iHeight));
	}

	// Commit viewport changes if pending.
	if (m_PendingViewportState != m_CurrentViewportState)
	{
		m_CurrentViewportState = m_PendingViewportState;

		SEOUL_OGLES2_VERIFY(glViewport(
			m_CurrentViewportState.m_iX,
			m_CurrentViewportState.m_iY,
			m_CurrentViewportState.m_iWidth,
			m_CurrentViewportState.m_iHeight));
	}

	if (!m_bRenderStateDirty)
	{
		return;
	}

	for (Int i = 0; i < (Int)RenderState::RENDER_STATE_COUNT; ++i)
	{
		if (m_auCurrentRenderStates[i] == m_auPendingRenderStates[i])
		{
			continue;
		}

		m_auCurrentRenderStates[i] = m_auPendingRenderStates[i];

		RenderState const eState = (RenderState)i;
		UInt32 const uValue = m_auCurrentRenderStates[i];
		switch (eState)
		{
		case RenderState::kAlphaBlendEnable:
			if (uValue == GL_FALSE) { glDisable(GL_BLEND); }
			else { glEnable(GL_BLEND); }
			break;
		case RenderState::kAlphaFunction:
			// TODO: kAlphaFunction is not supported in OpenGL ES
			break;
		case RenderState::kAlphaReference:
			// TODO: kAlphaReference is not supported in OpenGL ES
			break;
		case RenderState::kAlphaTestEnable:
			// TODO: kAlphaTestEnable is not supported in OpenGL ES
			break;
		case RenderState::kBackFacingStencilDepthFail:
			m_auCurrentRenderStates[(UInt32)RenderState::kTwoSidedStencilMode] = m_auPendingRenderStates[(UInt32)RenderState::kTwoSidedStencilMode];
			if (GL_TRUE == m_auCurrentRenderStates[(UInt32)RenderState::kTwoSidedStencilMode])
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kBackFacingStencilFail] = m_auPendingRenderStates[(UInt32)RenderState::kBackFacingStencilFail];
				m_auCurrentRenderStates[(UInt32)RenderState::kBackFacingStencilPass] = m_auPendingRenderStates[(UInt32)RenderState::kBackFacingStencilPass];

				glStencilOpSeparate(
					GL_BACK,
					m_auCurrentRenderStates[(UInt32)RenderState::kBackFacingStencilFail],
					uValue,
					m_auCurrentRenderStates[(UInt32)RenderState::kBackFacingStencilPass]);
			}
			break;
		case RenderState::kBackFacingStencilFail:
			m_auCurrentRenderStates[(UInt32)RenderState::kTwoSidedStencilMode] = m_auPendingRenderStates[(UInt32)RenderState::kTwoSidedStencilMode];
			if (GL_TRUE == m_auCurrentRenderStates[(UInt32)RenderState::kTwoSidedStencilMode])
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kBackFacingStencilDepthFail] = m_auPendingRenderStates[(UInt32)RenderState::kBackFacingStencilDepthFail];
				m_auCurrentRenderStates[(UInt32)RenderState::kBackFacingStencilPass] = m_auPendingRenderStates[(UInt32)RenderState::kBackFacingStencilPass];

				glStencilOpSeparate(
					GL_BACK,
					uValue,
					m_auCurrentRenderStates[(UInt32)RenderState::kBackFacingStencilDepthFail],
					m_auCurrentRenderStates[(UInt32)RenderState::kBackFacingStencilPass]);
			}
			break;
		case RenderState::kBackFacingStencilFunc:
			m_auCurrentRenderStates[(UInt32)RenderState::kTwoSidedStencilMode] = m_auPendingRenderStates[(UInt32)RenderState::kTwoSidedStencilMode];
			if (GL_TRUE == m_auCurrentRenderStates[(UInt32)RenderState::kTwoSidedStencilMode])
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilReference] = m_auPendingRenderStates[(UInt32)RenderState::kStencilReference];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilMask] = m_auPendingRenderStates[(UInt32)RenderState::kStencilMask];

				glStencilFuncSeparate(
					GL_BACK,
					uValue,
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilReference],
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilMask]);
			}
			break;
		case RenderState::kBackFacingStencilPass:
			m_auCurrentRenderStates[(UInt32)RenderState::kTwoSidedStencilMode] = m_auPendingRenderStates[(UInt32)RenderState::kTwoSidedStencilMode];
			if (GL_TRUE == m_auCurrentRenderStates[(UInt32)RenderState::kTwoSidedStencilMode])
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kBackFacingStencilFail] = m_auPendingRenderStates[(UInt32)RenderState::kBackFacingStencilFail];
				m_auCurrentRenderStates[(UInt32)RenderState::kBackFacingStencilDepthFail] = m_auPendingRenderStates[(UInt32)RenderState::kBackFacingStencilDepthFail];

				glStencilOpSeparate(
					GL_BACK,
					m_auCurrentRenderStates[(UInt32)RenderState::kBackFacingStencilFail],
					m_auCurrentRenderStates[(UInt32)RenderState::kBackFacingStencilDepthFail],
					uValue);
			}
			break;
		case RenderState::kBlendColor:
			{
				ColorARGBu8 color;
				color.m_Value = uValue;
				Color4 color4(color);
				glBlendColor(color4.R, color4.G, color4.B, color4.A);
			}
			break;
		case RenderState::kBlendOp:
			m_auCurrentRenderStates[(UInt32)RenderState::kSeparateAlphaBlendEnable] = m_auPendingRenderStates[(UInt32)RenderState::kSeparateAlphaBlendEnable];
			if (GL_FALSE != m_auCurrentRenderStates[(UInt32)RenderState::kSeparateAlphaBlendEnable])
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kBlendOpAlpha] = m_auPendingRenderStates[(UInt32)RenderState::kBlendOpAlpha];
				glBlendEquationSeparate(uValue, m_auCurrentRenderStates[(UInt32)RenderState::kBlendOpAlpha]);
			}
			else
			{
				glBlendEquationSeparate(uValue, uValue);
			}
			break;
		case RenderState::kBlendOpAlpha:
			m_auCurrentRenderStates[(UInt32)RenderState::kSeparateAlphaBlendEnable] = m_auPendingRenderStates[(UInt32)RenderState::kSeparateAlphaBlendEnable];
			if (GL_FALSE != m_auCurrentRenderStates[(UInt32)RenderState::kSeparateAlphaBlendEnable])
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kBlendOp] = m_auPendingRenderStates[(UInt32)RenderState::kBlendOp];
				glBlendEquationSeparate(m_auCurrentRenderStates[(UInt32)RenderState::kBlendOp], uValue);
			}
			break;
		case RenderState::kColorWriteEnable:
			glColorMask(
				RenderStateUtil::GetComponent8<Components8Bit::kColorMaskR>(uValue),
				RenderStateUtil::GetComponent8<Components8Bit::kColorMaskG>(uValue),
				RenderStateUtil::GetComponent8<Components8Bit::kColorMaskB>(uValue),
				RenderStateUtil::GetComponent8<Components8Bit::kColorMaskA>(uValue));
			break;
		case RenderState::kColorWriteEnable1:
			// Nop
			break;
		case RenderState::kColorWriteEnable2:
			// Nop
			break;
		case RenderState::kColorWriteEnable3:
			// Nop
			break;
		case RenderState::kCull:
			switch ((CullMode)uValue)
			{
			case CullMode::kNone:
				glDisable(GL_CULL_FACE);
				break;
			case CullMode::kClockwise:
				glCullFace(GL_BACK);
				glEnable(GL_CULL_FACE);
				glFrontFace(GL_CCW);
				break;
			case CullMode::kCounterClockwise:
				glCullFace(GL_FRONT);
				glEnable(GL_CULL_FACE);
				glFrontFace(GL_CCW);
				break;
			};
			break;
		case RenderState::kDepthBias:
			m_auCurrentRenderStates[(UInt32)RenderState::kSlopeScaleDepthBias] = m_auPendingRenderStates[(UInt32)RenderState::kSlopeScaleDepthBias];
			glPolygonOffset(
				ToFloat(m_auCurrentRenderStates[(UInt32)RenderState::kSlopeScaleDepthBias]),
				ToFloat(uValue));
			break;
		case RenderState::kDepthEnable:
			if (GL_FALSE == uValue) { glDisable(GL_DEPTH_TEST); }
			else { glEnable(GL_DEPTH_TEST); }
			break;
		case RenderState::kDepthFunction:
			glDepthFunc(uValue);
			break;
		case RenderState::kDepthWriteEnable:
			glDepthMask(uValue);
			break;
		case RenderState::kDestinationBlend:
			m_auCurrentRenderStates[(UInt32)RenderState::kSeparateAlphaBlendEnable] = m_auPendingRenderStates[(UInt32)RenderState::kSeparateAlphaBlendEnable];
			if (GL_FALSE != m_auCurrentRenderStates[(UInt32)RenderState::kSeparateAlphaBlendEnable])
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlend] = m_auPendingRenderStates[(UInt32)RenderState::kSourceBlend];
				m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlendAlpha] = m_auPendingRenderStates[(UInt32)RenderState::kSourceBlendAlpha];
				m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlendAlpha] = m_auPendingRenderStates[(UInt32)RenderState::kDestinationBlendAlpha];

				glBlendFuncSeparate(
					m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlend],
					uValue,
					m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlendAlpha],
					m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlendAlpha]);
			}
			else
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlend] = m_auPendingRenderStates[(UInt32)RenderState::kSourceBlend];
				glBlendFuncSeparate(
					m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlend],
					uValue,
					m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlend],
					uValue);
			}
			break;
		case RenderState::kDestinationBlendAlpha:
			m_auCurrentRenderStates[(UInt32)RenderState::kSeparateAlphaBlendEnable] = m_auPendingRenderStates[(UInt32)RenderState::kSeparateAlphaBlendEnable];
			if (GL_FALSE != m_auCurrentRenderStates[(UInt32)RenderState::kSeparateAlphaBlendEnable])
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlend] = m_auPendingRenderStates[(UInt32)RenderState::kSourceBlend];
				m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlend] = m_auPendingRenderStates[(UInt32)RenderState::kDestinationBlend];
				m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlendAlpha] = m_auPendingRenderStates[(UInt32)RenderState::kSourceBlendAlpha];

				glBlendFuncSeparate(
					m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlend],
					m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlend],
					m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlendAlpha],
					uValue);
			}
			break;
		case RenderState::kFillMode:
			// TODO: kFillMode is not supported in OpenGL ES
			break;
		case RenderState::kScissor:
			if (GL_FALSE != uValue)
			{
				glEnable(GL_SCISSOR_TEST);
			}
			else
			{
				glDisable(GL_SCISSOR_TEST);
			}
			break;
		case RenderState::kSeparateAlphaBlendEnable:
			if (GL_FALSE != uValue)
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kBlendOp] = m_auPendingRenderStates[(UInt32)RenderState::kBlendOp];
				m_auCurrentRenderStates[(UInt32)RenderState::kBlendOpAlpha] = m_auPendingRenderStates[(UInt32)RenderState::kBlendOpAlpha];
				m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlend] = m_auPendingRenderStates[(UInt32)RenderState::kSourceBlend];
				m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlend] = m_auPendingRenderStates[(UInt32)RenderState::kDestinationBlend];
				m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlendAlpha] = m_auPendingRenderStates[(UInt32)RenderState::kSourceBlendAlpha];
				m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlendAlpha] = m_auPendingRenderStates[(UInt32)RenderState::kDestinationBlendAlpha];

				glBlendEquationSeparate(
					m_auCurrentRenderStates[(UInt32)RenderState::kBlendOp],
					m_auCurrentRenderStates[(UInt32)RenderState::kBlendOpAlpha]);
				glBlendFuncSeparate(
					m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlend],
					m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlend],
					m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlendAlpha],
					m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlendAlpha]);
			}
			else
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kBlendOp] = m_auPendingRenderStates[(UInt32)RenderState::kBlendOp];
				m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlend] = m_auPendingRenderStates[(UInt32)RenderState::kSourceBlend];
				m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlend] = m_auPendingRenderStates[(UInt32)RenderState::kDestinationBlend];

				glBlendEquationSeparate(
					m_auCurrentRenderStates[(UInt32)RenderState::kBlendOp],
					m_auCurrentRenderStates[(UInt32)RenderState::kBlendOp]);
				glBlendFuncSeparate(
					m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlend],
					m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlend],
					m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlend],
					m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlend]);
			}
			break;
		case RenderState::kShadeMode:
			// TODO: kShadeMode is not supported in OpenGL ES
			break;
		case RenderState::kSlopeScaleDepthBias:
			m_auCurrentRenderStates[(UInt32)RenderState::kDepthBias] = m_auPendingRenderStates[(UInt32)RenderState::kDepthBias];

			glPolygonOffset(
				ToFloat(uValue),
				ToFloat(m_auCurrentRenderStates[(UInt32)RenderState::kDepthBias]));
			break;
		case RenderState::kSourceBlend:
			m_auCurrentRenderStates[(UInt32)RenderState::kSeparateAlphaBlendEnable] = m_auPendingRenderStates[(UInt32)RenderState::kSeparateAlphaBlendEnable];
			if (GL_FALSE != m_auCurrentRenderStates[(UInt32)RenderState::kSeparateAlphaBlendEnable])
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlend] = m_auPendingRenderStates[(UInt32)RenderState::kDestinationBlend];
				m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlendAlpha] = m_auPendingRenderStates[(UInt32)RenderState::kSourceBlendAlpha];

				m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlendAlpha] = m_auPendingRenderStates[(UInt32)RenderState::kDestinationBlendAlpha];
				glBlendFuncSeparate(
					uValue,
					m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlend],
					m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlendAlpha],
					m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlendAlpha]);
			}
			else
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlend] = m_auPendingRenderStates[(UInt32)RenderState::kDestinationBlend];

				glBlendFuncSeparate(
					uValue,
					m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlend],
					uValue,
					m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlend]);
			}
			break;
		case RenderState::kSourceBlendAlpha:
			m_auCurrentRenderStates[(UInt32)RenderState::kSeparateAlphaBlendEnable] = m_auPendingRenderStates[(UInt32)RenderState::kSeparateAlphaBlendEnable];
			if (GL_FALSE != m_auCurrentRenderStates[(UInt32)RenderState::kSeparateAlphaBlendEnable])
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlend] = m_auPendingRenderStates[(UInt32)RenderState::kSourceBlend];
				m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlend] = m_auPendingRenderStates[(UInt32)RenderState::kDestinationBlend];
				m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlendAlpha] = m_auPendingRenderStates[(UInt32)RenderState::kDestinationBlendAlpha];

				glBlendFuncSeparate(
					m_auCurrentRenderStates[(UInt32)RenderState::kSourceBlend],
					m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlend],
					uValue,
					m_auCurrentRenderStates[(UInt32)RenderState::kDestinationBlendAlpha]);
			}
			break;
		case RenderState::kSRGBWriteEnable:
			// TODO: kSRGBWriteEnable is not supported in OpenGL ES
			break;
		case RenderState::kStencilDepthFail:
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kTwoSidedStencilMode] = m_auPendingRenderStates[(UInt32)RenderState::kTwoSidedStencilMode];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilFail] = m_auPendingRenderStates[(UInt32)RenderState::kStencilFail];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilPass] = m_auPendingRenderStates[(UInt32)RenderState::kStencilPass];

				GLenum mode = (GL_FALSE == m_auCurrentRenderStates[(UInt32)RenderState::kTwoSidedStencilMode]) ? GL_FRONT_AND_BACK : GL_FRONT;
				glStencilOpSeparate(
					mode,
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilFail],
					uValue,
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilPass]);
			}
			break;
		case RenderState::kStencilEnable:
			if (GL_FALSE == uValue) { glDisable(GL_STENCIL_TEST); }
			else { glEnable(GL_STENCIL_TEST); }
			break;
		case RenderState::kStencilFail:
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kTwoSidedStencilMode] = m_auPendingRenderStates[(UInt32)RenderState::kTwoSidedStencilMode];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilDepthFail] = m_auPendingRenderStates[(UInt32)RenderState::kStencilDepthFail];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilPass] = m_auPendingRenderStates[(UInt32)RenderState::kStencilPass];

				GLenum mode = (GL_FALSE == m_auCurrentRenderStates[(UInt32)RenderState::kTwoSidedStencilMode]) ? GL_FRONT_AND_BACK : GL_FRONT;
				glStencilOpSeparate(
					mode,
					uValue,
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilDepthFail],
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilPass]);
			}
			break;
		case RenderState::kStencilFunction:
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kTwoSidedStencilMode] = m_auPendingRenderStates[(UInt32)RenderState::kTwoSidedStencilMode];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilReference] = m_auPendingRenderStates[(UInt32)RenderState::kStencilReference];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilMask] = m_auPendingRenderStates[(UInt32)RenderState::kStencilMask];

				GLenum mode = (GL_FALSE == m_auCurrentRenderStates[(UInt32)RenderState::kTwoSidedStencilMode]) ? GL_FRONT_AND_BACK : GL_FRONT;
				glStencilFuncSeparate(
					mode,
					uValue,
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilReference],
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilMask]);
			}
			break;
		case RenderState::kStencilMask:
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kTwoSidedStencilMode] = m_auPendingRenderStates[(UInt32)RenderState::kTwoSidedStencilMode];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilFunction] = m_auPendingRenderStates[(UInt32)RenderState::kStencilFunction];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilReference] = m_auPendingRenderStates[(UInt32)RenderState::kStencilReference];

				GLenum mode = (GL_FALSE == m_auCurrentRenderStates[(UInt32)RenderState::kTwoSidedStencilMode]) ? GL_FRONT_AND_BACK : GL_FRONT;
				glStencilFuncSeparate(
					mode,
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilFunction],
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilReference],
					uValue);
			}
			break;
		case RenderState::kStencilPass:
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kTwoSidedStencilMode] = m_auPendingRenderStates[(UInt32)RenderState::kTwoSidedStencilMode];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilFail] = m_auPendingRenderStates[(UInt32)RenderState::kStencilFail];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilDepthFail] = m_auPendingRenderStates[(UInt32)RenderState::kStencilDepthFail];

				GLenum mode = (GL_FALSE == m_auCurrentRenderStates[(UInt32)RenderState::kTwoSidedStencilMode]) ? GL_FRONT_AND_BACK : GL_FRONT;
				glStencilOpSeparate(
					mode,
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilFail],
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilDepthFail],
					uValue);
			}
			break;
		case RenderState::kStencilReference:
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kTwoSidedStencilMode] = m_auPendingRenderStates[(UInt32)RenderState::kTwoSidedStencilMode];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilFunction] = m_auPendingRenderStates[(UInt32)RenderState::kStencilFunction];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilMask] = m_auPendingRenderStates[(UInt32)RenderState::kStencilMask];

				GLenum mode = (GL_FALSE == m_auCurrentRenderStates[(UInt32)RenderState::kTwoSidedStencilMode]) ? GL_FRONT_AND_BACK : GL_FRONT;
				glStencilFuncSeparate(
					mode,
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilFunction],
					uValue,
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilMask]);
			}
			break;
		case RenderState::kStencilWriteMask:
			glStencilMask(uValue);
			break;
		case RenderState::kTwoSidedStencilMode:
			if (GL_FALSE == uValue)
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilFunction] = m_auPendingRenderStates[(UInt32)RenderState::kStencilFunction];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilReference] = m_auPendingRenderStates[(UInt32)RenderState::kStencilReference];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilMask] = m_auPendingRenderStates[(UInt32)RenderState::kStencilMask];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilFail] = m_auPendingRenderStates[(UInt32)RenderState::kStencilFail];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilDepthFail] = m_auPendingRenderStates[(UInt32)RenderState::kStencilDepthFail];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilPass] = m_auPendingRenderStates[(UInt32)RenderState::kStencilPass];

				glStencilFunc(
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilFunction],
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilReference],
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilMask]);
				glStencilOp(
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilFail],
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilDepthFail],
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilPass]);
			}
			else
			{
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilFunction] = m_auPendingRenderStates[(UInt32)RenderState::kStencilFunction];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilReference] = m_auPendingRenderStates[(UInt32)RenderState::kStencilReference];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilMask] = m_auPendingRenderStates[(UInt32)RenderState::kStencilMask];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilFail] = m_auPendingRenderStates[(UInt32)RenderState::kStencilFail];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilDepthFail] = m_auPendingRenderStates[(UInt32)RenderState::kStencilDepthFail];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilPass] = m_auPendingRenderStates[(UInt32)RenderState::kStencilPass];

				m_auCurrentRenderStates[(UInt32)RenderState::kBackFacingStencilFunc] = m_auPendingRenderStates[(UInt32)RenderState::kBackFacingStencilFunc];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilReference] = m_auPendingRenderStates[(UInt32)RenderState::kStencilReference];
				m_auCurrentRenderStates[(UInt32)RenderState::kStencilMask] = m_auPendingRenderStates[(UInt32)RenderState::kStencilMask];
				m_auCurrentRenderStates[(UInt32)RenderState::kBackFacingStencilFail] = m_auPendingRenderStates[(UInt32)RenderState::kBackFacingStencilFail];
				m_auCurrentRenderStates[(UInt32)RenderState::kBackFacingStencilDepthFail] = m_auPendingRenderStates[(UInt32)RenderState::kBackFacingStencilDepthFail];
				m_auCurrentRenderStates[(UInt32)RenderState::kBackFacingStencilPass] = m_auPendingRenderStates[(UInt32)RenderState::kBackFacingStencilPass];

				glStencilFuncSeparate(
					GL_FRONT,
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilFunction],
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilReference],
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilMask]);
				glStencilOpSeparate(
					GL_FRONT,
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilFail],
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilDepthFail],
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilPass]);
				glStencilFuncSeparate(
					GL_BACK,
					m_auCurrentRenderStates[(UInt32)RenderState::kBackFacingStencilFunc],
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilReference],
					m_auCurrentRenderStates[(UInt32)RenderState::kStencilMask]);
				glStencilOpSeparate(
					GL_BACK,
					m_auCurrentRenderStates[(UInt32)RenderState::kBackFacingStencilFail],
					m_auCurrentRenderStates[(UInt32)RenderState::kBackFacingStencilDepthFail],
					m_auCurrentRenderStates[(UInt32)RenderState::kBackFacingStencilPass]);
			}
			break;
		case RenderState::RENDER_STATE_COUNT:
			// Nop
			break;
		};
	}

	m_bRenderStateDirty = false;
}

} // namespace Seoul
