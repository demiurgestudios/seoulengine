/**
 * \file UIStack.cpp
 * \brief The overall stack of UI::States that fully define the current
 * state of UI inside UIManager. This class is an internal
 * class to UI::Manager and is not likely useful outside UIManager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "FalconStage3DSettings.h"
#include "FalconTextChunk.h"
#include "EventsManager.h"
#include "GamePaths.h"
#include "UIManager.h"
#include "UIStack.h"
#include "UIUtil.h"
#include "LocManager.h"
#include "Path.h"
#include "ReflectionCoreTemplateTypes.h"
#include "SettingsManager.h"
#include "TextureManager.h"

namespace Seoul::UI
{

static const HString kStage3DSettings("Stage3DSettings");
static const HString kTextEffects("TextEffects");

/**
 * Convenience utility, destroys the machine entries of rv.
 * Swaps first so that any accesses of UI::Stack while
 * destroying do no re-enter.
 */
static inline void DeleteStack(Stack::StackVector& rv)
{
	Stack::StackVector v;
	v.Swap(rv);

	for (Int32 i = (Int32)v.GetSize() - 1; i >= 0; --i)
	{
		SafeDelete(v[i].m_pMachine);
		v[i]  = Stack::StackEntry();
	}
	v.Clear();
}

Stack::Stack(FilePath settingsFilePath, StackFilter eStackFilter)
	: m_FilePath(settingsFilePath)
	, m_pSettings(SettingsManager::Get()->WaitForSettings(settingsFilePath))
	, m_hSettings(SettingsManager::Get()->GetSettings(settingsFilePath))
	, m_hStage3DSettings()
	, m_hTextEffectSettings()
	, m_vStack()
	, m_tStage3DSettings()
	, m_tTextEffectSettings()
	, m_eStackFilter(eStackFilter)
{
	// Must be called from the main thread.
	SEOUL_ASSERT(IsMainThread());

	// Load shadow settings.
	LoadStage3DSettings();

	// Load text effect settings.
	LoadTextEffectSettings();

	// Create the stack.
	CreateStack();

#if SEOUL_HOT_LOADING
	// Register file loaded event callback
	Events::Manager::Get()->RegisterCallback(
		Content::FileLoadCompleteEventId,
		SEOUL_BIND_DELEGATE(&Stack::OnFileLoadComplete, this));
#endif // /#if SEOUL_HOT_LOADING
}

Stack::~Stack()
{
	Destroy();

#if SEOUL_HOT_LOADING
	// Unregister file loaded event callback
	Events::Manager::Get()->UnregisterCallback(
		Content::FileLoadCompleteEventId,
		SEOUL_BIND_DELEGATE(&Stack::OnFileLoadComplete, this));
#endif // /#if SEOUL_HOT_LOADING
}

void Stack::Advance(Float fDeltaTime)
{
	// Settings advance.
	for (auto const& pair : m_tTextEffectSettings)
	{
		auto& settings = *pair.Second;
		
		// Advance the anim offset of the text effect detail, if defined.
		if (!IsZero(settings.m_vDetailSpeed.X))
		{
			settings.m_vDetailAnimOffsetInWorld.X += settings.m_vDetailSpeed.X * fDeltaTime;
		}
		// Otherwise set to 0.
		else
		{
			settings.m_vDetailAnimOffsetInWorld.X = 0.0f;
		}

		// Advance the anim offset of the text effect detail, if defined.
		if (!IsZero(settings.m_vDetailSpeed.Y))
		{
			settings.m_vDetailAnimOffsetInWorld.Y += settings.m_vDetailSpeed.Y * fDeltaTime;
		}
		// Otherwise set to 0.
		else
		{
			settings.m_vDetailAnimOffsetInWorld.Y = 0.0f;
		}
	}

	// Screen Advance
	{
		// Finally, advance all screens - this will inject C++ -> Lua mutations
		// (variable updates and function invoke), and then tick the flash movie via Falcon.
		for (auto const& e : GetStack())
		{
			auto pState = e.m_pMachine->GetActiveState();
			if (pState.IsValid())
			{
				pState->Advance(fDeltaTime);
			}
		}
	}
}

void Stack::Destroy()
{
	// Must be called from the main thread.
	SEOUL_ASSERT(IsMainThread());

	// Clear the stack first and then destroy it, so that any events
	// triggered in destructors don't try to fire on nullptr stacks.
	DeleteStack(m_vStack);

	// Cleanup text effect settings.
	SafeDeleteTable(m_tTextEffectSettings);

	// Cleanup shadow settings.
	SafeDeleteTable(m_tStage3DSettings);
}

