/**
 * \file UIManager.cpp
 * \brief Global singleton, manages game UI.
 *
 * UI::Manager owns the stack of UI::StateMachines that fully defined
 * the data driven layers of UI state and behavior. UI::Manager also
 * performs input management for the UI system.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ApplicationJson.h"
#include "ContentLoadManager.h"
#include "CookManager.h"
#include "DataStoreParser.h"
#include "Directory.h"
#include "Engine.h"
#include "FalconBitmapDefinition.h"
#include "FalconFlaChecker.h"
#include "FalconEditTextInstance.h"
#include "FalconEditTextLink.h"
#include "FalconFCNFile.h"
#include "FalconMovieClipDefinition.h"
#include "FalconMovieClipInstance.h"
#include "FalconShapeDefinition.h"
#include "FileManager.h"
#include "EventsManager.h"
#include "JobsFunction.h"
#include "LocManager.h"
#include "Logger.h"
#include "Path.h"
#include "PlatformData.h"
#include "ReflectionRegistry.h"
#include "RenderCommandStreamBuilder.h"
#include "RenderDevice.h"
#include "Renderer.h"
#include "RenderPass.h"
#include "ScopedAction.h"
#include "SeoulProfiler.h"
#include "SeoulWildcard.h"
#include "SettingsManager.h"
#include "ToString.h"
#include "UIAdvanceInterfaceDeferredDispatch.h"
#include "UIContext.h"
#include "UILetterboxPass.h"
#include "UIManager.h"
#include "UIMovie.h"
#include "UIRenderer.h"
#include "UIUtil.h"
namespace Seoul
{

#if SEOUL_ENABLE_CHEATS
SEOUL_LINK_ME_NS(class, UI, Commands)
#endif // /#if SEOUL_ENABLE_CHEATS

namespace UI
{

#if SEOUL_LOGGING_ENABLED
static const UInt32 kuTriggerHistorySize = 32u;
#endif // /#if SEOUL_LOGGING_ENABLED

/** Applies any aspect ratio min/max settings from application.json. */
static Byte const* const ksFixedAspectRatio("FixedAspectRatioMode%s");
static Byte const* const ksMinAspectRatio("MinAspectRatio%s");
static Byte const* const ksMinAspectRatioBoxed("MinAspectRatioBoxed%s");

#if SEOUL_PLATFORM_ANDROID
static const UInt32 kuDeadZonePixelsFromTopOnDrag = 32u;
static const UInt32 kuDeadZonePixelsFromBottomOnDrag = 16u;
#endif

static HString const kGameLoaded("GameLoaded");
static const HString kOnViewportChanged("HANDLER_OnViewportChanged");

/**
 * True if the current environment supports a visible mouse cursor.
 * Affects forms of query and reporting (e.g. mouse over and out
 * events are reported only if a cursor is present).
 */
static inline Bool HasMouseCursor()
{
	// TODO: Query?
#if SEOUL_PLATFORM_WINDOWS
	return true;
#else
	return false;
#endif
}

/** True if the current environment supports a mouse wheel. */
static inline Bool HasMouseWheel()
{
	// TODO: Query?
#if SEOUL_PLATFORM_WINDOWS
	return true;
#else
	return false;
#endif
}

/**
 * @return A root poseable that can be used to pose and render UI screens - in this
 * case, this always returns the global UI::Manager singleton.
 */
static IPoseable* PoseableSpawnHook(
	const DataStoreTableUtil& configSettings,
	Bool& rbRenderPassOwnsPoseableObject)
{
	return g_UIContext.SpawnUIManager(configSettings, rbRenderPassOwnsPoseableObject);
}

/** HString constant used to uniquely identify the UI and letterbox root poseables. */
static const HString kLetterboxSpawnType("LetterboxPass");
static const HString kUIPoseableSpawnType("UI");

static SharedPtr<Falcon::BitmapDefinition const> FindBitmap(Falcon::MovieClipDefinition const* pMovieClip)
{
	auto const& v = pMovieClip->GetDisplayListTags();
	for (auto pTag : v)
	{
		if (pTag->GetType() == Falcon::DisplayListTagType::kAddObject)
		{
			auto pAdd = (Falcon::AddObject const*)pTag;
			auto p(pAdd->GetDefinition());
			if (p.IsValid())
			{
				if (p->GetType() == Falcon::DefinitionType::kBitmap)
				{
					return SharedPtr<Falcon::BitmapDefinition const>((Falcon::BitmapDefinition const*)p.GetPtr());
				}
				else if (p->GetType() == Falcon::DefinitionType::kShape)
				{
					SharedPtr<Falcon::ShapeDefinition const> pShape((Falcon::ShapeDefinition const*)p.GetPtr());
					for (auto const& e : pShape->GetFillDrawables())
					{
						if (e.m_pBitmapDefinition.IsValid() &&
							e.m_bMatchesBounds)
						{
							return e.m_pBitmapDefinition;
						}
					}
				}
			}
		}
	}

	return SharedPtr<Falcon::BitmapDefinition const>();
}

static bool ResolveImageSource(HString baseURL, Byte const* sURL, FilePath& rFilePath, Int32& riWidth, Int32& riHeight)
{
	{
		FilePath filePath;
		if (DataStoreParser::StringAsFilePath(sURL, filePath))
		{
			rFilePath = filePath;
			return true;
		}
	}

	// Further processing requires the URL to be an existing HString (as a symbol name).
	HString h;
	if (!HString::Get(h, sURL))
	{
		return false;
	}

	// Kind of weird, but matches behvaior in AS. Basically, look for an exported
	// character with this name. If it is a Bitmap, we're done. If it is a MovieClip,
	// grab the first child Bitmap of that MovieClip.
	FilePath const filePath = FilePath::CreateContentFilePath(String(baseURL));

	SharedPtr<Falcon::FCNFile> pFCNFile;
	if (!Manager::Get()->GetInProgressFCNFile(filePath, pFCNFile))
	{
		Content::Handle<FCNFileData> hFCNFileData = Manager::Get()->GetFCNFileData(filePath);
		Content::LoadManager::Get()->WaitUntilLoadIsFinished(hFCNFileData);
		SharedPtr<FCNFileData> pFCNFileData(hFCNFileData.GetPtr());
		pFCNFile = pFCNFileData->GetFCNFile();
	}

	SharedPtr<Falcon::Definition> pDefinition;
	if (!pFCNFile->GetExportedDefinition(h, pDefinition))
	{
		if (!pFCNFile->GetImportedDefinition(h, pDefinition))
		{
			return false;
		}
	}

	// Now handle based on definition type.
	SharedPtr<Falcon::BitmapDefinition const> pBitmap;
	if (pDefinition->GetType() == Falcon::DefinitionType::kBitmap)
	{
		pBitmap.Reset((Falcon::BitmapDefinition const*)pDefinition.GetPtr());
	}
	else if (pDefinition->GetType() == Falcon::DefinitionType::kMovieClip)
	{
		// TODO: Do not support this, but if we are supporting it, check for Shapes as well.
		auto pMovieClip = (Falcon::MovieClipDefinition const*)pDefinition.GetPtr();
		pBitmap = FindBitmap(pMovieClip);
	}

	if (!pBitmap.IsValid())
	{
		return false;
	}

	rFilePath = pBitmap->GetFilePath();
	riWidth = (Int32)pBitmap->GetWidth();
	riHeight = (Int32)pBitmap->GetHeight();
	return true;
}

/**
 * Commit a text buffer to a Falcon text instance. Handling is XHTML
 * aware - if the text box supports XHTML parsing, characters will
 * first be converted to ensure that they do not form valid
 * XHTML control sequences.
 */
static void XhtmlAwareSetText(
	Falcon::EditTextInstance* pTextEditingInstance,
	const String& sTextEditingBuffer)
{
	// Nothing to do if no instance.
	if (nullptr == pTextEditingInstance)
	{
		return;
	}

	// If the target box supports XHTML parsing, clean the
	// text before passing it along.
	if (pTextEditingInstance->GetXhtmlParsing())
	{
		String const sConvertedString(HTMLEscape(sTextEditingBuffer));
		pTextEditingInstance->SetText(
			String(sConvertedString.CStr(), sConvertedString.GetSize()));
	}
	// Otherwise, just set the text straight away.
	else
	{
		pTextEditingInstance->SetText(
			String(sTextEditingBuffer.CStr(), sTextEditingBuffer.GetSize()));
	}

	// Make sure the text box has a chance to process and format the text (primarily,
	// we need this to happen so that HTML is unescaped in the text before we read it).
	pTextEditingInstance->CommitFormatting();
}

class FCNFileAnchor SEOUL_SEALED : public Falcon::FCNLibraryAnchor
{
public:
	FCNFileAnchor(
		const SharedPtr<FCNFileData>& pFCNFileData,
		const Content::Handle<FCNFileData>& hFCNFileData)
		: Falcon::FCNLibraryAnchor(pFCNFileData->GetFCNFile())
		, m_hFCNFileData(hFCNFileData)
	{
	}

private:
	Content::Handle<FCNFileData> m_hFCNFileData;

	FCNFileAnchor(const FCNFileAnchor&);
	FCNFileAnchor& operator=(const FCNFileAnchor&);
}; // class FCNFileAnchor

// In developer builds, protect against circular Flash referenes. This used
// to be impossible, but now seems to be allowed in Adobe Animate.
#if !SEOUL_SHIP
namespace
{

class CircularReferenceCheck SEOUL_SEALED
{
public:
	typedef HashTable<FilePath, Int, MemoryBudgets::UIRuntime> Dependencies;
	typedef HashTable<FilePath, Dependencies, MemoryBudgets::UIRuntime> DependencyTable;
	typedef Vector<FilePath, MemoryBudgets::UIRuntime> DependencyPath;

	CircularReferenceCheck(FilePath dependent, FilePath dependency)
		: m_Dependent(dependent)
		, m_Dependency(dependency)
		, m_bCircular(false)
	{
		Lock lock(s_Mutex);

		// Check to see if dependent can be reached from dependency.
		DependencyPath vPath;
		m_bCircular = IsDependentOn(dependency, dependent, vPath);
		if (m_bCircular)
		{
			String sPath;
			for (auto i = vPath.Begin(); vPath.End() != i; ++i)
			{
				sPath.Append("\"" + i->GetRelativeFilenameInSource() + "\"");
				sPath.Append(" is dependent on ");
			}
			sPath.Append("\"" + m_Dependent.GetRelativeFilenameInSource() + "\"");
			SEOUL_WARN("Circular SWF references detected with %s trying to import %s: %s\n",
				dependent.CStr(),
				dependency.CStr(),
				sPath.CStr());
		}
		else
		{
			auto p = s_t.Find(dependent);
			if (nullptr == p)
			{
				Dependencies s;
				SEOUL_VERIFY(s.Insert(dependency, 1).Second);
				SEOUL_VERIFY(s_t.Insert(dependent, s).Second);
			}
			else
			{
				auto pEntry = p->Find(dependency);
				if (nullptr == pEntry)
				{
					SEOUL_VERIFY(p->Insert(dependency, 1).Second);
				}
				else
				{
					(*pEntry)++;
				}
			}
		}
	}

	~CircularReferenceCheck()
	{
		Lock lock(s_Mutex);

		if (!m_bCircular)
		{
			auto p = s_t.Find(m_Dependent);
			SEOUL_ASSERT(nullptr != p);
			auto pEntry = p->Find(m_Dependency);
			SEOUL_ASSERT(nullptr != pEntry);
			SEOUL_ASSERT(*pEntry > 0);
			--(*pEntry);

			if (0 == *pEntry)
			{
				SEOUL_VERIFY(p->Erase(m_Dependency));
			}

			if (p->IsEmpty())
			{
				SEOUL_VERIFY(s_t.Erase(m_Dependent));
			}
		}
	}

	Bool IsCircular() const
	{
		return m_bCircular;
	}

private:
	static Bool IsDependentOn(FilePath dependent, FilePath dependency, DependencyPath& rv)
	{
		auto p = s_t.Find(dependent);
		if (nullptr != p)
		{
			auto const iBegin = p->Begin();
			auto const iEnd = p->End();
			for (auto i = iBegin; iEnd != i; ++i)
			{
				if (i->First == dependency)
				{
					rv.PushBack(dependent);
					return true;
				}
				else if (IsDependentOn(i->First, dependency, rv))
				{
					rv.PushBack(dependent);
					return true;
				}
			}
		}

		return false;
	}

	FilePath const m_Dependent;
	FilePath const m_Dependency;
	Bool m_bCircular;

	static DependencyTable s_t;
	static Mutex s_Mutex;
}; // struct CircularReferenceCheck
CircularReferenceCheck::DependencyTable CircularReferenceCheck::s_t;
Mutex CircularReferenceCheck::s_Mutex;

} // namespace anonymous
#endif

static Bool GetFCNFile(
	const HString& sBaseURL,
	const HString& sURL,
	Falcon::FCNLibraryAnchor*& rpFCNFileAnchor)
{
	String const sResolvedURL(Path::Combine(Path::GetDirectoryName(String(sBaseURL)), String(sURL)));
	FilePath const filePath = FilePath::CreateContentFilePath(sResolvedURL);

	// Check for circular referencing.
#if !SEOUL_SHIP
	FilePath const dependentFilePath = FilePath::CreateContentFilePath(String(sBaseURL));

	CircularReferenceCheck check(dependentFilePath, filePath);
	if (check.IsCircular())
	{
		return false;
	}
#endif // /#if !SEOUL_SHIP

	// Very simple recursion check - Flash (although it apparently does not allow
	// complex circular dependencies) can allow "imports" of itself.
	{
		String const sResolvedBaseURL(sBaseURL.CStr(), sBaseURL.GetSizeInBytes());
		FilePath const baseFilePath = FilePath::CreateContentFilePath(sResolvedBaseURL);
		if (baseFilePath == filePath)
		{
			return false;
		}
	}

	Content::Handle<FCNFileData> hFCNFileData = Manager::Get()->GetFCNFileData(filePath);
	Content::LoadManager::Get()->WaitUntilLoadIsFinished(hFCNFileData);
	SharedPtr<FCNFileData> pFCNFileData(hFCNFileData.GetPtr());

	// Sanity checking.
	if (!pFCNFileData.IsValid())
	{
		SEOUL_WARN("No FCN data for \"%s\".", filePath.CStr());
		return false;
	}
	if (!pFCNFileData->GetFCNFile().IsValid())
	{
		SEOUL_WARN("FCN loaded but is invalid: \"%s\".", filePath.CStr());
		return false;
	}

	rpFCNFileAnchor = SEOUL_NEW(MemoryBudgets::Falcon) FCNFileAnchor(pFCNFileData, hFCNFileData);
	return true;
}

static bool GetFont(
	const HString& sFontName,
	bool bBold,
	bool bItalic,
	Falcon::Font& rFont)
{
	Content::Handle<Falcon::CookedTrueTypeFontData> hTrueTypeFontData = Manager::Get()->GetTrueTypeFontData(
		sFontName,
		bBold,
		bItalic);
	Content::LoadManager::Get()->WaitUntilLoadIsFinished(hTrueTypeFontData);

	SharedPtr<Falcon::CookedTrueTypeFontData> pTrueTypeFontData(hTrueTypeFontData.GetPtr());
	if (pTrueTypeFontData.IsValid())
	{
		rFont.m_bBold = bBold;
		rFont.m_bItalic = bItalic;
		rFont.m_pData.Reset(pTrueTypeFontData.GetPtr());
		rFont.m_sName = sFontName;
		if (!Manager::Get()->GetFontOverrides(
			sFontName,
			bBold,
			bItalic,
			rFont.m_Overrides))
		{
			rFont.m_Overrides = Falcon::FontOverrides{};
		}

		return true;
	}

	SEOUL_WARN("Error loading font %s (Bold: %s, Italic: %s).\nMake sure an entry exists "
		"in \"FontAliases\" in gui.json and the file path points to a valid file",
		sFontName.CStr(), ToString(bBold).CStr(), ToString(bItalic).CStr());
	return false;
}

static Falcon::Stage3DSettings const* GetStage3DSettings(
	const HString& sStage3DSettings)
{
	auto pUI = Manager::Get();
	if (!pUI)
	{
		return nullptr;
	}

	return Manager::Get()->GetStage3DSettings(sStage3DSettings);
}

static Falcon::TextEffectSettings const* GetTextEffectSettings(
	const HString& sTextEffectSettings)
{
	auto pUI = Manager::Get();
	if (!pUI)
	{
		return nullptr;
	}

	return Manager::Get()->GetTextEffectSettings(sTextEffectSettings);
}

