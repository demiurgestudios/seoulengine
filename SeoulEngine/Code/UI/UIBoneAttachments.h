/**
 * \file UIBoneAttachments.h
 * \brief Utility instance that handles attaching UI element (Falcon::Instance subclasses)
 * to bones as defined by a UIAnimation2DNetworkInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_BONE_ATTACHMENTS_H
#define UI_BONE_ATTACHMENTS_H

#include "FalconInstance.h"
namespace Seoul { namespace UI { class Animation2DNetworkInstance; } }

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul::UI
{

class BoneAttachments SEOUL_SEALED
{
public:
	typedef Pair<Int16, SharedPtr<Falcon::Instance>> BoneAttachment;
	typedef Vector<BoneAttachment, MemoryBudgets::Falcon> BoneAttachmentsVector;

	BoneAttachments(Animation2DNetworkInstance& rAnimation2DNetwork);
	~BoneAttachments();

	const BoneAttachmentsVector& GetAttachments() const { return m_Attachments; }

	void Update();

	void AddAttachment(Int16 iBoneIndex, SharedPtr<Falcon::Instance> pAttachment);

private:
	BoneAttachmentsVector m_Attachments;
	Animation2DNetworkInstance& m_rAnimation2DNetwork;

	SEOUL_DISABLE_COPY(BoneAttachments);
}; // class UI::BoneAttachments

} // namespace Seoul::UI

#endif // /#if SEOUL_WITH_ANIMATION_2D

#endif // include guard