/**
 * Used for runtime updating. This is *not* the code path for hot loading -
 * that is handled automatically by OnFileLoadComplete(). Instead,
 * this is used for controlled updating (e.g. by Game::Patcher) of
 * a shipped product.
 */
void Stack::ApplyFileChange(FilePath filePath)
{
	// Must be called from the main thread.
	SEOUL_ASSERT(IsMainThread());

	// Main settings file, stage 3D settings, text effect settings,
	// or a state machine settings file.
	if (filePath == m_FilePath ||
		m_hStage3DSettings.GetKey() == filePath ||
		m_hTextEffectSettings.GetKey() == filePath ||
		(FileType::kJson == filePath.GetType() && m_vStack.Contains(filePath)))
	{
		// If we fail to fully apply the changes immediately, then we
		// need to enqueue remaining changes for application during
		// PrePose().
		if (!ApplyImmediateFileChange(filePath))
		{
			// If we get here, it's a file we're interested in.
			(void)m_PendingChanges.Insert(filePath);
		}
	}
}

/**
 * Called by UI::Manager when at least one state transition is activated
 * Lets Stack update its extended set of content that will be needed
 * by the UI system.
 */
void Stack::OnStateTransitionActivated()
{
	// Must be called from the main thread.
	SEOUL_ASSERT(IsMainThread());

	// Update the cached active state identifiers.
	for (UInt32 i = 0u; i < m_vStack.GetSize(); ++i)
	{
		m_vStack[i].m_ActiveStateId = m_vStack[i].m_pMachine->GetActiveStateIdentifier();
	}
}

/** @return FilePath to shadow settings, or the invalid FilePath if none are configured. */
FilePath Stack::GetStage3DSettingsFilePath() const
{
	// Must be called from the main thread.
	SEOUL_ASSERT(IsMainThread());

	if (!m_pSettings.IsValid())
	{
		return FilePath();
	}

	FilePath filePath;
	DataNode value;
	(void)m_pSettings->GetValueFromTable(m_pSettings->GetRootNode(), kStage3DSettings, value);
	(void)m_pSettings->AsFilePath(value, filePath);
	return filePath;
}

/** @return FilePath to text effect settings, or the invalid FilePath if none are configured. */
FilePath Stack::GetTextEffectSettingsFilePath() const
{
	// Must be called from the main thread.
	SEOUL_ASSERT(IsMainThread());

	if (!m_pSettings.IsValid())
	{
		return FilePath();
	}

	FilePath filePath;
	DataNode value;
	(void)m_pSettings->GetValueFromTable(m_pSettings->GetRootNode(), kTextEffects, value);
	(void)m_pSettings->AsFilePath(value, filePath);
	return filePath;
}

void Stack::ProcessDeferredChanges()
{
	SEOUL_ASSERT(IsMainThread());

	PendingChanges pending;
	pending.Swap(m_PendingChanges);

	for (auto const& e : pending)
	{
		ProcessDeferredFileChange(e);
	}
}

/**
 * Called to initialize the stack fresh, or reinitialize the stack with
 * a new configuration.
 */