#if SEOUL_LOGGING_ENABLED
/** Debug only utility for logging transition info. */
static void DebugLogTransitionInfo(
	HString prevStateId,
	const Stack::StateMachine& stateMachine,
	const DataNode& activatedTransition,
	UInt32 uTransitionIndex,
	HString triggerName = HString())
{
	const DataStore& ds = stateMachine.GetStateMachineConfiguration();

	SEOUL_LOG_STATE("State machine '%s' transitioned from state '%s' to state '%s'",
		stateMachine.GetName().CStr(),
		prevStateId.CStr(),
		stateMachine.GetActiveStateIdentifier().CStr());

	if (activatedTransition.IsNull())
	{
		SEOUL_LOG_STATE("- Transition occurred due to a global transition.");
	}
	else
	{
		SEOUL_LOG_STATE("- Transition occurred due to local transition '%u' of outgoing state '%s'.",
			uTransitionIndex,
			prevStateId.CStr());
	}

	if (triggerName.IsEmpty())
	{
		SEOUL_LOG_STATE("- Transition occurred due to condition variables, empty trigger.");
	}
	else
	{
		SEOUL_LOG_STATE("- Transition occurred due to trigger '%s'.", triggerName.CStr());
	}

	DataNode node;
	UInt32 uCount = 0u;
	if (ds.GetValueFromTable(
		activatedTransition,
		StateMachineCommon::kConditionsTableEntry,
		node) && node.IsArray() && ds.GetArrayCount(node, uCount) && uCount > 0u)
	{
		SEOUL_LOG_STATE("- Conditions passed:");
		for (UInt32 i = 0u; i < uCount; ++i)
		{
			DataNode value;
			HString name;
			if (ds.GetValueFromArray(node, i, value) && ds.AsString(value, name))
			{
				SEOUL_LOG_STATE("  - %s = true", name.CStr());
			}
		}
	}
	if (ds.GetValueFromTable(
		activatedTransition,
		StateMachineCommon::kNegativeConditionsTableEntry,
		node) && node.IsArray() && ds.GetArrayCount(node, uCount) && uCount > 0u)
	{
		SEOUL_LOG_STATE("- Negative conditions activated:");
		for (UInt32 i = 0u; i < uCount; ++i)
		{
			DataNode value;
			HString name;
			if (ds.GetValueFromArray(node, i, value) && ds.AsString(value, name))
			{
				SEOUL_LOG_STATE("  - %s = false", name.CStr());
			}
		}
	}
}
#endif // /#if SEOUL_LOGGING_ENABLED
FCNFileData::FCNFileData(const SharedPtr<Falcon::FCNFile>& pFCNFile, const FilePath& filePath)
	: m_pFCNFile()
	, m_pTemplateRootInstance()
	, m_pTemplateAdvanceInterface()
	, m_filePath(filePath)
{
	// Early out if we have no Falcon SWF data.
	if (!pFCNFile.IsValid())
	{
		return;
	}

	auto const scoepdAction(MakeScopedAction(
		[&]() { Manager::Get()->AddInProgressFCNFile(filePath, pFCNFile); },
		[&]() { Manager::Get()->RemoveInProgressFCNFile(filePath); }));

	// Create objects.
	SharedPtr<Falcon::MovieClipInstance> pRoot(SEOUL_NEW(MemoryBudgets::Falcon) Falcon::MovieClipInstance(pFCNFile->GetRoot()));
	ScopedPtr<AdvanceInterfaceDeferredDispatch> pAdvanceInterface(SEOUL_NEW(MemoryBudgets::UIRuntime) AdvanceInterfaceDeferredDispatch);

	// Advance once to initialize.
	pRoot->Advance(*pAdvanceInterface);

	// Assign
	m_pFCNFile = pFCNFile;
	m_pTemplateRootInstance = pRoot;
	m_pTemplateAdvanceInterface.Swap(pAdvanceInterface);
}

FCNFileData::~FCNFileData()
{
}

/** Populate arguments with a clone of the associated members of this FCNFileData. */
void FCNFileData::CloneTo(
	SharedPtr<Falcon::MovieClipInstance>& rpRootInstance,
	ScopedPtr<AdvanceInterfaceDeferredDispatch>& rpAdvanceInterface) const
{
	// If m_pTemplateRootInstance is valid, m_pAdvanceInterface is assumed to be valid
	if (m_pTemplateRootInstance.IsValid())
	{
		rpAdvanceInterface.Reset(m_pTemplateAdvanceInterface->Clone());

		// Mark/unmark nodes watched by the cloned interface, so cloned nodes will
		// be appropriately refereshed as they are cloned. Do this on the original interface,
		// not the clone, since the clone will be updated as the root instance
		// hierarchy is cloned.
		m_pTemplateAdvanceInterface->MarkWatched();
		rpRootInstance.Reset((Falcon::MovieClipInstance*)m_pTemplateRootInstance->Clone(*rpAdvanceInterface));
		m_pTemplateAdvanceInterface->MarkNotWatched();
	}
	else
	{
		rpAdvanceInterface.Reset();
		rpRootInstance.Reset();
	}
}

static Viewport ApplyFixedAspectRatioToViewport(
	Viewport viewport,
	const Vector2D& vFixed)
{
	auto const iWidth = viewport.m_iViewportWidth;
	auto const iHeight = viewport.m_iViewportHeight;
	auto const fNumerator = vFixed.X;
	auto const fDenominator = vFixed.Y;

	// First try fitting to Y, adjusting X.
	auto const iNewWidth = (Int32)Round((fNumerator * (Float)iHeight) / fDenominator);
	if (iNewWidth < iWidth)
	{
		viewport.m_iViewportX += (Int32)Round(0.5f * (iWidth - iNewWidth));
		viewport.m_iViewportWidth = iNewWidth;
	}
	else
	{
		// Next, fit to X, adjust Y.
		auto const iNewHeight = (Int32)Round((fDenominator * (Float)iWidth) / fNumerator);
		if (iNewHeight < iHeight)
		{
			viewport.m_iViewportY += (Int32)Round(0.5f * (iHeight - iNewHeight));
			viewport.m_iViewportHeight = iNewHeight;
		}
	}

	return viewport;
}

// Enable drag file support in non-ship builds.
#if !SEOUL_SHIP
namespace
{

/** Used throughout. */
static const String ksFla(".fla");
static const String ksSwf(".swf");

/**
 * Given a filename, check if it is a supported UI file
 * for validation (.swf or .fla) and validate.
 */
Bool PossiblyValidateUiFile(const String& sFilename)
{
	SEOUL_ASSERT(IsMainThread()); // Sanity/expectation.

	// Check extension for known/supported type.
	auto const sExt(Path::GetExtension(sFilename));
	if (sExt.CompareASCIICaseInsensitive(ksFla) == 0 ||
		sExt.CompareASCIICaseInsensitive(ksSwf) == 0)
	{
		// Environment check.
		if (Manager::Get())
		{
			// Perform the validation asynchronously.
			(void)Manager::Get()->ValidateUiFile(sFilename, false);

			// Handled.
			return true;
		}
	}

	// Keep dispatching, we can't handle this file.
	return false;
}

} // namespace anonymous
#endif // /!SEOUL_SHIP

Manager::Manager(
	FilePath guiConfigFilePath,
	StackFilter eStackFilter)
	: m_GuiConfigFilePath(guiConfigFilePath)
	, m_vFixedAspectRatio()
	, m_vMinAspectRatio()
	, m_fLastBackBufferAspectRatio(-1.0f)
	, m_pUIStack(SEOUL_NEW(MemoryBudgets::UIRuntime) Stack(guiConfigFilePath, eStackFilter))
	, m_pRenderer()
	, m_pTextEditingMovie(nullptr)
	, m_pTextEditingInstance()
	, m_pTextEditingEventReceiver()
	, m_TextEditingConstraints()
	, m_sTextEditingBuffer()
	, m_pInputOverMovie(nullptr)
	, m_pInputOverInstance()
	, m_pInputCaptureMovie(nullptr)
	, m_pInputCaptureInstance()
	, m_pInputCaptureLink()
	, m_bMouseIsDownOutsideOriginalCaptureInstance()
	, m_InputCaptureMousePosition(0, 0)
	, m_MousePosition(0, 0)
	, m_PreviousMousePosition(0, 0)
	, m_bInputActionsEnabled(true)
#if SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS
	, m_iHorizontalInputCaptureDragThreshold(25)
	, m_iVerticalInputCaptureDragThreshold(15)
#else
	, m_iHorizontalInputCaptureDragThreshold(5)
	, m_iVerticalInputCaptureDragThreshold(3)
#endif
	, m_uInputCaptureHitTestMask(Falcon::kuClickMouseInputHitTest)
	, m_eStackFilter(eStackFilter)
	, m_FCNFiles(false)
	, m_UIFonts(false)
	, m_bInPrePose(false)
#if SEOUL_ENABLE_CHEATS
	, m_uInputVisualizationMode(0u)
#endif // /#if SEOUL_ENABLE_CHEATS
	, m_ePendingClear(kClearActionNone)
#if SEOUL_HOT_LOADING
	, m_bInHotReload(false)
	, m_bPendingHotReload(false)
#endif // /#if SEOUL_HOT_LOADING
	, m_tConditions()
	, m_ConditionTableMutex()
	, m_bWantsRestart(false)
#if SEOUL_LOGGING_ENABLED
	, m_vTriggerHistory(kuTriggerHistorySize)
	, m_uTriggerHistoryHead(0u)
#endif // /#if SEOUL_LOGGING_ENABLED
{
	SEOUL_ASSERT(IsMainThread());

	// Initialize Falcon
	Falcon::GlobalConfig globalConfig;
	globalConfig.m_pGetFCNFile = UI::GetFCNFile;
	globalConfig.m_pGetFont = UI::GetFont;
	globalConfig.m_pGetStage3DSettings = UI::GetStage3DSettings;
	globalConfig.m_pGetTextEffectSettings = UI::GetTextEffectSettings;
	globalConfig.m_pResolveImageSource = UI::ResolveImageSource;
	Falcon::GlobalInit(globalConfig);

	// Register the root poseable hook for rendering UI screens.
	RenderPass::RegisterPoseableSpawnDelegate(
		kLetterboxSpawnType,
		LetterboxPass::SpawnUILetterboxPass);
	RenderPass::RegisterPoseableSpawnDelegate(
		kUIPoseableSpawnType,
		PoseableSpawnHook);

	// Register input callbacks
	Events::Manager::Get()->RegisterCallback(
		g_EventAxisEvent,
		SEOUL_BIND_DELEGATE(&Manager::HandleAxisEvent, this));

	Events::Manager::Get()->RegisterCallback(
		g_EventButtonEvent,
		SEOUL_BIND_DELEGATE(&Manager::HandleButtonEvent, this));

	Events::Manager::Get()->RegisterCallback(
		g_MouseMoveEvent,
		SEOUL_BIND_DELEGATE(&Manager::HandleMouseMoveEvent, this));

	// Instantiate Seoul's bindings for Falcon::Renderer.
	m_pRenderer.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) Renderer);

	// Apply any aspect ratio settings on startup.
	InternalApplyAspectRatioSettings(false);

	// Enable drag file support in non-ship builds.
#if !SEOUL_SHIP
	Events::Manager::Get()->RegisterCallback(EngineDropFileEventId, SEOUL_BIND_DELEGATE(&PossiblyValidateUiFile));
#endif // /!SEOUL_SHIP
}

Manager::~Manager()
{
	SEOUL_ASSERT(IsMainThread());

	// Enable drag file support in non-ship builds.
#if !SEOUL_SHIP
	Events::Manager::Get()->UnregisterCallback(EngineDropFileEventId, SEOUL_BIND_DELEGATE(&PossiblyValidateUiFile));
#endif // /!SEOUL_SHIP

	// Clear all the UI state modification AtomicRingBuffers.
	while (PackedUpdate* p = m_UIConditionQueue.Pop())
	{
		SafeDelete(p);
	}

	while (PackedUpdate* p = m_UIGotoStateQueue.Pop())
	{
		SafeDelete(p);
	}

	while (PackedUpdate* p = m_UITriggerQueue.Pop())
	{
		SafeDelete(p);
	}

	// Clear our global cache of condition variable state.
	m_tConditions.Clear();

	// Clear the stack.
	if (m_pUIStack.IsValid())
	{
		m_pUIStack->Destroy();
		m_pUIStack.Reset();
	}

	// Clear the waiting loads lists.
	m_WaitingForLoads.Clear();

	// Cleanup suspended.
	ClearSuspended();

	// Free movie data.
	SEOUL_VERIFY(m_FCNFiles.Clear());

	// Free font data.
	SEOUL_VERIFY(m_UIFonts.Clear());

	// Cleanup the renderer.
	m_pRenderer.Reset();

	Events::Manager::Get()->UnregisterCallback(
		g_MouseMoveEvent,
		SEOUL_BIND_DELEGATE(&Manager::HandleMouseMoveEvent, this));

	Events::Manager::Get()->UnregisterCallback(
		g_EventButtonEvent,
		SEOUL_BIND_DELEGATE(&Manager::HandleButtonEvent, this));

	Events::Manager::Get()->UnregisterCallback(
		g_EventAxisEvent,
		SEOUL_BIND_DELEGATE(&Manager::HandleAxisEvent, this));

	// Unregister handling of the UI screen root poseable.
	RenderPass::UnregisterPoseableSpawnDelegate(
		kUIPoseableSpawnType);
	RenderPass::UnregisterPoseableSpawnDelegate(
		kLetterboxSpawnType);

	// Shutdown Falcon.
	Falcon::GlobalShutdown();
}

/**
 * Used for runtime updating. This is *not* the code path for hot loading -
 * that is handled automatically by OnFileLoadComplete(). Instead,
 * this is used for controlled updating (e.g. by Game::Patcher) of
 * a shipped product.
 */
void Manager::ApplyFileChange(FilePath filePath)
{
	m_pUIStack->ApplyFileChange(filePath);
}

/** Clear any suspended movies. Destroys the movies and discards their data. */
void Manager::ClearSuspended()
{
	SEOUL_ASSERT(IsMainThread());

	m_WaitingForLoads.ClearSuspended();
}

/**
 * First step of UI::Manager shutdown - places the
 * stack in the default (no initialize) state but
 * does not clear structures.
 */
void Manager::ShutdownPrep()
{
	SEOUL_ASSERT(IsMainThread());

	// It is an error to call this method when
	// m_bInPrePose is true.
	SEOUL_ASSERT(!m_bInPrePose);

	// Identical to clear prep except we don't
	// destroy the stack.
	InternalClearPrep(false);
}

/**
 * Second step of UI::Manager shutdown - call after disabling
 * network file IO/waiting for content loads (if applicable).
 */
void Manager::ShutdownComplete()
{
	SEOUL_ASSERT(IsMainThread());

	// It is an error to call this method when
	// m_bInPrePose is true.
	SEOUL_ASSERT(!m_bInPrePose);

	// If an explicit Clear() is issued while a hot reload is still
	// pending, unset it.
#if SEOUL_HOT_LOADING
	m_bPendingHotReload = false;
#endif // /#if SEOUL_HOT_LOADING

	// Perform a clear and unload FCN file data.
	InternalClear(true, true);
}

#if SEOUL_HOT_LOADING
/**
 * Triggers a hot reload of the UI system.
 */
void Manager::HotReload()
{
	SEOUL_ASSERT(IsMainThread());

	// Don't hot reload if hot loading is suppressed.
	if (Content::LoadManager::Get()->IsHotLoadingSuppressed())
	{
		return;
	}

	m_bPendingHotReload = true;
}

void Manager::ShelveDataForHotLoad(const String& sId, const DataStore& ds)
{
	SharedPtr<DataStore> pDataStore(SEOUL_NEW(MemoryBudgets::DataStore) DataStore);
	pDataStore->CopyFrom(ds);
	(void)m_tHotLoadStash.Insert(sId, pDataStore);
}

SharedPtr<DataStore> Manager::UnshelveDataFromHotLoad(const String& sId) const
{
	SharedPtr<DataStore> pDataStore;
	m_tHotLoadStash.GetValue(sId, pDataStore);
	return pDataStore;
}
#endif // /#if SEOUL_HOT_LOADING

/**
 * Add a transition to the queue.
 */
void Manager::TriggerTransition(HString triggerName)
{
#if SEOUL_ENABLE_STACK_TRACES
	{
#if SEOUL_LOGGING_ENABLED
		if (Logger::GetSingleton().IsChannelEnabled(LoggerChannel::UIDebug))
		{
			size_t aFrames[4];
			auto const uFrames = Core::GetCurrentCallStack(1, SEOUL_ARRAY_COUNT(aFrames), aFrames);

			Byte aBuffer[512];
			Core::PrintStackTraceToBuffer(aBuffer, sizeof(aBuffer), "- ", aFrames, uFrames);

			SEOUL_LOG_UI_DEBUG("UIManager::TriggerTransition: '%s'", triggerName.CStr());
			SEOUL_LOG_UI_DEBUG("%s", aBuffer);
		}
#endif // /#if SEOUL_LOGGING_ENABLED
	}
#endif

	m_UITriggerQueue.Push(SEOUL_NEW(MemoryBudgets::UIRuntime) PackedUpdate(triggerName, HString()));
}

Bool Manager::HasPendingTransitions() const
{
	return !m_UITriggerQueue.IsEmpty();
}

/**
 * Force an unevaluated state transition to the target state - the state
 * transition will occur, ignoring condition or transition requirements, unless
 * the target state machine or state does not exist.
 *
 * \pre stateMachineName must not be the empty string.
 */
void Manager::GotoState(HString stateMachineName, HString stateName)
{
	SEOUL_ASSERT(!stateMachineName.IsEmpty());

	m_UIGotoStateQueue.Push(SEOUL_NEW(MemoryBudgets::UIRuntime) PackedUpdate(stateMachineName, stateName));
}

/**
 * Return a capture of all conditions currently set to the UI system.
 */
void Manager::GetConditions(Conditions& rt) const
{
	Conditions t;
	{
		Lock lock(m_ConditionTableMutex);
		t = m_tConditions;
	}
	rt.Swap(t);
}

/**
 * Returns the value of the given condition. Condition
 * variables are used to enable/disable transitions in the state machine stack.
 *
 * \pre conditionName must not be the empty string.
 */
Bool Manager::GetCondition(HString conditionName) const
{
	SEOUL_ASSERT(!conditionName.IsEmpty());

	Bool bValue = false;
	{
		Lock lock(m_ConditionTableMutex);
		(void)m_tConditions.GetValue(conditionName, bValue);
	}
	return bValue;
}

/**
 * Updates the condition variable conditionName to bValue. Condition
 * variables are used to enable/disable transitions in the state machine stack.
 *
 * \pre conditionName must not be the empty string.
 */
void Manager::SetCondition(HString conditionName, Bool bValue)
{
	SEOUL_ASSERT(!conditionName.IsEmpty());

	{
		Lock lock(m_ConditionTableMutex);
		SEOUL_VERIFY(m_tConditions.Overwrite(conditionName, bValue).Second);
	}

	m_UIConditionQueue.Push(SEOUL_NEW(MemoryBudgets::UIRuntime) PackedUpdate(
		conditionName,
		bValue ? FalconConstants::kTrue : FalconConstants::kFalse));
}

namespace
{

struct MainThreadBroadcastUtil
{
	HString m_sTarget;
	HString m_sEvent;
	Reflection::MethodArguments m_aArguments;
	Int m_nArgumentCount;
	Bool m_bPersistent;

