/**
 * \file UIData.h
 * \brief Miscellaneous data types of the UI project.
 *
 * UI::Data contains mostly low-level data abstractions for integration
 * of the Falcon project into the UI project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_DATA_H
#define UI_DATA_H

#include "ContentHandle.h"
#include "FalconFCNFile.h"
#include "FalconTypes.h"
#include "Geometry.h"
#include "Matrix3D.h"
#include "Prereqs.h"
#include "ReflectionDeclare.h"
#include "Vector.h"
#include "Viewport.h"
namespace Seoul { namespace Falcon { class EditTextInstance; } }
namespace Seoul { namespace Falcon { class MovieClipInstance; } }

namespace Seoul
{

// Forward declarations
class BaseTexture;
namespace UI
{

class Movie;
class FCNFileData;

} // namespace UI

namespace Content
{

/**
 * Specialization of Content::Traits<> for FCNFileData, allows FCNFiles
 * to be managed as loadable content by the content system.
 */
template <>
struct Traits<UI::FCNFileData>
{
	typedef FilePath KeyType;
	static const Bool CanSyncLoad = false;

	static SharedPtr<UI::FCNFileData> GetPlaceholder(FilePath filePath);
	static Bool FileChange(FilePath filePath, const Handle<UI::FCNFileData>& hEntry);
	static void Load(FilePath filePath, const Handle<UI::FCNFileData>& hEntry);
	static Bool PrepareDelete(FilePath filePath, Entry<UI::FCNFileData, KeyType>& entry);
	static void SyncLoad(FilePath filePath, const Handle<UI::FCNFileData>& pEntry) {}
	static UInt32 GetMemoryUsage(const SharedPtr<UI::FCNFileData>& p) { return 0u; }
}; // struct Content::Traits<FCNFileData>

/**
 * Specialization of Content::Traits<> for Falcon::CookedTrueTypeFontData, allows Falcon::CookedTrueTypeFontData
 * to be managed as loadable content by the content system.
 */
template <>
struct Traits<Falcon::CookedTrueTypeFontData>
{
	typedef FilePath KeyType;
	static const Bool CanSyncLoad = false;

	static SharedPtr<Falcon::CookedTrueTypeFontData> GetPlaceholder(KeyType key);
	static Bool FileChange(KeyType key, const Handle<Falcon::CookedTrueTypeFontData>& pEntry);
	static void Load(KeyType key, const Handle<Falcon::CookedTrueTypeFontData>& pEntry);
	static Bool PrepareDelete(KeyType key, Entry<Falcon::CookedTrueTypeFontData, KeyType>& entry);
	static void SyncLoad(FilePath filePath, const Handle<Falcon::CookedTrueTypeFontData>& pEntry) {}
	static UInt32 GetMemoryUsage(const SharedPtr<Falcon::CookedTrueTypeFontData>& p) { return 0u; }
}; // struct Content::Traits<Falcon::CookedTrueTypeFontData>

} // namespace Content

namespace UI
{

enum class InputEvent
{
	kUnknown,
	kAction,
	kBackButton,
	kDone
};

enum class MovieHitTestResult
{
	/** Used in cases where you want to close the panel when you tap outside of it */
	kNoHitTriggerBack = -2,
	/** Used in cases where a hit did not occur, but a movie wants to prevent hits against movies below it. */
	kNoHitStopTesting = -1,

	/** Used to report no hit occurred. */
	kNoHit,

	/** Used to report that a hit occurred. */
	kHit,
};

/**
 * Developer only utility. Point and associated data used
 * to track potentially hit testable points from the current
 * Falcon scene state.
 */
struct HitPoint SEOUL_SEALED
{
	HitPoint();
	HitPoint(const HitPoint& b);
	HitPoint& operator=(const HitPoint& b);
	~HitPoint();

	Bool operator==(const HitPoint& b) const
	{
		return
			(m_vTapPoint == b.m_vTapPoint) &&
			(m_vCenterPoint == b.m_vCenterPoint) &&
			(m_StateMachine == b.m_StateMachine) &&
			(m_State == b.m_State) &&
			(m_DevOnlyInternalStateId == b.m_DevOnlyInternalStateId) &&
			(m_Class == b.m_Class) &&
			(m_Id == b.m_Id);
	}

	SharedPtr<Falcon::MovieClipInstance> m_pInstance;
	Vector2D m_vTapPoint;
	Vector2D m_vCenterPoint;
	HString m_StateMachine;
	HString m_State;
	HString m_DevOnlyInternalStateId;
	HString m_Movie;
	HString m_Class;
	HString m_Id;
}; // struct UI::HitPoint

} // namespace UI

} // namespace Seoul

#endif // include guard
