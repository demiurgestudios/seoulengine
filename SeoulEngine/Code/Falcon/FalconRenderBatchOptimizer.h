/**
 * \file FalconRenderBatchOptimizer.h
 * This render optimizer consumes the flattened scene
 * graph (generated by the Poser) and optimizes it by
 * - re-ordering viable draw calls (those that do not
 *   overlap) to maximize batch sizes.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_RENDER_BATCH_OPTIMIZER_H
#define FALCON_RENDER_BATCH_OPTIMIZER_H

#include "FalconRenderFeature.h"
#include "FalconRenderCommand.h"
#include "Prereqs.h"
#include "Vector.h"

namespace Seoul::Falcon
{

namespace Render
{

/**
 * The BatchOptimizer consumes an input render
 * buffer and optimizes it to maximize
 * batch sizes and eliminate draw
 * calls that do not contribute to
 * the final output.
 */
class BatchOptimizer SEOUL_SEALED
{
public:
	BatchOptimizer();
	~BatchOptimizer();

	// Reorder and prune the command stream in rBuffer
	// to maximize batch sizes and reduce draw call
	// overhead.
	void Optimize(Falcon::Render::CommandBuffer& rBuffer);

private:
	void AddPose(
		const CommandBuffer& buffer,
		const Command& cmd);
	void Flush();
	Bool MoveLane(const CommandBuffer& buffer, Int32 iFrom, Int32 iTo);

	CommandBuffer::Buffer m_vBuffer;

	struct Lane SEOUL_SEALED
	{
		Lane()
			: m_pTexture(nullptr)
			, m_eFeature(Feature::kNone)
			, m_Rect(Falcon::Rectangle::InverseMax())
			, m_vBuffer()
		{
		}

		// Scan this lane for an intersection. On intersection,
		// the return value will be >= 0, which is the index
		// of intersection into m_vBuffer.
		Bool Intersects(const CommandBuffer& buffer, const Falcon::Rectangle& rect) const;

		// Scan this lane for an intersection against laneB.
		Bool Intersects(const CommandBuffer& buffer, const Lane& b) const;

		Texture* m_pTexture;
		Render::Feature::Enum m_eFeature;
		Falcon::Rectangle m_Rect;
		CommandBuffer::Buffer m_vBuffer;
	}; // struct Lane
	typedef Vector<Lane*, MemoryBudgets::Falcon> Lanes;
	Lanes m_vLanes;
	Int32 m_iLanes;

	SEOUL_DISABLE_COPY(BatchOptimizer);
}; // class BatchOptimizer

} // namespace Render

} // namespace Seoul::Falcon

#endif // include guard
