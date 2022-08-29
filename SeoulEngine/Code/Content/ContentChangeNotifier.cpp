/**
 * \file ContentChangeNotifier.cpp
 * \brief Content::ChangeNotifier is the common base class for platform/config specific
 * subclasses which monitor and dispatch events to Source/ and Data/Config/ content,
 * allowing the game to react to these events (typically, to recook and reload the
 * content).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentChangeNotifier.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_TYPE(Content::ChangeEvent);

namespace Content
{

ChangeNotifier::ChangeNotifier()
	: m_OutgoingContentChanges()
{
}

ChangeNotifier::~ChangeNotifier()
{
	// Destroy any remaining entries in the outgoing changes queue.
	ChangeEvent* pEvent = m_OutgoingContentChanges.Pop();
	while (nullptr != pEvent)
	{
		::Seoul::SeoulGlobalDecrementReferenceCount(pEvent);

		pEvent = m_OutgoingContentChanges.Pop();
	}
}

} // namespace Content

} // namespace Seoul
