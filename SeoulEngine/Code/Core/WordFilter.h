/**
 * \file WordFilter.h
 *
 * \brief Provides whitelist/blacklist filtering of a string of text. Uses
 * a phonetic parser (based on double metaphone) and a collection of other
 * heuristic rules to normalize text, and a trie of whitelist/blacklist words
 * to perform filtering.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef WORD_FILTER_H
#define WORD_FILTER_H

#include "HashTable.h"
#include "Prereqs.h"
#include "ScopedPtr.h"
#include "SeoulString.h"
#include "StringUtil.h"
#include "Vector.h"
namespace Seoul { class DataNode; }
namespace Seoul { class DataStore; }
namespace Seoul { struct WordFilterAlternative; }
namespace Seoul
{

/** Options that control word filtering, in general. */
namespace WordFilterOptions
{
	// IMPORTANT: Bits used for WordFilterWordOptions are mutually exclusive
	// from bits used for WordFilterOptions, since they are merged in several
	// contexts.
	enum Enum
	{
		// If enabled, all matches that resolve to the default substitution
		// will apply filtering to the entire word of the match portion (e.g.
		// A match against "ball" to the word "baseball" will produce "***"
		// instead of "base***".
		kEnableApplyDefaultSubstitutionToWholeWords = (1 << 0),

		// If enabled, leetspeak (e.g. "1337 sp3ak") will be converted to special
		// characters for further processing.
		kEnableLeetSpeak = (1 << 1),
	};
} // namespace WordFilterOptions

/**
 * Control options for individual words entered into the trie.
 * Mostly, controls matching behavior, but can also control filtering behavior.
 */
namespace WordFilterWordOptions
{
	// IMPORTANT: Bits used for WordFilterWordOptions are mutually exclusive
	// from bits used for WordFilterOptions, since they are merged in several
	// contexts.
	enum Enum
	{
		/**
		 * If a match occurs against the word in the trie, the *entire* input string
		 * should be filtered away. This immediately returns true and sets rsString
		 * to the empty string from a call to FilterString().
		 */
		kDropEntireInputString = (1 << 27),

		/**
		 * Matches only occur against the word in the trie if the input is a "likely"
		 * word. Either, a whole word (surrounded by white space or string begin/end),
		 * or a word starting at a word boundary, as identified by more advanced
		 * heuristics using the known word trie.
		 */
		kLikelyWordOnly = (1 << 28),

		/**
		 * Valid only in the context of a likely word set, this word only matches
		 * when it is a suffix (the first match of a IsLikelyLast() check.
		 */
		kSuffix = (1 << 29),

		/**
		 * Matches only occur against the word in the trie if the input is a whole word
		 * (e.g. "ball" with whole word enabled will match "ball" but not "baseball".
		 */
		kWholeWordOnly = (1 << 30),
	};
} // namespace WordFilterWordOptions

/**
 * String converted into a normalized form for filtering. Used for
 * both whitelist and blacklist words, as well as strings to be filtered.
 */
class WordFilterEncodedString SEOUL_SEALED
{
public:
	class Character SEOUL_SEALED
	{
	public:
		Character()
			: m_Character((UniChar)0)
			, m_uOriginalStartOffsetInBytes(0u)
			, m_uOriginalCharacterSizeInBytesMinusOne(0u)
			, m_bWholeWordBegin(0u)
			, m_bExactWholeWordEnd(0u)
			, m_bFuzzyWholeWordEnd(0u)
		{
		}

		Character(UniChar ch, UInt32 uOriginalStartOffsetInBytes)
			: m_Character(ch)
			, m_uOriginalStartOffsetInBytes(uOriginalStartOffsetInBytes)
			, m_uOriginalCharacterSizeInBytesMinusOne((UInt32)(UTF8BytesPerChar(ch) - 1u))
			, m_bWholeWordBegin(0u)
			, m_bExactWholeWordEnd(0u)
			, m_bFuzzyWholeWordEnd(0u)
		{
		}

		Character(const Character& b)
			: m_Character(b.m_Character)
			, m_uOriginalStartOffsetInBytes(b.m_uOriginalStartOffsetInBytes)
			, m_uOriginalCharacterSizeInBytesMinusOne(b.m_uOriginalCharacterSizeInBytesMinusOne)
			, m_bWholeWordBegin(0u)
			, m_bExactWholeWordEnd(0u)
			, m_bFuzzyWholeWordEnd(0u)
		{
		}

