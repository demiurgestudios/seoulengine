/**
 * \file NetworkConnectionType.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef NETWORK_CONNECTION_TYPE_H
#define NETWORK_CONNECTION_TYPE_H

#include "Prereqs.h"

namespace Seoul
{

// TODO: Don't send these unfiltered so we can remove
// the requirement that the enum can't be renamed.

// DO NOT rename these unless you need to - these symbols
// are sent to analytics as-is.
enum class NetworkConnectionType
{
	/** Unknown or unqueryable connection type. */
	kUnknown,

	/** Device is connected via Wi-Fi. */
	kWiFi,

	/** Device is connected via cellular data. */
	kMobile,

	/** Device is connected via a wired connection. */
	kWired,
};

} // namespace Seoul

#endif // include guard
