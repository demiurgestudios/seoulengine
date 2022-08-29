/**
 * \file CoreVirtuals.h
 * \brief Psuedo-vtable global that encapsulates miscellaneous up references
 * from Core into Seoul Engine libraries that otherwise depend on Core.
 *
 * TODO: Stop-gap until this bit bubbles up high enough in priority
 * to warrant a better design. Ideally, we refactor relevant functionality
 * so that no up references are needed (or so that those up references are
 * injected in a more typical/expected way - e.g. polymorphic children).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CORE_VIRTUALS_H
#define CORE_VIRTUALS_H

#include "Delegate.h"
#include "Prereqs.h"
#include "SeoulHString.h"
#include "SeoulString.h"
#include "SeoulTime.h"

namespace Seoul
{

/**
 * Enumeration of different buttons which can be displayed by message boxes
 *
 * NOTE: Must be kept in sync with the EMessageBoxButton enum in Java land.
 */
enum EMessageBoxButton
{
	// 1-button message box buttons
	kMessageBoxButtonOK,

	// 2-button message box buttons
	kMessageBoxButtonYes,
	kMessageBoxButtonNo,

	// 3-button message box buttons
	kMessageBoxButton1,
	kMessageBoxButton2,
	kMessageBoxButton3
};

/** Callback used to signal when a message box is closed. */
typedef Delegate<void (EMessageBoxButton eButton)> MessageBoxCallback;

/**
 * Interface for platform-specific global functions or Engine-level functions
 * which need to be callable from the Core project.
 */
struct CoreVirtuals SEOUL_SEALED
{
	/** Shows a platform-specific message box. */
	void (*ShowMessageBox)(
		const String& sMessage,
		const String& sTitle,
		MessageBoxCallback onCompleteCallback,
		EMessageBoxButton eDefaultButton,
		const String& sButtonLabel1,
		const String& sButtonLabel2,
		const String& sButtonLabel3);

	/**
	 * Localizes the given string, if the LocManager is available, or returns
	 * the default value.
	 */
	String (*Localize)(HString sLocToken, const String& sDefaultValue);

	/**
	 * @return A platform specific UUID for the current user+application (user UUID).
	 *
	 * \note If engine functionality for a persistent UUID is not available, the default
	 * will return a UUID that persistents only for the current session life (a process
	 * restart will return a new UUID).
	 */
	String(*GetPlatformUUID)();

	/**
	 * @return A platform dependent measurement of uptime. Can be system uptime or app uptime
	 * depending on platform (or even specific device). Expected, only useful as a baseline
	 * for measuring persistent delta time, unaffected by system clock changes or app sleep.
	 *
	 * \note If engine functionality for a "deep sleep" independent uptime is not available, the
	 * return value will be (Int64)Floor(SeoulTime::GetGameTime()), which obeys
	 * the requirements of GetUptime() except that it *will* be affected by any "deep sleep"
	 * on the current platform (e.g. "deep sleep" on Android, hibernate on Windows).
	 */
	TimeInterval(*GetUptime)();
};

// Default CoreVirtuals implementation, which can be used by any projects that
// depend on Core but not other engine projects.
extern const CoreVirtuals g_DefaultCoreVirtuals;

// Global function table for accessing platform-specific global functions from
// Core.  This must be defined in a dependent project to point a CoreVirtuals
// subclass instance to provide Engine-level functionality into Core.
extern const CoreVirtuals *g_pCoreVirtuals;

} // namespace Seoul

#endif // include guard