	static void Do(const MainThreadBroadcastUtil& util)
	{
		if (!Manager::Get())
		{
			return;
		}

		(void)Manager::Get()->BroadcastEventTo(
			util.m_sTarget,
			util.m_sEvent,
			util.m_aArguments,
			util.m_nArgumentCount,
			util.m_bPersistent);
	}
};

} // namespace anonymous

/**
 * Broadcasts an event to all active movies in all state machines
 * (if sTarget is empty) or to a specific movie (if sTarget is
 * not empty and is set to a movie type name).
 *
 * If bPersistent is true and the delivery fails, it will
 * be queued for delivery. Delivery will be attempted
 * repeatedly until it succeeds.
 *
 * Return true if the event was received, false otherwise.
 */
Bool Manager::BroadcastEventTo(
	HString sTarget,
	HString sEvent,
	const Reflection::MethodArguments& aArguments,
	Int nArgumentCount,
	Bool bPersistent /*= false*/)
{
	// Off main thread handling.
	if (!IsMainThread())
	{
		MainThreadBroadcastUtil util;
		util.m_sTarget = sTarget;
		util.m_sEvent = sEvent;
		util.m_aArguments = aArguments;
		util.m_nArgumentCount = nArgumentCount;
		util.m_bPersistent = bPersistent;

		Jobs::AsyncFunction(
			GetMainThreadId(),
			&MainThreadBroadcastUtil::Do,
			util);
		return false;
	}

	if (!m_pUIStack.IsValid())
	{
		// TODO: Need to look into why this is even needed: AppPersistenceDataSaveOnComplete::OnSaveComplete.
		return false;
	}

	Bool bReturn = false;

	// For each state machine, dispatch the event with arguments.
	Stack::StackVector const& vStack = m_pUIStack->GetStack();
	UInt32 const uStateMachines = vStack.GetSize();
	for (UInt32 i = 0u; i < uStateMachines; ++i)
	{
		CheckedPtr<State> pState = vStack[i].m_pMachine->GetActiveState();
		if (pState)
		{
			bReturn = pState->OnBroadcastEvent(
				sTarget,
				sEvent,
				aArguments,
				nArgumentCount) || bReturn;
		}
	}

	// Also broadcasts to any suspended movies.
	bReturn = m_WaitingForLoads.BroadcastEventToSuspended(
		sTarget,
		sEvent,
		aArguments,
		nArgumentCount) || bReturn;

	// Queue if needed.
	if (!bReturn && bPersistent)
	{
		PersistentBroadcastEvent entry;
		entry.m_aArguments = aArguments;
		entry.m_uArgumentCount = nArgumentCount;
		entry.m_sEvent = sEvent;
		entry.m_sTarget = sTarget;
		m_lPersistentBroadcastEvents.PushBack(entry);
	}

	return bReturn;
}

void Manager::UpdateTextureReplacement(
	FilePathRelativeFilename symbolName,
	FilePath filePath)
{
	GetRenderer().UpdateTextureReplacement(symbolName, filePath);
}

SharedPtr<Falcon::MovieClipInstance> Manager::GetRootMovieClip(
	HString sStateMachine,
	HString sTarget,
	CheckedPtr<Movie>& rpOwner) const
{
	SEOUL_ASSERT(IsMainThread());

	if (!m_pUIStack.IsValid())
	{
		// TODO: Need to look into why this is even needed: AppPersistenceDataSaveOnComplete::OnSaveComplete.
		return SharedPtr<Falcon::MovieClipInstance>();
	}

	// For each state machine, check if machine and return state.
	auto const& v = m_pUIStack->GetStack();
	for (auto i = v.Begin(); v.End() != i; ++i)
	{
		auto pMachine(i->m_pMachine);
		if (pMachine->GetActiveState().IsValid() &&
			pMachine->GetName() == sStateMachine)
		{
			auto pState = pMachine->GetActiveState();

			for (auto pMovie = pState->GetMovieStackHead(); pMovie.IsValid(); pMovie = pMovie->GetNextMovie())
			{
				if (pMovie->GetMovieTypeName() == sTarget)
				{
					SharedPtr<Falcon::MovieClipInstance> pInstance;
					if (pMovie->GetRootMovieClip(pInstance))
					{
						rpOwner = pMovie;
						return pInstance;
					}
					return SharedPtr<Falcon::MovieClipInstance>();
				}
			}
			return SharedPtr<Falcon::MovieClipInstance>();
		}
	}
	return SharedPtr<Falcon::MovieClipInstance>();
}

/**
 * @return true if movieTypeName describes a native movie
 * (instantiated with Reflection and corresponding 1-to-1 with
 * a C++ instance or false if movieTypeName describes a
 * custom movie, which is backed by an application specific
 * implementation.
 */
Bool Manager::IsNativeMovie(HString movieTypeName) const
{
	SharedPtr<DataStore> pSettings(GetSettings());
	if (!pSettings.IsValid())
	{
		// All movies default to script.
		return false;
	}

	DataNode movieConfig;
	if (!pSettings->GetValueFromTable(pSettings->GetRootNode(), movieTypeName, movieConfig))
	{
		// All movies default to script.
		return false;
	}

	DataNode value;
	if (!pSettings->GetValueFromTable(movieConfig, FalconConstants::kNativeMovieInstance, value))
	{
		// All movies default to script.
		return false;
	}

	Bool bReturn = true;
	if (!pSettings->AsBoolean(value, bReturn))
	{
		// All movies default to script.
		return false;
	}

	return bReturn;
}

/**
 * PrePose is the main thread, per frame update point of the UI system - it handles input
 * capture (dispatch is performed on a worker thread) as well as updating the state of state machines
 * based on conditions and trigger events.
 */
void Manager::PrePose(
	Float fDeltaTime,
	RenderPass& rPass,
	IPoseable* pParent /*= nullptr*/)
{
	// Per frame update of aspect ratio settings. Only reapply if
	// the back buffer target aspect ratio has changed.
	InternalApplyAspectRatioSettings(true);

	// If a clear was scheduled, perform it now.
	if (kClearActionNone != m_ePendingClear)
	{
		InternalClear(kClearActionIncludingFCN == m_ePendingClear, false);
	}

	// Give the stack a change to apply any pending file changes.
	m_pUIStack->ProcessDeferredChanges();

	// Mark that we're now in prepose - used for sanity checks in various functions
	// that happen on the main thread.
	auto const inPrePose(MakeScopedAction(
		SEOUL_BIND_DELEGATE(&Manager::BeginPrePose, this),
		SEOUL_BIND_DELEGATE(&Manager::EndPrePose, this)));

	// Must be called on the main thread.
	SEOUL_ASSERT(IsMainThread());

	// Prepare input for dispatch.
	m_vPendingInputEvents.Swap(m_vInputEventsToDispatch);
	m_vPendingInputEvents.Clear();

	// Process our waiting list.
	{
		SEOUL_PROF("WaitingForLoads.Process");
		m_WaitingForLoads.Process();
	}

	// TODO: This is a hack - we've introduced dependencies in per-movie
	// logic that can be render dependent (specifically, screen resolution, viewport
	// clamping, and the mapping between UI world space to viewport space).
	//
	// Generally need to fix this. Likely solution is to promote all values to be
	// stored in the movie and hide the UI's renderer from public access.
	if (!m_pUIStack->GetStack().IsEmpty())
	{
		auto activeState = m_pUIStack->GetStack().Back().m_pMachine->GetActiveState();
		if (activeState && activeState->GetMovieStackHead())
		{
			activeState->GetMovieStackHead()->SetMovieRendererDependentState();
		}
	}

	// Process delay restart requests.
	InternalEvaluateWantsRestart();

	// Process condition and transition applications to all state machines,
	// determine the current state of the UI system.
	UInt32 const uStateMachines = m_pUIStack->GetStack().GetSize();
	{
		// Flush the goto state, conditions, and trigger queues - this may also trigger state machine
		// transitions.
		Bool bStateTransitionActivated = false;
		ApplyUIConditionsAndTriggersToStateMachines(bStateTransitionActivated);
		ApplyGotoStates(bStateTransitionActivated);

#if SEOUL_HOT_LOADING
		// Now that we've performed normal transition processing.
		bStateTransitionActivated = ApplyHotReload() || bStateTransitionActivated;
#endif

		// If at least one state transition occurred, tell the UI::Stack to update
		// itself after a transition.
		if (bStateTransitionActivated)
		{
			// Before we update the state names, trigger events for any transitions that occured.
			for (UInt32 i = 0u; i < uStateMachines; ++i)
			{
				HString previousStateIdentifier = m_pUIStack->GetStack()[i].m_ActiveStateId;
				HString currentStateIdentifier = m_pUIStack->GetStack()[i].m_pMachine->GetActiveStateIdentifier();
				if (previousStateIdentifier != currentStateIdentifier)
				{
					Events::Manager::Get()->TriggerEvent(
						StateChangeEventId,
						m_pUIStack->GetStack()[i].m_pMachine->GetName(),
						previousStateIdentifier,
						currentStateIdentifier);
				}
			}

			m_pUIStack->OnStateTransitionActivated();
		}
	}

	// fire off an event if the viewport has changed.
	Viewport viewport = ComputeViewport();
	if (m_LastViewport.m_iViewportWidth > 0 &&
		m_LastViewport != viewport)
	{
		(void)BroadcastEvent(kOnViewportChanged, viewport.GetViewportAspectRatio());
	}
	m_LastViewport = viewport;

	// Finally, execute pre pose on each active state in all state machines - this
	// is expected to call UI::Movie::OnUpdate(), and dispatch any deferred events
	// from initialization of a UI::Movie's Falcon scene graph.
	//
	// If we're waiting for loads, treat updates as initially blocked - this will
	// prevent calls to UI::Movie::Update() (and other client facing per-frame
	// logic) until loads are complete. We specifically do this for PrePose()
	// only, not Pose(), which performs Advance(), so that movies will continue
	// to animate.
	Bool bBlockUpdate = IsWaitingForLoads();
	for (UInt32 i = 0u; i < uStateMachines; ++i)
	{
		CheckedPtr<State> pState = m_pUIStack->GetStack()[i].m_pMachine->GetActiveState();
		if (pState.IsValid())
		{
			if (!bBlockUpdate)
			{
				pState->PrePose(rPass, fDeltaTime);
			}
			else
			{
				pState->PrePoseWhenBlocked(rPass, fDeltaTime);
			}
		}
	}
}

/**
 * Per frame work and draw setup performed off the main thread - performs input
 * dispatch to UI movies and Flash movie per-frame update.
 */
void Manager::Pose(
	Float fDeltaTime,
	RenderPass& rPass,
	IPoseable* pParent)
{
	RenderCommandStreamBuilder& rBuilder = *rPass.GetRenderCommandStreamBuilder();

	// Kick off drawing for the UI pass.
	BeginPass(rBuilder, rPass);
	PassThroughPose(fDeltaTime, rPass);
	EndPass(rBuilder, rPass);
}

/**
 * \brief Per frame work and draw setup performed off the main thread - performs input
 * dispatch to UI movies and Flash movie per-frame update.
 */
void Manager::SkipPose(Float fDeltaTime)
{
	InternalHandleInputAndAdvance(fDeltaTime);
}

/**  Hook from enclosing UI (DevUI). */
void Manager::PassThroughPose(Float fDeltaTime, RenderPass& rPass)
{
	// Process persistent broadcast events.
	{
		auto const iBegin = m_lPersistentBroadcastEvents.Begin();
		auto const iEnd = m_lPersistentBroadcastEvents.End();
		for (auto t = iBegin; iEnd != t; )
		{
			// Grab the iterator and advance passed it.
			auto i = t;
			++t;

			// Get the event and remove it.
			PersistentBroadcastEvent const evt = *i;
			m_lPersistentBroadcastEvents.Erase(i);

			// Broadcast the event - it will be re-queued if it failed to deliver
			// again.
			(void)BroadcastEventTo(
				evt.m_sTarget,
				evt.m_sEvent,
				evt.m_aArguments,
				evt.m_uArgumentCount,
				true);
		}
	}

	InternalHandleInputAndAdvance(fDeltaTime);
	InternalPose(rPass);
}

/**
 * \brief Per frame work and draw setup performed off the main thread - performs input
 * plus any fixed aspect ratio settings.
 */
Viewport Manager::ComputeViewport() const
{
	if (m_vFixedAspectRatio.IsZero())
	{
		return g_UIContext.GetRootViewport();
	}
	else
	{
		return ApplyFixedAspectRatioToViewport(
			g_UIContext.GetRootViewport(),
			m_vFixedAspectRatio);
	}
}

/** Psuedo world height used by the FX camera. */
static const Float kfUIRendererFxCameraWorldHeight = 300.0f;

Float Manager::ComputeUIRendererFxCameraWorldHeight(const Viewport& viewport) const
{
	// Easy case, no min, so return base.
	if (m_vMinAspectRatio.IsZero()) { return kfUIRendererFxCameraWorldHeight; }

	// Compute viewport ratio.
	auto const fViewportRatio = viewport.GetViewportAspectRatio();

	// If below min, rescale.
	auto const fMinRatio = (m_vMinAspectRatio.X / m_vMinAspectRatio.Y);
	if (fViewportRatio < fMinRatio)
	{
		auto const fDesiredWidth = (kfUIRendererFxCameraWorldHeight * fMinRatio);
		return (fDesiredWidth / fViewportRatio);
	}

	// Done.
	return kfUIRendererFxCameraWorldHeight;
}

CheckedPtr<Movie> Manager::InstantiateMovie(HString typeName)
{
	return m_WaitingForLoads.Instantiate(typeName);
}

