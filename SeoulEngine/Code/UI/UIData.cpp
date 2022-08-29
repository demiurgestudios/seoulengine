/**
 * \file UIData.cpp
 * \brief Miscellaneous data types of the UI project.
 *
 * UI::Data contains mostly low-level data abstractions for integration
 * of Falcon into SeoulEngine via the UI project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoaderBase.h"
#include "ContentLoadManager.h"
#include "FalconFCNFile.h"
#include "FalconMovieClipInstance.h"
#include "Path.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "UIContentLoader.h"
#include "UIData.h"
#include "UIFontLoader.h"
#include "UIManager.h"
#include "UIUtil.h"

namespace Seoul
{

SEOUL_SPEC_TEMPLATE_TYPE(Vector<UI::HitPoint, 54>)
SEOUL_BEGIN_ENUM(UI::InputEvent)
	SEOUL_ENUM_N("Action", UI::InputEvent::kAction)
	SEOUL_ENUM_N("BackButton", UI::InputEvent::kBackButton)
	SEOUL_ENUM_N("Done", UI::InputEvent::kDone)
SEOUL_END_ENUM()

SEOUL_BEGIN_TYPE(UI::HitPoint)
	SEOUL_PROPERTY_N("TapPoint", m_vTapPoint)
	SEOUL_PROPERTY_N("CenterPoint", m_vCenterPoint)
	SEOUL_PROPERTY_N("StateMachine", m_StateMachine)
	SEOUL_PROPERTY_N("State", m_State)
	SEOUL_PROPERTY_N("DevOnlyInternalStateId", m_DevOnlyInternalStateId)
	SEOUL_PROPERTY_N("Movie", m_Movie)
	SEOUL_PROPERTY_N("Class", m_Class)
	SEOUL_PROPERTY_N("Id", m_Id)
SEOUL_END_TYPE()

SharedPtr<Falcon::CookedTrueTypeFontData> Content::Traits<Falcon::CookedTrueTypeFontData>::GetPlaceholder(KeyType key)
{
	return SharedPtr<Falcon::CookedTrueTypeFontData>();
}

Bool Content::Traits<Falcon::CookedTrueTypeFontData>::FileChange(KeyType key, const Content::Handle<Falcon::CookedTrueTypeFontData>& pEntry)
{
	// Need to reload fonts in the gui configuration file changes
	if (FileType::kFont == key.GetType())
	{
		// Trigger a reload of the UI system so that the new font data is applied.
#if SEOUL_HOT_LOADING
		UI::Manager::Get()->HotReload();
#endif // /#if SEOUL_HOT_LOADING

		// Reload the font - must destroy the old font first, since the new font takes its place
		// upon construction.
		pEntry.GetContentEntry()->AtomicReplace(SharedPtr<Falcon::CookedTrueTypeFontData>());
		Load(key, pEntry);
		return true;
	}

	return false;
}

void Content::Traits<Falcon::CookedTrueTypeFontData>::Load(KeyType key, const Content::Handle<Falcon::CookedTrueTypeFontData>& pEntry)
{
	Content::LoadManager::Get()->Queue(
		SharedPtr<Content::LoaderBase>(SEOUL_NEW(MemoryBudgets::Content) UI::FontLoader(key, pEntry)));
}

Bool Content::Traits<Falcon::CookedTrueTypeFontData>::PrepareDelete(KeyType key, Content::Entry<Falcon::CookedTrueTypeFontData, KeyType>& entry)
{
	return true;
}

SharedPtr<UI::FCNFileData> Content::Traits<UI::FCNFileData>::GetPlaceholder(FilePath filePath)
{
	return SharedPtr<UI::FCNFileData>();
}

Bool Content::Traits<UI::FCNFileData>::FileChange(FilePath filePath, const Content::Handle<UI::FCNFileData>& pEntry)
{
	if (FileType::kUIMovie == filePath.GetType())
	{
		// Trigger a full clear if we're reloading a library.
		Bool bReloadFCNFiles = false;

#if SEOUL_HOT_LOADING
		bReloadFCNFiles = true;
#endif // /#if SEOUL_HOT_LOADING

		// If we're not already loading all FCN files, trigger
		// a load of the changed FCN file.
		if (!bReloadFCNFiles)
		{
			Load(filePath, pEntry);
		}

#if SEOUL_HOT_LOADING
		UI::Manager::Get()->HotReload();
#endif // /#if SEOUL_HOT_LOADING

		return true;
	}

	return false;
}

void Content::Traits<UI::FCNFileData>::Load(FilePath filePath, const Content::Handle<UI::FCNFileData>& pEntry)
{
	Content::LoadManager::Get()->Queue(
		SharedPtr<Content::LoaderBase>(SEOUL_NEW(MemoryBudgets::Content) UI::ContentLoader(filePath, pEntry)));
}

Bool Content::Traits<UI::FCNFileData>::PrepareDelete(FilePath filePath, Content::Entry<UI::FCNFileData, KeyType>& entry)
{
	return true;
}

namespace UI
{

HitPoint::HitPoint() = default;
UI::HitPoint& UI::HitPoint::operator=(const UI::HitPoint& b) = default;
HitPoint::HitPoint(const HitPoint& b) = default;
HitPoint::~HitPoint() = default;

} // namespace UI

} // namespace Seoul
