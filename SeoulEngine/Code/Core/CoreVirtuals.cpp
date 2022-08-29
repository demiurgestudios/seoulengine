/**
 * \file CoreVirtuals.cpp
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

#include "CoreVirtuals.h"
#include "SeoulTime.h"
#include "SeoulUUID.h"

namespace Seoul
{

/**
 * Default implementation of ShowMessageBox() -- noop
 */
static void DefaultShowMessageBox(
	const String& sMessage,
	const String& sTitle,
	MessageBoxCallback onCompleteCallback,
	EMessageBoxButton eDefaultButton,
	const String& sButtonLabel1,
	const String& sButtonLabel2,
	const String& sButtonLabel3)
{
	if (onCompleteCallback)
	{
		onCompleteCallback(eDefaultButton);
	}
}

/**
 * Default implementation of Localize() -- returns default value
 */
static String DefaultLocalize(HString sLocToken, const String& sDefaultValue)
{
	return sDefaultValue;
}

/**
 * Default implementation of GetPlatformUUID() - returns a process persisted value.
 */
static String DefaultGetPlatformUUID()
{
	static const String s_UUID = UUID::GenerateV4().ToString();
	return s_UUID;
}

/**
 * Default implementation of GetUptime() - returns SeoulTime::GetGameTime(),
 * which *can* be affected by "deep sleep"/hibernate.
 */
static TimeInterval DefaultGetUptime()
{
	return TimeInterval::FromMicroseconds((Int64)Floor(SeoulTime::GetGameTimeInMicroseconds()));
}

/**
 * Default implementation of core platform functions
 */
const CoreVirtuals g_DefaultCoreVirtuals =
{
	&DefaultShowMessageBox,
	&DefaultLocalize,
	&DefaultGetPlatformUUID,
	&DefaultGetUptime,
};

} // namespace Seoul
