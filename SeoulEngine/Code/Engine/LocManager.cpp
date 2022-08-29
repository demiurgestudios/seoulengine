/**
 * \file LocManager.cpp
 * \brief Implements the localized string manager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "DataStoreParser.h"
#include "Engine.h"
#include "FileManager.h"
#include "EventsManager.h"
#include "HtmlReader.h"
#include "LocManager.h"
#include "Logger.h"
#include "Platform.h"
#include "Prereqs.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDataStoreTableUtil.h"
#include "SeoulTypes.h"
#include "SettingsManager.h"
#include "ThreadId.h"

namespace Seoul
{

/** Constants used to configure LocManager from loc.json. */
static const HString ksAutoLocalizeTextFieldPrefix("AutoLocalizeTextFieldPrefix");
static const HString ksDefaultLanguage("DefaultLanguage");
static const HString ksLocalization("Localization");
static const HString ksSupportedLanguages("SupportedLanguages");

static const int kMinNumtoformat = 10000;

static const String kasLocaleFilenames[] =
{
	String("locale.json"),
	String("locale_patch.json"),
};

#if !SEOUL_SHIP
/** Utility, checks for reasonable characters in a loc token, to help identify malformed loc tokens (misuse of the Localize() API). */
static inline Bool IsProperlyFormedLocToken(Byte const* sToken, UInt32 zTokenSizeInBytes)
{
	for (UInt32 i = 0u; i < zTokenSizeInBytes; ++i)
	{
		Byte const c = sToken[i];
		if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || c == '_' || c == '-'))
		{
			return false;
		}
	}

	return true;
}
#endif // /#if !SEOUL_SHIP

/**
 * Flag controlling usage of a fallback language when a loc token does not have
 * a string.  If set to 1, an English string will be used if a translated string
 * does not exist; if set to 0, the raw loc token will be used instead, allowing
 * for easier identification of untranslated strings.
 */
#define SEOUL_USE_DEFAULT_FALLBACK_LANGUAGE SEOUL_SHIP

const HString LocManager::ksThousandsSeparatorLocToken("ThousandsSeparator");

const String LocManager::ksLanguageEnglish("English");
const String LocManager::ksLanguageFrench("French");
const String LocManager::ksLanguageGerman("German");
const String LocManager::ksLanguageItalian("Italian");
const String LocManager::ksLanguageJapanese("Japanese");
const String LocManager::ksLanguageKorean("Korean");
const String LocManager::ksLanguageSpanish("Spanish");
const String LocManager::ksLanguageRussian("Russian");
const String LocManager::ksLanguagePortuguese("Portuguese");

/**
 * Constructor sets language to English and prepares to read real config and
 * string data from json files.
 */
LocManager::LocManager()
	: m_Mutex()
#if SEOUL_ENABLE_CHEATS
	, m_eDebugPlatform(keCurrentPlatform)
	, m_bDebugOnlyShowTokens(false)
#endif
	, m_JsonFilePath(FilePath::CreateConfigFilePath("loc.json"))
	, m_StringDataStore()
	, m_aDefaultLocaleFilePaths()
	, m_aLocaleFilePaths()
	, m_bLocalFileOpen(false)
	, m_aDefaultPlatformOverrideFilePaths()
	, m_aPlatformOverrideFilePaths()
	, m_bUsePlatformOverride(false)
	, m_sDefaultLanguage(ksLanguageEnglish)
	, m_sCurrentLanguage(m_sDefaultLanguage)
#if SEOUL_USE_DEFAULT_FALLBACK_LANGUAGE
	, m_bUseDefaultFallbackLanguage(true)
#else
	, m_bUseDefaultFallbackLanguage(false)
#endif
{
	// Json tells us allowed and default languages
	{
		Lock lock(m_Mutex);
		InsideLockReadConfigFromJson();
	}

	// Select the language that the system is running
	SetLanguageFromSystem();
}

/**
 * Destructor writes string usage report when shutting down.  See
 * LocManager::DebugWriteReport for details and caveats.
 */
LocManager::~LocManager()
{
#if !SEOUL_SHIP
	{
		Lock lock(m_Mutex);
		InsideLockDebugWriteMissingLocReport();
	}
#endif
}

#if SEOUL_HOT_LOADING
void LocManager::RegisterForHotLoading()
{
	SEOUL_ASSERT(IsMainThread());

	// Register for appropriate callbacks with ContentLoadManager.
	Events::Manager::Get()->RegisterCallback(
		Content::FileChangeEventId,
		SEOUL_BIND_DELEGATE(&LocManager::OnFileChange, this));
	// Make sure we're first in line for the file change event,
	// so we come before the Content::Store that will actually handle the
	// change event.
	Events::Manager::Get()->MoveLastCallbackToFirst(Content::FileChangeEventId);

	Events::Manager::Get()->RegisterCallback(
		Content::FileIsLoadedEventId,
		SEOUL_BIND_DELEGATE(&LocManager::OnIsFileLoaded, this));

	Events::Manager::Get()->RegisterCallback(
		Content::FileLoadCompleteEventId,
		SEOUL_BIND_DELEGATE(&LocManager::OnFileLoadComplete, this));
}

