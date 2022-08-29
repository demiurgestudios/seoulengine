/**
 * \file ScriptEngineWordFilter.h
 * \brief Binder instance for exposing WordFilter functionality into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_ENGINE_WORDFILTER_H
#define SCRIPT_ENGINE_WORDFILTER_H

#include "Prereqs.h"
#include "ScopedPtr.h"
namespace Seoul { namespace Script { class FunctionInterface; } }
namespace Seoul { class FilePath; }
namespace Seoul { class WordFilter; }

namespace Seoul
{

/** Script binding for Seoul::WordFilter into a script VM. */
class ScriptEngineWordFilter SEOUL_SEALED
{
public:
	ScriptEngineWordFilter();
	void Construct(Script::FunctionInterface *pInterface);
	~ScriptEngineWordFilter();

	void FilterString(Script::FunctionInterface *pInterface);

private:
	WordFilter *m_pFilter;

	SEOUL_DISABLE_COPY(ScriptEngineWordFilter);
}; // class ScriptEngineWordFilter

} // namespace Seoul

#endif // include guard
