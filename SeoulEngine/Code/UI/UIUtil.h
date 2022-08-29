/**
 * \file UIUtil.h
 * \brief Constants, macros, and basic functions to help with integration
 * of the Falcon project into the UI project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_UTIL_H
#define UI_UTIL_H

#include "FixedArray.h"
#include "Prereqs.h"
#include "InputKeys.h"
#include "ReflectionDeclare.h"
#include "ReflectionMethod.h"
#include "ReflectionMethodTypeInfo.h"
#include "ReflectionType.h"

namespace Seoul::FalconConstants
{

/** Method name of the Lua method in the root movie clip that will be called on a binding pressed event. */
static const HString kBindingButtonEventDown("OnBindingDown");

/** Method name of the Lua method in the root movie clip that will be called on a binding released event. */
static const HString kBindingButtonEventUp("OnBindingUp");

/** Key used to lookup camera settings. */
static const HString kCamera("Camera");

/** Key of an array of TTF fonts that can be used to override embedded flash fonts. */
static const HString kFonts("Fonts");

/** Key of font remapping, maps an embedded font name to an alternative name. */
static const HString kFontAliases("FontAliases");

/** Key of font settings, key on the name of the font. */
static const HString kFontSettings("FontSettings");

/** FX associated with a UI movie. */
static const HString kFx("FX");

/** When false, FXFactory preloads all FX it defines. Otherwise, they are manually preloaded or loaded on demand. */
static const HString kPreloadFx("PreloadFX");

/** Key used to lookup the array of movies that construct an UI::State's Flash Movie stack. */
static const HString kMoviesTableKey("Movies");

/** Key used to lookup the list of conditions to set when entering an UIState. */
static const HString kOnEnterConditionsTableKey("OnEnterConditions");

/** Key used to lookup the list of conditions to set when exiting an UIState. */
static const HString kOnExitConditionsTableKey("OnExitConditions");

/** Key used to lookup the file path associated with a movie in gui.json. */
static const HString kMovieFilePath("MovieFilePath");

/**
 * Key used to define whether a movie uses a native instance (C++ class
 * created via reflection) or a custom instance (application specific backing, typically
 * script).
 */
static const HString kNativeMovieInstance("NativeMovie");

/** Table lookup for the table of settings file (json files) dependencies of a movie. */
static const HString kSettings("Settings");

/** Table lookup for the array of StateMachines listed in gui.json. */
static const HString kStateMachines("StateMachines");

/** Starting StateMachine for UI::Manager InputWhiteList processing. */
static const HString kInputWhiteListBeginsAtState("InputWhiteListBeginsAtState");

/** Sound::Events associated with a UI movie. */
static const HString kSoundEvents("SoundEvents");

/** Shared Sound::Events associated with all UI movies. */
static const HString kSharedMovieSoundEvents("SharedMovieSoundEvents");

/** Array of sound duckers associated with a UI movie. */
static const HString kSoundDuckers("SoundDuckers");

/** State specific value that specifies whether a state suppresses occlusion optimization. */
static const HString kSuppressOcclusionOptimizer("SuppressOcclusionOptimizer");

/** Table lookup for the table of image substitution dimensions listed in gui.json. */
static const HString kTextureSubstitutions("TextureSubstitutions");

/** True/false values used by UI::Manager to pack condition variable updates. */
static const HString kTrue("true");
static const HString kFalse("false");

static const HString kFontBold("bold");
static const HString kFontItalic("italic");
static const HString kFontRegular("regular");

static const HString kFontAscent("Ascent");
static const HString kFontDescent("Descent");
static const HString kFontLineGap("LineGap");
static const HString kFontRescale("Rescale");

/**
 * Entry in a Transition= in a UI state machine that
 * applies modifications to condition variables after the transition
 * is activated.
 */
static const HString kModifyConditionsTableEntry("ModifyConditions");

/**
 * Entry in a Transition= in a UI state machine that,
 * if set to true, the transition will "capture" the Trigger
 * that activated the Transition, preventing it from
 * activating transitions in state machines lower in the
 * UI stack.
 */
static const HString kCaptureTriggers("CaptureTriggers");

/**
 * HString that corresponds to an ENTER_FRAME event.
 */
static const HString kEnterFrame("enterFrame");

/**
 * HString that corresponds to a TICK event.
 */
static const HString kTickEvent("tick");

/**
* HString that corresponds to a time-scaled TICK event.
*/
static const HString kTickScaledEvent("tickScaled");

} // namespace Seoul::FalconConstants

#endif // include guard