		Character& operator=(const Character& b)
		{
			m_Character = b.m_Character;
			m_uOriginalStartOffsetInBytes = b.m_uOriginalStartOffsetInBytes;
			m_uOriginalCharacterSizeInBytesMinusOne = b.m_uOriginalCharacterSizeInBytesMinusOne;
			m_bWholeWordBegin = b.m_bWholeWordBegin;
			m_bExactWholeWordEnd = b.m_bExactWholeWordEnd;
			m_bFuzzyWholeWordEnd = b.m_bFuzzyWholeWordEnd;
			return *this;
		}

		UniChar GetCharacter() const
		{
			return m_Character;
		}

		UInt32 GetOriginalStartOffsetInBytes() const
		{
			return m_uOriginalStartOffsetInBytes;
		}

		UInt32 GetOriginalEndOffsetInBytes() const
		{
			return m_uOriginalStartOffsetInBytes + m_uOriginalCharacterSizeInBytesMinusOne + 1u;
		}

		Bool IsWholeWordBegin() const { return m_bWholeWordBegin; }
		Bool IsExactWholeWordEnd() const { return m_bExactWholeWordEnd; }
		Bool IsFuzzyWholeWordEnd() const { return m_bFuzzyWholeWordEnd; }

		void SetCharacter(UniChar ch)
		{
			m_Character = ch;
		}

		void SetWholeWordBegin(Bool bWholeWordBegin)
		{
			m_bWholeWordBegin = (bWholeWordBegin ? 1u : 0u);
		}

		void SetExactWholeWordEnd(Bool bExactWholeWordEnd)
		{
			m_bExactWholeWordEnd = (bExactWholeWordEnd ? 1u : 0u);
		}

		void SetFuzzyWholeWordEnd(Bool bFuzzyWholeWordEnd)
		{
			m_bFuzzyWholeWordEnd = (bFuzzyWholeWordEnd ? 1u : 0u);
		}

	private:
		UniChar m_Character;
		UInt32 m_uOriginalStartOffsetInBytes : 27;
		UInt32 m_uOriginalCharacterSizeInBytesMinusOne : 2;
		UInt32 m_bWholeWordBegin : 1;
		UInt32 m_bExactWholeWordEnd : 1;
		UInt32 m_bFuzzyWholeWordEnd : 1;
	}; // class Character
	typedef Vector<Character, MemoryBudgets::Strings> Buffer;

	typedef Buffer::ConstIterator ConstIterator;

	WordFilterEncodedString(
		Byte const* s,
		UInt32 zStringLengthInBytes,
		UInt32 uOptions);
	~WordFilterEncodedString();

	/** @return Iterator to the first character of the encoded buffer of characters. */
	ConstIterator Begin() const
	{
		return m_vBuffer.Begin();
	}

	/** @return Iterator to one passed the last character of the encoded buffer of characters. */
	ConstIterator End() const
	{
		return m_vBuffer.End();
	}

private:
	Buffer m_vBuffer;

	void InternalCollapseDuplicateRuns();
	void InternalConvertLeetSpeakToLetters();
	void InternalConvertSpecialSymbols();
	void InternalConvertToLowerCase();
	void InternalMarkWholeWordBeginAndEnd();
	void InternalNormalizeWhiteSpace();
	void InternalPopulate(Byte const* s, UInt32 zStringLengthInBytes);
	void InternalRemoveExtraSymbols();
}; // class WordFilterEncodedString

SEOUL_STATIC_ASSERT(sizeof(WordFilterEncodedString::Character) == 8);

class WordFilterTreeNode;
typedef HashTable<UniChar, WordFilterTreeNode*, MemoryBudgets::Strings> WordFilterTreeChildren;

/**
 * Word data encoded into the filtering trie.
 */
class WordFilterWord SEOUL_SEALED
{
public:
	WordFilterWord(Byte const* sWord, UInt32 zWordSizeInBytes, UInt32 uOptions)
		: m_sWord(sWord, zWordSizeInBytes)
		, m_uOptions(uOptions)
	{
	}

	~WordFilterWord()
	{
	}

	/** @return The original word that is terminated by the container of this WordFilterWord. */
	const String& GetWord() const
	{
		return m_sWord;
	}

	/** @return True if matches against this word should cause the entire input string to be filtered away. */
	Bool IsSetDropEntireInputString() const
	{
		return (WordFilterWordOptions::kDropEntireInputString == (WordFilterWordOptions::kDropEntireInputString & m_uOptions));
	}

