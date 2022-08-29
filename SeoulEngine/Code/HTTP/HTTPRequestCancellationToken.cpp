/**
 * \file HTTPRequestCancellationToken.cpp
 * \brief Wrapper handle that allows a request to be cancelled before completion.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "HTTPManager.h"
#include "HTTPRequestCancellationToken.h"

namespace Seoul::HTTP
{

void RequestCancellationToken::Cancel()
{
	m_bCancelled = true;

	// Activate the Tick worker so it processes the request.
	(void)Manager::s_TickWorkerSignal.Activate();
	// Activate the API thread - in the curl implementation,
	// we may need to let progress callbacks run so it will
	// notice the now cancelled state of the request.
	(void)Manager::s_ApiSignal.Activate();
}

} // namespace Seoul::HTTP
