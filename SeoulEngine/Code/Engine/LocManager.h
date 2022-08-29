/**
 * \file LocManager.h
 * \brief Defines the localized string manager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef LOC_MANAGER_H
#define LOC_MANAGER_H

#include "HashSet.h"
#include "DataStore.h"
#include "Delegate.h"
#include "FilePath.h"
#include "HashTable.h"
#include "Mutex.h"
#include "SeoulHString.h"
#include "Singleton.h"
#include "StringUtil.h"

#if SEOUL_HOT_LOADING
#include "Settings.h"
#endif

namespace Seoul
{

// Forward declarations
namespace Content { struct ChangeEvent; }

class LocManager SEOUL_SEALED : public Singleton<LocManager>
{
public:
	SEOUL_DELEGATE_TARGET(LocManager);

	LocManager();
	~LocManager();

#if SEOUL_HOT_LOADING
	// Call to register LocManager for hot loading. Unregister
	// must be called before shutdown. Both register and unregister
	// must be called on the main thread.
	void RegisterForHotLoading();
	void UnregisterFromHotLoading();
#endif // /#if SEOUL_HOT_LOADING

	String Localize(HString token);
	String Localize(Byte const* sToken, UInt32 zTokenLengthInBytes);
	String Localize(const String& sToken)
	{
		return Localize(sToken.CStr(), sToken.GetSize());
	}
	String Localize(Byte const* sToken)
	{
		return Localize(sToken, StrLen(sToken));
	}

	/**
	 * Localizes the token and then replaces the given sVariable in the
	 * localized string with the given integer value formatted for the locale.
	 */
	template <typename TNumberType>
	String LocalizeAndReplaceNumber(HString sToken, const String& sVariable, TNumberType nValue)
	{
		String sLocString = Localize(sToken);
		return sLocString.ReplaceAll(sVariable, FormatNumber(nValue));
	}

	// Hook for g_pCoreVirtuals, must be explicitly assigned in main process code.
	static String CoreLocalize(HString token, const String& sDefaultValue);

	Bool IsValidToken(HString token) const;
	Bool IsValidToken(Byte const* sToken, UInt32 zTokenLengthInBytes) const;
	Bool IsValidToken(const String& sToken) const
	{
		return IsValidToken(sToken.CStr(), sToken.GetSize());
	}
	Bool IsValidToken(Byte const* sToken) const
	{
		return IsValidToken(sToken, StrLen(sToken));
	}

	String LocalizeByQuantity(UInt32 uNumberOfThings, const HString& hTokenSingular, const HString& hTokenPlural);
	String GetCurrentLanguage() const;
	String GetCurrentLanguageCode() const;
	Bool SetLanguage(const String& lang);
	void SetLanguageFromSystem();

	// Formats the given number according to the current locale, using the
	// appropriate thousands separator
	String FormatNumber(Double iValue, Int decimals);
	String FormatNumber(Int iValue);

	// Formats a time interval in seconds into abbreviated days, hours, minutes, and seconds.
	String TimeToString(Float seconds,
		HString sDaysAbbreviation,
		HString sHoursAbbreviation,
		HString sMinutesAbbreviation,
		HString sSecondsAbbreviation);

#if SEOUL_ENABLE_CHEATS
	void DebugSetLanguage(const String& sLanguage);
	void DebugSetOnlyShowTokens(Bool bShow);
	void ToggleDontUseFallbackLanguage();
	void DebugSetPlatform(Platform ePlatform);
	void DebugSwitchToNextLanguage();

	Bool DebugOnlyShowTokens() const
	{
		return m_bDebugOnlyShowTokens;
	}
	Platform DebugPlatform() const
	{
		return m_eDebugPlatform;
	}

	typedef HashTable<HString, UInt32, MemoryBudgets::Strings> FontEffectUseCount;
	void DebugGetFontEffectUseCount(HString id, UInt32& ru) const;

	void DebugGetAllMatchingTokens(String const& sSearchString, Vector<HString>& rvTokens) const;
