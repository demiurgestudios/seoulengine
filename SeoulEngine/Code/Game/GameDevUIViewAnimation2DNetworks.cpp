/**
 * \file GameDevUIViewAnimation2DNetworks.cpp
 * \brief Supports for visualization and debugging of animation
 * networks.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Prereqs.h"

#if (SEOUL_ENABLE_DEV_UI && SEOUL_WITH_ANIMATION_2D && !SEOUL_SHIP)

#include "AnimationBlendDefinition.h"
#include "AnimationBlendInstance.h"
#include "AnimationNetworkDefinition.h"
#include "AnimationPlayClipDefinition.h"
#include "AnimationStateMachineDefinition.h"
#include "AnimationStateMachineInstance.h"
#include "Animation2DPlayClipInstance.h"
#include "Animation2DManager.h"
#include "Animation2DNetworkInstance.h"
#include "DevUIImGui.h"
#include "GameDevUIViewAnimation2DNetworks.h"
#include "GameDevUIViewAnimationNetworksCommon.h"
#include "ReflectionDefine.h"
#include "Renderer.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(Game::DevUIViewAnimation2DNetworks, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(DisplayName, "Animation2D Networks")
	SEOUL_PARENT(DevUI::View)
SEOUL_END_TYPE()

namespace Game
{

DevUIViewAnimation2DNetworks::DevUIViewAnimation2DNetworks()
	: m_sSelected()
	, m_Trigger()
{
}

DevUIViewAnimation2DNetworks::~DevUIViewAnimation2DNetworks()
{
}

static void AnimState2D(Byte const* sName, const SharedPtr<Animation::NodeInstance>& p)
{
	using namespace Animation;
	using namespace Animation2D;
	using namespace ImGui;

	switch (p->GetType())
	{
	case NodeType::kBlend:
		{
			SharedPtr<Animation::BlendInstance> pBlend((Animation::BlendInstance*)p.GetPtr());
			if (TreeNodeEx(sName, ImGuiTreeNodeFlags_DefaultOpen, "%s (Blend)", sName))
			{
				// Properties.
				ImGui::Columns(2);
				BeginValue("Param"); Text("%s", pBlend->GetBlend()->GetMixParameterId().CStr()); EndValue();
				ImGui::Columns();

				// Children.
				AnimState2D("ChildA", pBlend->GetChildA());
				AnimState2D("ChildB", pBlend->GetChildB());

				TreePop();
			}
		}
		break;
	case NodeType::kPlayClip:
		{
			SharedPtr<Animation2D::PlayClipInstance> pPlayClip((Animation2D::PlayClipInstance*)p.GetPtr());
			if (TreeNodeEx(sName, ImGuiTreeNodeFlags_DefaultOpen, "%s (PlayClip)", sName))
			{
				// Properties.
				ImGui::Columns(2);
				BeginValue("Loop"); Text("%s", pPlayClip->GetPlayClip()->GetLoop() ? "true" : "false"); EndValue();
				BeginValue("Name"); Text("%s", pPlayClip->GetPlayClip()->GetName().CStr()); EndValue();
				BeginValue("OnComplete"); Text("%s", pPlayClip->GetPlayClip()->GetOnComplete().CStr()); EndValue();
				BeginValue("Time"); Text("%f", pPlayClip->GetCurrentTime()); EndValue();
				ImGui::Columns();

				TreePop();
			}
		}
		break;
	case NodeType::kStateMachine:
		{
			SharedPtr<Animation::StateMachineInstance> pStateMachine((Animation::StateMachineInstance*)p.GetPtr());
			if (TreeNodeEx(sName, ImGuiTreeNodeFlags_DefaultOpen, "%s (StateMachine)", sName))
			{
				// Properties.
				ImGui::Columns(2);
				if (pStateMachine->InTransition())
				{
					BeginValue("Transition"); Text("%f", pStateMachine->GetTransitionAlpha()); EndValue();
				}
				ImGui::Columns();

				// Children.
				if (!pStateMachine->GetNewId().IsEmpty())
				{
					AnimState2D(pStateMachine->GetNewId().CStr(), pStateMachine->GetNew());
				}
				if (!pStateMachine->GetOldId().IsEmpty())
				{
					ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
					AnimState2D(pStateMachine->GetOldId().CStr(), pStateMachine->GetOld());
					ImGui::PopStyleColor();
				}

				TreePop();
			}
		}
		break;
	default:
		break;
	};
}

static String GetNetworkId(const SharedPtr<Animation2D::NetworkInstance>& p)
{
	String sName(Path::GetFileNameWithoutExtension(p->GetDataHandle().GetKey().GetRelativeFilenameWithoutExtension().ToString()));
	return String::Printf("%s(%p)", sName.CStr(), p.GetPtr());
}

static Int32 GetSelectedIndex(const String& sSelected, const Animation2D::Manager::Instances& vInstances)
{
	for (auto i = vInstances.Begin(); vInstances.End() != i; ++i)
	{
		String const sId(GetNetworkId(*i));
		if (sId == sSelected)
		{
			return (Int32)(i - vInstances.Begin());
		}
	}

	return -1;
}

void DevUIViewAnimation2DNetworks::DoPrePose(DevUI::Controller& rController, RenderPass& rPass)
{
	using namespace Animation2D;
	using namespace ImGui;

	// TODO: Cache.
	Manager::Instances vInstances;
	Manager::Get()->GetActiveNetworkInstances(vInstances);

	AnimationNetworkSorter sorter;
	QuickSort(vInstances.Begin(), vInstances.End(), sorter);

	Int32 iSelected = GetSelectedIndex(m_sSelected, vInstances);
	if (CollapsingHeader("Networks", ImGuiTreeNodeFlags_DefaultOpen))
	{
		Int32 const iCount = (Int32)vInstances.GetSize();
		for (Int32 i = 0; i < iCount; ++i)
		{
			auto p(vInstances[i]);
			String const sId(GetNetworkId(p));
			if (Selectable(sId.CStr(), (sId == m_sSelected)))
			{
				m_sSelected = sId;
				iSelected = i;
			}
		}
	}

	if (iSelected >= 0 && vInstances[iSelected]->IsReady())
	{
		auto p(vInstances[iSelected]);

		// Conditions.
		auto const& tConditions(p->GetConditions());
		if (!tConditions.IsEmpty())
		{
			if (CollapsingHeader("Conditions"))
			{
				Conditions vConditions;
				{
					vConditions.Reserve(tConditions.GetSize());
					auto const iBegin = tConditions.Begin();
					auto const iEnd = tConditions.End();
					for (auto i = iBegin; iEnd != i; ++i)
					{
						vConditions.PushBack(ConditionEntry(i->First, i->Second));
					}
				}

				QuickSort(vConditions.Begin(), vConditions.End());
				for (auto i = vConditions.Begin(); vConditions.End() != i; ++i)
				{
					if (Checkbox(i->m_Name.CStr(), &i->m_bValue))
					{
						p->SetCondition(i->m_Name, i->m_bValue);
					}
				}
			}
		}

		// Parameters.
		auto const& tParameterDefs(p->GetNetwork()->GetParameters());
		auto const& tParameters(p->GetParameters());
		if (!tParameters.IsEmpty())
		{
			if (CollapsingHeader("Parameters"))
			{
				Parameters vParameters;
				{
					vParameters.Reserve(tParameters.GetSize());
					auto const iBegin = tParameters.Begin();
					auto const iEnd = tParameters.End();
					for (auto i = iBegin; iEnd != i; ++i)
					{
						vParameters.PushBack(ParameterEntry(i->First, i->Second));
					}
				}

				QuickSort(vParameters.Begin(), vParameters.End());
				for (auto i = vParameters.Begin(); vParameters.End() != i; ++i)
				{
					Float fMin = 0.0f;
					Float fMax = 1.0f;
					{
						Animation::NetworkDefinitionParameter parameter;
						if (tParameterDefs.GetValue(i->m_Name, parameter))
						{
							fMin = parameter.m_fMin;
							fMax = parameter.m_fMax;
						}
					}

					Float const fIncrement = Abs(fMax - fMin) / 100.0f;
					if (DragFloat(i->m_Name.CStr(), &i->m_fValue, fIncrement, fMin, fMax, "%.2f"))
					{
						p->SetParameter(i->m_Name, i->m_fValue);
					}
				}
			}
		}

		// Triggers.
		{
			ViableTriggers triggers;

			auto p(vInstances[iSelected]);
			GatherTriggers(p->GetRoot(), triggers);

			if (!triggers.IsEmpty())
			{
				Vector<HString, MemoryBudgets::DevUI> vTriggers;
				vTriggers.Reserve(triggers.GetSize());
				{
					auto const iBegin = triggers.Begin();
					auto const iEnd = triggers.End();
					for (auto i = iBegin; iEnd != i; ++i)
					{
						vTriggers.PushBack(*i);
					}
				}

				LexographicalSorter sorter;
				QuickSort(vTriggers.Begin(), vTriggers.End(), sorter);

				Int32 iCurrent = 0;
				{
					auto i = vTriggers.Find(m_Trigger);
					if (vTriggers.End() != i)
					{
						iCurrent = (Int32)(i - vTriggers.Begin());
					}
				}

				if (CollapsingHeader("Triggers"))
				{
					PushItemWidth(100.0f);
					(void)Combo("", &iCurrent, HStringVectorGetter, (void*)&vTriggers, (Int)vTriggers.GetSize(), (Int)vTriggers.GetSize());
					PopItemWidth();
					SameLine();
					m_Trigger = vTriggers[iCurrent];

					if (Button("Trigger"))
					{
						p->TriggerTransition(m_Trigger);
					}
				}
			}
		}

		// State.
		if (CollapsingHeader("State", ImGuiTreeNodeFlags_DefaultOpen))
		{
			AnimState2D("Root", p->GetRoot());
		}
	}
}

UInt32 DevUIViewAnimation2DNetworks::GetFlags() const
{
	return ImGuiWindowFlags_HorizontalScrollbar;
}

} // namespace Game

} // namespace Seoul

#endif // /(SEOUL_ENABLE_DEV_UI && SEOUL_WITH_ANIMATION_2D && !SEOUL_SHIP)