	/**
	 * @return Trie if matches against this word should only occur when it is a whole word, or when
	 * it is likely a word, as determined by heuristic word boundaries determined via the
	 * known word trie.
	 */
	Bool IsSetLikelyWordOnly() const
	{
		return (WordFilterWordOptions::kLikelyWordOnly == (WordFilterWordOptions::kLikelyWordOnly & m_uOptions));
	}

	/**
	 * @return True if matches against this word should only occur when it appears in
	 * context as a suffix. Only valid when used as part of a known word list.
	 */
	Bool IsSetSuffix() const
	{
		return (WordFilterWordOptions::kSuffix == (WordFilterWordOptions::kSuffix & m_uOptions));
	}

	/**
	 * @return True if matches against this word should only occur against whole input words
	 * (e.g. "ball" matches "ball" but not "baseball".
	 */
	Bool IsSetWholeWordOnly() const
	{
		return (WordFilterWordOptions::kWholeWordOnly == (WordFilterWordOptions::kWholeWordOnly & m_uOptions));
	}

private:
	String const m_sWord;
	UInt32 const m_uOptions;
}; // class WordFilterWord

/**
 * One node in the trie used for word filtering.
 * This structure represents leaf or inner nodes, not root nodes.
 */
class WordFilterTreeNode SEOUL_SEALED
{
public:
	WordFilterTreeNode()
		: m_tChildren()
		, m_pWord(nullptr)
	{
	}

	~WordFilterTreeNode()
	{
		SafeDelete(m_pWord);
		SafeDeleteTable(m_tChildren);
	}

	/**
	 * @return The current value of m_pWord, nullptr if this WordFilterTreeNode
	 * does not terminate a word.
	 */
	WordFilterWord const* GetWord() const
	{
		return m_pWord;
	}

	/**
	 * @return A read-only collection of this WordFilterTreeNode's children,
	 * keyed on the UniChar of the node.
	 */
	const WordFilterTreeChildren& GetChildren() const
	{
		return m_tChildren;
	}

	/**
	 * @return A read-write collection of this WordFilterTreeNode's children,
	 * keyed on the UniChar of the node.
	 */
	WordFilterTreeChildren& GetChildren()
	{
		return m_tChildren;
	}

	/**
	 * Update the current word associated with this node. Set to String()
	 * to untag this node as a word terminator.
	 */
	void SetWord(Byte const* sWord, UInt32 zWordSizeInBytes, UInt32 uOptions)
	{
		SafeDelete(m_pWord);
		if (nullptr != sWord && zWordSizeInBytes > 0u)
		{
			m_pWord = SEOUL_NEW(MemoryBudgets::Strings) WordFilterWord(sWord, zWordSizeInBytes, uOptions);
		}
	}

private:
	WordFilterTreeChildren m_tChildren;
	WordFilterWord const* m_pWord;

	SEOUL_DISABLE_COPY(WordFilterTreeNode);
}; // class WordFilterTreeNode

/**
 * One node in the trie used for word filtering.
 *
 * This structure represents the root node of the trie (it has
 * no parent letter, so all children of this node form the set
 * of starting letters encoded in the trie).
 */
class WordFilterRootNode SEOUL_SEALED
{
public:
	WordFilterRootNode()
		: m_tChildren()
	{
	}

	~WordFilterRootNode()
	{
		Clear();
	}

	/** Destroy the contents of the trie formed by this WordFilterRootNode. */
	void Clear()
	{
		SafeDeleteTable(m_tChildren);
	}

	WordFilterWord const* FindLongestMatch(
		WordFilterRootNode const* pLikelyWords,
		WordFilterEncodedString::ConstIterator const iBufferBegin,
		WordFilterEncodedString::ConstIterator const iBufferEnd,
		WordFilterEncodedString::ConstIterator const iWordFirst,
		WordFilterEncodedString::ConstIterator& riWordLast,
		Bool bAllowSuffix) const;

	/** Encode a new word into the trie. Builds a patch in the trie for fulfilling FindLongestMatch(). */
	void InsertWord(Byte const* sWord, UInt32 zWordSizeInBytes, UInt32 uOptions);

	/** Exchange the state of this WordFilterRootNode with rWordFilterRootNode. */
	void Swap(WordFilterRootNode& rWordFilterRootNode)
	{
		m_tChildren.Swap(rWordFilterRootNode.m_tChildren);
	}

private:
	WordFilterTreeChildren m_tChildren;