// Block of SWF and FLA validation code and handling. Developer only.
#if !SEOUL_SHIP
namespace
{

/**
 * Developer function, iterates all FLA files in the game's
 * source folder and runs validation logic on them.
 */
static Bool FlaValidate(const String& sExcludeWildcard)
{
	auto const iStart = SeoulTime::GetGameTimeInTicks();

	Wildcard wildcard(sExcludeWildcard);

	UInt32 uSuccess = 0u;

	Vector<String> vs;
	auto const sDirPath(GamePaths::Get()->GetSourceDir());
	Int32 iId = 0;
	g_UIContext.DisplayTrackedNotification("Validating FLAs...", iId);
	if (FileManager::Get()->GetDirectoryListing(sDirPath, vs, false, true, ".fla"))
	{
		for (auto const& s : vs)
		{
			// Skip
			if (wildcard.IsExactMatch(s))
			{
				continue;
			}

			// Perform the check.
			if (!Falcon::CheckFla(s))
			{
				SEOUL_WARN("%s: failed validation checks.", s.CStr());
				continue;
			}

			// Track the success.
			++uSuccess;
		}
	}
	auto const iEnd = SeoulTime::GetGameTimeInTicks();

	auto const bReturn = (uSuccess == vs.GetSize());
	SEOUL_LOG("Validated %u FLA files in %.2f s (%u passed)", vs.GetSize(), SeoulTime::ConvertTicksToSeconds(iEnd - iStart), uSuccess);
	if (Manager::Get())
	{
		g_UIContext.DisplayNotification(String::Printf("FLA (%u files): %s", vs.GetSize(), (bReturn ? "SUCCESS" : "FAILURE")));
		g_UIContext.KillNotification(iId);
	}

	return bReturn;
}

/**
 * Developer function, iterates all SWF files available to the app
 * and runs validation logic on them.
 */
static Bool SwfValidate(const String& sExcludeWildcard)
{
	auto const iStart = SeoulTime::GetGameTimeInTicks();

	Wildcard wildcard(sExcludeWildcard);

	UInt32 uSuccess = 0u;

	Vector<String> vs;
	auto const sDirPath(GamePaths::Get()->GetSourceDir());
	Int32 iId = 0;
	g_UIContext.DisplayTrackedNotification("Validating SWFs...", iId);
	if (FileManager::Get()->GetDirectoryListing(sDirPath, vs, false, true, FileTypeToSourceExtension(FileType::kUIMovie)))
	{
		for (auto const& s : vs)
		{
			auto const filePath = FilePath::CreateContentFilePath(s);

			// Skip
			if (wildcard.IsExactMatch(filePath.GetRelativeFilenameInSource()))
			{
				// Track the skip as a success.
				++uSuccess;
				continue;
			}

			auto hData = Manager::Get()->GetFCNFileData(filePath);
			Content::LoadManager::Get()->WaitUntilLoadIsFinished(hData);
			auto pData(hData.GetPtr());
			if (!pData.IsValid())
			{
				SEOUL_WARN("%s: failed to load SWF data, is corrupt or invalid.", s.CStr());
				continue;
			}

			if (!pData->GetFCNFile().IsValid() ||
				!pData->GetFCNFile()->IsOk())
			{
				SEOUL_WARN("%s: failed to parse SWF data, is corrupt or invalid.", s.CStr());
				continue;
			}

			if (!pData->GetFCNFile()->Validate())
			{
				// No need to add an additional message here, fundamentally,
				// a validation failure means individual warniexitngs will have
				// been emitted.
				continue;
			}

			// Track the success.
			++uSuccess;
		}
	}
	auto const iEnd = SeoulTime::GetGameTimeInTicks();

	auto const bReturn = (uSuccess == vs.GetSize());
	SEOUL_LOG("Validated %u SWF files in %.2f s (%u passed)", vs.GetSize(), SeoulTime::ConvertTicksToSeconds(iEnd - iStart), uSuccess);
	if (Manager::Get())
	{
		g_UIContext.DisplayNotification(String::Printf("SWF (%u files): %s", vs.GetSize(), (bReturn ? "SUCCESS" : "FAILURE")));
		g_UIContext.KillNotification(iId);
	}

	return bReturn;
}

/**
 * Combined full file SWF + FLA validation.
 */
Bool DoValidate(const String& sExcludeWildcard)
{
	Bool bOk = true;
	bOk = FlaValidate(sExcludeWildcard) && bOk;
	bOk = SwfValidate(sExcludeWildcard) && bOk;
	return bOk;
}

/**
 * Validate a single .fla file.
 *
 * @param [out] psSwfFilename If set, outputs the (relative) path to the
 *                            SWF file of this FLA file, if available.
 */
static Bool FlaValidateSingle(const String& sFilename, String* psSwfFilename = nullptr)
{
	// For reporting and tracking.
	Bool bReturn = true;
	auto const sName(Path::GetFileName(sFilename));

	Int32 iId = 0;
	g_UIContext.DisplayTrackedNotification(String::Printf("Validating FLA: %s", sName.CStr()), iId);

	// Perform the check.
	if (!Falcon::CheckFla(sFilename, psSwfFilename))
	{
		SEOUL_WARN("%s: failed validation checks.", sName.CStr());
		bReturn = false;
	}

	// Convert to an absolute path if specified.
	if (nullptr != psSwfFilename && !psSwfFilename->IsEmpty())
	{
		*psSwfFilename = Path::GetExactPathName(Path::Combine(Path::GetDirectoryName(sFilename), *psSwfFilename));
	}
	// Check if the resultant file exists - if not, clear.
	if (nullptr != psSwfFilename)
	{
		// Does not exist, return an empty filename.
		if (!FileManager::Get()->Exists(*psSwfFilename))
		{
			psSwfFilename->Clear();
		}
	}

	if (Manager::Get())
	{
		g_UIContext.DisplayNotification(String::Printf("%s: %s", sName.CStr(), (bReturn ? "SUCCESS" : "FAILURE")));
		g_UIContext.KillNotification(iId);
	}

	return bReturn;
}

/**
 * Validate a single .swf file.
 */
static Bool SwfValidateSingle(FilePath filePath)
{
	// For reporting and tracking.
	Bool bReturn = true;
	auto const sName(Path::GetFileName(filePath.GetRelativeFilenameInSource()));

	Int32 iId = 0;
	g_UIContext.DisplayTrackedNotification(String::Printf("Validating SWF: %s", sName.CStr()), iId);

	// Load the data.
	auto hData = Manager::Get()->GetFCNFileData(filePath);
	Content::LoadManager::Get()->WaitUntilLoadIsFinished(hData);
	auto pData(hData.GetPtr());

	// Sanity check.
	if (bReturn && !pData.IsValid())
	{
		SEOUL_WARN("%s: failed to load SWF data, is corrupt or invalid.", filePath.CStr());
		bReturn = false;
	}

	// Sanity check.
	if (bReturn &&  (!pData->GetFCNFile().IsValid() || !pData->GetFCNFile()->IsOk()))
	{
		SEOUL_WARN("%s: failed to parse SWF data, is corrupt or invalid.", filePath.CStr());
		bReturn = false;
	}

	// Validation.
	if (bReturn && !pData->GetFCNFile()->Validate())
	{
		bReturn = false;
	}

	if (Manager::Get())
	{
		g_UIContext.DisplayNotification(String::Printf("%s: %s", sName.CStr(), (bReturn ? "SUCCESS" : "FAILURE")));
		g_UIContext.KillNotification(iId);
	}

	return bReturn;
}

// TODO: Slow, and relies on a bunch of assumptions W.R.T.
// the naming of the FLA vs. the SWF, etc. as well as the location.
static Bool GetFlaFilenameImpl(void* pUserData, Directory::DirEntryEx& entry)
{
	// Quick check, must be a .fla file.
	if (0 != Path::GetExtension(entry.m_sFileName).CompareASCIICaseInsensitive(ksFla))
	{
		return true;
	}

	// If the base name of both the SWF and FLA match, we've found the match.
	auto& rs = *((String*)pUserData);
	auto const sBase(Path::GetFileNameWithoutExtension(entry.m_sFileName));
	if (0 == sBase.CompareASCIICaseInsensitive(rs))
	{
		// Done - return false to terminate enumeration.
		rs.Assign(entry.m_sFileName);
		return false;
	}

	// Keep searching.
	return true;
}

// TODO: Slow, and relies on a bunch of assumptions W.R.T.
// the naming of the FLA vs. the SWF, etc. as well as the location.
static String GetFlaFilename(FilePath filePath)
{
	auto const sFilename(filePath.GetAbsoluteFilenameInSource());
	auto const sDir(Path::GetDirectoryName(sFilename));

	// Directory enumerate to find a FLA with the same base
	// name as the SWF.
	auto const sBase(Path::GetFileNameWithoutExtension(sFilename));
	String sReturn(sBase);
	(void)Directory::GetDirectoryListingEx(sDir, SEOUL_BIND_DELEGATE(GetFlaFilenameImpl, &sReturn));

	// Failure if not changed from base.
	if (sBase == sReturn)
	{
		return String();
	}
	else
	{
		return sReturn;
	}
}

/**
 * Combined FLA + SWF check, assuming filePath is a SWF.
 */
Bool DoValidateSingleFilePath(FilePath filePath)
{
	Bool bOk = true;
	bOk = SwfValidateSingle(filePath) && bOk;

	auto const sFlaFilename(GetFlaFilename(filePath));
	if (!sFlaFilename.IsEmpty())
	{
		bOk = FlaValidateSingle(sFlaFilename) && bOk;
	}
	return bOk;
}

/**
 * Combined FLA + SWF check, supports sFilename as a SWF
 * or FLA (will also check the corresponding sibling FLA
 * or SWF if that file exists).
 */
Bool DoValidateSingleFilename(const String& sFilename)
{
	auto const sExt(Path::GetExtension(sFilename));

	// Checking a FLA.
	if (sExt.CompareASCIICaseInsensitive(ksFla) == 0)
	{
		// Validate the FLA, will provide the SWF if it exists.
		String sSwfFilename;
		Bool bOk = true;
		bOk = FlaValidateSingle(sFilename, &sSwfFilename) && bOk;

		// Now validate the SWF if found.
		if (!sSwfFilename.IsEmpty())
		{
			bOk = SwfValidateSingle(FilePath::CreateContentFilePath(sSwfFilename)) && bOk;
		}
		return bOk;
	}
	else
	{
		// Assume the path is to the .swf.
		return DoValidateSingleFilePath(FilePath::CreateContentFilePath(sFilename));
	}
}

} // namespace anonymous

/**
 * Validate a specific file - expected to be a .SWF file. Will also
 * validate against the corresponding .FLA file (if it exists).
 */
Bool Manager::ValidateUiFile(FilePath filePath, Bool bSynchronous)
{
	if (bSynchronous)
	{
		return DoValidateSingleFilePath(filePath);
	}
	else
	{
		Jobs::AsyncFunction(&DoValidateSingleFilePath, filePath);
		return true;
	}
}

/**
 * Validate a specific file - can be a .SWF or .FLA file. Will also
 * validate against the corresponding .FLA or .SWF file (if it exists).
 */
Bool Manager::ValidateUiFile(const String& sFilename, Bool bSynchronous)
{
	if (bSynchronous)
	{
		return DoValidateSingleFilename(sFilename);
	}
	else
	{
		Jobs::AsyncFunction(&DoValidateSingleFilename, sFilename);
		return true;
	}
}

Bool Manager::ValidateUiFiles(const String& sExcludeWildcard, Bool bSynchronous)
{
	if (bSynchronous)
	{
		return DoValidate(sExcludeWildcard);
	}
	else
	{
		Jobs::AsyncFunction(&DoValidate, sExcludeWildcard);
		return true;
	}
}
#endif // /!SEOUL_SHIP

void Manager::AddToInputWhitelist(const SharedPtr<Falcon::MovieClipInstance>& p)
{
	Lock lock(m_InputWhitelistMutex);
	(void)m_InputWhitelist.Insert(p);
}

void Manager::ClearInputWhitelist()
{
	Lock lock(m_InputWhitelistMutex);
	m_InputWhitelist.Clear();
}

void Manager::RemoveFromInputWhitelist(const SharedPtr<Falcon::MovieClipInstance>& p)
{
	Lock lock(m_InputWhitelistMutex);
	(void)m_InputWhitelist.Erase(p);
}

#if !SEOUL_SHIP
Vector<String> Manager::DebugGetInputWhitelistPaths() const
{
	Vector<String> v;

	{
		Lock lock(m_InputWhitelistMutex);
		for (auto const& p : m_InputWhitelist)
		{
			v.PushBack(GetPath(p.GetPtr()));
		}
	}

	QuickSort(v.Begin(), v.End());
	return v;
}
#endif

/** Read min-max config settings for aspect ratio. */
void Manager::InternalApplyAspectRatioSettings(Bool bUpdate)
{
	// TODO: Not a fan of making UI::Manager directly dependent on application.json

	auto const viewport = g_UIContext.GetRootViewport();
	auto const fRatio = viewport.GetTargetAspectRatio();

	// Early out if the same.
	if (m_fLastBackBufferAspectRatio > 0.0f && m_fLastBackBufferAspectRatio == fRatio)
	{
		return;
	}

	// Update.
	m_fLastBackBufferAspectRatio = fRatio;

	m_vMinAspectRatio = Vector2D::Zero();

	// Min aspect ratio.
	{
		FixedAspectRatio::Enum e = FixedAspectRatio::kOff;
		{
			HString const h(String::Printf(ksMinAspectRatio, GetCurrentPlatformName()));
			if (GetApplicationJsonValue(h, e))
			{
				ToRatio(e, m_vMinAspectRatio);
			}
		}

		// Apply as a viewport changed if boxing is enabled - otherwise,
		// it will be applied only as a render coordinate top/bottom
		// changed.
		{
			HString const h(String::Printf(ksMinAspectRatioBoxed, GetCurrentPlatformName()));
			Bool bBoxed = false;
			if (GetApplicationJsonValue(h, bBoxed) && bBoxed)
			{
				if (!IsZero(m_vMinAspectRatio.Y) && ((m_vMinAspectRatio.X / m_vMinAspectRatio.Y) > fRatio))
				{
					SetFixedAspectRatio(e);
				}
			}
		}
	}

	// Clear out.
	if (IsZero(m_vMinAspectRatio.X) ||
		IsZero(m_vMinAspectRatio.Y))
	{
		m_vMinAspectRatio = Vector2D::Zero();
	}

	// Aspect ratio absolute setting.
	if (!bUpdate)
	{
		HString const h(String::Printf(ksFixedAspectRatio, GetCurrentPlatformName()));
		FixedAspectRatio::Enum e = FixedAspectRatio::kOff;
		if (GetApplicationJsonValue(h, e))
		{
			SetFixedAspectRatio(e);
		}
	}
}

/**
 * Given a target state in a state machine, check for any content
 * that needs to finish loading to enter that state. If present,
 * sets that content to the manager's m_vWaitingForLoads vector,
 * and returns true.
 *
 * @return True if the manager is waiting for loads, false otherwise.
 */
Bool Manager::InternalCheckAndWaitForLoads(
	CheckedPtr<Stack::StateMachine> pStateMachine,
	HString targetStateIdentifier)
{
	// Get settings - if not available, don't process transitions.
	SharedPtr<DataStore> pSettings(GetSettings());
	if (!pSettings.IsValid())
	{
		return true;
	}

	// This situation can occur on condition changes while loads are in progress.
	// Update the waiting for loads set when this occurs.
	if (m_WaitingForLoads.HasEntries() && m_WaitingForLoads.GetMachine() != pStateMachine)
	{
		m_WaitingForLoads.Clear();
	}

	// Movies and settings.
	if (!m_WaitingForLoads.HasEntries())
	{
		// Get the array of movies associated with the state.
		auto const& stateMachineDataStore = pStateMachine->GetStateMachineConfiguration();
		DataNode node = stateMachineDataStore.GetRootNode();
		(void)stateMachineDataStore.GetValueFromTable(node, targetStateIdentifier, node);
		(void)stateMachineDataStore.GetValueFromTable(node, FalconConstants::kMoviesTableKey, node);

		// Enumerate the movies, get file paths, and check that movie data for loading status.
		UInt32 uMoviesCount = 0u;
		(void)stateMachineDataStore.GetArrayCount(node, uMoviesCount);
		for (UInt32 i = 0u; i < uMoviesCount; ++i)
		{
			// Get the type name of the movie.
			HString movieTypeName;
			{
				DataNode value;
				(void)stateMachineDataStore.GetValueFromArray(node, i, value);

				Byte const* s = nullptr;
				UInt32 u = 0u;
				if (stateMachineDataStore.AsString(value, s, u))
				{
					movieTypeName = HString(s, u);
				}
			}

			// Get the movie node.
			DataNode movieNode = pSettings->GetRootNode();
			(void)pSettings->GetValueFromTable(movieNode, movieTypeName, movieNode);

			// Get the movie file path and track it as a dependency if necessary.
			{
				DataNode movieFilePathNode;
				(void)pSettings->GetValueFromTable(movieNode, FalconConstants::kMovieFilePath, movieFilePathNode);
				FilePath movieFilePath;
				(void)pSettings->AsFilePath(movieFilePathNode, movieFilePath);

				// If we have a valid FilePath, check for loading.
				if (movieFilePath.IsValid())
				{
					// Add the entry.
					WaitingForLoads::WaitingForData data;
					data.m_hData = GetFCNFileData(movieFilePath);
					data.m_MovieTypeName = movieTypeName;
					m_WaitingForLoads.Add(data);
				}
			}
		}

		// If we added some entries, associate the state machine.
		if (m_WaitingForLoads.HasEntries())
		{
			m_WaitingForLoads.SetMachine(pStateMachine);
		}
	}

	// Check and return.
	return IsWaitingForLoads();
}

/**
 * Reset input capture to the default no-capture
 * state. Does not report the release event
 * to any handlers, that is expected to be handled
 * separately.
 */
void Manager::InternalClearInputCapture()
{
	m_pInputCaptureInstance.Reset();
	m_pInputCaptureLink.Reset();
	m_pInputCaptureMovie = nullptr;
	m_InputCaptureMousePosition = Point2DInt(0, 0);
	m_uInputCaptureHitTestMask = Falcon::kuClickMouseInputHitTest;
}

void Manager::InternalClearInputOver()
{
	m_pInputOverInstance.Reset();
	m_pInputOverMovie = nullptr;
}

/**
 * First chunk of an InternalClear() operation. Also used
 * in ShutdownPrep() to handle the first half of UI::Manager
 * shutdown.
 *
 * IMPORTANT: It is only safe to destroy the stack
 * during developer hot loading or in ShutdownComplete.
 */
void Manager::InternalClearPrep(Bool bDestroyStack)
{
	// Clear all the UI state modification AtomicRingBuffers.
	while (PackedUpdate* p = m_UIConditionQueue.Pop())
	{
		SafeDelete(p);
	}

	while (PackedUpdate* p = m_UIGotoStateQueue.Pop())
	{
		SafeDelete(p);
	}

	while (PackedUpdate* p = m_UITriggerQueue.Pop())
	{
		SafeDelete(p);
	}

	// Clear our global cache of condition variable state.
	m_tConditions.Clear();

	// Clear input state.
	{
		Lock lock(m_InputWhitelistMutex);
		m_InputWhitelist.Clear();
	}
	m_vInputEventsToDispatch.Clear();
	m_vPendingInputEvents.Clear();

	// If requested, destroy the stack.
	if (bDestroyStack)
	{
		m_pUIStack->Destroy();
		m_pUIStack.Reset();
	}
	// Otherwise, just force all states to the null state.
	else
	{
		// Force all states into the no state.
		// Only capture state of state machines with at least one movie
		// that is part of hot loading.
		for (auto const& e : m_pUIStack->GetStack())
		{
			auto p(e.m_pMachine);

			// Force to no state.
			SEOUL_VERIFY(p->GotoState(HString()));
		}
	}

	// Clear the waiting loads lists.
	m_WaitingForLoads.Clear();

	// Cleanup suspended.
	ClearSuspended();
}

static const HString kWantsRestartGate("WantsRestartGate");
static HString GetWantsRestartGate()
{
	SharedPtr<DataStore> pSettings(Manager::Get()->GetSettings());
	if (!pSettings.IsValid())
	{
		return HString();
	}

	DataNode value;
	(void)pSettings->GetValueFromTable(pSettings->GetRootNode(), kWantsRestartGate, value);

	HString out;
	(void)pSettings->AsString(value, out);
	return out;
}

/**
 * Called per-frame prior to condition application
 * to trigger a requested restart if all conditions
 * for that restart have been satisfied.
 */
void Manager::InternalEvaluateWantsRestart()
{
	// If not pending, done.
	if (!m_bWantsRestart) { return; }

	// Otherwise, check if gating condition is true - if true, can't restart.
	auto const gate(GetWantsRestartGate());
	if (!gate.IsEmpty() && GetCondition(gate)) { return; }

	// Finally, unset m_bWantsRestart and trigger it.
	m_bWantsRestart = false;
	TriggerRestart(true);
}

/**
 * Fully clear and flush the UI system.
 */
void Manager::InternalClear(Bool bClearFCNData, Bool bShutdown)
{
	SEOUL_ASSERT(IsMainThread());

	// Should never be in prepose when a shutdown has been triggered.
	SEOUL_ASSERT(!bShutdown || !m_bInPrePose);

	// If we're inside pre-pose, mark this clear to occur later.
	if (m_bInPrePose)
	{
		if (m_ePendingClear == kClearActionNone ||
			(m_ePendingClear == kClearActionExcludingFCN && bClearFCNData))
		{
			m_ePendingClear = (bClearFCNData ? kClearActionIncludingFCN : kClearActionExcludingFCN);
		}
		return;
	}

	// Make sure the render job is not running while we clear the UI system.
	if (!bShutdown)
	{
		if (Seoul::Renderer::Get())
		{
			Seoul::Renderer::Get()->WaitForRenderJob();
		}
	}

	// Handle the first chunk of clearing operations.
	InternalClearPrep(true);

	// If clearing FCN, flush and clear all FCN files.
	if (bClearFCNData)
	{
		// Free movie data.
		SEOUL_VERIFY(m_FCNFiles.Clear());

		// Free font data.
		SEOUL_VERIFY(m_UIFonts.Clear());
	}

	// Clear the renderer
	m_pRenderer->PurgeTextureCache();

	// Recreate the stack unless we're shutting down.
	if (!bShutdown)
	{
		m_pUIStack.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) Stack(m_GuiConfigFilePath, m_eStackFilter));
	}

	// Reset the pending clear flag.
	m_ePendingClear = kClearActionNone;
}

