/**
 * \file DevUIUtil.h
 * \brief Miscellaneous shared functions of DevUI code.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DEV_UI_UTIL_H
#define DEV_UI_UTIL_H

#include "DataStore.h"
#include "Prereqs.h"
#include "SeoulString.h"
namespace Seoul { struct Vector2D; }

#if SEOUL_ENABLE_DEV_UI

namespace Seoul::DevUI::Util
{

struct TextDataStoreEntry SEOUL_SEALED
{
	String m_sKey;
	DataNode m_Value;

	Bool operator<(const TextDataStoreEntry& b) const
	{
		return (m_sKey < b.m_sKey);
	}
}; // struct TextDataStoreEntry

void TextDataStore(const DataStore& store, const DataNode& node);
void ValueText(Byte const* sPrefix, Byte const* sFormat, ...);

/** For data driven window scale - inverse since this makes more sense to a user. */
static const Float kfMinInverseWindowScale = 1.0f;
static const Float kfMaxInverseWindowScale = 3.0f;

} // namespace Seoul::DevUI::Util

#endif // /SEOUL_ENABLE_DEV_UI

#endif // include guard
