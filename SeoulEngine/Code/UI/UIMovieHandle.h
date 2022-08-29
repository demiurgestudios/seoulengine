/**
 * \file UIMovieHandle.h
 * \brief Specialization of AtomicHandle<> for UI::Movie, allows thread-
 * safe, weak referencing of UI::Movie instances.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_MOVIE_HANDLE_H
#define UI_MOVIE_HANDLE_H

#include "AtomicHandle.h"
#include "CheckedPtr.h"
namespace Seoul { namespace UI { class Movie; } }

namespace Seoul
{

namespace UI { typedef AtomicHandle<Movie> MovieHandle; }
namespace UI { typedef AtomicHandleTable<Movie> MovieHandleTable; }

/** Conversion to pointer convenience functions. */
template <typename T>
inline CheckedPtr<T> GetPtr(UI::MovieHandle h)
{
	CheckedPtr<T> pReturn(static_cast<T*>((UI::Movie*)UI::MovieHandleTable::Get(h)));
	return pReturn;
}

static inline CheckedPtr<UI::Movie> GetPtr(UI::MovieHandle h)
{
	CheckedPtr<UI::Movie> pReturn((UI::Movie*)UI::MovieHandleTable::Get(h));
	return pReturn;
}

} // namespace Seoul

#endif  // include guard
