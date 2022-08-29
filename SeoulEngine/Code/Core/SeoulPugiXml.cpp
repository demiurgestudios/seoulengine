/**
 * \file SeoulPugiXml.cpp
 * \brief SeoulEngine wrapper around pugixml. We don't wrap the API
 * itself like most integrations because there's just too much of it.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Prereqs.h"
#include "SeoulPugiXml.h"

// Define.
#if !defined(PUGIXML_HEADER_ONLY) && !defined(PUGIXML_SOURCE)
#	define PUGIXML_SOURCE <pugixml.inl>
#	include PUGIXML_SOURCE
#endif

// Configure memory hooks.
namespace
{

static void* Allocate(size_t z)
{
	return Seoul::MemoryManager::Allocate(z, Seoul::MemoryBudgets::Strings);
}

static void Deallocate(void* p)
{
	return Seoul::MemoryManager::Deallocate(p);
}

struct Init
{
	Init()
	{
		pugi::set_memory_management_functions(Allocate, Deallocate);
	}
};
static const Init s_Init;

} // anonymous namespace
