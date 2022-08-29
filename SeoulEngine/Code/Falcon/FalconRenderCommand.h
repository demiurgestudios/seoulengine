/**
 * \file FalconRenderCommand.h
 * \brief Data structure and utility functions for building the
 * Falcon command buffer.
 *
 * The Poser builds this flat list
 * of draw operations, and the Drawer compiles it and submits
 * it to the graphics hardware.
 *
 * An Optimizer may be inserted between these two to
 * rearrange and optimizer the buffer prior to submission.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_RENDER_COMMAND_H
#define FALCON_RENDER_COMMAND_H

#include "FalconClipper.h"
#include "FalconRenderFeature.h"
#include "FalconTexture.h"
#include "FalconTypes.h"
#include "Prereqs.h"
#include "Vector.h"
namespace Seoul { namespace Falcon { class Renderable; } }

namespace Seoul
{

namespace Falcon
{

namespace Render
{

/** Various operations necessary to fully draw a Falcon scene graph. */
enum class CommandType
{
	/** Placeholder, nop. */
	kUnknown,

	/** Start of the developer-only draw pass used to visualize input regions and rectangles. */
	kBeginInputVisualization, // TODO: Hack - up reference to our UI project.

	/** Start of planar shadow rendering. May require a flush, dependent on the backend. */
	kBeginPlanarShadows,

	/**
	 * Indicates the start of scissor clipping. Unlike default masking or clipping, scissor
	 * clipping uses the GPU scissor rectangle. It must be screen aligned, and it interrupts
	 * batches (a new batch must be started at the start and end of a scissor clip).
	 *
	 * In general, scissor clipping is inferior to default masking or clipping. However,
	 * scissor clipping *must* be used to clip meshes with arbitrary 3D depth. Standard
	 * masking or clipping will not clip shapes that contain per-vertex 3D depth. Scissor
	 * clipping is also faster in some specific uses cases (e.g. to clip an entire movie's
	 * contents for horizontal full movie scrolling).
	 */
	kBeginScissorClip,

	/** Insert a command that will draw an out-of-band operation. */
	kCustomDraw, // TODO: Hack - up reference to our UI project.

	/** Stop rendering the developer-only draw pass used to visualize input. */
	kEndInputVisualization, // TODO: Hack - up reference to our UI project.

	/** End of planar shadow rendering. */
	kEndPlanarShadows,

	/** Complete scissor clipping. */
	kEndScissorClip,

	/** Meat of the Falcon command buffer. Draw of a single renderable instance. */
	kPose,

	/** Meat of the developer-only input visualization mode. Draw a single input instance. */
	kPoseInputVisualization,

	/** Update the viewport, requires a Flush depending on the backend. */
	kViewportChange, // TODO: Hack - up reference to our UI project.

	/** Update the view projection transform, requires a Flush depending on the backend. */
	kViewProjectionChange, // TODO: Hack - up reference to our UI project.

	/** Update the world culling and scaling parameters. */
	kWorldCullChange,
};

/**
 * A single render command, filled into a buffer
 * by a render Poser.
 *
 * IMPORTANT: Commands are treated as memcopyable
 * and zeroable. Take care of the types placed
 * in this struct so that assumption can be
 * maintained (no complex types with complex copy
 * or default constructors). Likewise, all
 * values placed in this struct must have
 * default value of 0.
 */
struct Command SEOUL_SEALED
{
	Command()
		: m_uType((UInt16)CommandType::kUnknown)
		, m_u(0)
	{
	}

	UInt16 m_uType;
	UInt16 m_u;
}; // struct Command

/**
 * Command data for a Begin() command.
 *
 * IMPORTANT: Treated as memcopyable. See Command comment.
 */
struct CommandWorldCull SEOUL_SEALED
{
	CommandWorldCull()
		: m_WorldCullRectangle()
		, m_fWorldWidthToScreenWidth(0.0f)
		, m_fWorldHeightToScreenHeight(0.0f)
	{
	}