void Stack::CreateStack()
{
	// Must be called from the main thread.
	SEOUL_ASSERT(IsMainThread());

	// If we don't have valid settings, nothing more we can do.
	if (!m_pSettings.IsValid())
	{
		return;
	}

	// If we fail to get the root state machines config, return without further processing.
	DataNode stack;
	if (!m_pSettings->GetValueFromTable(m_pSettings->GetRootNode(), FalconConstants::kStateMachines, stack) ||
		!stack.IsArray())
	{
		SEOUL_WARN("Failed loading UI config - %s entry in %s is missing or invalid.\n",
			FalconConstants::kStateMachines.CStr(),
			m_FilePath.CStr());

		return;
	}

	StackVector vStack;

	// Populate the configuration.
	{
		// Get the length of the array, then resize and reload each config.
		UInt32 uArrayCount = 0u;
		m_pSettings->GetArrayCount(stack, uArrayCount);

		vStack.Reserve(uArrayCount);
		for (UInt32 i = 0u; i < uArrayCount; ++i)
		{
			// If we fail getting an entry, warn and return.
			DataNode stateMachine;
			Bool bSuccess = m_pSettings->GetValueFromArray(stack, i, stateMachine);

			// (FilePath, Bool) entry. When defined, the second value indicates that
			// a state machine is a developer-only state machine.
			StackEntry entry;
			if (stateMachine.IsArray())
			{
				DataNode value;

				// FilePath.
				bSuccess = bSuccess && m_pSettings->GetValueFromArray(stateMachine, 0u, value);
				bSuccess = bSuccess && m_pSettings->AsFilePath(value, entry.m_FilePath);

				// Enum stackFilter argument.
				bSuccess = bSuccess && m_pSettings->GetValueFromArray(stateMachine, 1u, value);
				HString stackFilter;
				bSuccess = bSuccess && m_pSettings->AsString(value, stackFilter);
				if (bSuccess)
				{
					bSuccess = bSuccess && EnumOf<StackFilter>().TryGetValue(stackFilter, entry.m_eFilter);
				}
			}
			else if (stateMachine.IsFilePath())
			{
				bSuccess = m_pSettings->AsFilePath(stateMachine, entry.m_FilePath);
				entry.m_eFilter = StackFilter::kAlways;
			}

			// On successful read, configure the entry.
			if (bSuccess)
			{
				// Wait first so we sync load to reduce thread contention. Calling GetSettings()
				// first would instead start the load via the normal async path.
				entry.m_pSettings = SettingsManager::Get()->WaitForSettings(entry.m_FilePath);
				entry.m_hSettings = SettingsManager::Get()->GetSettings(entry.m_FilePath);

				// Success if m_pSettings is not null.
				bSuccess = bSuccess && entry.m_pSettings.IsValid();
			}

			if (bSuccess)
			{
				// Check filtering - only add stacks that pass the filter.
				if ((Int32)entry.m_eFilter <= (Int32)m_eStackFilter)
				{
					vStack.PushBack(entry);
				}
			}
			else
			{
				SEOUL_WARN("Failed loading UI config - the array of state machines %s has invalid "
					"entry %u, it must be a valid FilePath or an array with a FilePath and a boolean.\n",
					FalconConstants::kStateMachines.CStr(),
					i);

				return;
			}
		}
	}

	// Now actually create the machines. Swap out existing now,
	// so that any re-entrance due to a state machine construction
	// does not see a partially constructed stack.
	StackVector vExisting;
	vExisting.Swap(m_vStack);
	auto const uStack = vStack.GetSize();
	for (UInt32 i = 0u; i < uStack; ++i)
	{
		// Cache values.
		auto const filePath(vStack[i].m_FilePath);
		auto const p(vStack[i].m_pSettings);

		// Try to carry the machine through on a recreate.
		auto iExisting = vExisting.Find(filePath);
		if (iExisting != vExisting.End())
		{
			// Reuse existing machine. Leave all
			// other settings in the output stack
			// as-is.
			vStack[i].m_pMachine = iExisting->m_pMachine;

			// Need to zero-out the entry to avoid
			// a double free.
			*iExisting = StackEntry();
		}
		else
		{
			// Create a new machine.
			// Use the root of the filename as the state machine name.
			HString const stateMachineName(Path::GetFileName(
				filePath.GetRelativeFilenameWithoutExtension().ToString()));

			vStack[i].m_pMachine.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) StateMachine(stateMachineName));
		}

		// Merge the configuration into the new or existing machine.
		vStack[i].m_pMachine->GetStateMachineConfiguration().CopyFrom(*p);
	}

	// Destroy the existing stack first, if it exists.
	DeleteStack(vExisting);

	// Done, populate.
	m_vStack.Swap(vStack);
}

/**
 * Call to apply the current state (on disk) of shadow
 * settings to the global table of shadow settings.
 */
void Stack::LoadStage3DSettings()
{
	// Must be called from the main thread.
	SEOUL_ASSERT(IsMainThread());

	// Get and check the FilePath to deserialize from.
	FilePath const filePath(GetStage3DSettingsFilePath());
	if (!filePath.IsValid())
	{
		return;
	}

	// Deserialize into a local table.
	Stage3DSettingsTable t;
	if (!SettingsManager::Get()->DeserializeObject(
		filePath,
		&t))
	{
		SafeDeleteTable(t);
		SEOUL_WARN("Failed loading text effect settings: \"%s\".", filePath.CStr());
		return;
	}

	// Replace and cleanup the old table.
	m_tStage3DSettings.Swap(t);
	SafeDeleteTable(t);

	// Cache the handle.
	m_hStage3DSettings = SettingsManager::Get()->GetSettings(filePath);
}

/**
 * Call to apply the current state (on disk) of text effect
 * settings to the global table of text effect settings.
 */
