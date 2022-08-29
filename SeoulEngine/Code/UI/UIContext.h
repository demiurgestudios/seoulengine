/**
 * \file UIContext.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_CONTEXT_H
#define UI_CONTEXT_H

#include "Prereqs.h"
#include "Viewport.h"
namespace Seoul { class DataStoreTableUtil; }
namespace Seoul { class IPoseable; }
namespace Seoul { class String; }

namespace Seoul
{

namespace UI
{

struct Context SEOUL_SEALED
{
	void(*DisplayNotification)(const String& sMessage);
	void(*DisplayTrackedNotification)(const String& sMessage, Int32& riId);
	void(*KillNotification)(Int32 iId);
	Viewport(*GetRootViewport)();
	IPoseable* (*SpawnUIManager)(const DataStoreTableUtil& configSettings, Bool& rbRenderPassOwnsPoseable);
}; // struct UI::Context
extern Context g_UIContext;

} // namespace UI

} // namespace namespace Seoul

#endif // include guard