void Manager::InternalHandleInputAndAdvance(Float fDeltaTime)
{
	{
		Movie* pHitMovie = nullptr;
		// The MovieClip whose child was hit.
		SharedPtr<Falcon::MovieClipInstance> pHitInstance;
		// The child of the MovieClip that was actually hit
		SharedPtr<Falcon::Instance> pLeafInstance;

		Vector<Movie*> vPassthroughInputsRet;
		Bool bHit = HitTest(m_uInputCaptureHitTestMask, m_MousePosition, pHitMovie, pHitInstance, pLeafInstance, &vPassthroughInputsRet);

		// Sanity check - either bHit is false, or pHitMovie and pHitInstance are not nullptr.
		SEOUL_ASSERT(!bHit || (nullptr != pHitMovie && pHitInstance.IsValid() && pLeafInstance.IsValid()));

		// Cache for over/out checking.
		auto const bOverHit = bHit;
		auto const pOverMovie = pHitMovie;
		auto const pOverInstance = pHitInstance;

		// Handle mouse move.
		if (m_PreviousMousePosition != m_MousePosition)
		{
			if (m_pInputCaptureInstance.IsValid())
			{
				// Special handling on movement, switch between "click input" (taps on mobile) and
				// "drag input" (swipes on mobile).
				//
				// The rules are:
				// - if we're still performing click input tests, and the current mouse position deviates
				//   from the input capture position by a threshold, do another hit test with the drag input flag.
				//   If this test returns a hit, do the following:
				//   - switch to drag input tests.
				//   - if the shape has changed, deliver an OnMouseUp event with bInInstance set to false
				//     to the previous instance.
				//   If this test returns a miss, do the following:
				//   - switch to drag input tests.
				//   - switch to no capture shape, deliver an OnMouseUp event with bInInstance set to false
				//     to the previous instance.
				if (m_uInputCaptureHitTestMask == Falcon::kuClickMouseInputHitTest)
				{
					// Check the delta, if it's beyond the the threshold in stage space, we're now in drag
					// mode.
					const Vector2D vMousePosition = m_pInputCaptureMovie->GetMousePositionInWorld(m_MousePosition);
					const Vector2D vInputCaptureMousePosition = m_pInputCaptureMovie->GetMousePositionInWorld(m_InputCaptureMousePosition);
					if ((Int)Abs(vMousePosition.X - vInputCaptureMousePosition.X) >= m_iHorizontalInputCaptureDragThreshold ||
						(Int)Abs(vMousePosition.Y - vInputCaptureMousePosition.Y) >= m_iVerticalInputCaptureDragThreshold)
					{
						// Left/right drag amounts.
						auto const iDeltaX = (Int)Abs(vMousePosition.X - vInputCaptureMousePosition.X);
						auto const iDeltaY = (Int)Abs(vMousePosition.Y - vInputCaptureMousePosition.Y);

						// Drag mode based on which direction had the higher magnitude.
						UInt8 uDragMode = Falcon::kuDragMouseInputHitTest;
						if (iDeltaX >= m_iHorizontalInputCaptureDragThreshold &&
							(iDeltaY < m_iVerticalInputCaptureDragThreshold || (iDeltaX > iDeltaY)))
						{
							uDragMode = Falcon::kuHorizontalDragMouseInputHitTest;
						}
						else if (iDeltaY >= m_iVerticalInputCaptureDragThreshold &&
							(iDeltaX < m_iHorizontalInputCaptureDragThreshold || (iDeltaY >= iDeltaX)))
						{
							uDragMode = Falcon::kuVerticalDragMouseInputHitTest;
						}

						// Perform a drag hit test at the location that the original mouse down occurred (since that is where the player presumably wanted to start dragging).
						Movie* pDragHitMovie = nullptr;
						SharedPtr<Falcon::MovieClipInstance> pDragHitInstance;
						SharedPtr<Falcon::Instance> pDragLeafInstance;
						Bool const bDragHit = HitTest(
							uDragMode,
							m_InputCaptureMousePosition,
							pDragHitMovie,
							pDragHitInstance,
							pDragLeafInstance,
							nullptr);

						// Sanity check - either bDragHit is false, or pDragHitMovie and pDragHitInstance are not nullptr.
						SEOUL_ASSERT(!bDragHit || (nullptr != pDragHitMovie && pDragHitInstance.IsValid() && pDragLeafInstance.IsValid()));

						// Switch if the drag only test returned a new hit, or if the
						// previous m_uInputCaptureHitTestMask hit a shape other than the current
						// capture shape.
						if (bDragHit)
						{
							// Initially switch to drag input hit tests.
							m_uInputCaptureHitTestMask = uDragMode;

							// If we've changed shapes with the drag input test,
							// switch to that shape as the captured instance.
							// If the shape is nullptr and the original shape is setup for drag input,
							// assume we wanted to drag the original shape, and don't try to switch to
							// a nullptr shape.
							Bool const bOriginalShapeHasDragInput = (0 != (m_pInputCaptureInstance->GetHitTestSelfMask() & uDragMode));
							if (pDragHitInstance != m_pInputCaptureInstance &&
							   (pDragHitInstance.IsValid() || !bOriginalShapeHasDragInput))
							{
								// Release the old shape at the current mouse position.
								m_pInputCaptureMovie->OnMouseButtonReleased(
									m_MousePosition,
									m_pInputCaptureInstance,
									false,  // Release due to mode switch is never "in instance".
									m_uInputCaptureHitTestMask);

								// Update local and member variables.
								pHitMovie = pDragHitMovie;
								pHitInstance = pDragHitInstance;
								pLeafInstance = pDragLeafInstance;
								bHit = bDragHit;
								m_pInputCaptureInstance = pDragHitInstance;
								m_pInputCaptureMovie = pDragHitMovie;
								m_pInputCaptureLink.Reset();

								// Stop text editing in either case.
								StopTextEditing();

								// If we hit a shape with the drag only test, update.
								if (bHit && nullptr != m_pInputCaptureMovie && m_pInputCaptureInstance.IsValid())
								{
									// Send a down event to the new instance.
									// Since it didn't get the original down event, send it from the original down position.
									m_pInputCaptureMovie->OnMouseButtonPressed(
										m_InputCaptureMousePosition,
										m_pInputCaptureInstance,
										true);
								}
								// Otherwise, we've released all captures.
								else
								{
									// Reset input capture to the initial no-capture state.
									InternalClearInputCapture();
								}
							}
						}
						// We dragged off the original capture instance
						else if(pHitInstance != m_pInputCaptureInstance && !m_bMouseIsDownOutsideOriginalCaptureInstance)
						{
							// Set a flag so we only call this release event once per exit
							m_bMouseIsDownOutsideOriginalCaptureInstance = true;

							// Release the old shape at the current mouse position.
							m_pInputCaptureMovie->OnMouseButtonReleased(
								m_MousePosition,
								m_pInputCaptureInstance,
								false,  // Release due to mode switch is never "in instance".
								m_uInputCaptureHitTestMask);
						}
						// We dragged back on the original capture instance
						else if(pHitInstance == m_pInputCaptureInstance && m_bMouseIsDownOutsideOriginalCaptureInstance)
						{
							// Set a flag so we only call this press event once per return
							m_bMouseIsDownOutsideOriginalCaptureInstance = false;

							// Press at the current mouse position.
							m_pInputCaptureMovie->OnMouseButtonPressed(
								m_MousePosition,
								m_pInputCaptureInstance,
								false); // Do press that doesn't trigger things like an original press
						}
					}
				}
			}

			// If m_pInputCaptureInstance is still valid, send move events to the captured instance
			// if we're dragging.
			if (m_pInputCaptureInstance.IsValid() &&
				0 != (m_uInputCaptureHitTestMask & Falcon::kuDragMouseInputHitTest))
			{
				// Only report moves if the hit test self mask matches the current input capture mask.
				if (0 != (m_uInputCaptureHitTestMask & m_pInputCaptureInstance->GetHitTestSelfMask()))
				{
					m_pInputCaptureMovie->OnMouseMove(
						m_MousePosition,
						m_pInputCaptureInstance,
						(bHit && pHitInstance == m_pInputCaptureInstance));
				}
			}

			m_PreviousMousePosition = m_MousePosition;
		}

		// Process input events.
		UInt32 const nInputEventsToDispatch = m_vInputEventsToDispatch.GetSize();
		for (UInt32 i = 0; i < nInputEventsToDispatch; ++i)
		{
			auto event = m_vInputEventsToDispatch[i];

			// Handle mouse down or mouse up.
			if (event.m_eEventType == QueuedInputEvent::kEventTypeButton &&
				event.m_eButtonID == InputButton::MouseLeftButton)
			{
				// Down event.
				if (event.m_eButtonEventType == ButtonEventType::ButtonPressed)
				{
					// A down event when we already have an input capture.
					if (m_pInputCaptureInstance.IsValid())
					{
						// Ignore down events against the same instance.
						if (bHit &&
							pHitInstance == m_pInputCaptureInstance)
						{
							continue;
						}

						// Stop editing an active text box.
						StopTextEditing();

						// Otherwise, a hit against another instance, or not hit
						// at all, release the hit against the captured instance.
						m_pInputCaptureMovie->OnMouseButtonReleased(
							m_MousePosition,
							m_pInputCaptureInstance,
							false, // Release due to input refresh is never "in instance".
							m_uInputCaptureHitTestMask);

						// Reset input capture to the initial no-capture state.
						InternalClearInputCapture();
					}

					// If we don't have a hit, and we're in "click" input tests, try
					// a drag input test.
					if (!bHit &&
						(m_uInputCaptureHitTestMask == Falcon::kuClickMouseInputHitTest))
					{
						// TODO: Issue twice here is extra cost. Also,
						// preferencing vertical tests is what should be
						// a data driven detail.

						// See if we have a hit with a drag test. Try vertical
						// tests first, then horizontal test, in case the current
						// screen has horizontal scrolling enabled.
						UInt8 uHitTestMode = Falcon::kuVerticalDragMouseInputHitTest;
						bHit = HitTest(uHitTestMode, m_MousePosition, pHitMovie, pHitInstance, pLeafInstance, nullptr);
						if (!bHit)
						{
							uHitTestMode = Falcon::kuHorizontalDragMouseInputHitTest;
							bHit = HitTest(uHitTestMode, m_MousePosition, pHitMovie, pHitInstance, pLeafInstance, nullptr);
						}

						// Sanity check - either bDragHit is false, or pDragHitMovie and pDragHitInstance are not nullptr.
						SEOUL_ASSERT(!bHit || (nullptr != pHitMovie && pHitInstance.IsValid() && pLeafInstance.IsValid()));

					}

					// If we have a hit here, it means we're capturing a new instance.
					if (bHit)
					{
#if SEOUL_LOGGING_ENABLED
						if (Logger::GetSingleton().IsChannelEnabled(Seoul::LoggerChannel::UIDebug))
						{
							String sLeafInstanceFullName;
							pLeafInstance->GatherFullName(sLeafInstanceFullName);
							SEOUL_LOG_UI_DEBUG("UIManager: Mouse down (%i, %i) hit '%s'", m_MousePosition.X, m_MousePosition.Y, sLeafInstanceFullName.CStr());
						}
#endif

						m_pInputCaptureInstance = pHitInstance;
						m_pInputCaptureMovie = pHitMovie;
						m_pInputCaptureLink.Reset();

						if (m_pInputCaptureMovie && m_pInputCaptureInstance.IsValid())
						{
							// Stop editing an active text box.
							StopTextEditing();

							if (pLeafInstance->GetType() == Falcon::InstanceType::kEditText)
							{
								const Vector2D vMousePosition = m_pInputCaptureMovie->GetMousePositionInWorld(m_MousePosition);
								SharedPtr<Falcon::EditTextInstance> pEditTextInstance((Falcon::EditTextInstance*)pLeafInstance.GetPtr());
								SharedPtr<Falcon::EditTextLink> pLink;
								Bool const bLinkHit = pEditTextInstance->LinkHitTest(pLink, vMousePosition.X, vMousePosition.Y);
								if (bLinkHit)
								{
									m_pInputCaptureLink.Reset(pLink.GetPtr());
								}
							}

							m_pInputCaptureMovie->OnMouseButtonPressed(
								m_MousePosition,
								m_pInputCaptureInstance,
								true);
							m_InputCaptureMousePosition = m_MousePosition;
							m_bMouseIsDownOutsideOriginalCaptureInstance = false;
						}
					}

					// Hit has pass through screen and we are sending the associated trigger
					// Note, if our input whitelist is not empty, then we want to
					if (!vPassthroughInputsRet.IsEmpty() && (m_InputWhitelist.IsEmpty() || (m_pInputCaptureMovie && !m_pInputCaptureMovie->StateMachineRespectsInputWhitelist())))
					{
						// Swipe tolerance on Android is only applied if we're in immersive mode.
#if SEOUL_PLATFORM_ANDROID
						Bool bImmersiveMode = false;
						{
							PlatformData platformData;
							Engine::Get()->GetPlatformData(platformData);
							bImmersiveMode = platformData.m_bImmersiveMode;
						}
#endif

						if (!m_pInputCaptureMovie || m_pInputCaptureMovie->AllowClickPassthroughToProceed(
							m_InputCaptureMousePosition,
							m_pInputCaptureInstance))
						{
							for (auto pPassThroughMovie : vPassthroughInputsRet)
							{
#if SEOUL_PLATFORM_ANDROID
								Bool bSwipeStartedAtEdge = false;
								if (pPassThroughMovie)
								{
									auto const vMouseWorldspace = pPassThroughMovie->GetMousePositionInWorld(m_MousePosition);
									auto const vTargetTopLeftWorldspace = pPassThroughMovie->GetMousePositionInWorld(Point2DInt(0, 0));
									auto const backBufferViewport = RenderDevice::Get()->GetBackBufferViewport();
									auto const vTargetBottomRightWorldspace = pPassThroughMovie->GetMousePositionInWorld(Point2DInt(backBufferViewport.m_iTargetWidth, backBufferViewport.m_iTargetHeight));
									bSwipeStartedAtEdge = (
										vMouseWorldspace.Y < vTargetTopLeftWorldspace.Y + kuDeadZonePixelsFromTopOnDrag ||
										vMouseWorldspace.Y > vTargetBottomRightWorldspace.Y - kuDeadZonePixelsFromBottomOnDrag);
								}

								if (!bImmersiveMode || !bSwipeStartedAtEdge)
#endif
								{

									if (!pPassThroughMovie->PassthroughInputTrigger().IsEmpty())
									{
										Get()->TriggerTransition(pPassThroughMovie->PassthroughInputTrigger());
									}
									else
									{
										pPassThroughMovie->InvokePassthroughInputFunction();
									}
								}
							}
						}
					}

					// Finally, dispatch the movie mouse down to all state machines and states.
					for (auto const& e : m_pUIStack->GetStack())
					{
						auto pState(e.m_pMachine->GetActiveState());
						if (pState)
						{
							if (pState->OnGlobalMouseButtonPressed(m_MousePosition, bHit ? pHitInstance : SharedPtr<Falcon::MovieClipInstance>()))
							{
								break;
							}
						}
					}
				}
				// Up event.
				else if (event.m_eButtonEventType == ButtonEventType::ButtonReleased)
				{
					// Handle activation of a captured hyperlink.
					if (bHit &&
						pLeafInstance->GetType() == Falcon::InstanceType::kEditText &&
						m_pInputCaptureLink.IsValid())
					{
						const Vector2D vMousePosition = m_pInputCaptureMovie->GetMousePositionInWorld(m_MousePosition);
						SharedPtr<Falcon::EditTextInstance> pEditTextInstance((Falcon::EditTextInstance*)pLeafInstance.GetPtr());
						SharedPtr<Falcon::EditTextLink> pLink;
						Bool const bLinkHit = pEditTextInstance->LinkHitTest(pLink, vMousePosition.X, vMousePosition.Y);

						// If we hit a link and it's the same link as is currently captured, activate it.
						if (bLinkHit &&
							m_pInputCaptureLink == pLink)
						{
							m_pInputCaptureMovie->OnLinkClicked(
								pLink->m_sLinkString,
								pLink->m_sType,
								m_pInputCaptureInstance);
						}
					}

					// A captured link is always cleared by this point on button release.
					m_pInputCaptureLink.Reset();

					// Only meaningful if we have a captured instance.
					if (m_pInputCaptureInstance.IsValid())
					{
						// Stop editing an active text box.
						StopTextEditing();

						m_pInputCaptureMovie->OnMouseButtonReleased(
							m_MousePosition,
							m_pInputCaptureInstance,
							bHit && pHitInstance == m_pInputCaptureInstance,
							m_uInputCaptureHitTestMask);

						// Reset input capture to the initial no-capture state.
						InternalClearInputCapture();
					}

					// Finally, dispatch the movie mouse up to all state machines and states.
					for (auto const& e : m_pUIStack->GetStack())
					{
						auto pState(e.m_pMachine->GetActiveState());
						if (pState)
						{
							if (pState->OnGlobalMouseButtonReleased(m_MousePosition))
							{
								break;
							}
						}
					}
				}
			}

			// Handle UI special events.
			if (event.m_eEventType == QueuedInputEvent::kEventTypeButton &&
				event.m_eButtonEventType == ButtonEventType::ButtonReleased)
			{
				InputEvent eInputEvent = InputEvent::kUnknown;
				switch (event.m_eButtonID)
				{
				case InputButton::KeySpaceBar:
					// Handled in char handle with active text editing.
					if (!m_pTextEditingInstance.IsValid())
					{
						eInputEvent = InputEvent::kAction;
					}
					break;
				case InputButton::KeyBrowserBack:
					if (m_pTextEditingInstance.IsValid())
					{
						TextEditableStopEditing();
					}
					else
					{
						eInputEvent = InputEvent::kBackButton;
					}
					break;
				case InputButton::KeyEscape:
					if (m_pTextEditingInstance.IsValid())
					{
						TextEditableStopEditing();
					}
					else
					{
						eInputEvent = InputEvent::kBackButton;
					}
					break;
				case InputButton::KeyBackspace:
					if (m_pTextEditingInstance.IsValid())
					{
						if (!m_sTextEditingBuffer.IsEmpty())
						{
							m_sTextEditingBuffer.PopBack();
							XhtmlAwareSetText(m_pTextEditingInstance.GetPtr(), m_sTextEditingBuffer);
						}
					}
					else
					{
						eInputEvent = InputEvent::kBackButton;
					}
					break;
				case InputButton::KeyEnter:
					eInputEvent = InputEvent::kDone;
					break;

				default:
					break;
				};

				// Only send input events when we have an empty white list
				// (no input restrictions) and only when input actions
				// are explicitly enabled.
				Bool bInputWhitelistIsEmpty = true;
				{
					Lock lock(m_InputWhitelistMutex);
					bInputWhitelistIsEmpty = m_InputWhitelist.IsEmpty();
				}

				if (bInputWhitelistIsEmpty &&
					m_bInputActionsEnabled &&
					eInputEvent != InputEvent::kUnknown)
				{
					SendInputEvent(eInputEvent);
				}
			}

			// Any button pressed or released event
			if (event.m_eEventType == QueuedInputEvent::kEventTypeButton)
			{
				// Only send raw button events when we have an empty white list
				// (no input restrictions) and only when input button events
				// are explicitly enabled.
				Bool bInputWhitelistIsEmpty = true;
				{
					Lock lock(m_InputWhitelistMutex);
					bInputWhitelistIsEmpty = m_InputWhitelist.IsEmpty();
				}

				if (bInputWhitelistIsEmpty && m_bInputActionsEnabled)
				{
					SendButtonEvent(event.m_eButtonID, event.m_eButtonEventType);
				}
			}

			// Mouse wheel.
			if (HasMouseWheel())
			{
				if (event.m_eEventType == QueuedInputEvent::kEventTypeAxis &&
					event.m_eAxis == InputAxis::MouseWheel &&
					event.m_fState != 0)
				{
					using namespace Falcon;

					// TODO: Can avoid this additional traversal if we gather
					// multiple hits in a single hit test.

					// Mosue wheel is a bit special - if we have a captured instance,
					// it can only dispatch to that instance (and only if that
					// instance has vertical drag as part of its testing mask).
					//
					// If we do not have a captured instance, then we issue a new hit
					// test with the vertical drag mask and set the event to that
					// instance if found.
					if (m_pInputCaptureInstance.IsValid())
					{
						// Only dispatch if the captured instance supports vertical drag events.
						if (kuVerticalDragMouseInputHitTest == (kuVerticalDragMouseInputHitTest & m_pInputCaptureInstance->GetHitTestSelfMask()))
						{
							m_pInputCaptureMovie->OnMouseWheel(
								m_MousePosition,
								m_pInputCaptureInstance,
								event.m_fState);
						}
					}
					// If we don't have a captured instance, perform a unique test with the vertical
					// drag bit (unless that bit was set with the outer hit test, which is never
					// true right now).
					else
					{
						Bool bMouseWheelHit = false;
						SharedPtr<Falcon::MovieClipInstance> pMouseWheelInstance;
						Movie* pMouseWheelMovie = nullptr;
						if (bHit && kuVerticalDragMouseInputHitTest == (kuVerticalDragMouseInputHitTest & m_uInputCaptureHitTestMask))
						{
							bMouseWheelHit = bHit;
							pMouseWheelMovie = m_pInputCaptureMovie;
							pMouseWheelInstance = m_pInputCaptureInstance;
						}
						else
						{
							SharedPtr<Falcon::Instance> pUnusedLeafInstance;
							bMouseWheelHit = HitTest(
								kuVerticalDragMouseInputHitTest,
								m_MousePosition,
								pMouseWheelMovie,
								pMouseWheelInstance,
								pUnusedLeafInstance,
								nullptr);
						}

						if (bMouseWheelHit)
						{
							pMouseWheelMovie->OnMouseWheel(
								m_MousePosition,
								pMouseWheelInstance,
								event.m_fState);
						}
					}
				}
			}
		}

		// Dispatch over events on changes.
		if (HasMouseCursor()) // Out and over valid only for platforms with visible cursor.
		{
			if (m_pInputOverInstance.IsValid())
			{
				// Potentially dispatch an out if no hit or if
				// we hit a different instance.
				//
				// Different if we hit a different over or hit
				// no over.
				if (!bOverHit || pOverInstance != m_pInputOverInstance)
				{
					// Dispatch out.
					m_pInputOverMovie->OnMouseOut(
						m_MousePosition,
						m_pInputOverInstance);

					// No longer have an over.
					InternalClearInputOver();
				}
			}

			// Potentially fill a new over.
			if (!m_pInputOverInstance.IsValid())
			{
				// If we hit an over, assign the new over.
				if (bOverHit)
				{
					m_pInputOverInstance = pOverInstance;
					m_pInputOverMovie = pOverMovie;
				}

				// Dispatch over.
				if (m_pInputOverInstance.IsValid())
				{
					m_pInputOverMovie->OnMouseOver(
						m_MousePosition,
						m_pInputOverInstance);
				}
			}
		}
	}

	// Stack advance.
	m_pUIStack->Advance(fDeltaTime);
}