	WordFilterWord const* InternalFindLongestMatch(
		WordFilterRootNode const* pLikelyWords,
		UniChar const* sAlternativeCharacters,
		WordFilterTreeChildren const* ptChildren,
		WordFilterEncodedString::ConstIterator const iBufferBegin,
		WordFilterEncodedString::ConstIterator const iBufferEnd,
		WordFilterEncodedString::ConstIterator const iWordFirst,
		WordFilterEncodedString::ConstIterator const iStartSearch,
		WordFilterEncodedString::ConstIterator& riWordLast,
		Bool bHasSkippedWhiteSpaceDuringSearch,
		Bool bAllowSuffix) const;
	void InternalTryAlternatives(
		WordFilterRootNode const* pLikelyWords,
		WordFilterAlternative const* pAlternatives,
		WordFilterTreeChildren const* ptChildren,
		WordFilterEncodedString::ConstIterator const iBufferBegin,
		WordFilterEncodedString::ConstIterator const iBufferEnd,
		WordFilterEncodedString::ConstIterator const iWordFirst,
		WordFilterEncodedString::ConstIterator const i,
		WordFilterWord const*& rpWord,
		WordFilterEncodedString::ConstIterator& riWordLast,
		Bool bHasSkippedWhiteSpaceDuringSearch,
		Bool bAllowSuffix) const;

	SEOUL_DISABLE_COPY(WordFilterRootNode);
}; // class WordFilterRootNode

/**
 * The public WordFilter class. This is the object you want to use to integrate word filtering
 * functionality into your code.
 */
class WordFilter SEOUL_SEALED
{
public:
	typedef HashTable<String, String, MemoryBudgets::Strings> Substitutions;

	WordFilter();
	~WordFilter();

	/**
	 * Apply filtering, based on black and white lists, to rsString. Modifies
	 * the String in place if filtering occurs.
	 *
	 * @param[inout] rsString The word to filter. Modified in place on filtering
	 * success.
	 * @param[out] psLastMatch (optional) If non-null, this String will be set
	 * to the last match in the input string.
	 *
	 * @return True if rsString was filtered/modified, false otherwise.
	 */
	Bool FilterString(String& rsString, String* psLastMatch = nullptr) const;

	/**
	 * Reconfigure filtering based on values set in the dataNode table.
	 *
	 * @return True on success, false on failure. On failure, the existing
	 * configuration state is left unmodified.
	 */
	Bool LoadConfiguration(const DataStore& dataStore, const DataNode& dataNode);

	/**
	 * Load the whitelist, the blacklist, and the known words list.
	 * - whitelistNode - explicitly allowed words.
	 * - blacklistNode - explicitly disallowed words.
	 * - knownWordNodes - additional words used for sentence structure understanding
	 *   in the absence of valid spaces. The knownWords list also appends/includes
	 *   the whitelist and blacklist, so specified knownWords are in addition to those
	 *   2 lists.
	 *
	 * @return True on success, false on failure. On failure, the existing lists
	 * are left umodified.
	 */
	Bool LoadLists(
		const DataStore& dataStore,
		const DataNode& blacklistNode,
		const DataNode& knownWordsNode,
		const DataNode& whitelistNode);

	/**
	 * Reset the internal substitution table state to one defined at dataNode
	 *
	 * @return True on success, false on failure. On failure, the existing
	 * substitution table state is left unmodified.
	 */
	Bool LoadSubstitutionTable(const DataStore& dataStore, const DataNode& dataNode);

	/** @return The currently set default substitution. */
	const String& GetDefaultSubstitution() const
	{
		return m_sDefaultSubstitution;
	}

	/** Set the fallback substitution, used when no explicit subsitution exists for the filtered word. */
	void SetDefaultSubstitution(const String& sDefaultSubstitution)
	{
		m_sDefaultSubstitution = sDefaultSubstitution;
	}

private:
	static Bool InternalBuildFilterTree(
		WordFilterRootNode& rRoot,
		const DataStore& dataStore,
		const DataNode& dataNode,
		UInt32 uGeneralOptions,
		Bool bProcessOptions);

	String m_sDefaultSubstitution;
	Substitutions m_tSubstitutions;
	WordFilterRootNode m_BlacklistRoot;
	WordFilterRootNode m_KnownWordsRoot;
	WordFilterRootNode m_WhitelistRoot;
	UInt32 m_uOptions;

	SEOUL_DISABLE_COPY(WordFilter);
}; // class WordFilter

} // namespace Seoul

#endif // include guard