void LocManager::UnregisterFromHotLoading()
{
	SEOUL_ASSERT(IsMainThread());

	Events::Manager::Get()->UnregisterCallback(
		Content::FileLoadCompleteEventId,
		SEOUL_BIND_DELEGATE(&LocManager::OnFileLoadComplete, this));

	Events::Manager::Get()->UnregisterCallback(
		Content::FileIsLoadedEventId,
		SEOUL_BIND_DELEGATE(&LocManager::OnIsFileLoaded, this));

	Events::Manager::Get()->UnregisterCallback(
		Content::FileChangeEventId,
		SEOUL_BIND_DELEGATE(&LocManager::OnFileChange, this));
}
#endif // /#if SEOUL_HOT_LOADING

/**
 * Retrieve the string for the current language that corresponds to the
 * specified token.
 */
String LocManager::Localize(HString token)
{
	// Special case handling for the empty token.
	if (token.IsEmpty())
	{
		return String();
	}

	Lock lock(m_Mutex);

	// Try to localize string
	String sLocalized;
	DataNode value;
	if (m_StringDataStore.GetValueFromTable(m_StringDataStore.GetRootNode(), token, value) &&
		m_StringDataStore.AsString(value, sLocalized))
	{
#if SEOUL_ENABLE_CHEATS
		if (m_bDebugOnlyShowTokens)
		{
			return token.CStr();
		}
#endif // /SEOUL_ENABLE_CHEATS
		return sLocalized;
	}

#if !SEOUL_SHIP
	// Don't track the token as missing if it is in the default
	// fallback, since we'll use that in Ship builds instead.
	// Lots of tokens are default only.
	if (!m_DefaultFallback.TableContainsKey(m_DefaultFallback.GetRootNode(), token))
	{
		// Track which strings are not localized.
		(void)m_TokensNotLocalized.Insert(String(token));
	}
#endif // /!SEOUL_SHIP

	// This is really an error. at this point we don't have a string in the
	// current language that corresponds to the specified token.  "For now" it
	// seems fair to show the original token without modification. alternatives
	// include changing the text to indicate that the token has no matching
	// string, changing the presentation of the string... or use scaleform to
	// make unicorns fly out on rainbows to drop piles of crap on the text.
	return token.CStr();
}

String LocManager::Localize(Byte const* sToken, UInt32 zTokenLengthInBytes)
{
	// In this variation, we use HString::Get(), instead of constructing
	// an HString, since we know all valid cases will already be in our
	// loc table and will already be defined as valid HStrings. This allows
	// us to avoid creating suprious HStrings.
	HString token;
	if (!HString::Get(token, sToken, zTokenLengthInBytes))
	{
		// Non-ship builds, add the token to our referenced-but-not localized
		// list, and also sanity check the token. If it appears malformed (e.g.
		// it contains spaces or other weird characters we wouldn't expect in
		// a loc token), issue a WAN_CACHED() against the token, to help identify
		// API misuse as quickly as possible.
#if !SEOUL_SHIP
		{
			Lock lock(m_Mutex);

			// Track which strings are not localized.
			(void)m_TokensNotLocalized.Insert(String(sToken, zTokenLengthInBytes));
		}

		if (!IsProperlyFormedLocToken(sToken, zTokenLengthInBytes))
		{
			SEOUL_WARN("LocManager::Localize() called with token \"%s\", which appears "
				"to be malformed (it contains characters not expected in a loc token).",
				sToken);
		}
#endif // /!SEOUL_SHIP

		// Return the string itself as the localization in this case.
		return String(sToken, zTokenLengthInBytes);
	}

	return Localize(token);
}

/**
 * Helper function to allow code in the Core project to localize strings via
 * the g_pCoreVirtuals method table
 */
String LocManager::CoreLocalize(HString sLocToken, const String& sDefaultValue)
{
	if (LocManager::Get())
	{
		return LocManager::Get()->Localize(sLocToken);
	}
	else
	{
		return sDefaultValue;
	}
}

Bool LocManager::IsValidToken(HString token) const
{
	Lock lock(m_Mutex);

	DataNode value;
	return m_StringDataStore.GetValueFromTable(m_StringDataStore.GetRootNode(), token, value);
}

Bool LocManager::IsValidToken(Byte const* sToken, UInt32 zTokenLengthInBytes) const
{
	HString token;
	if (!HString::Get(token, sToken, zTokenLengthInBytes))
	{
		return false;
	}

	return IsValidToken(token);
}

/**
 * Return the singular or plural version of a string based on quantity.
 * Note that this only works for some subset of Romance languages. Ask a
 * trained linguist for the correct answer, but I assume you want the plural
 * version of a string for zero quantity. For example:
 *
 * + I have 0 functioning brain cells.
 * + I have 1 functioning brain cell.
 * + I have 2 functioning brain cells. (It's an example, not an assertion.)
 *
 * In this case we'd have a token like brain_cell_singular="brain cell" and
 * a token like brain_cell_plural="brain cells". We'd switch between them
 * based on quantity.
 *
 * I'm guessing that you'll need to add special rules here to support SOV
 * languages but maybe I'm wrong. Again, ask a linguist and/or someone with
 * more functioning brain cells for the right answer.
 *
 * @param[in] uNumberOfThings	quantity of noun being localized
 * @param[in] hTokenSingular	token to use if there is one thing
 * @param[in] hTokenPlural		token to use if there are multiple things
 *
 * @return localized string corresponding to quantity of things
 */
