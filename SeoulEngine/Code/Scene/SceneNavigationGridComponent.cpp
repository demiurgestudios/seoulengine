/**
 * \file SceneNavigationGridComponent.cpp
 * \brief Defines a navigable 2D grid, using the Navigation project,
 * situated in a 3D scene.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "JobsFunction.h"
#include "NavigationCoverageRasterizer.h"
#include "NavigationGrid.h"
#include "NavigationQuery.h"
#include "NavigationQueryState.h"
#include "ReflectionDefine.h"
#include "SceneInterface.h"
#include "SceneNavigationGridComponent.h"
#include "SceneObject.h"
#include "ScenePrefabComponent.h"
#include "ScenePrimitiveRenderer.h"
#include "SceneRigidBodyComponent.h"
#include "StringUtil.h"

#if SEOUL_WITH_NAVIGATION && SEOUL_WITH_SCENE

namespace Seoul
{

static const HString kEffectTechniqueRenderNoDepthTest("seoul_RenderNoDepthTest");

SEOUL_BEGIN_TYPE(Scene::NavigationGridComponent, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(DisplayName, "Navigation Grid")
	SEOUL_ATTRIBUTE(Category, "Navigation")
	SEOUL_PARENT(Scene::Component)
	SEOUL_PROPERTY_PAIR_N("Data", GetGridData, SetGridData)
		SEOUL_ATTRIBUTE(DoNotEdit)
	SEOUL_PROPERTY_PAIR_N("Height", GetHeight, SetHeight)
		SEOUL_ATTRIBUTE(DoNotSerialize)
	SEOUL_PROPERTY_PAIR_N("Width", GetWidth, SetWidth)
		SEOUL_ATTRIBUTE(DoNotSerialize)

#if SEOUL_EDITOR_AND_TOOLS
	SEOUL_METHOD(EditorDrawPrimitives)
	SEOUL_METHOD_N("Generate Navigation", EditorGenerateNavigation)
		SEOUL_ATTRIBUTE(EditorButton, "Data")
#endif // /#if SEOUL_EDITOR_AND_TOOLS

SEOUL_END_TYPE()

namespace Scene
{

NavigationGridComponentSharedState* NavigationGridComponentSharedState::CreateFromGridData(const String& sGridData)
{
	Vector<Byte> vData;
	if (!Base64Decode(sGridData, vData) || vData.IsEmpty())
	{
		return nullptr;
	}

	auto pGrid = Navigation::Grid::CreateFromFileInMemory(
		(UInt8 const*)vData.Data(),
		vData.GetSizeInBytes());
	if (nullptr == pGrid)
	{
		return nullptr;
	}

	return SEOUL_NEW(MemoryBudgets::Navigation) NavigationGridComponentSharedState(pGrid);
}

NavigationGridComponentSharedState* NavigationGridComponentSharedState::CreateNewHeight(
	const SharedPtr<NavigationGridComponentSharedState>& pExisting,
	UInt32 uHeight)
{
	if (!pExisting.IsValid())
	{
		auto pGrid(Navigation::Grid::Create(uHeight, uHeight));
		if (nullptr == pGrid)
		{
			return nullptr;
		}

		return SEOUL_NEW(MemoryBudgets::Navigation) NavigationGridComponentSharedState(pGrid);
	}
	else
	{
		auto pGrid(Navigation::Grid::CreateFromGrid(
			pExisting->GetWidth(),
			uHeight,
			*pExisting->m_pGrid));
		if (nullptr == pGrid)
		{
			return nullptr;
		}

		return SEOUL_NEW(MemoryBudgets::Navigation) NavigationGridComponentSharedState(pGrid);
	}
}

NavigationGridComponentSharedState* NavigationGridComponentSharedState::CreateNewWidth(
	const SharedPtr<NavigationGridComponentSharedState>& pExisting,
	UInt32 uWidth)
{
	if (!pExisting.IsValid())
	{
		auto pGrid(Navigation::Grid::Create(uWidth, uWidth));
		if (nullptr == pGrid)
		{
			return nullptr;
		}

		return SEOUL_NEW(MemoryBudgets::Navigation) NavigationGridComponentSharedState(pGrid);
	}
	else
	{
		auto pGrid(Navigation::Grid::CreateFromGrid(
			uWidth,
			pExisting->GetHeight(),
			*pExisting->m_pGrid));
		if (nullptr == pGrid)
		{
			return nullptr;
		}

		return SEOUL_NEW(MemoryBudgets::Navigation) NavigationGridComponentSharedState(pGrid);
	}
}

NavigationGridComponentSharedState::NavigationGridComponentSharedState(
	const NavigationGridComponentSharedState& existing,
	const Navigation::CoverageRasterizer& rasterizer)
	: m_pGrid(nullptr)
	, m_pQuery()
{
	m_pGrid = Navigation::Grid::CreateFromGrid(existing.GetWidth(), existing.GetHeight(), *existing.m_pGrid);
	rasterizer.ApplyToGrid(*m_pGrid, 4u, 0u); // TODO: Expose configuration options.

	// TODO: Configuration.
	m_pQuery.Reset(SEOUL_NEW(MemoryBudgets::Navigation) Navigation::Query(
		*m_pGrid));
}

NavigationGridComponentSharedState::NavigationGridComponentSharedState(Navigation::Grid* pGrid)
	: m_pGrid(pGrid)
	, m_pQuery(SEOUL_NEW(MemoryBudgets::Navigation) Navigation::Query(*pGrid))
{
}

NavigationGridComponentSharedState::~NavigationGridComponentSharedState()
{
	m_pQuery.Reset();
	Navigation::Grid::Destroy(m_pGrid);
}

UInt8 NavigationGridComponentSharedState::GetCell(UInt32 uX, UInt32 uY) const
{
	return m_pGrid->GetCell(uX, uY);
}

UInt32 NavigationGridComponentSharedState::GetHeight() const
{
	return m_pGrid->GetHeight();
}

UInt32 NavigationGridComponentSharedState::GetWidth() const
{
	return m_pGrid->GetWidth();
}

UInt8* NavigationGridComponentSharedState::Save(UInt32& ruDataSizeInBytes) const
{
	return m_pGrid->Save(ruDataSizeInBytes);
}

Bool NavigationGridComponentSharedState::RobustFindStraightPath(
	Navigation::QueryState& rState,
	const Navigation::Position& start,
	const Navigation::Position& end,
	UInt32 uMaxStartDistance,
	UInt32 uMaxEndDistance) const
{
	return m_pQuery->RobustFindStraightPath(
		rState,
		start,
		end,
		uMaxStartDistance,
		uMaxEndDistance);
}

NavigationGridQuery::~NavigationGridQuery()
{
}

NavigationGridQuery::NavigationGridQuery(
	const SharedPtr<NavigationGridComponentSharedState>& pShared,
	const Matrix4D& mTransform)
	: m_pShared(pShared)
	, m_mTransform(mTransform)
	, m_pState(SEOUL_NEW(MemoryBudgets::Navigation) Navigation::QueryState)
	, m_bSuccess(false)
	, m_bDone(false)
{
}

Bool NavigationGridQuery::GetPoint(UInt32 u, Vector3D& rv) const
{
	if (!IsDone())
	{
		return false;
	}

	auto const& v = m_pState->m_vWaypoints;
	if (u >= v.GetSize())
	{
		return false;
	}

	rv = Matrix4D::TransformPosition(m_mTransform, Vector3D((Float)v[u].m_uX, 0.0f, -(Float)v[u].m_uY));
	return true;
}

namespace NavUtil
{

struct RobustData
{
	SharedPtr<NavigationGridQuery> m_pQuery;
	Navigation::Position m_Start{};
	Navigation::Position m_End{};
	UInt32 m_uMaxStartDistance{};
	UInt32 m_uMaxEndDistance{};
};

} // namespace NavUtil

void NavigationGridQuery::RobustQueryJob(const NavUtil::RobustData& data)
{
	auto pQuery(data.m_pQuery);
	pQuery->m_bSuccess = pQuery->m_pShared->RobustFindStraightPath(
		*pQuery->m_pState,
		data.m_Start,
		data.m_End,
		data.m_uMaxStartDistance,
		data.m_uMaxEndDistance);
	SeoulMemoryBarrier();
	pQuery->m_bDone = true;
}

NavigationGridComponent::NavigationGridComponent()
{
}

NavigationGridComponent::~NavigationGridComponent()
{
}

SharedPtr<Component> NavigationGridComponent::Clone(const String& sQualifier) const
{
	SharedPtr<NavigationGridComponent> pReturn(SEOUL_NEW(MemoryBudgets::SceneComponent) NavigationGridComponent);
	pReturn->m_pShared = m_pShared;
	return pReturn;
}

static void Traverse(
	Navigation::CoverageRasterizer& r,
	const Matrix4D& mParent,
	const Interface::Objects& vObjects)
{
	for (auto const& pObject : vObjects)
	{
		Matrix4D const mTransform(mParent * pObject->ComputeNormalTransform());

		// PrefabComponent
		{
			auto p = pObject->GetComponent<PrefabComponent>();
			if (p.IsValid())
			{
				Traverse(r, mTransform, p->GetObjects());
			}
		}

		// RigidBodyComponent
		{
			auto p = pObject->GetComponent<RigidBodyComponent>();
			if (p.IsValid())
			{
				auto const& bodyDef = p->GetBodyDef();

				// Only consider static bodies.
				if (bodyDef.m_eType != Physics::BodyType::kStatic)
				{
					continue;
				}

				AABB const aabb = bodyDef.m_Shape.ComputeAABB();
				Vector3D const vMin(aabb.m_vMin);
				Vector3D const vMax(aabb.m_vMax);

				Vector3D avCorners[8];
				avCorners[0] = Matrix4D::TransformPosition(mTransform, Vector3D(vMax.X, vMax.Y, vMax.Z));
				avCorners[1] = Matrix4D::TransformPosition(mTransform, Vector3D(vMax.X, vMin.Y, vMax.Z));
				avCorners[2] = Matrix4D::TransformPosition(mTransform, Vector3D(vMin.X, vMax.Y, vMax.Z));
				avCorners[3] = Matrix4D::TransformPosition(mTransform, Vector3D(vMin.X, vMin.Y, vMax.Z));
				avCorners[4] = Matrix4D::TransformPosition(mTransform, Vector3D(vMax.X, vMax.Y, vMin.Z));
				avCorners[5] = Matrix4D::TransformPosition(mTransform, Vector3D(vMax.X, vMin.Y, vMin.Z));
				avCorners[6] = Matrix4D::TransformPosition(mTransform, Vector3D(vMin.X, vMax.Y, vMin.Z));
				avCorners[7] = Matrix4D::TransformPosition(mTransform, Vector3D(vMin.X, vMin.Y, vMin.Z));

				r.RasterizeTriangle(avCorners[0], avCorners[2], avCorners[1]);
				r.RasterizeTriangle(avCorners[2], avCorners[3], avCorners[1]);

				r.RasterizeTriangle(avCorners[4], avCorners[0], avCorners[5]);
				r.RasterizeTriangle(avCorners[0], avCorners[1], avCorners[5]);

				r.RasterizeTriangle(avCorners[2], avCorners[6], avCorners[3]);
				r.RasterizeTriangle(avCorners[6], avCorners[7], avCorners[3]);

				r.RasterizeTriangle(avCorners[6], avCorners[4], avCorners[7]);
				r.RasterizeTriangle(avCorners[4], avCorners[5], avCorners[7]);

				r.RasterizeTriangle(avCorners[1], avCorners[3], avCorners[5]);
				r.RasterizeTriangle(avCorners[3], avCorners[7], avCorners[5]);

				r.RasterizeTriangle(avCorners[4], avCorners[6], avCorners[0]);
				r.RasterizeTriangle(avCorners[6], avCorners[2], avCorners[0]);
			}
		}
	}
}

void NavigationGridComponent::GenerateNavigation(Interface& rInterface)
{
	if (!m_pShared.IsValid() || m_pShared->GetHeight() == 0u || m_pShared->GetWidth() == 0u)
	{
		return;
	}

	ScopedPtr<Navigation::CoverageRasterizer> pRasterizer;
	{
		UInt32 const uWidth = m_pShared->GetWidth();
		UInt32 const uHeight = m_pShared->GetHeight();

		UInt32 const uCount = (
			uWidth * Navigation::CoverageRasterizer::kiRasterRes *
			uHeight * Navigation::CoverageRasterizer::kiRasterRes);
		Vector<Float, MemoryBudgets::Navigation> vfHeightData;
		vfHeightData.Resize(uCount, 0.0f);
		pRasterizer.Reset(SEOUL_NEW(MemoryBudgets::Navigation) Navigation::CoverageRasterizer(
			uWidth,
			uHeight,
			GetOwner()->GetPosition(),
			vfHeightData.Data(),
			Axis::kY));
	}

	Traverse(*pRasterizer, Matrix4D::Identity(), rInterface.GetObjects());

	m_pShared.Reset(SEOUL_NEW(MemoryBudgets::Navigation) NavigationGridComponentSharedState(
		*m_pShared,
		*pRasterizer));
}

Bool NavigationGridComponent::RobustFindStraightPath(
	const Vector3D& vStart,
	const Vector3D& vEnd,
	UInt32 uMaxStartDistance,
	UInt32 uMaxEndDistance,
	SharedPtr<NavigationGridQuery>& rpQuery) const
{
	if (!m_pShared.IsValid() || m_pShared->GetWidth() == 0u || m_pShared->GetHeight() == 0u)
	{
		return false;
	}

	// TODO: Eliminate redundancy between this an Project().
	auto const mTransform(GetOwner().IsValid() ? GetOwner()->ComputeNormalTransform() : Matrix4D::Identity());

	NavUtil::RobustData data;
	data.m_pQuery.Reset(SEOUL_NEW(MemoryBudgets::Navigation) NavigationGridQuery(m_pShared, mTransform));
	Project(vStart, vEnd, data.m_Start, data.m_End);
	data.m_uMaxStartDistance = uMaxStartDistance;
	data.m_uMaxEndDistance = uMaxEndDistance;
	Jobs::AsyncFunction(
		&NavigationGridQuery::RobustQueryJob,
		data);

	rpQuery.Swap(data.m_pQuery);
	return true;
}

void NavigationGridComponent::Project(
	const Vector3D& vStart,
	const Vector3D& vEnd,
	Navigation::Position& rStart,
	Navigation::Position& rEnd) const
{
	if (!m_pShared.IsValid())
	{
		rStart = Navigation::Position();
		rEnd = Navigation::Position();
	}

	auto const fWidth = (Float32)m_pShared->GetWidth();
	auto const fHeight = (Float32)m_pShared->GetHeight();

	Vector3D vLocalStart;
	Vector3D vLocalEnd;
	auto pOwner(GetOwner());
	if (pOwner.IsValid())
	{
		Matrix4D const mTransform(pOwner->ComputeNormalTransform().Inverse());
		vLocalStart = Matrix4D::TransformPosition(mTransform, vStart);
		vLocalEnd = Matrix4D::TransformPosition(mTransform, vEnd);
	}

	rStart = Navigation::Position(
		(UInt32)Round(Clamp(vLocalStart.X, 0.0f, fWidth - 1.0f)),
		(UInt32)Round(Clamp(-vLocalStart.Z, 0.0f, fHeight - 1.0f)));
	rEnd = Navigation::Position(
		(UInt32)Round(Clamp(vLocalEnd.X, 0.0f, fWidth - 1.0f)),
		(UInt32)Round(Clamp(-vLocalEnd.Z, 0.0f, fHeight - 1.0f)));
}

#if SEOUL_EDITOR_AND_TOOLS
void NavigationGridComponent::EditorGenerateNavigation(Interface* pInterface)
{
	GenerateNavigation(*pInterface);
}

void NavigationGridComponent::EditorDrawPrimitives(PrimitiveRenderer* pRenderer) const
{
	static const ColorARGBu8 kcNoCollision(ColorARGBu8::Create(127, 127, 127, 127));
	static const ColorARGBu8 kcCollision(ColorARGBu8::Create(255, 0, 0, 127));

	if (!m_pShared.IsValid())
	{
		return;
	}

	pRenderer->UseEffectTechnique(kEffectTechniqueRenderNoDepthTest);

	UInt32 const uWidth = m_pShared->GetWidth();
	UInt32 const uHeight = m_pShared->GetHeight();
	Matrix4D const mTransform(GetOwner()->ComputeNormalTransform());
	for (UInt32 uY = 0u; uY < uHeight; ++uY)
	{
		for (UInt32 uX = 0u; uX < uWidth; ++uX)
		{
			ColorARGBu8 const c = (m_pShared->GetCell(uX, uY) == 0
				? kcNoCollision
				: kcCollision);

			Vector3D const v0((Float)(uX + 0u), 0.0f, -(Float)(uY + 0u));
			Vector3D const v1((Float)(uX + 1u), 0.0f, -(Float)(uY + 0u));
			Vector3D const v2((Float)(uX + 0u), 0.0f, -(Float)(uY + 1u));
			Vector3D const v3((Float)(uX + 1u), 0.0f, -(Float)(uY + 1u));

			pRenderer->TriangleQuad(
				Matrix4D::TransformPosition(mTransform, v0),
				Matrix4D::TransformPosition(mTransform, v1),
				Matrix4D::TransformPosition(mTransform, v2),
				Matrix4D::TransformPosition(mTransform, v3),
				c);
		}
	}

	pRenderer->UseEffectTechnique();
}
#endif // /#if SEOUL_EDITOR_AND_TOOLS

String NavigationGridComponent::GetGridData() const
{
	if (!m_pShared.IsValid())
	{
		return String();
	}

	UInt32 uDataSizeInBytes = 0u;
	UInt8 const* pGrid = m_pShared->Save(uDataSizeInBytes);
	if (nullptr == pGrid)
	{
		return String();
	}

	String const sReturn(Base64Encode(pGrid, uDataSizeInBytes));
	MemoryManager::Deallocate((void*)pGrid);
	return sReturn;
}

void NavigationGridComponent::SetGridData(const String& sGridData)
{
	m_pShared.Reset(NavigationGridComponentSharedState::CreateFromGridData(sGridData));
}

UInt32 NavigationGridComponent::GetHeight() const
{
	return (m_pShared.IsValid() ? m_pShared->GetHeight() : 0u);
}

void NavigationGridComponent::SetHeight(UInt32 uHeight)
{
	m_pShared.Reset(NavigationGridComponentSharedState::CreateNewHeight(m_pShared, uHeight));
}

UInt32 NavigationGridComponent::GetWidth() const
{
	return (m_pShared.IsValid() ? m_pShared->GetWidth() : 0u);
}

void NavigationGridComponent::SetWidth(UInt32 uWidth)
{
	m_pShared.Reset(NavigationGridComponentSharedState::CreateNewWidth(m_pShared, uWidth));
}

} // namespace Scene

} // namespace Seoul

#endif // /#if SEOUL_WITH_NAVIGATION && SEOUL_WITH_SCENE