	Falcon::Rectangle m_WorldCullRectangle;
	Float m_fWorldWidthToScreenWidth;
	Float m_fWorldHeightToScreenHeight;
}; // struct CommandWorldCull

/**
 * Command data for a Pose() command.
*/
struct CommandPose SEOUL_SEALED
{
	CommandPose()
		: m_pRenderable(nullptr)
		, m_iSubRenderableId(0)
		, m_TextureReference()
		, m_cxWorld()
		, m_mWorld()
		, m_vShadowPlaneWorldPosition()
		, m_WorldRectangle()
		, m_WorldRectanglePreClip()
		, m_WorldOcclusionRectangle()
		, m_iClip(0)
		, m_fDepth3D(0.0f)
		, m_eFeature(Render::Feature::kNone)
	{
	}

	/** Instance that will be invoked if the posed instance is drawn. */
	Renderable* m_pRenderable;

	/** Passed to the renderable's Draw() method to identify sub-draw commands. */
	Int32 m_iSubRenderableId;

	/** Resolved texture reference when this command was posed. */
	TextureReference m_TextureReference;

	/** Full world color transform when this command was posed. */
	ColorTransformWithAlpha m_cxWorld;

	/** Full world spatial transform when this command was posed. */
	Matrix2x3 m_mWorld;

	/** If planar shadows are enabled, the central projection position. */
	Vector2D m_vShadowPlaneWorldPosition;

	/**
	 * The world space rectangle to use for visibility tests. This should
	 * be as tight fitting as is practical. For many renderables, this
	 * will be the world space visible rectangle (the bounds of the original
	 * shape adjusted inwards based on the visible sub-region of
	 * the shape's texture).
	 */
	Falcon::Rectangle m_WorldRectangle;

	/**
	 * The m_WorldRectangle member prior to any clipping or perspective
	 * projection. This is the rectangle that should be passed to the
	 * clipper in the render submission phase to early out from clipping
	 * operations.
	 */
	Falcon::Rectangle m_WorldRectanglePreClip;

	/**
	 * A zero sized rectangle if this posable cannot occlude other posables
	 * Otherwise, a world space occlusion rectangle to use for occlusion culling.
	 */
	Falcon::Rectangle m_WorldOcclusionRectangle;

	/**
	 * -1 if no clipping/masking should be applied when rendering this pose
	 * instance, or an index to the corresponding clip/mask capture
	 * when clipping should be applied.
	 */
	Int32 m_iClip;

	/**
	 * 0.0f for 2D shapes, or a value on (0, 1) for planar projected
	 * 3D shapes.
	 */
	Float32 m_fDepth3D;

	/**
	 * Features required when this command will be drawn.
	 */
	Render::Feature::Enum m_eFeature;
}; // struct CommandPose

/**
 * Command data for a Pose() command during input visualization.
 */
struct CommandPoseInputVisualization SEOUL_SEALED
{
	CommandPoseInputVisualization()
		: m_fDepth3D(0.0f)
		, m_InputBounds()
		, m_TextureReference()
		, m_cxWorld()
		, m_mWorld()
		, m_WorldRectangle()
		, m_WorldRectanglePreClip()
		, m_iClip(0)
	{
	}

	Float m_fDepth3D;
	Falcon::Rectangle m_InputBounds;
	TextureReference m_TextureReference;
	ColorTransformWithAlpha m_cxWorld;
	Matrix2x3 m_mWorld;
	Falcon::Rectangle m_WorldRectangle;
	Falcon::Rectangle m_WorldRectanglePreClip;
	Int32 m_iClip;
}; // struct CommandPoseInputVisualization

} // namespace Render

} // namespace Falcon

template <> struct CanMemCpy<Falcon::Render::Command> { static const Bool Value = true; };
template <> struct CanZeroInit<Falcon::Render::Command> { static const Bool Value = true; };
template <> struct CanMemCpy<Falcon::Render::CommandWorldCull> { static const Bool Value = true; };
template <> struct CanZeroInit<Falcon::Render::CommandWorldCull> { static const Bool Value = true; };

