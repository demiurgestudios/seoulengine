/**
 * \file BuildFeatures.h
 * \brief Single file to control whether certain engine features
 * are enabled/disabled for the current application. This is closely
 * equivalent to a file that is typically called "config.h" in many
 * OSS libraries, but *Config.h was already fairly overloaded in
 * SeoulEngine, so instead BuildFeatures.h was chosen.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef BUILD_FEATURES_H
#define BUILD_FEATURES_H

///////////////////////////////////////////////////////////////////////////////
//
// NOTE: This file is manually parsed by build systems other than the C
// preprocessor (for example, some of our .csx build scripts as well as
// .gradle files used for the Android build). These other environments
// are generally not proper preprocessors and are using simple find/replace
// or regular expressions. As such, strictly in this file:
// - limit this file to simple #define build flags.
// - expressions of the flags should be simple - limited to platform macros
//   or constant values. For example:
//     #define SEOUL_FOO (SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_WINDOWS)
//   not:
//     #if SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_WINDOWS
//     #    define SEOUL_FOO 1
//     #else ...
///////////////////////////////////////////////////////////////////////////////

// Exception to the above rules - build feature. Defines the name of the root
// folder for the current project ("App" once we standardized, project specific before that)
#define SEOUL_APP_ROOT_NAME "App"

// Exception to the above rules - build feature. Define the name of the company
// used in the save game path.
#define SOEUL_APP_SAVE_COMPANY_DIR "Demiurge Studios"

// Exception to the above rules - build feature. Defines the name of the nested
// folder used for user save data. The full path will be <platform-specific-user-storage>/SOEUL_APP_SAVE_COMPANY_DIR/SEOUL_APP_SAVE_DIR
#define SEOUL_APP_SAVE_DIR "SETestApp"

// Android specific configuration, exceptions to the above rules (string values explicitly).
#define SEOUL_LOCAL_NOTIFICATION_CHANNEL_ID "setestapp"
#define SEOUL_LOCAL_NOTIFICATION_CHANNEL_NAME "SETestApp"
#define SEOUL_LOCAL_NOTIFICATION_ICON_NAME "notification_icon"
#define SEOUL_LOCAL_NOTIFICATION_ACTION_BROADCAST_LOCAL "com.demiurgestudios.setestapp.BROADCAST_LOCAL"
#define SEOUL_LOCAL_NOTIFICATION_ACTION_POST_NOTIFICATION "com.demiurgestudios.setestapp.POST_NOTIFICATION"
#define SEOUL_LOCAL_NOTIFICATION_ACTION_CONSUME_NOTIFICATION "com.demiurgestudios.setestapp.CONSUME_NOTIFICATIONS"
#define SEOUL_LOCAL_NOTIFICATION_AFSENDER ""
#define SEOUL_ANDROID_LAUNCH_PACKAGE "com.demiurgestudios.setestapp"
#define SEOUL_ANDROID_LAUNCH_NAME "com.demiurgestudios.setestapp.AppAndroid"
#define SEOUL_NATIVE_ANDROID_LIBNAME "AppAndroid"

// Exception to the above rules - minimum SWF version of the current project.
// See also: UICookTask.cpp and FalconCooker.cs
#define SEOUL_MIN_SWF_VERSION 32 // Flash Player 21.0

// Flag controlling inclusion of Amazon first-party support.
#define SEOUL_WITH_AMAZON 0 // SEOUL_PLATFORM_ANDROID

// Flag controlling inclusion of Samsung first-party support.
#define SEOUL_WITH_SAMSUNG 0

// Flag controlling inclusion of 2D animation support.
#define SEOUL_WITH_ANIMATION_2D 0

// Flag controlling inclusion of 3D animation support.
#define SEOUL_WITH_ANIMATION_3D 1

// Flag controlling usage of the AppsFlyer library.
#define SEOUL_WITH_APPS_FLYER 0 // (SEOUL_PLATFORM_ANDROID | SEOUL_PLATFORM_IOS)

// Flag controlling libcurl usage
#define SEOUL_WITH_CURL (SEOUL_PLATFORM_WINDOWS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX)

// Flag controlling platform-specific Facebook SDK integration
#define SEOUL_WITH_FACEBOOK 0 // (SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID)

// Flag controlling inclusion of FxStudio curves and visual FX support.
#define SEOUL_WITH_FX_STUDIO 0

// Flag controlling platform-specific Google Analytics SDK integration
#define SEOUL_WITH_GOOGLE_ANALYTICS 0 // (SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS)

// Flag controlling whether persistence (game persistence) is available in this build of SeoulEngine.
#define SEOUL_WITH_GAME_PERSISTENCE 0

// Flag controlling the presence of GameCenter auth.
#define SEOUL_WITH_GAMECENTER SEOUL_PLATFORM_IOS

// Flag controlling the presence of Apple Sign-In auth.
#define SEOUL_WITH_APPLESIGNIN SEOUL_PLATFORM_IOS

// Flag controlling inclusion of FMOD audio.
#define SEOUL_WITH_FMOD 0 // (SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_WINDOWS)

// Flag controlling the presence of Google Play games auth.
#define SEOUL_WITH_GOOGLEPLAYGAMES SEOUL_PLATFORM_ANDROID

// Flag controlling platform-specific HelpShift integration
#define SEOUL_WITH_HELPSHIFT 0 // (SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS)

// Flag controlling inclusion of navigation support.
#define SEOUL_WITH_NAVIGATION 1

// Flag controlling inclusion of network support - this is synchronous
// networking, not (e.g.) HTTP support.
#define SEOUL_WITH_NETWORK 1

// Flag controlling OpenSSL usage
#define SEOUL_WITH_OPENSSL (SEOUL_PLATFORM_WINDOWS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX)

// Flag controlling inclusion of 3D physics support.
#define SEOUL_WITH_PHYSICS 1

// Flag controlling whether remote notification support is enabled.
#define SEOUL_WITH_REMOTE_NOTIFICATIONS (SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID)

// Flag controlling inclusion of 3D scene support.
#define SEOUL_WITH_SCENE 1

// Flag controlling inclusion of the Demiplane server browser
#define SEOUL_WITH_SERVER_BROWSER 0

// When true, SteamEngine will be used on compatible platforms.
#define SEOUL_WITH_STEAM 0

// Flag controlling urlsession (Apple HTTP) usage
#define SEOUL_WITH_URLSESSION SEOUL_PLATFORM_IOS

// Flag enabling or disabling implicit definition of
// reflection data for templated types.
//
// When enabled, use of SEOUL_SPEC_TEMPLATE_TYPE is not
// needed (the macro is defined to empty). Reflection
// hooks for templated types will be implicitly defined
// in compilation units (cpp files) that use the type
// in some way (e.g. as a property of another reflection
// definition).
//
// In general, setting this flag to 1 simplifies usage
// of reflection (specialized types that are reflected
// do not need to be explicitly enumerated with
// SEOUL_SPEC_TEMPLATE_TYPE macros) but it will also
// increase compilation times (possibly significantly)
// due to redundant template specialization/definition
// bloat.
//
// When defined to 1, template definitions are emitted
// in compilation units that reference the specialized
// type, likely many redundant times (the same definition
// will be emitted in many cpp files then resolved
// as redundant by the linker). When defined to 0,
// definition emit is controlled by explicitly enumerating
// the specializations with SEOUL_SPEC_TEMPLATE_TYPE,
// so that a particular specialization definition only exists
// in a single compilation unit (cpp file).
#define SEOUL_IMPLICIT_TEMPLATED_REFLECTION_DEFINITION 1

#endif // include guard
