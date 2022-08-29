/**
 * \file FxStudioErrorHandler.h
 * \brief Class used to connect FxStudio error handling into SeoulEngine logging
 * and warning messaging.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FX_STUDIO_ERROR_HANDLER_H
#define FX_STUDIO_ERROR_HANDLER_H

#include "Prereqs.h"

#if SEOUL_WITH_FX_STUDIO
#include "FxStudioRT.h"

namespace Seoul::FxStudio
{

class ErrorHandler SEOUL_SEALED : public ::FxStudio::ErrorHandler
{
public:
	ErrorHandler();
	virtual ~ErrorHandler();

	// Handles errors
	virtual void HandleError(const ::FxStudio::ErrorData& errorData);

private:
	// Cannot be copied.
	SEOUL_DISABLE_COPY(ErrorHandler);

	// Data...

	::FxStudio::ErrorHandler& m_OldErrorHandler;
}; // class FxStudio::ErrorHandler

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO

#endif // include guard