String LocManager::LocalizeByQuantity(UInt32 uNumberOfThings, const HString& hTokenSingular, const HString& hTokenPlural)
{
	if (uNumberOfThings == 1)
	{
		return Localize(hTokenSingular);
	}
	else
	{
		return Localize(hTokenPlural);
	}
}

/**
 * Getter for current language.
 */
String LocManager::GetCurrentLanguage() const
{
	String sReturn;
	{
		Lock lock(m_Mutex);
		sReturn = m_sCurrentLanguage;
	}
	return sReturn;
}

/**
 * Getter for the current language code (lowercase 2-letter ISO 639 language
 * code)
 */
String LocManager::GetCurrentLanguageCode() const
{
	return GetLanguageCode(GetCurrentLanguage());
}

/**
 * Setter for current language.
 * @param[lang] English name of language to switch to, see m_vSupportedLanguages
 * @return true if we switched to specified language
 */
Bool LocManager::SetLanguage(const String& lang)
{
	Lock lock(m_Mutex);

	// Bail out if the specified lang isn't valid
	if (!m_vSupportedLanguages.Contains(lang))
	{
		return false;
	}

	// Write debug report if appropriate
#if !SEOUL_SHIP
	InsideLockDebugWriteMissingLocReport();
#endif

	// Set new language
	m_sCurrentLanguage = lang;

	InsideLockInit();

	return true;
}

/**
 * Determine the system language and switch to it. You need to update this
 * method to support different flavors of the same language, e.g. Spanish in
 * Mexico vs Spanish in Spain. I suggest using the combination of language and
 * country names; see the disabled code below.
 */
void LocManager::SetLanguageFromSystem()
{
	if (!SetLanguage(Engine::Get()->GetSystemLanguage()))
	{
		String sDefaultLanguage;
		{
			Lock lock(m_Mutex);
			sDefaultLanguage = m_sDefaultLanguage;
		}

		// If the system is running an unsupported language, fall back on
		// the default language
		SEOUL_VERIFY(SetLanguage(sDefaultLanguage));
	}
}

/**
 * Formats the given number according to the current locale, using the
 * appropriate thousands separator
 */
String LocManager::FormatNumber(Double dNumber, Int decimals)
{
	//TODO: We need to consider some locales don't format decimals and delimiters in these locations.

	Double intPart;
	Double fractpart = modf(dNumber, &intPart);

	// Don't add the separator to numbers with less digits than kMinNumtoformat.
	if (dNumber < kMinNumtoformat && dNumber > -kMinNumtoformat)
	{
		if (decimals == 0)
		{
			return String::Printf("%" PRId64, (Int64)dNumber);
		}
		return String::Printf("%.*f", decimals, dNumber);
	}

	String sThousandsSeparator = Localize(ksThousandsSeparatorLocToken);

	Bool bNegative = false;
	if (dNumber < 0)
	{
		bNegative = true;
		dNumber = -dNumber;
	}

	// No need to preallocate the result, since most strings will be less
	// than the 15 characters which can be stored in the internal stack
	// buffer of the String object.
	String sResult;


	Int64 intPartWhole = (Int64)dNumber;
	// Construct the result backwards and then reverse it at the end
	int nDigit = 0;
	while (intPartWhole > 0)
	{
		if (nDigit >= 3 && nDigit % 3 == 0)
		{
			sResult.Append(sThousandsSeparator);
		}

		sResult.Append('0' + (intPartWhole % 10));
		intPartWhole /= 10;
		nDigit++;
	}

	if (bNegative)
	{
		sResult.Append('-');
	}

	// reverse it at the end
	sResult = sResult.Reverse();


	// if we had a fraction, throw it on at the end.
	if (decimals > 0)
	{
		auto const sFraction(String::Printf("%.*f", decimals, fractpart));
		auto const uFraction = sFraction.Find('.');
		sResult.Append(String::NPos == uFraction ? sFraction : sFraction.Substring(uFraction));
	}

	return sResult;
}

/**
 * Formats the given number according to the current locale, using the
 * appropriate thousands separator
 */
String LocManager::FormatNumber(Int iValue)
{
	Double dValue = (Double)iValue;
	return FormatNumber(dValue, 0);
}

#if SEOUL_HOT_LOADING
void LocManager::OnFileChange(Content::ChangeEvent* pFileChangeEvent)
{
	// Don't insert entries if hot loading is suppressed.
	if (Content::LoadManager::Get()->IsHotLoadingSuppressed())
	{
		return;
	}

	// If the changed file has been run in this VM, schedule it to rerun.
	FilePath const filePath = pFileChangeEvent->m_New;
	if (IsLocManagerFilePath(filePath))
	{
		Lock lock(m_Mutex);
		(void)m_tHotLoading.Insert(filePath, SettingsManager::Get()->GetSettings(filePath));
	}
}

Bool LocManager::OnIsFileLoaded(FilePath filePath)
{
	return IsLocManagerFilePath(filePath);
}

void LocManager::OnFileLoadComplete(FilePath filePath)
{
	Bool bReInit = false;
	{
		Lock lock(m_Mutex);
		if (m_tHotLoading.Erase(filePath))
		{
			if (m_tHotLoading.IsEmpty())
			{
				bReInit = true;
			}
		}
	}

	if (bReInit)
	{
		ReInit();
	}
}
#endif // /#if SEOUL_HOT_LOADING

