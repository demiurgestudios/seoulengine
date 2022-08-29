/**
 * \file CookPriority.h
 * \brief Fixed priorities for known BaseCookTask instances
 * included with the Cooking library.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef COOK_PRIORITY_H
#define COOK_PRIORITY_H

#include "Prereqs.h"

namespace Seoul::Cooking
{

namespace CookPriority
{

enum Enum
{
	// UI cooking is first, since it potentially outputs new
	// images to the source folder.
	kUI,

	// ScriptProject cooking must happen before Script, since
	// it will likely emit changed or new source .lua files.
	kScriptProject,

#if SEOUL_WITH_ANIMATION_2D
	// The following types have no particular order dependencies.
	kAnimation2D,
#endif // /#if SEOUL_WITH_ANIMATION_2D
	kScript,
	kProtobuf,
	kAudio,
	kEffect,
	kFxBank,
	kSceneAsset,
	kScenePrefab,
	kTexture,
	kFont,

	// Package cooking must last.
	kPackage,
};

} // namespace CookPriority

} // namespace Seoul::Cooking

#endif // include guard
