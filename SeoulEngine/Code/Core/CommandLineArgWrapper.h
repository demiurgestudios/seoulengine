/**
 * \file CommandLineArgWrapper.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef COMMAND_LINE_ARG_WRAPPER_H
#define COMMAND_LINE_ARG_WRAPPER_H

#include "Prereqs.h"

namespace Seoul
{

/** Utility wrapper for values set from the command-line - optional. If used, will record
 * whether a value has been set via the command-line or environment. Useful for types for
 * which the default value may also be a valid argument (e.g. empty string for Seoul::String). */
template <typename T>
class CommandLineArgWrapper SEOUL_SEALED
{
public:
	const T& Get() const { return m_v; }
	Bool IsSet() const { return m_b; }

	operator const T&() const
	{
		return m_v;
	}

	T& GetForWrite()
	{
		m_b = true;
		return m_v;
	}

	/**
	 * Index into the argv array passed to Reflection::CommandLineArgs::Parse()
	 * from which this arg was populated. Will be -1 if arg was not populated
	 * or if the arg is named and was populated from environment variables.
	 */
	Int32 GetCommandLineArgOffset() const
	{
		return m_iOffset;
	}

	/**
	 * Called from Reflection::CommandLineArgs::Parse() when viable (when this
	 * arg was populated from command-line args, not the environment.
	 */
	void SetCommandLineArgOffset(Int32 iOffset)
	{
		m_iOffset = iOffset;
	}

private:
	T m_v{};
	Int32 m_iOffset = -1;
	Bool m_b{};
}; // class CommandLineArgWrapper

} // namespace Seoul

#endif // include guard
