/**
 * \file DevUIController.cpp
 * \brief Base interface for a controller component (in the
 * model-view-controller).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DevUIController.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_TYPE(DevUI::Controller, TypeFlags::kDisableNew)
SEOUL_TYPE(DevUI::NullController, TypeFlags::kDisableCopy)

} // namespace Seoul