namespace Falcon
{

namespace Render
{

/**
 * A flattened sequence of commands for rendering. Generated
 * by a Poser and processed by a Drawer.
 */
class CommandBuffer SEOUL_SEALED
{
public:
	typedef Vector<Command, MemoryBudgets::Falcon> Buffer;
	typedef Buffer::ConstIterator ConstIterator;
	typedef Buffer::Iterator Iterator;

	CommandBuffer()
		: m_v()
		, m_vDeferred()
		, m_p(&m_v)
		, m_uClips()
		, m_vClips()
		, m_vClipStack()
		, m_vDepthStack()
		, m_vPoses()
		, m_vPoseIVs()
		, m_vRectangles()
		, m_vWorldCulls()
	{
	}

	~CommandBuffer()
	{
		SafeDeleteVector(m_vClips);
	}

	// Iterate access to the command buffer.
	Iterator Begin() { return m_p->Begin(); }
	ConstIterator Begin() const { return m_p->Begin(); }
	Iterator End() { return m_p->End(); }
	ConstIterator End() const { return m_p->End(); }

	/** @return The number of commands in the current command buffer. */
	UInt32 GetBufferSize() const
	{
		return m_p->GetSize();
	}

	/**
	 * Used during buffer generation (not execution).
	 *
	 * Index into the clip list of the current clipping data.
	 */
	Int16 GetClipStackTop() const
	{
		return (m_vClipStack.IsEmpty() ? -1 : m_vClipStack.Back());
	}

	// Access to generate command data. Used during buffer execution.
	ClipCapture const* GetClipCapture(UInt16 u) const { return m_vClips[u]; }
	const CommandPose& GetPose(UInt16 u) const { return m_vPoses[u]; }
	const Rectangle& GetRectangle(UInt16 u) const { return m_vRectangles[u]; }
	UInt32 GetRectangles() const { return m_vRectangles.GetSize(); }
	const CommandPoseInputVisualization& GetPoseIV(UInt16 u) const { return m_vPoseIVs[u]; }
	const CommandWorldCull& GetWorldCull(UInt16 u) const { return m_vWorldCulls[u]; }

	// All functions that begin with Issue() are used during buffer generation.

	void IssueBeginPlanarShadows()
	{
		IssueGeneric(CommandType::kBeginPlanarShadows);
	}

	void IssueEndPlanarShadows()
	{
		IssueGeneric(CommandType::kEndPlanarShadows);
	}

	void BeginDeferDraw()
	{
		m_p = &m_vDeferred;
	}

	void EndDeferDraw()
	{
		m_p = &m_v;
	}

	void IssueBeginScissorClip(const Rectangle& worldRectangle)
	{
		UInt32 const u = m_vRectangles.GetSize();
		m_vRectangles.PushBack(worldRectangle);

		IssueGeneric(CommandType::kBeginScissorClip, u);
	}

	void IssueEndScissorClip(const Rectangle& worldRectangle)
	{
		UInt32 const u = m_vRectangles.GetSize();
		m_vRectangles.PushBack(worldRectangle);

		IssueGeneric(CommandType::kEndScissorClip, u);
	}

	void IssueGeneric(CommandType eType, UInt16 u = 0)
	{
		m_p->Resize(m_p->GetSize() + 1u);
		auto& r = m_p->Back();
		r.m_uType = (UInt16)eType;
		r.m_u = u;
	}

	CommandPose& IssuePose()
	{
		UInt32 const u = m_vPoses.GetSize();
		m_vPoses.Resize(u + 1u);
		auto& r = m_vPoses.Back();

		IssueGeneric(CommandType::kPose, u);

		return r;
	}

	CommandPoseInputVisualization& IssuePoseInputVisualization()
	{
		UInt32 const u = m_vPoseIVs.GetSize();
		m_vPoseIVs.Resize(u + 1u);
		auto& r = m_vPoseIVs.Back();

		IssueGeneric(CommandType::kPoseInputVisualization, u);

		return r;
	}

