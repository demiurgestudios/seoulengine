/**
 * \file AnimationNetworkDefinitionManager.h
 * \brief Global singleton that manages animation and network data in
 * the SeoulEngine content system.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION_NETWORK_DEFINITION_MANAGER_H
#define ANIMATION_NETWORK_DEFINITION_MANAGER_H

#include "AnimationNetworkDefinition.h"
#include "ContentStore.h"
#include "Delegate.h"
#include "FilePath.h"
#include "Singleton.h"
#include "Singleton.h"
namespace Seoul { namespace Animation { class EventInterface; } }
namespace Seoul { namespace Animation { class NetworkInstance; } }

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul::Animation
{

class NetworkDefinitionManager SEOUL_SEALED : public Singleton<NetworkDefinitionManager>
{
public:
	NetworkDefinitionManager();
	~NetworkDefinitionManager();

	/**
	 * @return A persistent Content::Handle<> to the network filePath.
	 */
	AnimationNetworkContentHandle GetNetwork(FilePath filePath)
	{
		return m_NetworkContent.GetContent(filePath);
	}

	/** @return True if the specified JSON file is loaded as an animation network. */
	Bool IsNetworkLoaded(FilePath filePath) const
	{
		return m_NetworkContent.IsFileLoaded(filePath);
	}

private:
	friend class NetworkContentLoader;
	Content::Store<NetworkDefinition> m_NetworkContent;

	SEOUL_DISABLE_COPY(NetworkDefinitionManager);
}; // class NetworkDefinitionManager

} // namespace Seoul::Animation

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

#endif // include guard