/**
 * Handles actually drawing the UI screens - walks the entire UI stack
 * from top to bottom and draws each state (which will draw each movie).
 */
void Manager::InternalPose(RenderPass& rPass)
{
	Int32 const uStateMachines = (Int32)m_pUIStack->GetStack().GetSize();

	// Find the bottom state to render
	Int32 iBottomRenderState = uStateMachines - 1;
	for (Int32 i = 0; i < uStateMachines; ++i)
	{
		CheckedPtr<State> pState = m_pUIStack->GetStack()[i].m_pMachine->GetActiveState();
		if (pState.IsValid())
		{
			if (pState->BlocksRenderBelow())
			{
				iBottomRenderState = i;
				break;
			}
		}
	}

	if (iBottomRenderState < 0)
	{
		return;
	}

	auto const viewport = ComputeViewport();
	m_pRenderer->BeginFrame(viewport);

#if SEOUL_ENABLE_CHEATS
	// If enabled, render input visualization. This is inserted
	// immediately before rendering the first developer-only
	// state machine.
	if (0u != GetInputVisualizationMode())
	{
		Int32 iState = iBottomRenderState;

		// Render all states from the bottom up, stop
		// at the first state that is developer only.
		for (; iState >= 0; --iState)
		{
			// Skip any that aren't at the kAlways filter level.
			if (m_pUIStack->GetStack()[iState].m_eFilter != StackFilter::kAlways)
			{
				break;
			}

			// Render.
			CheckedPtr<State> pState = m_pUIStack->GetStack()[iState].m_pMachine->GetActiveState();
			if (pState.IsValid())
			{
				pState->Pose(rPass, *m_pRenderer);
			}
		}

		// Now pose input visualization.
		if (!IsWaitingForLoads())
		{
			m_pRenderer->BeginInputVisualizationMode();

			Bool inputWhiteListStarted = false;
			HString inputWhiteListBeginState = GetInputWhiteListBeginState();
			if(inputWhiteListBeginState.IsEmpty())
			{
				inputWhiteListStarted = true;
			}
			InputWhitelist emptyList;

			// For input viz., render states top-down (front-to-back).
			auto const& vStack = m_pUIStack->GetStack();
			for (auto i = vStack.Begin(); vStack.End() != i; ++i)
			{
				auto pState = i->m_pMachine->GetActiveState();
				inputWhiteListStarted = inputWhiteListStarted || inputWhiteListBeginState == i->m_pMachine->GetName();

				if (pState)
				{
					// When RenderInputVisualization() returns true, it means a movie
					// in that state blocks all input below it, so we're done.
					Lock lock(m_InputWhitelistMutex);
					if (pState->PoseInputVisualization(
						inputWhiteListStarted ? m_InputWhitelist : emptyList,
						GetInputVisualizationMode(),
						rPass,
						*m_pRenderer))
					{
						break;
					}
				}
			}

			m_pRenderer->EndInputVisualizationMode();
		}

		// Render any remaining states.
		for (; iState >= 0; --iState)
		{
			// Render.
			CheckedPtr<State> pState = m_pUIStack->GetStack()[iState].m_pMachine->GetActiveState();
			if (pState.IsValid())
			{
				pState->Pose(rPass, *m_pRenderer);
			}
		}
	}
	else
#endif // /#if SEOUL_ENABLE_CHEATS
	{
		// Render all states from the bottom up
		for (Int32 i = iBottomRenderState; i >= 0; --i)
		{
			CheckedPtr<State> pState = m_pUIStack->GetStack()[i].m_pMachine->GetActiveState();
			if (pState.IsValid())
			{
				pState->Pose(rPass, *m_pRenderer);
			}
		}
	}

	m_pRenderer->EndFrame(*rPass.GetRenderCommandStreamBuilder(), &rPass);
}

Bool Manager::MovieStateMachineRespectsInputWhiteList(const Movie* pMovie) const
{
	HString stateMachineName = pMovie->GetStateMachineName();
	return stateMachineName.IsEmpty() || StateMachineRespectsInputWhiteList(pMovie->GetStateMachineName());
}

Bool Manager::StateMachineRespectsInputWhiteList(const HString sStateMachineName) const
{
	Bool inputWhiteListStarted = false;
	HString inputWhiteListBeginState = GetInputWhiteListBeginState();
	if(inputWhiteListBeginState.IsEmpty())
	{
		inputWhiteListStarted = true;
	}

	auto const& vStack = m_pUIStack->GetStack();
	for (auto const& state : vStack)
	{
		auto pState = state.m_pMachine->GetActiveState();
		HString machineName = state.m_pMachine->GetName();

		inputWhiteListStarted = inputWhiteListStarted || inputWhiteListBeginState == machineName;
		if (machineName == sStateMachineName)
		{
			return inputWhiteListStarted;
		}
	}

	// default to true.
	return true;
}

/**
 * Special handling around condition variables used to control transition from
 * patching to full game state in a game application.
 *
 * @params[in] bForceImmediate If true, restart is triggered immediately and occurs
 * without delay. Otherwise, may be gated by one or more latching variables
 * that must become false before the restart will be triggered.
 */
void Manager::TriggerRestart(Bool bForceImmediate)
{
	// If immediately, just set kGameLoaded to false.
	if (bForceImmediate)
	{
		SetCondition(kGameLoaded, false);
	}
	// Otherwise, set our pending condition, then evaluate.
	else
	{
		m_bWantsRestart = true;
		InternalEvaluateWantsRestart();
	}
}

void Manager::AddInProgressFCNFile(const FilePath& filePath, const SharedPtr<Falcon::FCNFile>& pFileData)
{
	Lock lock(m_InProgressFCNFileMutex);
	SEOUL_VERIFY(m_tInProgressFCNFiles.Insert(filePath, pFileData).Second);
}

Bool Manager::GetInProgressFCNFile(const FilePath& filePath, SharedPtr<Falcon::FCNFile>& pFileData) const
{
	Lock lock(m_InProgressFCNFileMutex);
	return m_tInProgressFCNFiles.GetValue(filePath, pFileData);
}

void Manager::RemoveInProgressFCNFile(const FilePath& filePath)
{
	Lock lock(m_InProgressFCNFileMutex);
	SEOUL_VERIFY(m_tInProgressFCNFiles.Erase(filePath));
}

/**
 * Calls EvaluateConditions() iteratively until nMaxIterations is hit, or until
 * none of the UI state machines transition as a result of their conditions being evaluated.
 *
 * @return True if this method exited cleanly (all state machines did not transition after
 * a condition evaluation), false otherwise (the max iterations count was hit).
 */
Bool Manager::EvaluateConditionsUntilSettled(Bool& rbStateTransitionActivated, UInt32 nMaxIterations /* = 10u */)
{
	SEOUL_ASSERT(m_bInPrePose);
	SEOUL_ASSERT(IsMainThread());

	UInt32 const uStateMachines = m_pUIStack->GetStack().GetSize();

	UInt32 nIterations = 0u;

	Bool bDone = false;
	while (!bDone && nIterations < nMaxIterations)
	{
		++nIterations;

		// First apply conditions to all state machines.
		ApplyConditionsToStateMachines();

		// Initially done.
		bDone = true;

		// If the UI manager is waiting for loads, stop evaluating
		// conditions.
		if (IsWaitingForLoads())
		{
			continue;
		}

		// Enumerate all active state machines.
		for (UInt32 i = 0u; i < uStateMachines; ++i)
		{
			// Cache the state machine.
			CheckedPtr<Stack::StateMachine> pStateMachine = m_pUIStack->GetStack()[i].m_pMachine;

			// Check for activation - if none, continue to the next state machine.
			HString targetStateIdentifier;
			DataNode activatedTransition;
			UInt32 uTransitionIndex = 0u;
			if (!pStateMachine->CheckConditions(targetStateIdentifier, activatedTransition, uTransitionIndex))
			{
				continue;
			}

			// Pending activation, check for loads first.
			// If CheckAndWaitForLoads() returns true, it means the target state
			// has dependencies that still need to load. Immediately stop further
			// processing of conditions.
			if (InternalCheckAndWaitForLoads(
				pStateMachine,
				targetStateIdentifier))
			{
#if SEOUL_LOGGING_ENABLED
				SEOUL_LOG_STATE("About to change to state, %s, but there is a pending load...", targetStateIdentifier.CStr());
#endif // /#if SEOUL_LOGGING_ENABLED
				return true;
			}

#if SEOUL_LOGGING_ENABLED
			HString const currentStateIdentifier = pStateMachine->GetActiveStateIdentifier();
#endif // /#if SEOUL_LOGGING_ENABLED

			// Now try to activate the transition - a failure here is an error that
			// we report.
			Bool const bSuccess = pStateMachine->GotoState(targetStateIdentifier);

			// Waiting for loads is clear after a state transition attempt,
			// always.
			m_WaitingForLoads.Clear();

			if (!bSuccess)
			{
				SEOUL_WARN("Failed conditional transition from current state '%s' "
					"to target state '%s', check for errors in the configuration of "
					"state machine '%s' or in any movie configurations in gui.json",
					currentStateIdentifier.CStr(),
					targetStateIdentifier.CStr(),
					pStateMachine->GetName().CStr());
				continue;
			}

			// Debug logging.
#if SEOUL_LOGGING_ENABLED
			DebugLogTransitionInfo(
				currentStateIdentifier,
				*pStateMachine,
				activatedTransition,
				uTransitionIndex,
				HString());
#endif // /#if SEOUL_LOGGING_ENABLED

			// Done, post handling after a transition.
			rbStateTransitionActivated = true;
			bDone = false;
			SetConditionsForTransition(
				pStateMachine->GetStateMachineConfiguration(),
				activatedTransition);
		}
	}

	return bDone;
}

/**
 * Process any goto state entries, immediately forcing state machines
 * to the corresponding state.
 */
void Manager::ApplyGotoStates(Bool& rbStateTransitionActivated)
{
	SEOUL_PROF("ApplyGotoStates");
	SEOUL_ASSERT(m_bInPrePose);
	SEOUL_ASSERT(IsMainThread());

	// If the UI manager is waiting for loads, don't evaluate
	// goto state entries.
	if (IsWaitingForLoads())
	{
		return;
	}

	auto const& vMachines = m_pUIStack->GetStack();
	UInt32 const uStateMachines = vMachines.GetSize();

	// Keep processing until we run out of goto state entries.
	PackedUpdate* pGotoState = m_UIGotoStateQueue.Peek();
	while (nullptr != pGotoState)
	{
		// Walk the list of state machines - when we find the one
		// corresponding to the target machine, call GotoState() on it with
		// the target state.
		for (UInt32 i = 0u; i < uStateMachines; ++i)
		{
			auto pMachine = vMachines[i].m_pMachine;
			if (pMachine->GetName() != pGotoState->m_Name)
			{
				continue;
			}

			// Pending activation, check for loads first.
			if (InternalCheckAndWaitForLoads(
				pMachine,
				pGotoState->m_Value))
			{
				// Return with no activation in this case.
				return;
			}

			HString sPreviousState = pMachine->GetActiveStateIdentifier();

			// Activate.
			Bool bSuccess = pMachine->GotoState(pGotoState->m_Value);
			rbStateTransitionActivated = bSuccess || rbStateTransitionActivated;
			if (bSuccess)
			{
				SEOUL_LOG_STATE("ApplyGotoStates: Succeeded going from state \"%s\" to state \"%s\".", sPreviousState.CStr(), pGotoState->m_Value.CStr());
			}
			else
			{
				SEOUL_LOG_STATE("ApplyGotoStates: Failed going from state \"%s\" to state \"%s\".", sPreviousState.CStr(), pGotoState->m_Value.CStr());
			}

			// Break to complete.
			break;
		}

		// Cleanup the entry.
		pGotoState = m_UIGotoStateQueue.Pop();
		SafeDelete(pGotoState);

		// Get the next goto state entry.
		pGotoState = m_UIGotoStateQueue.Peek();
	}
}

/**
 * Process the conditions update queue and apply changes to the state machine
 * conditions state - must be called on the main thread.
 */
void Manager::ApplyConditionsToStateMachines()
{
	// Constant condition used to advertise whether the current hardware
	// meets minimum requirements or not.
	static const HString kMeetsMinimumHardwareRequirements("MeetsMinimumHardwareRequirements");

	SEOUL_ASSERT(IsMainThread());

	UInt32 const uStateMachines = m_pUIStack->GetStack().GetSize();

	// Apply built-in conditions to all state machines. This is done
	// first so the client environment can override these settings in
	// special circumstances (debug prefetching, for example).
	Bool const bMeetsMinimumHardwareRequirements = Engine::Get()->MeetsMinimumHardwareRequirements();
	for (UInt32 i = 0u; i < uStateMachines; ++i)
	{
		m_pUIStack->GetStack()[i].m_pMachine->SetCondition(kMeetsMinimumHardwareRequirements, bMeetsMinimumHardwareRequirements);
	}

	// Apply conditions to all state machines.
	PackedUpdate* pCondition = m_UIConditionQueue.Pop();
	while (nullptr != pCondition)
	{
		// Condition variable is true if the value is "true", otherwise it is false.
		Bool const bValue = (pCondition->m_Value == FalconConstants::kTrue);

		// Apply the condition to all state machines.
		for (UInt32 i = 0u; i < uStateMachines; ++i)
		{
			m_pUIStack->GetStack()[i].m_pMachine->SetCondition(pCondition->m_Name, bValue);
		}

		// Cleanup the entry.
		SafeDelete(pCondition);

		// Get the next condition update.
		pCondition = m_UIConditionQueue.Pop();
	}
}

