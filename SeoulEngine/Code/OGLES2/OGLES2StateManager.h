/**
 * \file OGLES2StateManager.h
 * \brief OGLES2StateManager manages render states and sampler states.
 * It eliminates redundant state setting and can be used to
 * restore the render device to its default state. OGLES2StateManager
 * is roughly equivalent to ID3DXEffectStateManager, except more
 * low-level and absolutely necessary on OGLES2 (since there is no equivalent
 * functionality to map enum render state codes with values to actual
 * render state changes).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef OGLES2_STATE_MANAGER_H
#define OGLES2_STATE_MANAGER_H

#include "FixedArray.h"
#include "OGLES2Util.h"
#include "Prereqs.h"

namespace Seoul
{

/**
 * Valid values for the CullMode render state.
 */
enum class CullMode : UInt32
{
	kNone = 1,
	kClockwise,
	kCounterClockwise
};

/**
 * Keys of various RenderStates supported by SeoulEngine
 * on OGLES2 - map to a variety of gl* function calls which
 * modify render state.
 */
enum class RenderState : UInt32
{
	kAlphaBlendEnable,
	kAlphaFunction,
	kAlphaReference,
	kAlphaTestEnable,

	kBackFacingStencilDepthFail,
	kBackFacingStencilFail,
	kBackFacingStencilFunc,
	kBackFacingStencilPass,

	kBlendColor,
	kBlendOp,
	kBlendOpAlpha,

	kColorWriteEnable,
	kColorWriteEnable1,
	kColorWriteEnable2,
	kColorWriteEnable3,

	kCull,

	kDepthBias,
	kDepthEnable,
	kDepthFunction,
	kDepthWriteEnable,

	kDestinationBlend,
	kDestinationBlendAlpha,
	kFillMode,

	kScissor,
	kSeparateAlphaBlendEnable,
	kShadeMode,
	kSlopeScaleDepthBias,

	kSourceBlend,
	kSourceBlendAlpha,

	kSRGBWriteEnable,

	kStencilDepthFail,
	kStencilEnable,
	kStencilFail,
	kStencilFunction,
	kStencilMask,
	kStencilPass,
	kStencilReference,
	kStencilWriteMask,
	kTwoSidedStencilMode,

	RENDER_STATE_COUNT
};

/**
 * Individual values are packaged into a single UInt32.
 * These are the offsets of components that occupy 8 bits in the
 * UInt32.
 */
enum class Components8Bit : Int32
{
	// Grouped together as red, green, blue, alpha mask booleans
	kColorMaskR = 0,
	kColorMaskG = 1,
	kColorMaskB = 2,
	kColorMaskA = 3
};

namespace RenderStateUtil
{

// Conversion functions for packing and unpacking 8-bit and 16-bit
// values to and from UInt32 sampler state values.

template <Components8Bit I>
inline UInt8 GetComponent8(UInt32 uValue);

template <> inline UInt8 GetComponent8<Components8Bit::kColorMaskR>(UInt32 uValue)
{
	return (UInt8)(uValue & 0x000000FF);
}

template <> inline UInt8 GetComponent8<Components8Bit::kColorMaskG>(UInt32 uValue)
{
	return (UInt8)((uValue >> 8u) & 0x000000FF);
}

template <> inline UInt8 GetComponent8<Components8Bit::kColorMaskB>(UInt32 uValue)
{
	return (UInt8)((uValue >> 16u) & 0x000000FF);
}

template <> inline UInt8 GetComponent8<Components8Bit::kColorMaskA>(UInt32 uValue)
{
	return (UInt8)((uValue >> 24u) & 0x000000FF);
}

template <Int I>
inline UInt16 GetComponent16(UInt32 uValue);

template <>
inline UInt16 GetComponent16<0>(UInt32 uValue)
{
	return (UInt16)(uValue & 0x0000FFFF);
}

template <>
inline UInt16 GetComponent16<1>(UInt32 uValue)
{
	return (UInt16)((uValue >> 16u) & 0x0000FFFF);
}

template <Components8Bit I>
inline void SetComponent8(UInt8 uComponentValue, UInt32& ruValue);

template <> inline void SetComponent8<Components8Bit::kColorMaskR>(UInt8 uComponentValue, UInt32& ruValue)
{
	ruValue &= (0xFFFFFF00);
	ruValue |= (0x000000FF & ((UInt32)uComponentValue));
}

template <> inline void SetComponent8<Components8Bit::kColorMaskG>(UInt8 uComponentValue, UInt32& ruValue)
{
	ruValue &= (0xFFFF00FF);
	ruValue |= (0x0000FF00 & (((UInt32)uComponentValue) << 8u));
}

template <> inline void SetComponent8<Components8Bit::kColorMaskB>(UInt8 uComponentValue, UInt32& ruValue)
{
	ruValue &= (0xFF00FFFF);
	ruValue |= (0x00FF0000 & (((UInt32)uComponentValue) << 16u));
}

template <> inline void SetComponent8<Components8Bit::kColorMaskA>(UInt8 uComponentValue, UInt32& ruValue)
{
	ruValue &= (0x00FFFFFF);
	ruValue |= (0xFF000000 & (((UInt32)uComponentValue) << 24u));
}

template <Int I>
inline void SetComponent16(UInt16 uComponentValue, UInt32& ruValue);

template <>
inline void SetComponent16<0>(UInt16 uComponentValue, UInt32& ruValue)
{
	ruValue &= (0xFFFF0000);
	ruValue |= (0x0000FFFF & ((UInt32)uComponentValue));
}

template <>
inline void SetComponent16<1>(UInt16 uComponentValue, UInt32& ruValue)
{
	ruValue &= (0x0000FFFF);
	ruValue |= (0xFFFF0000 & (((UInt32)uComponentValue) << 16u));
}

} // namespace RenderStateUtil