#endif // /SEOUL_ENABLE_CHEATS

	// Converts a language name in English (e.g. "French") to that language's
	// lowercase ISO 639-1 language code (e.g. "fr")
	static String GetLanguageCode(const String& sLanguageName);

	// Converts an ISO 639-1 language code (e.g. "fr") to the name of that
	// language in English (e.g. "French")
	static String GetLanguageNameFromCode(const String& sLanguageCode);

	typedef HashTable<HString, String, MemoryBudgets::Strings> LocStringTable;

	// Call to flush the LocManager and reload all data from JSON.
	void ReInit();

	typedef Vector<String, MemoryBudgets::Strings> SupportedLanguages;

	/** @return The currently configured default language name. */
	String GetDefaultLanguageName() const
	{
		String s;
		{
			Lock lock(m_Mutex);
			s = m_sDefaultLanguage;
		}
		return s;
	}

	/**
	 * @return The list of currently supported languages.
	 */
	void GetSupportedLanguages(SupportedLanguages& rvSupportedLanguages) const
	{
		{
			Lock lock(m_Mutex);
			rvSupportedLanguages = m_vSupportedLanguages;
		}
	}

	/**
	 * Override the list of supported languages. Will
	 * be overriden by a call to ReInit().
	 */
	void SetSupportedLanguages(const SupportedLanguages& vSupportedLanguages)
	{
		{
			Lock lock(m_Mutex);
			m_vSupportedLanguages = vSupportedLanguages;
		}
		SetLanguageFromSystem();
	}

	/**
	 * @return The current UseDefaultFallbackLanguage setting.
	 */
	Bool GetUseDefaultFallbackLanguage() const
	{
		return m_bUseDefaultFallbackLanguage;
	}

	/**
	 * @return True if a FilePath identifies a file in use by LocManager, false otherwise.
	 */
	Bool IsLocManagerFilePath(FilePath filePath) const
	{
		return m_LocManagerFiles.HasKey(filePath);
	}

	/**
	 * Call to update the UseDefaultFallbackLanguage setting - if true,
	 * missing tokens in languages other than the default will be populated from
	 * the default language, if defined.
	 *
	 * \warning Calling this method invokes ReInit() is b is not equal to
	 * the value of GetUseDefaultFallbackLanguage().
	 */
	void SetUseDefaultFallbackLanguage(Bool b)
	{
		Bool bReInit = false;
		{
			Lock lock(m_Mutex);
			if (m_bUseDefaultFallbackLanguage != b)
			{
				m_bUseDefaultFallbackLanguage = b;
				bReInit = true;
			}
		}

		if (bReInit)
		{
			ReInit();
		}
	}

#if !SEOUL_SHIP
	Bool ValidateTokens(UInt32& ruNumChecked) const;
#endif // /!SEOUL_SHIP

	static const String ksLanguageEnglish;
	static const String ksLanguageFrench;
	static const String ksLanguageGerman;
	static const String ksLanguageItalian;
	static const String ksLanguageJapanese;
	static const String ksLanguageKorean;
	static const String ksLanguageSpanish;
	static const String ksLanguageRussian;
	static const String ksLanguagePortuguese;

private:
	SEOUL_DISABLE_COPY(LocManager);

#if SEOUL_HOT_LOADING
	void OnFileChange(Content::ChangeEvent* pFileChangeEvent);
	Bool OnIsFileLoaded(FilePath filePath);
	void OnFileLoadComplete(FilePath filePath);
	HashTable<FilePath, SettingsContentHandle, MemoryBudgets::Strings> m_tHotLoading;
#endif // /#if SEOUL_HOT_LOADING

	void InsideLockReadConfigFromJson();
	void InsideLockInit();
	typedef Delegate<void(FilePath, FilePath)> DependencyDelegate;
	static void StaticApplyStringsAsPatch(
		const DataStore& inDataStore,
		DataStore& rOutDataStore);
	static void StaticRefreshStrings(
		FilePath localeFilePath,
		FilePath platformOverrideFilePath,
		DataStore& rStringDataStore);
	void InsideLockRefreshStrings();

	Mutex m_Mutex;

	typedef HashSet<String, MemoryBudgets::Strings> Tokens;
#if !SEOUL_SHIP
	// Debug tools
	void InsideLockDebugWriteMissingLocReport();
	Tokens m_TokensNotLocalized;
	DataStore m_DefaultFallback;
#endif // /!SEOUL_SHIP

#if SEOUL_ENABLE_CHEATS
	FontEffectUseCount m_tFontEffectUseCount;
	Atomic32Value<Platform> m_eDebugPlatform;
	Atomic32Value<Bool> m_bDebugOnlyShowTokens;
#endif // /SEOUL_ENABLE_CHEATS

	// Query table of files used by LocManager.
	HashSet<FilePath, MemoryBudgets::Strings> m_LocManagerFiles;

	// JSON file
	FilePath const m_JsonFilePath;

	// Number of locale files to load from. Main and patch.
	static const UInt32 kuLocaleFiles = 2u;

	// Locale file
	DataStore m_StringDataStore;
	typedef FixedArray<FilePath, kuLocaleFiles> FilePaths;
	FilePaths m_aDefaultLocaleFilePaths;
	FilePaths m_aLocaleFilePaths;
	Bool m_bLocalFileOpen;

	// Platform-specific string overrides
	FilePaths m_aDefaultPlatformOverrideFilePaths;
	FilePaths m_aPlatformOverrideFilePaths;
	Bool m_bUsePlatformOverride;

	// Languages
	String m_sDefaultLanguage;
	String m_sCurrentLanguage;
	SupportedLanguages m_vSupportedLanguages;

	// If true, missing tokens in languages other than the default
	// will be populated from the default language, if defined.
	Bool m_bUseDefaultFallbackLanguage;

	static const HString ksThousandsSeparatorLocToken;

}; // LocManager

} // Seoul

#endif // include guard