/**
 * Read loc.json and apply contained configuration.
 */
void LocManager::InsideLockReadConfigFromJson()
{
	// Read list of supported languages
	DataStore dataStore;
	if (!DataStoreParser::FromFile(m_JsonFilePath, dataStore, DataStoreParserFlags::kLogParseErrors))
	{
		SEOUL_WARN("loc.json not found or invalid.");

		m_sDefaultLanguage = ksLanguageEnglish;
		m_vSupportedLanguages.PushBack(m_sDefaultLanguage);
		return;
	}

	(void)m_LocManagerFiles.Insert(m_JsonFilePath);

	DataStoreTableUtil locSection(dataStore, ksLocalization);

	// Get default language.  We assume English if the default isn't specified
	// in the json file.
	if (!locSection.GetValue(ksDefaultLanguage, m_sDefaultLanguage))
	{
		m_sDefaultLanguage = ksLanguageEnglish;
	}

	// Get list of supported languages
	if (!locSection.GetValue(ksSupportedLanguages, m_vSupportedLanguages))
	{
		// Place the default language into the list of supported languages if
		// we couldn't read the SupportedLanguages.
		m_vSupportedLanguages.PushBack(m_sDefaultLanguage);
	}
}

/**
 * Select json files to read from based on current language and platform.  Flush
 * the string table and repopulate it.
 */
void LocManager::InsideLockInit()
{
	// Set paths to the main locale (string definition) file and the English
	// locale file
	for (UInt32 i = 0u; i < kuLocaleFiles; ++i)
	{
		m_aDefaultLocaleFilePaths[i] = FilePath::CreateConfigFilePath(Path::Combine("Loc", m_sDefaultLanguage, kasLocaleFilenames[i]));
		m_aLocaleFilePaths[i] = FilePath::CreateConfigFilePath(Path::Combine("Loc", m_sCurrentLanguage, kasLocaleFilenames[i]));

		(void)m_LocManagerFiles.Insert(m_aDefaultLocaleFilePaths[i]);
		(void)m_LocManagerFiles.Insert(m_aLocaleFilePaths[i]);
	}

	// Apply platform-specific overrides
#if SEOUL_ENABLE_CHEATS
	String sPlatformOverride(kaPlatformNames[(UInt32)(Platform)m_eDebugPlatform]);
#else
	String sPlatformOverride(GetCurrentPlatformName());
#endif

	// Set path to platform-specific locale (string definition) file
	if (!sPlatformOverride.IsEmpty())
	{
		m_bUsePlatformOverride = true;
		for (UInt32 i = 0u; i < kuLocaleFiles; ++i)
		{
			m_aDefaultPlatformOverrideFilePaths[i] = FilePath::CreateConfigFilePath(
				Path::Combine("Loc", m_sDefaultLanguage, sPlatformOverride, kasLocaleFilenames[i]));
			m_aPlatformOverrideFilePaths[i] = FilePath::CreateConfigFilePath(
				Path::Combine("Loc", m_sCurrentLanguage, sPlatformOverride, kasLocaleFilenames[i]));
			(void)m_LocManagerFiles.Insert(m_aDefaultPlatformOverrideFilePaths[i]);
			(void)m_LocManagerFiles.Insert(m_aPlatformOverrideFilePaths[i]);
		}
	}
	else
	{
		m_bUsePlatformOverride = false;
	}

	InsideLockRefreshStrings();
}

/**
 * Apply the string table in rStringDataStore based on the current language and
 * (optional) platformOverrideFilePath.
 */
void LocManager::StaticApplyStringsAsPatch(
	const DataStore& inDataStore,
	DataStore& rOutDataStore)
{
	auto const stringTableNode = inDataStore.GetRootNode();
	auto stringIter = inDataStore.TableBegin(stringTableNode);
	auto const stringEnd = inDataStore.TableEnd(stringTableNode);
	auto const rootNode = rOutDataStore.GetRootNode();
	for ( ; stringIter != stringEnd; ++stringIter)
	{
		// Check for an explicit null value - in this case, delete the existing entry. We
		// use "Token": null in our loc patch files to mean "erase entry".
		if (stringIter->Second.IsNull() || stringIter->Second.IsSpecialErase())
		{
			(void)rOutDataStore.EraseValueFromTable(rootNode, stringIter->First);
			continue;
		}

		// Get the string value
		Byte const* sValue = nullptr;
		UInt32 zLength = 0;
		if (!inDataStore.AsString(stringIter->Second, sValue, zLength))
		{
			SEOUL_WARN("[LocManager]: Value of \"%s\" is not a string", stringIter->First.CStr());
			continue;
		}

		// Save the new string value into our data store
		if (!rOutDataStore.SetStringToTable(rootNode, stringIter->First, sValue, zLength))
		{
			SEOUL_WARN("[LocManager]: Failed to set string value for \"%s\"", stringIter->First.CStr());
			continue;
		}
	}

}

/**
 * Reload the string table rStringDataStore based on the current language and
 * (optional) platformOverrideFilePath.
 */
