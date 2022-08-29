/**
* \file UIMotion.cpp
* \brief A UI::Motion is applied to a Falcon::Instance to perform
* runtime custom movement of the instance
*
* Copyright (c) Demiurge Studios, Inc.
* 
* This source code is licensed under the MIT license.
* Full license details can be found in the LICENSE file
* in the root directory of this source tree.
*/

#include "UIMotion.h"
#include "ReflectionDefine.h"
#include "FalconMovieClipInstance.h"

namespace Seoul
{

SEOUL_TYPE(UI::Motion)

namespace UI
{

Motion::Motion()
	: m_pInstance()
	, m_pCompletionInterface()
	, m_iIdentifier(0)
{
}

Motion::~Motion()
{
}

} // namespace UI

} // namespace Seoul
