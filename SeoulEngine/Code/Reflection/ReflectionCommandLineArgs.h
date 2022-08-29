/**
 * \file ReflectionCommandLineArgs.h
 * \brief Reflection driven utility for reading, enumerating, and printing
 * command-line arguments loaded from the literal command-line or the system
 * environment.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_COMMAND_LINE_ARGS_H
#define REFLECTION_COMMAND_LINE_ARGS_H

#include "Prereqs.h"

namespace Seoul::Reflection
{

/** Command-line arguments are specified using CommandLineArg
 *  reflection attributes on static members of classes. See
 *  ReflectionAttributes.h for more information. */
class CommandLineArgs SEOUL_SEALED
{
public:
	// Variations to load command-line arguments. Should be called once
	// at startup. Note that this function assumes the input is the full
	// command-line - it will fill-in any missing args from the system
	// environment and then return false if required arguments are missing.
	static Bool Parse(String const* iBegin, String const* iEnd);
	static Bool Parse(Byte const* const* iBegin, Byte const* const* iEnd);
	static Bool Parse(wchar_t const* const* iBegin, wchar_t const* const* iEnd);

	static void PrintHelp();

private:
	CommandLineArgs() = delete; // Static class.
	SEOUL_DISABLE_COPY(CommandLineArgs);
}; // class CommandLineArgs

} // namespace Seoul::Reflection

#endif // include guard