void LocManager::StaticRefreshStrings(
	FilePath localeFilePath,
	FilePath platformOverrideFilePath,
	DataStore& rStringDataStore)
{
	{
		{
			// Wipe table if not already blank
			DataStore emptyDataStore;
			rStringDataStore.Swap(emptyDataStore);
		}

		// Parse the base .json file
		if (!DataStoreParser::FromFile(localeFilePath, rStringDataStore, DataStoreParserFlags::kLogParseErrors))
		{
			SEOUL_WARN("Failed refreshing LocManager strings, could not parse "
				 "loc json file \"%s\".\n", localeFilePath.CStr());
			return;
		}
	}

	// Apply platform-specific overrides if appropriate
	if (platformOverrideFilePath.IsValid())
	{
		// Load the override json file
		DataStore dataStore;
		if (!DataStoreParser::FromFile(platformOverrideFilePath, dataStore, DataStoreParserFlags::kLogParseErrors))
		{
			SEOUL_WARN("Unable to load platform-specific string table from file %s.",
				 platformOverrideFilePath.CStr());
			return;
		}

		// Walk the overrides table and replace entries in the StringTable with the overrides.
		auto const iBegin = dataStore.TableBegin(dataStore.GetRootNode());
		auto const iEnd = dataStore.TableEnd(dataStore.GetRootNode());
		for (auto i = iBegin; iEnd != i; ++i)
		{
			Byte const* sValue = nullptr;
			UInt32 zStringSizeInBytes = 0u;
			if (!dataStore.AsString(i->Second, sValue, zStringSizeInBytes))
			{
				SEOUL_WARN("Value of '%s' is not a string, failed loading platform-specific string table from file %s.",
					i->First.CStr(),
					platformOverrideFilePath.CStr());
				return;
			}

			if (!rStringDataStore.SetStringToTable(rStringDataStore.GetRootNode(), i->First, sValue, zStringSizeInBytes))
			{
				SEOUL_WARN("Failed setting '%s', failed loading platform-specific string table from file %s.",
					i->First.CStr(),
					platformOverrideFilePath.CStr());
				return;
			}
		}
	}
}

/**
 * Reload the string table based on the current language and platform
 */
void LocManager::InsideLockRefreshStrings()
{
#if !SEOUL_SHIP
	m_TokensNotLocalized.Clear();
	{
		DataStore empty;
		m_DefaultFallback.Swap(empty);
	}
#endif // /#if !SEOUL_SHIP

	// Sanity check, need to have at least one file.
	SEOUL_STATIC_ASSERT(FilePaths::StaticSize >= 1u);

	// Load the language specific .json file.
	StaticRefreshStrings(
		m_aLocaleFilePaths.Front(),
		(m_bUsePlatformOverride ? m_aPlatformOverrideFilePaths.Front() : FilePath()),
		m_StringDataStore);

	// Now apply any additional files as patches.
	for (UInt32 i = 1u; i < FilePaths::StaticSize; ++i)
	{
		DataStore patchDataStore;
		StaticRefreshStrings(
			m_aLocaleFilePaths[i],
			(m_bUsePlatformOverride ? m_aPlatformOverrideFilePaths[i] : FilePath()),
			patchDataStore);
		StaticApplyStringsAsPatch(patchDataStore, m_StringDataStore);
	}

#if !SEOUL_SHIP
	// Always so we can access the fallback data - will not be aunless
	// m_bUseDefaultFallbackLanguage is true.
	auto const bBuildFallback = true;
#else
	auto const bBuildFallback = m_bUseDefaultFallbackLanguage;
#endif

	// If using the default fallback functionality and the
	// language specific .json is different from the
	// default, load it, and then merge any strings which
	// are defined in the default but are not defined in
	// the language specific.
	if (bBuildFallback && m_aLocaleFilePaths.Front() != m_aDefaultLocaleFilePaths.Front())
	{
		DataStore dataStore;

		// Load the default language .json file.
		StaticRefreshStrings(
			m_aDefaultLocaleFilePaths.Front(),
			(m_bUsePlatformOverride ? m_aDefaultPlatformOverrideFilePaths.Front() : FilePath()),
			dataStore);

		// Now apply any additional files as patches.
		for (UInt32 i = 1u; i < FilePaths::StaticSize; ++i)
		{
			DataStore patchDataStore;
			StaticRefreshStrings(
				m_aDefaultLocaleFilePaths[i],
				(m_bUsePlatformOverride ? m_aDefaultPlatformOverrideFilePaths[i] : FilePath()),
				patchDataStore);
			StaticApplyStringsAsPatch(patchDataStore, dataStore);
		}

		if (m_bUseDefaultFallbackLanguage)
		{
			DataNode stringTableNode = dataStore.GetRootNode();
			auto stringIter = dataStore.TableBegin(stringTableNode);
			auto stringEnd = dataStore.TableEnd(stringTableNode);
			DataNode rootNode = m_StringDataStore.GetRootNode();
			for (; stringIter != stringEnd; ++stringIter)
			{
				// Skip loc keys for which we already have strings
				if (m_StringDataStore.TableContainsKey(rootNode, stringIter->First))
				{
					continue;
				}

				// Get the string value
				const Byte *sValue = nullptr;
				UInt32 zLength = 0;
				if (!dataStore.AsString(stringIter->Second, sValue, zLength))
				{
					SEOUL_WARN("[LocManager]: Value of \"%s\" is not a string", stringIter->First.CStr());
					continue;
				}

				// Save the new string value into our data store
				if (!m_StringDataStore.SetStringToTable(rootNode, stringIter->First, sValue, zLength))
				{
					SEOUL_WARN("[LocManager]: Failed to set string value for \"%s\"", stringIter->First.CStr());
					continue;
				}
			}
		}

#if !SEOUL_SHIP
		// Keep the default fallback for later query if logging missing tokens.
		m_DefaultFallback.Swap(dataStore);
#endif
	}

	// Compact the string data store.
	m_StringDataStore.CollectGarbageAndCompactHeap();

	// When cheats are enabled, generate a use count for font effects.
#if SEOUL_ENABLE_CHEATS
	m_tFontEffectUseCount.Clear();
	for (auto i = m_StringDataStore.TableBegin(m_StringDataStore.GetRootNode()), iEnd = m_StringDataStore.TableEnd(m_StringDataStore.GetRootNode()); iEnd != i; ++i)
	{
		Byte const* s = nullptr;
		UInt32 u = 0u;
		if (!m_StringDataStore.AsString(i->Second, s, u))
		{
			continue;
		}

		HtmlTag eNextTag = HtmlTag::kUnknown;
		HtmlTagStyle eNextTagStyle = HtmlTagStyle::kNone;

		String sUnused;
		HtmlReader reader(String::Iterator(s, 0u), String::Iterator(s, u), sUnused);
		while (true)
		{
			reader.ReadTag(eNextTag, eNextTagStyle);
			if (HtmlTag::kFont == eNextTag && HtmlTagStyle::kTerminator != eNextTagStyle)
			{
				HtmlAttribute eAttribute = HtmlAttribute::kUnknown;
				HtmlTagStyle eStyle = HtmlTagStyle::kNone;
				while (reader.ReadAttribute(eAttribute, eStyle))
				{
					if (HtmlAttribute::kEffect == eAttribute)
					{
						HString effect;
						reader.ReadAttributeValue(effect);
						if (!effect.IsEmpty())
						{
							auto pu = m_tFontEffectUseCount.Find(effect);
							if (nullptr == pu)
							{
								pu = &m_tFontEffectUseCount.Insert(effect, 0u).First->Second;
							}
							(*pu)++;
						}
					}
				}
			}
			else if (HtmlTag::kTextChunk == eNextTag)
			{
				String::Iterator unusedBegin;
				String::Iterator unusedEnd;
				// Termination is indicated by a failure to read a text chunk.
				if (!reader.ReadTextChunk(unusedBegin, unusedEnd))
				{
					break;
				}
			}
		}
	}
#endif // /SEOUL_ENABLE_CHEATS
}