void Stack::LoadTextEffectSettings()
{
	// Must be called from the main thread.
	SEOUL_ASSERT(IsMainThread());

	// Get and check the FilePath to deserialize from.
	FilePath const filePath(GetTextEffectSettingsFilePath());
	if (!filePath.IsValid())
	{
		return;
	}

	// Deserialize into a local table.
	TextEffectSettingsTable t;
	if (!SettingsManager::Get()->DeserializeObject(
		filePath,
		&t))
	{
		SafeDeleteTable(t);
		SEOUL_WARN("Failed loading text effect settings: \"%s\".", filePath.CStr());
		return;
	}

	// TODO: Due to wrap mode inconsistency in DX style API vs. OpenGL vs. low-level
	// (e.g. Metal). This is an ugly workaround for OpenGL style API that requires the wrap
	// mode to be specified on the texture object (vs. on the sampler and determined by
	// the sampler configuration).
	for (auto const& pair : t)
	{
		if (pair.Second->m_bDetail && pair.Second->m_DetailFilePath.IsValid())
		{
			auto filePath = pair.Second->m_DetailFilePath;
			
			// For all variations of this file path (all texture types).
			for (Int i = (Int)FileType::FIRST_TEXTURE_TYPE; i <= (Int)FileType::LAST_TEXTURE_TYPE; ++i)
			{
				filePath.SetType((FileType)i);
				
				// Set this texture to be wrap sampled instead of the default, which is clamped.
				auto config = TextureManager::Get()->GetTextureConfig(filePath);
				config.m_bWrapAddressU = true;
				config.m_bWrapAddressV = true;
				TextureManager::Get()->UpdateTextureConfig(filePath, config);
			}
		}
	}

	// Replace and cleanup the old table.
	m_tTextEffectSettings.Swap(t);
	SafeDeleteTable(t);

	// Cache the handle.
	m_hTextEffectSettings = SettingsManager::Get()->GetSettings(filePath);
}

/**
 * Apply effects of a file change that can be done immediately.
 * If this method returns true, then all effects of the file
 * change have been applied. Otherwise, the file must be enqueued
 * for deferred processing.
 */
Bool Stack::ApplyImmediateFileChange(FilePath filePath)
{
	// Must be called from the main thread.
	SEOUL_ASSERT(IsMainThread());

	// If the main settings file changed, merge
	// those settings and recreate the stack.
	if (filePath == m_FilePath)
	{
		// Get updated settings.
		auto h(m_hSettings);
		Content::LoadManager::Get()->WaitUntilLoadIsFinished(h);
		auto p(h.GetPtr());
		if (!p.IsValid())
		{
			// Failure to load means we're done processing, but also warning about this.
			SEOUL_WARN("UIStack: failed loading file \"%s\" after file change.",
				m_FilePath.CStr());
			return true;
		}

		// Merge settings.
		m_pSettings->CopyFrom(*p);

		// Leave remaining processing for deferred step.
		return false;
	}
	// If the shadow settings file changed, reload shadow settings.
	else if (m_hStage3DSettings.GetKey() == filePath)
	{
		LoadStage3DSettings();

		// Fall-through, fully processed.
	}
	// If the text effect settings file changed, reload text effect settings.
	else if (m_hTextEffectSettings.GetKey() == filePath)
	{
		LoadTextEffectSettings();

		// Fall-through, fully processed.
	}
	else if (FileType::kJson == filePath.GetType())
	{
		// Otherwise, if any of the state machine config files have changed,
		// just apply the changes.
		auto i = m_vStack.Find(filePath);
		if (m_vStack.End() != i)
		{
			i->m_pSettings = SettingsManager::Get()->WaitForSettings(filePath);
			i->m_hSettings = SettingsManager::Get()->GetSettings(filePath);
			if (i->m_pSettings.IsValid())
			{
				i->m_pMachine->GetStateMachineConfiguration().CopyFrom(*i->m_pSettings);
			}
			else
			{
				// Failure to load means we're done processing, but also warning about this.
				SEOUL_WARN("UIStack: failed loading file \"%s\" after file change.",
					filePath.CStr());
				return true;
			}
		}

		// Fall-through, fully processed.
	}

	// Done, fully processed.
	return true;
}

/** Apply effects of a file change that must wait until pre-pose to be applied. */
void Stack::ProcessDeferredFileChange(FilePath filePath)
{
	// Must be called from the main thread.
	SEOUL_ASSERT(IsMainThread());

	// If the main settings file changed, merge
	// those settings and recreate the stack.
	if (filePath == m_FilePath)
	{
		// Recreate stack - CreateStack() will handle the details of
		// merging any existing machines that remain the same.
		CreateStack();
	}
}

#if SEOUL_HOT_LOADING
/**
 * Called by Events::Manager when a file load has finished. Used by
 * Stack to track changes to settings files.
 */
void Stack::OnFileLoadComplete(FilePath filePath)
{
	// Must be called from the main thread.
	SEOUL_ASSERT(IsMainThread());

	// Don't react to the file change if hot loading is suppressed.
	if (Content::LoadManager::Get()->IsHotLoadingSuppressed())
	{
		return;
	}

	ApplyFileChange(filePath);
}
#endif // /#if SEOUL_HOT_LOADING

} // namespace Seoul::UI
