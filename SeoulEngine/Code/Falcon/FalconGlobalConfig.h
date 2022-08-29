/**
 * \file FalconGlobalConfig.h
 * \brief Global accessors and up references for Falcon.
 *
 * The GlobalConfig must be set once before any Falcon functionality
 * is used and cannot be destroyed until all Falcon instances are
 * released.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_GLOBAL_CONFIG_H
#define FALCON_GLOBAL_CONFIG_H

#include "Prereqs.h"
#include <stdio.h>
#include <stdarg.h>
namespace Seoul { class FilePath; }
namespace Seoul { class HString; }
namespace Seoul { namespace Falcon { class FCNLibraryAnchor; } }
namespace Seoul { namespace Falcon { struct Font; } }
namespace Seoul { namespace Falcon { class Stage3DSettings; } }
namespace Seoul { namespace Falcon { struct TextEffectSettings; } }

namespace Seoul::Falcon
{

typedef Bool (*GetFCNFileCallback)(
	const HString& sBaseURL,
	const HString& sURL,
	FCNLibraryAnchor*& rpFCNFileAnchor);
typedef Bool (*GetFontCallback)(
	const HString& sFontName,
	Bool bBold,
	Bool bItalic,
	Font& rFont);
typedef Stage3DSettings const* (*GetStage3DSettingsCallback)(
	const HString& sStage3DSettings);
typedef TextEffectSettings const* (*GetTextEffectSettingsCallback)(
	const HString& sTextEffectSettings);
typedef Bool (*ResolveImageSourceCallback)(HString baseURL, Byte const* sURL, FilePath& rID, Int32& riOptionalWidth, Int32& riOptionalHeight);

// Culling is a special feature that can be enabled
// per MovieClipInstance. It automatically detects sub nodes
// that are outside the rendering world culling region and
// disables them - they are removed from advance, display, and
// hit test processing. This is only valuable for nodes with
// many children, most of which will be outside the world
// culling region (a "scrolling" MovieClip).
//
// "Auto culling" is a utility feature. When enabled on a
// MovieClip, the MovieClip automatically evaluates itself
// and a limited depth of its children each frame, and
// enables/disables culling as appropriate. This allows
// culling to react to dynamically changing graphs
// of children nodes.
//
// AutoCullingConfig controls the global behavior of
// auto culling when it is enabled on a MovieClip.
struct AutoCullingConfig
{
	AutoCullingConfig()
		: m_uMaxTraversalDepth(2)
		, m_uMinChildCountForCulling(5)
		, m_uMaxChildCountForTraversal(8)
	{
	}

	// Max traversal depth is the maximum distance
	// from where auto culling was started that it
	// will traverse to enable/disable culling on
	// children nodes. Traversal stops at depths
	// greater than this value.
	UInt32 m_uMaxTraversalDepth;

	// Culling will be enabled on nodes with at least
	// this many children.
	UInt32 m_uMinChildCountForCulling;

	// In additional to max traversal depth, traversal
	// to evaluate culling will stop if a node has
	// more than this many children.
	UInt32 m_uMaxChildCountForTraversal;
}; // struct AutoCullingConfig

struct GlobalConfig
{
	GlobalConfig();

	AutoCullingConfig m_AutoCullingConfig;
	GetFCNFileCallback m_pGetFCNFile;
	GetFontCallback m_pGetFont;
	GetStage3DSettingsCallback m_pGetStage3DSettings;
	GetTextEffectSettingsCallback m_pGetTextEffectSettings;
	ResolveImageSourceCallback m_pResolveImageSource;
}; // struct GlobalConfig

extern GlobalConfig g_Config;

static inline void GlobalInit(const GlobalConfig& config)
{
	g_Config = config;
}

static inline void GlobalShutdown()
{
	g_Config = GlobalConfig();
}

} // namespace Seoul::Falcon

#endif // include guard