#if SEOUL_ENABLE_CHEATS
/**
 * Debug only functionality to set the current language.
 */
void LocManager::DebugSetLanguage(const String& sLanguage)
{
	Bool bSetLanguage = false;
	{
		Lock lock(m_Mutex);
		if (m_vSupportedLanguages.Contains(sLanguage))
		{
			bSetLanguage = true;
		}
	}

	if (bSetLanguage)
	{
		(void)SetLanguage(sLanguage);
	}
}

void LocManager::DebugSetOnlyShowTokens(Bool bShow)
{
	m_bDebugOnlyShowTokens = bShow;
}

void LocManager::ToggleDontUseFallbackLanguage()
{
	String currentLanguage = m_sCurrentLanguage;
	m_bUseDefaultFallbackLanguage = !m_bUseDefaultFallbackLanguage;
	ReInit();
	SetLanguage(currentLanguage);
}

void LocManager::DebugSetPlatform(Platform ePlatform)
{
	if (ePlatform != m_eDebugPlatform)
	{
		m_eDebugPlatform = ePlatform;

		Lock lock(m_Mutex);
		InsideLockInit();
	}
}

void LocManager::DebugSwitchToNextLanguage()
{
	String sLanguage;
	{
		Lock lock(m_Mutex);
		if (m_vSupportedLanguages.IsEmpty())
		{
			return;
		}

		auto i = m_vSupportedLanguages.Find(m_sCurrentLanguage);
		if (m_vSupportedLanguages.End() != i)
		{
			++i;
		}

		if (m_vSupportedLanguages.End() == i)
		{
			i = m_vSupportedLanguages.Begin();
		}

		sLanguage = *i;
	}

	if (!sLanguage.IsEmpty())
	{
		(void)SetLanguage(sLanguage);
	}
}

void LocManager::DebugGetFontEffectUseCount(HString id, UInt32& ru) const
{
	Lock lock(m_Mutex);
	if (!m_tFontEffectUseCount.GetValue(id, ru))
	{
		ru = 0u;
	}
}

void LocManager::DebugGetAllMatchingTokens(String const& sSearchString, Vector<HString>& rvTokens) const
{
	Lock lock(m_Mutex);

	auto const iBegin = m_StringDataStore.TableBegin(m_StringDataStore.GetRootNode());
	auto const iEnd = m_StringDataStore.TableEnd(m_StringDataStore.GetRootNode());
	for (auto i = iBegin; iEnd != i; ++i)
	{
		String sTranslated;
		if (!m_StringDataStore.AsString(i->Second, sTranslated))
		{
			continue;
		}

		// TODO: Given a more flexible regex engine, check for matches between
		// search="<b>100x</b>" and translated="<b>${Replacement}</b>"

		// If the loc token is in the final string and the token is at least
		// 10% the size of the final string, it might be a match.
		auto sizeThreshold = Max(2u, UInt(sSearchString.GetSize() * 0.1));
		if (sSearchString.Find(sTranslated) != String::NPos && sTranslated.GetSize() >= sizeThreshold)
		{
			rvTokens.PushBack(i->First);
		}
		// If the loc token matches the search exactly, assume match
		else if (sSearchString == sTranslated)
		{
			rvTokens.PushBack(i->First);
		}
	}
}
#endif // /SEOUL_ENABLE_CHEATS

#if !SEOUL_SHIP
/**
 * Log information about how strings are being used.
 * It's sometimes useful to know which strings aren't being used
 * and which have not been translated for a particular language.
 * We probably don't want to always write this report; it's
 * probably best controlled in json.
 */
void LocManager::InsideLockDebugWriteMissingLocReport()
{
	// Copy the set locally, since SEOUL_WARN() may call back into LocManager.
	auto const set(m_TokensNotLocalized);
	for (auto const& e : set)
	{
		SEOUL_LOG_LOCALIZATION_WARNING("Loc token '%s' has no localization in '%s'.",
			e.CStr(),
			m_sCurrentLanguage.CStr());
	}
}
#endif

/**
 * Converts a language name in English (e.g. "French") to that language's
 * lowercase ISO 639-1 language code (e.g. "fr")
 */
String LocManager::GetLanguageCode(const String& sLanguageName)
{
	// TODO: Use a lookup table instead of hard-coding all of the languages
	// here in code
	if (sLanguageName == ksLanguageEnglish)
	{
		return "en";
	}
	else if (sLanguageName == ksLanguageFrench)
	{
		return "fr";
	}
	else if (sLanguageName == ksLanguageGerman)
	{
		return "de";
	}
	else if (sLanguageName == ksLanguageItalian)
	{
		return "it";
	}
	else if (sLanguageName == ksLanguageJapanese)
	{
		return "ja";
	}
	else if (sLanguageName == ksLanguageKorean)
	{
		return "ko";
	}
	else if (sLanguageName == ksLanguageSpanish)
	{
		return "es";
	}
	else if (sLanguageName == ksLanguageRussian)
	{
		return "ru";
	}
	else if (sLanguageName == ksLanguagePortuguese)
	{
		return "pt";
	}
	else
	{
		SEOUL_WARN("Unknown language: %s", sLanguageName.CStr());
		return "en";
	}
}

/**
 * Converts an ISO 639-1 language code (e.g. "fr") to the name of that language
 * in English (e.g. "French")
 */
String LocManager::GetLanguageNameFromCode(const String& sLanguageCode)
{
	// TODO: Use a lookup table instead of hard-coding all of the languages
	// here in code
	if (sLanguageCode == "en")
	{
		return ksLanguageEnglish;
	}
	else if (sLanguageCode == "fr")
	{
		return ksLanguageFrench;
	}
	else if (sLanguageCode == "de")
	{
		return ksLanguageGerman;
	}
	else if (sLanguageCode == "it")
	{
		return ksLanguageItalian;
	}
	else if (sLanguageCode == "ja")
	{
		return ksLanguageJapanese;
	}
	else if (sLanguageCode == "ko")
	{
		return ksLanguageKorean;
	}
	else if (sLanguageCode == "es")
	{
		return ksLanguageSpanish;
	}
	else if (sLanguageCode == "ru")
	{
		return ksLanguageRussian;
	}
	else if (sLanguageCode == "pt")
	{
		return ksLanguagePortuguese;
	}
	else
	{
		SEOUL_WARN("Unknown language code: %s", sLanguageCode.CStr());
		return ksLanguageEnglish;
	}
}

/**
 * When called, tears down the LocManager and then re-initializes.
 * This is equivalent to calling SEOUL_DELETE LocManager::Get()
 * and then SEOUL_NEW LocManager(), except that calling ReInit() is
 * thread-safe and does not risk crashes due to dangling references to
 * a stale LocManager instance after reinitialization.
 */
void LocManager::ReInit()
{
	// Clear and reinitialize
	{
		Lock lock(m_Mutex);

#if !SEOUL_SHIP
		m_TokensNotLocalized.Clear();
#endif // /#if !SEOUL_SHIP
#if SEOUL_ENABLE_CHEATS
		m_bDebugOnlyShowTokens = false;
#endif
		m_LocManagerFiles.Clear();
		DataStore emptyDataStore;
		m_StringDataStore.Swap(emptyDataStore);
		m_aDefaultLocaleFilePaths.Fill(FilePath());
		m_aLocaleFilePaths.Fill(FilePath());
		m_bLocalFileOpen = false;
		m_aDefaultPlatformOverrideFilePaths.Fill(FilePath());
		m_aPlatformOverrideFilePaths.Fill(FilePath());
		m_bUsePlatformOverride = false;
		m_sDefaultLanguage = ksLanguageEnglish;
		m_sCurrentLanguage = m_sDefaultLanguage;
		m_vSupportedLanguages.Clear();

		// JSON tells us allowed and default languages
		InsideLockReadConfigFromJson();
	}

	// Select the language that the system is running
	SetLanguageFromSystem();
}

