/**
 * \file FxStudioErrorHandler.cpp
 * \brief Class used to connect FxStudio error handling into SeoulEngine logging
 * and warning messaging.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FxStudioErrorHandler.h"
#include "Logger.h"

#if SEOUL_WITH_FX_STUDIO

namespace Seoul::FxStudio
{

/**
 * Constructor
 *
 * Registers this with FxStudio.  Keeps pointer to old error handler to register when this is destroyed
 *
 */
ErrorHandler::ErrorHandler()
	: m_OldErrorHandler(::FxStudio::GetErrorHandler())
{
	::FxStudio::RegisterErrorHandler(*this);
}

/**
 * Destructor
 *
 * Re-registers the previous error handler used by FxStudio
 *
 */
ErrorHandler::~ErrorHandler()
{
	::FxStudio::RegisterErrorHandler(m_OldErrorHandler);
}

/**
 * Helper function to indicate if we should pop-up a warning for a given
 *          FxStudio error message
 */
inline Bool WarnOnError(::FxStudio::ErrorData::Reason eReason)
{
	return true;
}

/**
 * Error handler used by FxStudio
 *
 * Error handler used by FxStudio.  Writes a warning message to the log.
 *
 * @param[in] errorData Error info from FxStudio
 */
void ErrorHandler::HandleError(const ::FxStudio::ErrorData& errorData)
{
	char sMessage[1024];
	errorData.CreateMessage(sMessage, sizeof(sMessage));

	if (WarnOnError(errorData.m_Reason))
	{
		SEOUL_WARN("FxStudio Error: %s\n", sMessage);
	}
	else
	{
		SEOUL_LOG_RENDER("FxStudio Error: %s\n", sMessage);
	}
}

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO
