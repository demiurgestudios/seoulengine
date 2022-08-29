/**
 * \file PostProcess.h
 * \brief A poseable which represents a post-process.
 *
 * Post-processing is typically applied to the final rendered image, to
 * apply screen space effects such as coloration, DOF, motion blur, etc.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef POST_PROCESS_H
#define POST_PROCESS_H

#include "Effect.h"
#include "IPoseable.h"
#include "Prereqs.h"
namespace Seoul { class DataStoreTableUtil; }
namespace Seoul { class IndexBuffer; }
namespace Seoul { class RenderTarget; }
namespace Seoul { class VertexBuffer; }
namespace Seoul { class VertexFormat; }

namespace Seoul
{

/**
 * A poseable that represents a post process.
 * 
 * PostProcess just sets itself up for render, it does not query
 * the scene for any other poseables.
 */
class PostProcess SEOUL_ABSTRACT : public IPoseable
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(PostProcess);

	PostProcess(const DataStoreTableUtil& configSettings);
	virtual ~PostProcess();

	// IPoseable overrides
	virtual void Pose(
		Float fDeltaTime,
		RenderPass& rPass,
		IPoseable* pParent = nullptr) SEOUL_OVERRIDE;
	// /IPoseable overrides

protected:
	virtual HString GetEffectTechnique() const = 0;

private:
	SharedPtr<IndexBuffer> m_pIndexBuffer;
	SharedPtr<RenderTarget> m_pSourceTarget;
	SharedPtr<VertexBuffer> m_pVertexBuffer;
	SharedPtr<VertexFormat> m_pVertexFormat;

	SEOUL_DISABLE_COPY(PostProcess);
}; // class PostProcess

} // namespace Seoul

#endif // include guard