/**
 * Process the conditions update queue and the UI triggers queue, which
 * will trigger any state transitions that are now fulfilled.
 */
void Manager::ApplyUIConditionsAndTriggersToStateMachines(Bool& rbStateTransitionActivated)
{
	SEOUL_PROF("CondsAndTriggers");
	SEOUL_ASSERT(m_bInPrePose);
	SEOUL_ASSERT(IsMainThread());

	UInt32 const uStateMachines = m_pUIStack->GetStack().GetSize();

	// Next evaluate all state machines - this will activate transitions
	// that do not wait for a trigger and should occur as soon as some conditions
	// are true.
	if (!EvaluateConditionsUntilSettled(rbStateTransitionActivated))
	{
		SEOUL_WARN("UIManager hit the maximum iteration count when evaluating state transition conditions. "
			"This likely indicates an infinite state transition loop, check the state machine graphs for an "
			"infinite loop.");
	}

	// If the UI manager is waiting for loads, don't evaluate
	// triggers.
	if (IsWaitingForLoads())
	{
		return;
	}

	// Now apply each trigger in our internal queue to each state machine one
	// at a time.
	PackedUpdate* pTrigger = m_UITriggerQueue.Peek();
	while (nullptr != pTrigger)
	{
		HString const triggerName = pTrigger->m_Name;

		// First apply conditions to all state machines.
		ApplyConditionsToStateMachines();

		// Fire the trigger for every machine.
		Bool bEvaluateAgain = false;
		for (UInt32 i = 0u; i < uStateMachines; ++i)
		{
			// Cache the state machine.
			CheckedPtr<Stack::StateMachine> pStateMachine = m_pUIStack->GetStack()[i].m_pMachine;

			// First, check if the trigger will activate the current state machine. If not,
			// continue to the next machine.
			HString targetStateIdentifier;
			DataNode activatedTransition;
			UInt32 uTransitionIndex = 0u;
			if (!pStateMachine->CheckTrigger(triggerName, targetStateIdentifier, activatedTransition, uTransitionIndex))
			{
				continue;
			}

			// If CheckAndWaitForLoads() returns true, it means the target state
			// has dependencies that still need to load. Immediately stop further
			// processing of triggers and leave the trigger in the queue for processing
			// again after loading is complete.
			if (InternalCheckAndWaitForLoads(
				pStateMachine,
				targetStateIdentifier))
			{
#if SEOUL_LOGGING_ENABLED
				SEOUL_LOG_STATE("About to change to state, %s, but there is a pending load...", targetStateIdentifier.CStr());
#endif // /#if SEOUL_LOGGING_ENABLED
				return;
			}

#if SEOUL_LOGGING_ENABLED
			HString const currentStateIdentifier = pStateMachine->GetActiveStateIdentifier();
#endif // /#if SEOUL_LOGGING_ENABLED

			// Commit any condition changes of the transition prior to
			// the call to GotoState, so that any calls to GetCondition()
			// from within (e.g.) the state's various OnEnterState() or constructor
			// code see the updated condition state.
			//
			// TODO: If activation fails (this is an unexpected error case),
			// then we should probably restore the conditions to their state
			// prior to this call.
			SetConditionsForTransition(pStateMachine->GetStateMachineConfiguration(), activatedTransition);

			// Activate the trigger - failure here is an error.
			Bool const bActivated = pStateMachine->GotoState(targetStateIdentifier);

			// Waiting for loads is clear after a state transition attempt,
			// always.
			m_WaitingForLoads.Clear();

			if (!bActivated)
			{
				SEOUL_WARN("Trigger '%s' failed transition from current state '%s' "
					"to target state '%s', check for errors in the configuration of "
					"state machine '%s' or in any movie configurations in gui.json",
					currentStateIdentifier.CStr(),
					triggerName.CStr(),
					targetStateIdentifier.CStr(),
					pStateMachine->GetName().CStr());
			}

			// Debug logging.
#if SEOUL_LOGGING_ENABLED
			DebugLogTransitionInfo(
				currentStateIdentifier,
				*pStateMachine,
				activatedTransition,
				uTransitionIndex,
				triggerName);
			AddTriggerHistory(
				triggerName,
				pStateMachine->GetName(),
				currentStateIdentifier,
				targetStateIdentifier);
#endif // /#if SEOUL_LOGGING_ENABLED

			// Track activation.
			bEvaluateAgain = bActivated || bEvaluateAgain;

			if (bActivated)
			{
				// If the transition was activated and if it captures the trigger that activated it, break out of the
				// loop (don't pass the trigger down to state machines below this one).
				if (TransitionCapturesTriggers(
					pStateMachine->GetStateMachineConfiguration(),
					activatedTransition))
				{
					break;
				}
			}
		}

		// If at least one state machine transitioned due to the Trigger,
		// re-evaluate conditions to see if we require another transition.
		if (bEvaluateAgain)
		{
			rbStateTransitionActivated = true;
			if (!EvaluateConditionsUntilSettled(rbStateTransitionActivated))
			{
				SEOUL_WARN("UIManager hit the maximum iteration count when evaluating state transition conditions. "
					"This likely indicates an infinite state transition loop, check the state machine graphs for an "
					"infinite loop.");
			}
		}
		else
		{
			SEOUL_LOG_UI("Trigger %s failed to trigger any active state machines\n", triggerName.CStr());

#if SEOUL_LOGGING_ENABLED
			AddTriggerHistory(triggerName);
#endif // /#if SEOUL_LOGGING_ENABLED

			// Tell the outer world about this triggered transition failure.
			Events::Manager::Get()->TriggerEvent(TriggerFailedToFireTransitionEventId, triggerName);
		}

		// Pop the trigger was just processed now that it has been applied to all machines.
		pTrigger = m_UITriggerQueue.Pop();
		SafeDelete(pTrigger);

		// Peek the next trigger - it will be popped after it has been fully processed.
		pTrigger = m_UITriggerQueue.Peek();
	}
}

#if SEOUL_HOT_LOADING
Bool Manager::ApplyHotReload()
{
	// Only perform a hot reload if requested.
	if (!m_bPendingHotReload)
	{
		return false;
	}

	// Mark that we're in the process of hot reloading and
	// when we're done, that we unmark pending hot reload.
	auto const scoped(MakeScopedAction(
	[&]()
	{
		m_bInHotReload = true;
	},
	[&]()
	{
		m_bInHotReload = false;
		m_bPendingHotReload = false;
	}));

	// Tell the environment we're performing a reload.
	Events::Manager::Get()->TriggerEvent(HotReloadBeginEventId);

	// Dispatch hot load begin.
	{
		auto const& v = m_pUIStack->GetStack();
		for (auto const& e : v)
		{
			auto const& pState(e.m_pMachine->GetActiveState());
			if (!pState)
			{
				continue;
			}

			pState->HotLoadBegin();
		}
	}

	// Need to track restoration states by name, because a stack reconfiguration
	// may include the deletion or addition of state machines.
	HashTable<HString, HString, MemoryBudgets::UIRuntime> tStateRestore;

	// Enumerate state machines and capture state restoration as necessary.

	// Only capture state of state machines with at least one movie
	// that is part of hot loading.
	{
		auto const& v = m_pUIStack->GetStack();
		UInt32 const u = v.GetSize();
		for (UInt32 i = 0u; i < u; ++i)
		{
			// Skip machines without an active state.
			auto p = v[i].m_pMachine;
			if (!p->GetActiveState())
			{
				continue;
			}

			auto pState = p->GetActiveState();

			// Enumerate movies - if at least one wants to hot load, mark the state
			// machine as a hot load target.
			Bool bNeedsReload = false;
			for (auto pMovie = pState->GetMovieStackHead(); pMovie.IsValid(); pMovie = pMovie->GetNextMovie())
			{
				if (pMovie->IsPartOfHotReload())
				{
					bNeedsReload = true;
					break;
				}
			}

			// If true, mark the state machine as a hot reload target.
			if (bNeedsReload)
			{
				SEOUL_VERIFY(tStateRestore.Insert(p->GetName(), p->GetActiveStateIdentifier()).Second);

				// Also need to go to the empty state in this case, since we're not recreating the entire stack.
				SEOUL_VERIFY(p->GotoState(HString()));
			}
		}
	}

	// Only need to reload SWF data on m_bPendingHotReload requests.
	// A stack change alone is only a config change.
	if (m_bPendingHotReload)
	{
		// TODO: Instead, trigger a reload of anything that has remained
		// loaded.
		//
		// Clear movies and fonts to force a reload - we can't assert here,
		// since we allow movies to refuse the hot reload, and that may leave
		// a reference to the movie data here.
		(void)m_FCNFiles.Clear();
		(void)m_UIFonts.Clear();

		// Clear the renderer - make sure any images in the hot loaded SWF
		// files are recached in the Falcon system.
		m_pRenderer->PurgeTextureCache();
	}

	// Finally, restore state.
	{
		auto const& v = m_pUIStack->GetStack();
		for (UInt32 i = 0u; i < v.GetSize(); ++i)
		{
			// Cache the state machine pointer.
			auto p(v[i].m_pMachine);

			HString targetState;
			if (tStateRestore.GetValue(p->GetName(), targetState))
			{
				// This may fail, if the desired target state no longer exists in
				// the reconfigured state machine.
				(void)p->GotoState(targetState);
			}
		}
	}

	// Dispatch hot load end.
	{
		auto const& v = m_pUIStack->GetStack();
		for (auto const& e : v)
		{
			auto const& pState(e.m_pMachine->GetActiveState());
			if (!pState)
			{
				continue;
			}

			pState->HotLoadEnd();
		}
	}

	Events::Manager::Get()->TriggerEvent(HotReloadEndEventId); // Global hook.

	// Flush stashed data.
	m_tHotLoadStash.Clear();

	// A state transition has now occurred, always.
	return true;
}
#endif

HString Manager::GetInputWhiteListBeginState() const
{
	SharedPtr<DataStore> pSettings(GetSettings());
	if (!pSettings.IsValid())
	{
		return HString();
	}

	DataNode value;
	(void)pSettings->GetValueFromTable(pSettings->GetRootNode(), FalconConstants::kInputWhiteListBeginsAtState, value);

	HString out;
	(void)pSettings->AsString(value, out);
	return out;
}

void Manager::SetConditionsForTransition(
	const DataStore& stateMachineDataStore,
	const DataNode& activatedTransition)
{
	if (!activatedTransition.IsNull())
	{
		DataNode ConditionsToSet;
		stateMachineDataStore.GetValueFromTable(activatedTransition, FalconConstants::kModifyConditionsTableEntry, ConditionsToSet);

		auto const iBegin = stateMachineDataStore.TableBegin(ConditionsToSet);
		auto const iEnd = stateMachineDataStore.TableEnd(ConditionsToSet);
		for (auto iter = iBegin; iter != iEnd; ++iter)
		{
			if (iter->Second.IsBoolean())
			{
				SetCondition(iter->First, stateMachineDataStore.AssumeBoolean(iter->Second));
			}
			else
			{
				SEOUL_WARN("Transition that was just fired is defined to modify the condition, %s, but the value defined for it is not a Bool",
					iter->First.CStr());
			}
		}
	}
}

Bool Manager::TransitionCapturesTriggers(const DataStore& stateMachineDataStore, const DataNode& activatedTransition)
{
	if (!activatedTransition.IsNull())
	{
		DataNode captureTriggers;
		Bool bCaptureTriggers = false;

		// Default to true - assume the transition captures triggers if the CaptureTriggers
		// property is not explicitly defined to false.
		if (!stateMachineDataStore.GetValueFromTable(activatedTransition, FalconConstants::kCaptureTriggers, captureTriggers) ||
			(stateMachineDataStore.AsBoolean(captureTriggers, bCaptureTriggers) && bCaptureTriggers))
		{
			return true;
		}
	}

	return false;
}

/**
 * Dispatch input events to all movies in the UI stack.
 */
Bool Manager::HandleMouseMoveEvent(Int iX, Int iY)
{
	// Can't happen during prepose
	SEOUL_ASSERT(!m_bInPrePose);

	// Only valid when called on the main thread.
	SEOUL_ASSERT(IsMainThread());

	// Store the mouse position, this will be dispatched to screens
	// in Pose().
	m_MousePosition.X = iX;
	m_MousePosition.Y = iY;

	return false;
}

/**
 * Queue up input events, these will be dispatched to movies in Pose().
 */
Bool Manager::HandleAxisEvent(InputDevice* pInputDevice, InputDevice::Axis* pAxis)
{
	// Can't happen during prepose
	SEOUL_ASSERT(!m_bInPrePose);

	// Only valid when called on the main thread.
	SEOUL_ASSERT(IsMainThread());

	m_vPendingInputEvents.PushBack(QueuedInputEvent(
		pInputDevice->GetDeviceType(),
		pAxis->GetID(),
		pAxis->GetState()));

	return false;
}

/**
 * Queue up input events, these will be dispatched to movies in Pose().
 */
Bool Manager::HandleButtonEvent(InputDevice* pInputDevice, InputButton eButtonID, ButtonEventType eEventType)
{
	// Can't happen during prepose
	SEOUL_ASSERT(!m_bInPrePose);

	// Only valid when called on the main thread.
	SEOUL_ASSERT(IsMainThread());

	m_vPendingInputEvents.PushBack(QueuedInputEvent(
		pInputDevice->GetDeviceType(),
		eButtonID,
		eEventType));

	return false;
}

Content::Handle<Falcon::CookedTrueTypeFontData> Manager::GetTrueTypeFontData(HString fontName, Bool bBold, Bool bItalic)
{
	using namespace FalconConstants;

	SharedPtr<DataStore> pSettings(GetSettings());
	if (!pSettings.IsValid())
	{
		return Content::Handle<Falcon::CookedTrueTypeFontData>();
	}

	DataNode node;
	(void)pSettings->GetValueFromTable(pSettings->GetRootNode(), kFontAliases, node);
	(void)pSettings->GetValueFromTable(node, HString(LocManager::Get()->GetCurrentLanguage()), node);
	(void)pSettings->GetValueFromTable(node, fontName, node);
	HString const key = (bBold ? kFontBold : (bItalic ? kFontItalic : kFontRegular));
	(void)pSettings->GetValueFromTable(node, key, node);

	HString fontAlias;
	{
		Byte const* s = nullptr;
		UInt32 u = 0u;
		if (pSettings->AsString(node, s, u))
		{
			fontAlias = HString(s, u);
		}
	}

	(void)pSettings->GetValueFromTable(pSettings->GetRootNode(), kFonts, node);
	(void)pSettings->GetValueFromTable(node, fontAlias, node);

	FilePath filePath;
	(void)pSettings->AsFilePath(node, filePath);

	if (!filePath.IsValid())
	{
		return Content::Handle<Falcon::CookedTrueTypeFontData>();
	}

	return m_UIFonts.GetContent(filePath);
}

Bool Manager::GetFontOverrides(HString fontName, Bool bBold, Bool bItalic, Falcon::FontOverrides& rOverrides) const
{
	using namespace FalconConstants;

	SharedPtr<DataStore> pSettings(GetSettings());
	if (!pSettings.IsValid())
	{
		return false;
	}

	DataNode root;
	Bool b = true;
	b = b && pSettings->GetValueFromTable(pSettings->GetRootNode(), kFontAliases, root);
	b = b && pSettings->GetValueFromTable(root, HString(LocManager::Get()->GetCurrentLanguage()), root);
	b = b && pSettings->GetValueFromTable(root, fontName, root);
	HString const key = (bBold ? kFontBold : (bItalic ? kFontItalic : kFontRegular));
	DataNode node;
	b = b && pSettings->GetValueFromTable(root, key, node);

	HString fontAlias;
	b = b && pSettings->AsString(node, fontAlias);

	b = b && pSettings->GetValueFromTable(pSettings->GetRootNode(), kFontSettings, root);
	b = b && pSettings->GetValueFromTable(root, fontAlias, root);

	if (!b)
	{
		return false;
	}

	// Ascent
	if (!pSettings->GetValueFromTable(root, kFontAscent, node) ||
		!pSettings->AsInt32(node, rOverrides.m_iAscentOverride))
	{
		rOverrides.m_iAscentOverride = -1;
	}

	// Descent
	if (!pSettings->GetValueFromTable(root, kFontDescent, node) ||
		!pSettings->AsInt32(node, rOverrides.m_iDescentOverride))
	{
		rOverrides.m_iDescentOverride = -1;
	}

	// LineGap
	if (!pSettings->GetValueFromTable(root, kFontLineGap, node) ||
		!pSettings->AsInt32(node, rOverrides.m_iLineGapOverride))
	{
		rOverrides.m_iLineGapOverride = -1;
	}

	// Rescale
	if (!pSettings->GetValueFromTable(root, kFontRescale, node) ||
		!pSettings->AsFloat32(node, rOverrides.m_fRescale))
	{
		rOverrides.m_fRescale = 1.0f;
	}

	return true;
}

Bool Manager::StartTextEditing(
	Movie* pOwnerMovie,
	SharedPtr<Falcon::MovieClipInstance> pEventReceiver,
	Falcon::EditTextInstance* pInstance,
	const String& sDescription,
	const StringConstraints& constraints,
	Bool bAllowNonLatinKeyboard)
{
	// If the target is the current edit session, just keep editing.
	if (m_pTextEditingInstance == pInstance &&
		m_pTextEditingMovie == pOwnerMovie)
	{
		return false;
	}

	StopTextEditing();

	String const sText(pInstance->GetText().CStr());

	m_pTextEditingEventReceiver = pEventReceiver;
	m_TextEditingConstraints = constraints;
	m_pTextEditingMovie = pOwnerMovie;
	m_pTextEditingInstance.Reset(pInstance);
	m_sTextEditingBuffer = sText;
	Engine::Get()->StartTextEditing(this, sText, sDescription, m_TextEditingConstraints, bAllowNonLatinKeyboard);

	m_pTextEditingMovie->OnEditTextStartEditing(m_pTextEditingEventReceiver);

	return true;
}

