/**
 * \file SceneNavigationGridComponent.h
 * \brief Defines a navigable 2D grid, using the Navigation project,
 * situated in a 3D scene.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_NAVIGATION_GRID_COMPONENT_H
#define SCENE_NAVIGATION_GRID_COMPONENT_H

#include "SceneComponent.h"
namespace Seoul { namespace Navigation { class CoverageRasterizer; } }
namespace Seoul { namespace Navigation { class Grid; } }
namespace Seoul { namespace Navigation { struct Position; } }
namespace Seoul { namespace Navigation { class Query; } }
namespace Seoul { namespace Navigation { class QueryState; } }
namespace Seoul { namespace Scene { class PrimitiveRenderer; } }
namespace Seoul { namespace Scene { namespace NavUtil { struct RobustData; } } }

#if SEOUL_WITH_NAVIGATION && SEOUL_WITH_SCENE

namespace Seoul::Scene
{

class NavigationGridComponentSharedState SEOUL_SEALED
{
public:
	static NavigationGridComponentSharedState* CreateFromGridData(const String& sGridData);
	static NavigationGridComponentSharedState* CreateNewHeight(const SharedPtr<NavigationGridComponentSharedState>& pExisting, UInt32 uHeight);
	static NavigationGridComponentSharedState* CreateNewWidth(const SharedPtr<NavigationGridComponentSharedState>& pExisting, UInt32 uWidth);

	NavigationGridComponentSharedState(
		const NavigationGridComponentSharedState& existing,
		const Navigation::CoverageRasterizer& rasterizer);
	~NavigationGridComponentSharedState();

	UInt8 GetCell(UInt32 uX, UInt32 uY) const;
	UInt32 GetHeight() const;
	UInt32 GetWidth() const;
	UInt8* Save(UInt32& ruDataSizeInBytes) const;

	Bool RobustFindStraightPath(
		Navigation::QueryState& rState,
		const Navigation::Position& start,
		const Navigation::Position& end,
		UInt32 uMaxStartDistance,
		UInt32 uMaxEndDistance) const;

private:
	Navigation::Grid* m_pGrid;
	ScopedPtr<Navigation::Query> m_pQuery;

	explicit NavigationGridComponentSharedState(Navigation::Grid* pGrid);

	SEOUL_DISABLE_COPY(NavigationGridComponentSharedState);
	SEOUL_REFERENCE_COUNTED(NavigationGridComponentSharedState);
}; // class NavigationGridComponentSharedState

class NavigationGridQuery SEOUL_SEALED
{
public:
	~NavigationGridQuery();

	Bool GetPoint(UInt32 u, Vector3D& rv) const;

	Bool IsDone() const
	{
		return m_bDone;
	}

	Bool WasSuccessful() const
	{
		return (m_bDone && m_bSuccess);
	}

private:
	friend class NavigationGridComponent;
	NavigationGridQuery(
		const SharedPtr<NavigationGridComponentSharedState>& pShared,
		const Matrix4D& mTransform);

	SharedPtr<NavigationGridComponentSharedState> const m_pShared;
	Matrix4D const m_mTransform;
	ScopedPtr<Navigation::QueryState> m_pState;
	Bool m_bSuccess;
	Atomic32Value<Bool> m_bDone;

	static void RobustQueryJob(const NavUtil::RobustData& data);

	SEOUL_DISABLE_COPY(NavigationGridQuery);
	SEOUL_REFERENCE_COUNTED(NavigationGridQuery);
}; // class NavigationGridQuery

class NavigationGridComponent SEOUL_SEALED : public Scene::Component
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(NavigationGridComponent);

	NavigationGridComponent();
	~NavigationGridComponent();

	virtual SharedPtr<Component> Clone(const String& sQualifier) const SEOUL_OVERRIDE;

	void GenerateNavigation(Interface& rInterface);

	// Issues an (asynchronous) straight path query. Semantics
	// are the same as Navigation::Query::RobustFindStraightPath(),
	// with one addition:
	// - can return false if the contained navgrid is not configured
	//   properly to accept queries. rpQuery will be left unchanged
	//   in this case.
	Bool RobustFindStraightPath(
		const Vector3D& vStart,
		const Vector3D& vEnd,
		UInt32 uMaxStartDistance,
		UInt32 uMaxEndDistance,
		SharedPtr<NavigationGridQuery>& rpQuery) const;

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(NavigationGridComponent);
	SEOUL_REFLECTION_FRIENDSHIP(NavigationGridComponent);

	SharedPtr<NavigationGridComponentSharedState> m_pShared;

	void Project(
		const Vector3D& vStart,
		const Vector3D& vEnd,
		Navigation::Position& rStart,
		Navigation::Position& rEnd) const;

#if SEOUL_EDITOR_AND_TOOLS
	// Button hook in the editor.
	void EditorGenerateNavigation(Interface* pInterface);

	// Hook for rendering in the editor.
	void EditorDrawPrimitives(Scene::PrimitiveRenderer* pRenderer) const;
#endif // /#if SEOUL_EDITOR_AND_TOOLS

	String GetGridData() const;
	void SetGridData(const String& sGridData);

	UInt32 GetHeight() const;
	void SetHeight(UInt32 uHeight);
	UInt32 GetWidth() const;
	void SetWidth(UInt32 uWidth);

	SEOUL_DISABLE_COPY(NavigationGridComponent);
}; // class NavigationGridComponent

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_NAVIGATION && SEOUL_WITH_SCENE

#endif // include guard
