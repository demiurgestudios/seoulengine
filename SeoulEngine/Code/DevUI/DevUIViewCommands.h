/**
 * \file DevUIViewCommands.h
 * \brief In-game cheat/developer command support. Used
 * to run cheats or otherwise interact with the running simulation.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DEV_UI_VIEW_COMMANDS_H
#define DEV_UI_VIEW_COMMANDS_H

#include "DevUIView.h"
#include "List.h"
#include "ReflectionAny.h"
#include "ReflectionAttribute.h"
#include "ReflectionWeakAny.h"
namespace Seoul { namespace Reflection { class Method; } }
namespace Seoul { namespace Reflection { namespace Attributes { class Category; } } }
namespace Seoul { namespace Reflection { namespace Attributes { class Description; } } }
extern "C" { typedef struct ImGuiTextEditCallbackData ImGuiTextEditCallbackData; }

#if (SEOUL_ENABLE_DEV_UI && SEOUL_ENABLE_CHEATS)

namespace Seoul::DevUI
{

class ViewCommands SEOUL_SEALED : public View
{
public:
	ViewCommands();
	~ViewCommands();

	static HString StaticGetId()
	{
		static const HString kId("Commands");
		return kId;
	}

	// DevUI::View overrides
	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticGetId();
	}

	virtual Bool OnKeyPressed(InputButton eButton, UInt32 uModifiers) SEOUL_OVERRIDE;

private:
	virtual void DoPrePose(Controller& rController, RenderPass& rPass) SEOUL_OVERRIDE;
	virtual void DoTick(Controller& rController, Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;
	virtual UInt32 GetFlags() const SEOUL_OVERRIDE;
	virtual Vector2D GetInitialSize() const SEOUL_OVERRIDE
	{
		return Vector2D(400, 600);
	}
	// /DevUIView overrides

	struct CommandEntry SEOUL_SEALED
	{
		CommandEntry()
			: m_sCategory()
			, m_sName()
			, m_aArguments()
			, m_Instance()
			, m_pDescription(nullptr)
			, m_pMethod(nullptr)
			, m_pType(nullptr)
			, m_IsDisabledFunc(nullptr)
		{
		}

		String m_sCategory;
		String m_sName;
		Reflection::MethodArguments m_aArguments;
		Reflection::WeakAny m_Instance;
		Reflection::Attributes::Description const* m_pDescription;
		Reflection::Method const* m_pMethod;
		Reflection::Type const* m_pType;
		typedef HString(*IsDisabledFunc)();
		IsDisabledFunc m_IsDisabledFunc;

		Bool operator<(const CommandEntry& b) const
		{
			if (m_sCategory == b.m_sCategory)
			{
				return (m_sName < b.m_sName);
			}
			else
			{
				return (m_sCategory < b.m_sCategory);
			}
		}
	}; // struct CommandEntry
	typedef Vector<CommandEntry, MemoryBudgets::DevUI> Commands;
	Commands m_vCommands;
	typedef Vector<Reflection::WeakAny, MemoryBudgets::DevUI> Instances;
	Instances m_vInstances;
	HString m_PendingBinding;

	static void LogCommand(const CommandEntry& entry);

	void DestroyCommands();
	void GatherCommands();
	void PoseHotkeyDialogue();

	SEOUL_DISABLE_COPY(ViewCommands);
}; // class DevUI::ViewCommands

} // namespace Seoul::DevUI

#endif // /(SEOUL_ENABLE_DEV_UI && SEOUL_ENABLE_CHEATS)

#endif // include guard