String LocManager::TimeToString(Float seconds,
								HString sDaysAbbreviation,
								HString sHoursAbbreviation,
								HString sMinutesAbbreviation,
								HString sSecondsAbbrevaition)
{
	String sResult;
	Float fSecondsCounter = seconds;

	// days, hours, minutes, seconds. Pick the two most-significant digits in the time remaining and render them to a string
	Int iCurrentSigDigit = 0; // 0 = days, 1 = hours, 2 = minutes, 3 = seconds
	static const Int kuNumUnits = 4;
	static Float fSecondsPerUnit[kuNumUnits] = { 60*60*24, 60*60, 60, 1}; // Array with the number of seconds in each
	static const HString unitAbbreviation[kuNumUnits] =
	{
		sDaysAbbreviation,
		sHoursAbbreviation,
		sMinutesAbbreviation,
		sSecondsAbbrevaition,
	};

	if(fSecondsCounter < 1.0f) {
		//Special case: 0 seconds should not be rendered as the empty string
		sResult = String::Printf("0%s", unitAbbreviation[kuNumUnits - 1].CStr());
		return sResult;
	}

	Int digitCount = 0;

	// While we haven't yet rendered two significant digits and we're not finished with seconds
	while(digitCount < 2 && iCurrentSigDigit < kuNumUnits)
	{
		// If the current unit of time is >= 1
		if(fSecondsCounter >= fSecondsPerUnit[iCurrentSigDigit])
		{
			// It's significant. Render it and subtract it off the top

			if (digitCount > 0)
			{
				// space out the components
				sResult.Append(' ');
			}

			const UInt valuetoWrite = (UInt)(fSecondsCounter / fSecondsPerUnit[iCurrentSigDigit]);
			sResult += String::Printf("%d%s", valuetoWrite, unitAbbreviation[iCurrentSigDigit].CStr());
			fSecondsCounter -= valuetoWrite * fSecondsPerUnit[iCurrentSigDigit];
			// Next sig digit
			digitCount++;
		}
		iCurrentSigDigit++;
	}

	return sResult;
}

#if !SEOUL_SHIP
static Bool ValidateToken(HString token, const String& s)
{
	auto const count = Logger::GetSingleton().GetWarningCount();

	String unused;
	HtmlReader rReader(s.Begin(), s.End(), unused);

	HtmlTag eNextTag = HtmlTag::kUnknown;
	HtmlTagStyle eNextTagStyle = HtmlTagStyle::kNone;

	while (true)
	{
		rReader.ReadTag(eNextTag, eNextTagStyle);
		if (HtmlTag::kTextChunk == eNextTag)
		{
			String::Iterator unusedBegin;
			String::Iterator unusedEnd;
			// Termination is indicated by a failure to read a text chunk.
			if (!rReader.ReadTextChunk(unusedBegin, unusedEnd))
			{
				break;
			}
		}
	}

	auto const afterCount = Logger::GetSingleton().GetWarningCount();
	if (count != afterCount)
	{
		SEOUL_WARN("HTML parse failed (%s): |%s|", token.CStr(), s.CStr());
		return false;
	}

	return true;
}

/**
 * Iterate all tokens (all supported languages and platforms) and validate that
 * they are succesfully parsed by HTMLReader.
 */
Bool LocManager::ValidateTokens(UInt32& ruNumChecked) const
{
	auto bReturn = true;

	UInt32 uNum = 0u;
	SupportedLanguages v;
	GetSupportedLanguages(v);
	for (auto const& sLanguage : v)
	{
		for (auto const& sPlatform : kaPlatformNames)
		{
			auto const basePlatformFilePath(
				FilePath::CreateConfigFilePath(Path::Combine("Loc", sLanguage, sPlatform, kasLocaleFilenames[0])));
			if (!FileManager::Get()->Exists(basePlatformFilePath))
			{
				continue;
			}

			auto const baseFilePath(
				FilePath::CreateConfigFilePath(Path::Combine("Loc", sLanguage, kasLocaleFilenames[0])));

			DataStore ds;

			// Load the language specific .json file.
			StaticRefreshStrings(baseFilePath, basePlatformFilePath, ds);

			// Now apply any additional files as patches.
			for (UInt32 i = 1u; i < FilePaths::StaticSize; ++i)
			{
				DataStore patchDataStore;
				StaticRefreshStrings(
					FilePath::CreateConfigFilePath(Path::Combine("Loc", sLanguage, kasLocaleFilenames[i])),
					FilePath::CreateConfigFilePath(Path::Combine("Loc", sLanguage, sPlatform, kasLocaleFilenames[i])),
					patchDataStore);
				StaticApplyStringsAsPatch(patchDataStore, ds);
			}

			for (auto i = ds.TableBegin(ds.GetRootNode()), iEnd = ds.TableEnd(ds.GetRootNode()); iEnd != i; ++i)
			{
				String sValue;
				SEOUL_VERIFY(ds.AsString(i->Second, sValue));
				bReturn = ValidateToken(i->First, sValue) && bReturn;
				++uNum;
			}
		}
	}

	ruNumChecked = uNum;
	return bReturn;
}
#endif // /!SEOUL_SHIP

} // namespace Seoul