void Manager::StopTextEditing()
{
	if (m_pTextEditingInstance.IsValid())
	{
		m_pTextEditingMovie->OnEditTextStopEditing(m_pTextEditingEventReceiver);

		m_pTextEditingInstance->SetHasTextEditFocus(false);
		Engine::Get()->StopTextEditing(this);
		m_sTextEditingBuffer.Clear();
		m_TextEditingConstraints = StringConstraints();
		m_pTextEditingInstance.Reset();
		m_pTextEditingEventReceiver.Reset();
		m_pTextEditingMovie = nullptr;
	}
}

/** Debug only utility for logging ui state info from C# code. */
void Manager::DebugLogEntireUIState() const
{
	SEOUL_ASSERT(IsMainThread());

#if SEOUL_LOGGING_ENABLED
	Seoul::LogMessage(LoggerChannel::UI, "Dumping Entire UI State:");

	auto const& v = m_pUIStack->GetStack();
	for (auto i = v.Begin(); v.End() != i; ++i)
	{
		auto pMachine(i->m_pMachine);
		if (pMachine->GetActiveState().IsValid() &&
			pMachine->GetActiveStateIdentifier() != pMachine->GetDefaultStateIdentifier())
		{
			String rsOutput;
			rsOutput.Append("Stack=");
			rsOutput.Append(pMachine->GetName().CStr());
			rsOutput.Append(", CurrentState=");
			rsOutput.Append(pMachine->GetActiveStateIdentifier().CStr());
			rsOutput.Append(", Screens={ ");
			auto pState = pMachine->GetActiveState();

			for (auto pMovie = pState->GetMovieStackHead(); pMovie.IsValid(); pMovie = pMovie->GetNextMovie())
			{
				rsOutput.Append(String(pMovie->GetMovieTypeName()));
				if (pMovie->GetNextMovie().IsValid())
				{
					rsOutput.Append(", ");
				}
			}

			rsOutput.Append(" }\n");
			Seoul::LogMessage(LoggerChannel::UI, "%s", rsOutput.CStr());
		}
	}
#endif // /#if SEOUL_LOGGING_ENABLED
}

HString Manager::GetStateMachineCurrentStateId(HString sStateMachineName) const
{
	SEOUL_ASSERT(IsMainThread());

	auto const& v = m_pUIStack->GetStack();
	for (auto i = v.Begin(); v.End() != i; ++i)
	{
		auto pMachine(i->m_pMachine);
		if (pMachine->GetName() == sStateMachineName)
		{
			CheckedPtr<State> p = pMachine->GetActiveState();
			if (p.IsValid())
			{
				return p->GetStateIdentifier();
			}

			break;
		}
	}

	return HString();
}

/** Called by a UI::Movie in its DestroyMovie() method to allow for cleanup operations. */
void Manager::DestroyMovie(CheckedPtr<Movie>& rpMovie)
{
	// Always reset.
	auto p = rpMovie;
	rpMovie.Reset();

	// Reset input capture if rMovie is the active capture.
	if (p == m_pInputCaptureMovie)
	{
		InternalClearInputCapture();
	}
	// Reset input over if rMovie is the active over.
	if (p == m_pInputOverMovie)
	{
		InternalClearInputOver();
	}

	// Stop text editing if rMovie is the active text editing movie.
	if (p == m_pTextEditingMovie)
	{
		StopTextEditing();
	}

	// By default, check the movie itself.
	Bool bCanSuspend = p->CanSuspendMovie();

	// If hot loading is enabled and we're in a hot reload,
	// never suspend.
#if SEOUL_HOT_LOADING
	if (m_bInHotReload)
	{
		bCanSuspend = false;
	}
#endif // /#if SEOUL_HOT_LOADING

	// If suspendable, do that now.
	if (!bCanSuspend || !m_WaitingForLoads.SuspendMovie(p))
	{
		// Otherwise, destroy it.
		p->OnDestroyMovie();
		SafeDelete(p);
	}
}

/**
 * Developer only utility. Return a list of points that can be potentially
 * hit based on the input test max. This applies to all state machines and all
 * movies currently active.
 */
void Manager::GetHitPoints(
	UInt8 uInputMask,
	HitPoints& rvHitPoints) const
{
	rvHitPoints.Clear();

	// We step outside "UI space" for this - if the system binding lock
	// is active, we return nothing, as the lock blocks all input.
	if (InputManager::Get()->HasSystemBindingLock())
	{
		return;
	}

	// If the UI manager is waiting for loads, always return
	// an empty set.
	if (IsWaitingForLoads())
	{
		return;
	}

	auto const& v = m_pUIStack->GetStack();
	for (auto const& e : v)
	{
		auto pMachine(e.m_pMachine);
		CheckedPtr<State> p = pMachine->GetActiveState();
		if (!p.IsValid())
		{
			continue;
		}

		if (p->GetHitPoints(
			pMachine->GetName(),
			uInputMask,
			rvHitPoints))
		{
			break;
		}
	}

	// Filter hit points if the input whitelist is not empty.
	// Must be in the whitelist unless the whitelist is empty.
	if (!rvHitPoints.IsEmpty())
	{
		Lock lock(m_InputWhitelistMutex);
		if (!m_InputWhitelist.IsEmpty())
		{
			Int32 iCount = (Int32)rvHitPoints.GetSize();
			for (Int32 i = 0; i < iCount; ++i)
			{
				if (StateMachineRespectsInputWhiteList(rvHitPoints[i].m_StateMachine)
					&& !m_InputWhitelist.HasKey(rvHitPoints[i].m_pInstance))
				{
					Swap(rvHitPoints[i], rvHitPoints[iCount - 1]);
					--iCount;
					--i;
				}
			}

			rvHitPoints.Resize(iCount);
		}
	}
}

Manager::WaitingForLoads::WaitingForLoads()
	: m_pMachine()
	, m_vWaiting()
	, m_uLastConstructFrame(0u)
	, m_bLoading(false)
{
}

Manager::WaitingForLoads::~WaitingForLoads()
{
	Clear();
}

/** Append a new instance to the waiting for loads set. */
void Manager::WaitingForLoads::Add(const WaitingForData& data)
{
	// Always loading now.
	m_bLoading = true;

	// Try to merge - otherwise, push back.
	for (auto& e : m_vWaiting)
	{
		if (e.m_MovieTypeName == data.m_MovieTypeName)
		{
			e.m_hData = data.m_hData;
			return;
		}
	}

	// Add a new entry.
	m_vWaiting.PushBack(data);
}

/** Dispatch broadcast events to any suspended movies. */
Bool Manager::WaitingForLoads::BroadcastEventToSuspended(
	HString sTarget,
	HString sEvent,
	const Reflection::MethodArguments& aArguments,
	Int nArgumentCount) const
{
	Bool bReturn = false;
	for (auto const& e : m_tSuspended)
	{
		if (sTarget.IsEmpty() || sTarget == e.First)
		{
			bReturn = e.Second->OnTryBroadcastEvent(
				sEvent,
				aArguments,
				nArgumentCount) || bReturn;
		}
	}
	return bReturn;
}

/**
 * Immediately clear waitings load - not a part of normal code flow. Meant
 * to be used in UI::Manager::Clear() and similar code paths only.
 */
void Manager::WaitingForLoads::Clear()
{
	// Reset state.
	m_bLoading = false;
	m_pMachine.Reset();

	// Cleanup any instantiated movie instances that
	// weren't consumed.
	for (auto& e : m_vWaiting)
	{
		if (e.m_pMovieData.IsValid())
		{
			e.m_pMovieData->OnDestroyMovie();
			SafeDelete(e.m_pMovieData);
		}
		e.m_hData.Reset();
	}

	// Done.
	m_vWaiting.Clear();
}

/**
 * Immediately clear any movies from the suspended
 * table - destroys the instances.
 */
void Manager::WaitingForLoads::ClearSuspended()
{
	for (auto const& e : m_tSuspended)
	{
		e.Second->OnDestroyMovie();
		SafeDelete(e.Second);
	}
	m_tSuspended.Clear();
}

/**
 * Attempt to create a new movie instance by consuming it from
 * the pre-fetched set. If this fails, a fresh instance will be
 * created and constructed.
 */
CheckedPtr<Movie> Manager::WaitingForLoads::Instantiate(HString typeName)
{
	// Search for and consume already instantiated data,
	// if available.
	for (auto i = m_vWaiting.Begin(); m_vWaiting.End() != i; ++i)
	{
		if (i->m_MovieTypeName == typeName)
		{
			auto pReturn = i->m_pMovieData;
			i = m_vWaiting.Erase(i);
			return pReturn;
		}
	}

	// Fallback to a fresh instantiate.
	return NewMovie(typeName);
}

/** Fresh instantiation and construction of a UI::Movie instance. */
CheckedPtr<Movie> Manager::WaitingForLoads::NewMovie(HString movieTypeName)
{
	// Check for a suspended movie - if available, reuse it.
	auto pMovie = ResumeMovie(movieTypeName);
	if (pMovie.IsValid())
	{
		return pMovie;
	}

	// Check whether the movie is "native" (handled by the reflection system) or not.
	// if the movie is not native, it must be instantiated using the custom instantiator.
	Bool const bNativeInstantiator = Manager::Get()->IsNativeMovie(movieTypeName);

	// Native movie, instantiate with reflection.
	if (bNativeInstantiator)
	{
		Reflection::Type const* pType = Reflection::Registry::GetRegistry().GetType(movieTypeName);
		if (nullptr != pType)
		{
			pMovie = pType->New<Movie>(MemoryBudgets::UIRuntime);
		}
	}
	// Otherwise, instantiate with the custom instantiator.
	else
	{
		Manager::CustomUIMovieInstantiator instantiator = Manager::Get()->GetCustomUIMovieInstantiator();
		if (instantiator)
		{
			pMovie = instantiator(movieTypeName);
		}
	}

	// If the movie was successfully created, construct it.
	if (pMovie.IsValid())
	{
		// Need to do this inline here since OnConstructMovie
		// is otherwise what sets up the cached HString variables.
		SEOUL_PROF_VAR(HString(String(movieTypeName) + ".OnConstructMovie"));

		// Give the movie a chance to construct.
		pMovie->ConstructMovie(movieTypeName);
	}

	if (!pMovie.IsValid())
	{
		SEOUL_WARN("%s: could not instantiate movieTypeName: %s",
			__FUNCTION__, movieTypeName.CStr());
	}

	return pMovie;
}

/** Per-frame maintenance work of the waiting-for-loads set. */
void Manager::WaitingForLoads::Process()
{
	auto pFirstMovieCurrent = (m_pMachine.IsValid() && m_pMachine->GetActiveState().IsValid()
		? m_pMachine->GetActiveState()->GetMovieStackHead()
		: nullptr);
	for (auto& e : m_vWaiting)
	{
		// Early out if not loading yet.
		if (e.m_hData.IsLoading())
		{
			return;
		}

		// If we have an instance already, done.
		if (e.m_pMovieData.IsValid())
		{
			continue;
		}

		// Search the machine's active state for any existing instance - these do not need to be
		// created.
		auto const movieTypeName = e.m_MovieTypeName;
		Bool bDone = false;
		for (auto p = pFirstMovieCurrent; p.IsValid(); p = p->GetNextMovie())
		{
			if (p->GetMovieTypeName() == movieTypeName)
			{
				bDone = true;
				break;
			}
		}

		if (bDone)
		{
			continue;
		}

		// TODO: Replace usage of FrameCount here
		// with a general purpose time slicing system in UIManager.

		// Need to instantiate the movie -- immediately return false if we can't yet do so.
		auto const uFrameCount = Engine::Get()->GetFrameCount();
		if (uFrameCount == m_uLastConstructFrame)
		{
			return;
		}

		// Now a construction frame.
		m_uLastConstructFrame = uFrameCount;

		// Make a new instance of the movie.
		e.m_pMovieData = NewMovie(movieTypeName);
	}

	// Successfully created all movie instances.
	m_bLoading = false;
}

CheckedPtr<Movie> Manager::WaitingForLoads::ResumeMovie(HString movieTypeName)
{
	CheckedPtr<Movie> pReturn = nullptr;
	if (m_tSuspended.GetValue(movieTypeName, pReturn))
	{
		SEOUL_VERIFY(m_tSuspended.Erase(movieTypeName));
		pReturn->OnResumeMovie();
		return pReturn;
	}

	return pReturn;
}

Bool Manager::WaitingForLoads::SuspendMovie(CheckedPtr<Movie> p)
{
	// Insert - on failure, return false.
	auto const typeName = p->GetMovieTypeName();
	if (!m_tSuspended.Insert(typeName, p).Second)
	{
		return false;
	}

	// Otherwise, suspend.
	p->OnSuspendMovie();
	return true;
}

Bool Manager::HitTest(
	UInt8 uMask,
	const Point2DInt& mousePosition,
	Movie*& rpHitMovie,
	SharedPtr<Falcon::MovieClipInstance>& rpHitInstance,
	SharedPtr<Falcon::Instance>& rpLeafInstance,
	Vector<Movie*>* rvPassthroughInputs) const
{
	if (nullptr != rvPassthroughInputs)
	{
		rvPassthroughInputs->Clear();
	}

	// If the UI manager is waiting for loads, HitTest()
	// always returns false.
	if (IsWaitingForLoads())
	{
		return false;
	}

	// only block input starting at specific State.
	Bool inputWhiteListStarted = false;
	HString inputWhiteListBeginState = GetInputWhiteListBeginState();
	if(inputWhiteListBeginState.IsEmpty())
	{
		inputWhiteListStarted = true;
	}
	auto const& v = m_pUIStack->GetStack();
	for (auto const& e : v)
	{
		auto pMachine(e.m_pMachine);
		CheckedPtr<State> p = pMachine->GetActiveState();
		inputWhiteListStarted = inputWhiteListStarted || inputWhiteListBeginState == pMachine->GetName();

		if (p.IsValid())
		{
			MovieHitTestResult const eResult = p->HitTest(
				uMask,
				mousePosition,
				rpHitMovie,
				rpHitInstance,
				rpLeafInstance,
				rvPassthroughInputs);

			if (MovieHitTestResult::kHit == eResult)
			{
				// Must be in the whitelist unless the whitelist is empty.
				{
					Lock lock(m_InputWhitelistMutex);
					if (inputWhiteListStarted && !m_InputWhitelist.IsEmpty())
					{
						if (!m_InputWhitelist.HasKey(rpHitInstance))
						{
							// Cleanup and return false.
							rpHitMovie = nullptr;
							rpHitInstance.Reset();
							rpLeafInstance.Reset();
							if (nullptr != rvPassthroughInputs) { rvPassthroughInputs->Clear(); }
							return false;
						}
					}
				}

				return true;
			}
			else if (MovieHitTestResult::kNoHitStopTesting == eResult)
			{
				return false;
			}
			else if (MovieHitTestResult::kNoHitTriggerBack == eResult)
			{
				return false;
			}
			// Otherwise, keep testing.
		}
	}

	return false;
}

Bool Manager::SendInputEvent(InputEvent eInputEvent)
{
	// If the UI manager is waiting for loads, SendInputEvent()
	// always returns false.
	if (IsWaitingForLoads())
	{
		return false;
	}

	auto const& v = m_pUIStack->GetStack();
	for (auto i = v.Begin(); v.End() != i; ++i)
	{
		auto pMachine(i->m_pMachine);
		CheckedPtr<State> p = pMachine->GetActiveState();
		if (p.IsValid())
		{
			MovieHitTestResult const eResult = p->SendInputEvent(eInputEvent);

			if (MovieHitTestResult::kHit == eResult)
			{
				return true;
			}
			else if (MovieHitTestResult::kNoHitStopTesting == eResult)
			{
				return false;
			}
			// Otherwise, keep testing.
		}
	}

	return false;
}

Bool Manager::SendButtonEvent(Seoul::InputButton eButtonID, Seoul::ButtonEventType eButtonEventType)
{
	// If the UI manager is waiting for loads, SendButtonEvent()
	// always returns false.
	if (IsWaitingForLoads())
	{
		return false;
	}

	auto const& v = m_pUIStack->GetStack();
	for (auto i = v.Begin(); v.End() != i; ++i)
	{
		auto pMachine(i->m_pMachine);
		CheckedPtr<State> p = pMachine->GetActiveState();
		if (p.IsValid())
		{
			MovieHitTestResult const eResult = p->SendButtonEvent(eButtonID, eButtonEventType);

			if (MovieHitTestResult::kHit == eResult)
			{
				return true;
			}
			else if (MovieHitTestResult::kNoHitStopTesting == eResult)
			{
				return false;
			}
			// Otherwise, keep testing.
		}
	}

	return false;
}

// ITextEditable overrides
void Manager::TextEditableApplyChar(UniChar c)
{
	if (m_pTextEditingInstance.IsValid())
	{
		// We rely on constraints to strip characters that can't be printed.
		m_sTextEditingBuffer.Append(c);
		ITextEditable::TextEditableApplyConstraints(m_TextEditingConstraints, m_sTextEditingBuffer);
		XhtmlAwareSetText(m_pTextEditingInstance.GetPtr(), m_sTextEditingBuffer);
	}
}

void Manager::TextEditableApplyText(const String& sText)
{
	if (m_pTextEditingInstance.IsValid())
	{
		m_sTextEditingBuffer = sText;
		XhtmlAwareSetText(m_pTextEditingInstance.GetPtr(), m_sTextEditingBuffer);

		m_pTextEditingMovie->OnEditTextApply(m_pTextEditingEventReceiver);
	}
}

void Manager::TextEditableEnableCursor()
{
	if (m_pTextEditingInstance.IsValid())
	{
		m_pTextEditingInstance->SetHasTextEditFocus(true);
	}
}

void Manager::TextEditableStopEditing()
{
	StopTextEditing();
}

// /ITextEditable

} // namespace UI

} // namespace Seoul
