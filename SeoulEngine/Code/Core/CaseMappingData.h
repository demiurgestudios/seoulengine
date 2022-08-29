/**
 * \file CaseMappingData.h
 * \brief Helper structs for case mapping strings
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CASE_MAPPING_DATA_H
#define CASE_MAPPING_DATA_H

/**
 * CASE MAPPING OVERVIEW
 *
 * Case mapping with full Unicode support is hard:
 *
 * - Some characters like U+0061 (LATIN SMALL LETTER A) map one-to-one with
 *   their uppcerase counterparts like U+0041 (LATIN CAPITAL LETTER A)
 *
 * - Some characters like U+00DF (LATIN SMALL LETTER SHARP S) map to multiple
 *   uppercase letters ("SS", i.e. U+0053 U+0053)
 *
 * - Some characters have different mappings depending on the locale.  In
 *   English, the uppercase of "i" (U+0069, LATIN SMALL LETTER I) is "I"
 *   (U+0049, LATIN CAPITAL LETTER I).  But in Turkish and Azeri, the uppercase
 *   of "i" is a dotted capital I (U+0130, LATIN CAPITAL LETTER I WITH DOT
 *   ABOVE)
 *
 * - Some characters have different mappings depending on the surrounding
 *   context characters.  The lowercase of U+03A3 (GREEK CAPITAL LETTER SIGMA)
 *   is ordinarily U+03C3 (GREEK SMALL LETTER SIGMA), but its lowercase is
 *   instead U+03C2 (GREEK SMALL LETTER FINAL SIGMA) when it's at the end of a
 *   word.
 *
 * - etc. etc.
 *
 * - There's also a third case called title case which we don't yet support in
 *   the Seoul engine
 *
 * So, how do we deal with this?
 *
 * We have a bunch of tables containing all of this case mapping data.  This
 * data is generated from the databases provided by the Unicode consoritum (see
 * Tools/gen_unicode_case_mapping.py for the script that does that).
 *
 * The tables contain a whole bunch of case mapping entries which say what the
 * corresponding uppercase and lowercase strings are for each character, along
 * with flags indicating the conditions under which those case mappings apply
 * (locale, context, etc.).  Not all of these flags are currently supported.
 *
 * For optimal decoding speed, the tables are represented as a variable-height
 * N-ary tree.  For any given Unicode character, that character's case mapping
 * data is stored at a height equal to its UTF-8-encoded length in bytes.  So
 * the root of the tree has 128 leaf children (one for each possible 1-byte
 * character) and 56 non-leaf children (one for each valid lead byte of a
 * multibyte character in UTF-8).  Then, each subsequent level of the tree has
 * 64 children (one for each possible values of non-lead bytes in UTF-8).
 *
 * If a character or character range has no case mapping data, those tree nodes
 * are nullptr.
 *
 * If a character has multiple entries (e.g. due to locale-specific rules),
 * those entries follow each other directly in memory, and all but the last
 * entry have the kFlagMoreEntries flag set.  So, the code can just walk
 * through memory until that flag is not set to enumerate all of the entries
 * for a given character.
 *
 * For more details, see the gorey implementation in String::InternalMapCase().
 */

#include "SeoulTypes.h"

namespace Seoul
{

namespace CaseMappingInternal
{

/**
 * Flags indicating special behavior in case mapping operations.
 *
 * NOTE: These flags must fit in 16 bits
 */
enum ECaseMappingFlags
{
	/** Case mapping only applies in the lt locale */
	kFlagLithuanian      = 0x0001,
	/** Case mapping only applies in the tr and az locales */
	kFlagTurkishAzeri    = 0x0002,
	/** Case mapping only applies at the end of a word */
	kFlagFinalSigma      = 0x0004,
	/** Case mapping only applies after a soft dotted character */
	kFlagAfterSoftDotted = 0x0008,
	/** Case mapping only applies after "I" */
	kFlagAfterI          = 0x0010,
	/** Case mapping only applies before combining characters above */
	kFlagMoreAbove       = 0x0020,
	/** Case mapping only applies not before a combining dot above */
	kFlagNotBeforeDot    = 0x0040,
	/** This character has another case mapping for it */
	kFlagMoreEntries     = 0x0080
};

/**
 * Structure describing the case mapping for one character.  If m_uFlags has the
 * kFlagMoreEntries bit set in it, this structure is immediately followed in
 * memory by another structure containing another case mapping for the same
 * character.
 *
 * Each structure is 8 bytes long for maximum memory performance.
 */
struct CharEntry
{
	/**
	 * Offset to the UTF-8 string for this case mapping within the current
	 * table's case mapping string pool.  That string may be more than one
	 * character long, and it is NOT null-terminated.
	 */
	UInt m_uStrOffset;

	/** Length of the case mapped string for the character */
	UByte m_Length;

	/** Case mapping flags (ECaseMappingFlags) */
	UShort m_uFlags;
};

/**
 * Structure representing a non-leaf, non-root node in the case mapping table.
 * This node has 64 children; each child may be either a leaf node (CharEntry),
 * another subtree (SubTable), or nullptr, depending on where in the tree we are.
 *
 * A child will be a leaf node only if its depth equals the length in bytes of
 * the UTF-8 character being looked up from the root node.
 */
struct SubTable
{
	/**
	 * Indices of the children of this node.  If the child is a CharEntry, this
	 * is the index into the table's m_pAllEntries array.  If this child is a
	 * SubTable, this is the index into the table's m_pAllSubTables array.
	 */
	UShort m_auChildIndices[64];
};

/**
 * Root of the case mapping table structure
 *
 * - The first 128 children are leaf nodes for the first 128 Unicode characters
 *   (ASCII), which are encoded as 1 byte in UTF-8.
 *
 * - The next 32 children are the 2-level subtrees for the 2-byte UTF-8
 *   characters
 *
 * - The next 16 children are the 3-level subtrees for the 3-byte UTF-8
 *   characters
 *
 * - The last 8 children are the 4-level subtrees for the 4-byte UTF-8
 *   characters
 */
struct RootTable
{
	/** Pointer to array of all entries in this table */
	const CharEntry *m_pAllEntries;

	/** Pointer to array of all subtables in this table */
	const SubTable *m_pAllSubTables;

	/** Character entries for the first 1-byte UTF-8 characters */
	const CharEntry *m_apBaseEntries[128];

	/** Subtrees for the 2-, 3-, and 4-byte UTF-8 characters */
	const SubTable *m_apSubTables[56];

	/**
	 * String pool containing all of the case mapped strings in this table.
	 * These are indexed into by all of the CharEntrys' m_uStrOffset members.
	 * The strings in here are NOT null-terimnated.
	 */
	const Byte *m_pStringPool;
};

// Case mapping table for performing uppercase conversions
extern const RootTable g_UppercaseTable;

// Case mapping table for performing lowercase conversions
extern const RootTable g_LowercaseTable;

}  // namespace CaseMappingInternal

} // namespace Seoul

#endif // include guard
