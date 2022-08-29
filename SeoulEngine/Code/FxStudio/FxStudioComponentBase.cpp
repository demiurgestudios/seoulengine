/**
 * \file FxStudioComponentBase.cpp
 * \brief Base class for all SeoulEngine types that will implement
 * the FxStudio::Component interface.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FxStudioComponentBase.h"

#if SEOUL_WITH_FX_STUDIO

namespace Seoul::FxStudio
{

ComponentBase::ComponentBase(const InternalDataType& internalData)
	: ::FxStudio::Component(internalData)
{
}

ComponentBase::~ComponentBase()
{
}

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO
