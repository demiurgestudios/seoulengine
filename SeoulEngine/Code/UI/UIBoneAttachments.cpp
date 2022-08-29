/**
 * \file UIBoneAttachments.cpp
 * \brief Utility instance that handles attaching UI element (Falcon::Instance subclasses)
 * to bones as defined by a UIAnimation2DNetworkInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "UIBoneAttachments.h"
#include "UIAnimation2DNetworkInstance.h"

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul::UI
{

BoneAttachments::BoneAttachments(Animation2DNetworkInstance& rAnimation2DNetwork)
	: m_Attachments()
	, m_rAnimation2DNetwork(rAnimation2DNetwork)
{

}

BoneAttachments::~BoneAttachments()
{
}


void BoneAttachments::Update()
{
	auto iter = m_Attachments.Begin();
	while(iter != m_Attachments.End())
	{
		if (iter->Second->GetParent() != nullptr)
		{
			auto worldTransform = m_rAnimation2DNetwork.GetWorldSpaceBoneTransform(iter->First);
			iter->Second->SetWorldTransform(worldTransform);
			++iter;
		}
		else
		{
			// we are cleaning up our list ourselves if there is no parent
			// if this proves problematic then maybe there can be a flag to trigger deletion manually
			iter = m_Attachments.Erase(iter);
		}
	}
}

void BoneAttachments::AddAttachment(Int16 iBoneIndex, SharedPtr<Falcon::Instance> pAttachment)
{
	BoneAttachment attachment;
	attachment.First = iBoneIndex;
	attachment.Second = pAttachment;
	m_Attachments.PushBack(attachment);
}

} // namespace Seoul::UI

#endif // /#if SEOUL_WITH_ANIMATION_2D