/**
 * OGLES2StateManager is the rough equivalent to ID3DXEffectStateManager
 * for the OGLES2 platform, except it is more low-level and a required component
 * (since it does not just filter redundant state sets but also provides
 * a mechanism to set sampler and render states using key-value pairs).
 */
class OGLES2StateManager SEOUL_SEALED
{
public:
	OGLES2StateManager();
	~OGLES2StateManager();

	// When called, commits changes to set render states and samplers
	// to their default state.
	void ApplyDefaultRenderStates();

	// Apply any state changes since the last call to CommitPendingState
	// to the render device.
	void CommitPendingStates()
	{
		InternalCommitPendingStates();
	}

	// Return the current value (desired/pending) of eState.
	UInt32 GetRenderState(RenderState eState) const
	{
		return m_auPendingRenderStates[(UInt32)eState];
	}

	// Set render state eState to value uValue.
	void SetRenderState(RenderState eState, UInt32 uValue);

	// Bind and activate a texture.
	void SetActiveTexture(
		GLenum eTextureType,
		UInt32 uSamplerIndex,
		GLuint uTextureId);

	// Call after an external glBindTexture call
	// (to update texture data, for example) to restore bound texture settings,
	// if the expected bound texture settings is not the invalid texture (0).
	void RestoreActiveTextureIfSet(GLenum eTextureType);

	// Mark the scissor rectangle as dirty - forces a commit the next
	// time a commit happens.
	void MarkScissorRectangleDirty()
	{
		m_CurrentScissorState = ViewportOrScissorState();
	}

	// Mark the viewport rectangle as dirty - forces a commit the next
	// time a commit happens.
	void MarkViewportRectangleDirty()
	{
		m_CurrentViewportState = ViewportOrScissorState();
	}

	// Make a change to the scissor rectangle.
	void SetScissor(GLint iX, GLint iY, GLsizei iWidth, GLsizei iHeight);

	// Make a change to the viewport rectangle.
	void SetViewport(GLint iX, GLint iY, GLsizei iWidth, GLsizei iHeight);

private:
	/**
	 * Utility structure for tracking scissor or viewport rectangle state.
	 * Default value is "invalid".
	 */
	struct ViewportOrScissorState SEOUL_SEALED
	{
		ViewportOrScissorState()
			: m_iX(-1)
			, m_iY(-1)
			, m_iWidth(-1)
			, m_iHeight(-1)
		{
		}

		GLint m_iX;
		GLint m_iY;
		GLsizei m_iWidth;
		GLsizei m_iHeight;

		/** @return True if a scissor that can be applied, false if a placeholder invalid value. */
		Bool IsValid() const
		{
			return (
				m_iX >= 0 &&
				m_iY >= 0 &&
				m_iWidth >= 0 &&
				m_iHeight >= 0);
		}

		Bool operator==(const ViewportOrScissorState& b) const
		{
			return (
				m_iX == b.m_iX &&
				m_iY == b.m_iY &&
				m_iWidth == b.m_iWidth &&
				m_iHeight == b.m_iHeight);
		}

		Bool operator!=(const ViewportOrScissorState& b) const
		{
			return !(*this == b);
		}
	}; // struct ViewportOrScissorState
	ViewportOrScissorState m_CurrentScissorState;
	ViewportOrScissorState m_PendingScissorState;
	ViewportOrScissorState m_CurrentViewportState;
	ViewportOrScissorState m_PendingViewportState;

	FixedArray<UInt32, (UInt32)RenderState::RENDER_STATE_COUNT> m_auCurrentRenderStates;
	FixedArray<UInt32, (UInt32)RenderState::RENDER_STATE_COUNT> m_auPendingRenderStates;
	FixedArray<UInt32, (UInt32)RenderState::RENDER_STATE_COUNT> m_auDefaultRenderStates;
	FixedArray<GLuint, (GL_TEXTURE31 - GL_TEXTURE0) + 1> m_auActiveTexture2d;
	UInt32 m_uActiveTexture2d;
	bool m_bRenderStateDirty;

	void InternalCommitPendingStates();

	friend class OGLES2RenderDevice;
	friend class OGLES2RenderCommandStreamBuilder;

	SEOUL_DISABLE_COPY(OGLES2StateManager);
}; // class OGLES2StateManager

} // namespace Seoul

#endif // include guard