	void IssuePopClip()
	{
		m_vClipStack.PopBack();
	}

	void IssuePushClip(const ClipStack& clipStack)
	{
		ClipCapture* p = nullptr;
		Int16 const i = InternalAllocateClipCapture(p);
		p->Capture(clipStack);
		m_vClipStack.PushBack(i);
	}

	void IssueWorldCullChange(
		const Falcon::Rectangle& worldCullRectangle,
		Float fWorldWidthToScreenWidth,
		Float fWorldHeightToScreenHeight)
	{
		UInt32 const u = m_vWorldCulls.GetSize();
		m_vWorldCulls.Resize(u + 1u);
		auto& r = m_vWorldCulls.Back();

		r.m_WorldCullRectangle = worldCullRectangle;
		r.m_fWorldWidthToScreenWidth = fWorldWidthToScreenWidth;
		r.m_fWorldHeightToScreenHeight = fWorldHeightToScreenHeight;

		IssueGeneric(CommandType::kWorldCullChange, u);
	}

	void FlushDeferredDraw()
	{
		m_v.Append(m_vDeferred);
		m_vDeferred.Clear();
	}

	/**
	 * Fully reset this buffer into its default state.
	 * Used after buffer execution.
	 */
	void Reset()
	{
		m_vWorldCulls.Clear();
		m_vRectangles.Clear();
		m_vPoseIVs.Clear();
		m_vPoses.Clear();
		m_vDepthStack.Clear();
		m_vClipStack.Clear();
		// NOTE: m_vClips is deliberately not cleared here,
		// since we reuse elements. m_uClips tracks the
		// currently in use elements.
		m_uClips = 0u;
		m_v.Clear();
		m_vDeferred.Clear();
	}

	/**
	 * Internal utility, used by the Optimizer. The passed in buffer
	 * must have valid offsets or Drawer will crash when compiling
	 * this buffer against the other data in the CommandBuffer.
	 */
	 void SwapBuffer(Buffer& rv)
	 {
		 m_p->Swap(rv);
	 }

private:
	Buffer m_v;
	Buffer m_vDeferred;
	CheckedPtr<Buffer> m_p;

	typedef Vector<ClipCapture*, MemoryBudgets::Falcon> Clips;
	typedef Vector<Int16, MemoryBudgets::Falcon> ClipStack;
	typedef Vector<UInt16, MemoryBudgets::Falcon> DepthStack;
	typedef Vector<CommandPose, MemoryBudgets::Falcon> Poses;
	typedef Vector<CommandPoseInputVisualization, MemoryBudgets::Falcon> PoseIVs;
	typedef Vector<Rectangle, MemoryBudgets::Falcon> Rectangles;
	typedef Vector<CommandWorldCull, MemoryBudgets::Falcon> WorldCulls;

	UInt32 m_uClips;
	Clips m_vClips;
	ClipStack m_vClipStack;
	DepthStack m_vDepthStack;
	Poses m_vPoses;
	PoseIVs m_vPoseIVs;
	Rectangles m_vRectangles;
	WorldCulls m_vWorldCulls;

	/** Get a new clip capture for storing clipping data. May heap allocate a new instance. */
	Int16 InternalAllocateClipCapture(ClipCapture*& rp)
	{
		// Out of captures, need to allocate a new one.
		if (m_uClips >= m_vClips.GetSize())
		{
			m_vClips.PushBack(SEOUL_NEW(MemoryBudgets::Falcon) ClipCapture);
		}

		// Reserve the capture at m_uClips and increment.
		Int16 const iReturn = (Int16)m_uClips;
		rp = m_vClips[m_uClips++];
		return iReturn;
	}

	SEOUL_DISABLE_COPY(CommandBuffer);
}; // class CommandBuffer

} // namespace Render

} // namespace Falcon

} // namespace Seoul

#endif // include guard
