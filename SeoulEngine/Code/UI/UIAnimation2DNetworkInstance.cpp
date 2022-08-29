/**
 * \file UIAnimation2DNetworkInstance.cpp
 * \brief SeoulEngine subclass/extension of Falcon::Instance for Animation2D playback.
 *
 * UI::Animation2DNetworkInstance binds the SeoulEngine Animation2D system into the Falcon scene graph.
 * Animation2Ds are rendered with the Falcon renderer and can be freely layered with
 * Falcon scene elements.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnimationBlendInstance.h"
#include "AnimationNodeInstance.h"
#include "AnimationStateMachineInstance.h"
#if SEOUL_WITH_ANIMATION_2D
#include "Animation2DDataDefinition.h"
#include "Animation2DDataInstance.h"
#include "Animation2DNetworkInstance.h"
#endif
#include "Engine.h"
#include "FalconMovieClipInstance.h"
#include "FalconRenderDrawer.h"
#include "FalconRenderPoser.h"
#include "FileManager.h"
#include "ReflectionDefine.h"
#include "Renderer.h"
#include "UIAnimation2DNetworkInstance.h"
#include "UIBoneAttachments.h"
#include "UIMovie.h"
#include "UIRenderer.h"

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul
{

#if SEOUL_ENABLE_CHEATS

/** Radius of the box drawn for debugging bone attachments. */
static const Float kfDebugAttachmentRadius = 5.0f;

namespace UI
{

class Animation2DCommands SEOUL_SEALED
{
public:
	static Bool s_bAttachmentDebug;

	Animation2DCommands()
	{
	}

	void AttachmentDebug(Bool bEnable)
	{
		s_bAttachmentDebug = bEnable;
	}

private:
	SEOUL_DISABLE_COPY(Animation2DCommands);
}; // class UI::Animation2DCommands
Bool Animation2DCommands::s_bAttachmentDebug = false;

} // namespace UI

SEOUL_BEGIN_TYPE(UI::Animation2DCommands, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(CommandsInstance)
	SEOUL_METHOD(AttachmentDebug)
		SEOUL_ATTRIBUTE(Category, "Rendering")
		SEOUL_ATTRIBUTE(Description,
			"Enable or disable display of active bone\n"
			"attachments.")
		SEOUL_ATTRIBUTE(DisplayName, "Show Bone Attachments")
SEOUL_END_TYPE()

#endif // /#if SEOUL_ENABLE_CHEATS

/**
 * Fixed tolerance used when clipping in texture coordinate space.
 *
 * Based on our max mobile texture resolution.
 */
static const Float kfTextureSpaceClipTolerance = (Float)(1.0 / 2048.0);

SEOUL_BEGIN_TYPE(UI::Animation2DNetworkInstance, TypeFlags::kDisableNew)
	SEOUL_PARENT(Falcon::Instance)
SEOUL_END_TYPE()

namespace UI
{

Animation2DNetworkInstance::Animation2DNetworkInstance(
	const Movie& owner,
	const SharedPtr<Animation2D::NetworkInstance>& pNetworkInstance)
	: Falcon::Instance(0)
	, m_hOwner(owner.GetHandle())
	, m_pNetworkInstance(pNetworkInstance)
	, m_pAttachments()
	, m_tPalette()
	, m_pMeshClipCache(Falcon::Clipper::NewMeshClipCache<Falcon::Clipper::UtilityVertex>())
	, m_tPosed()
	, m_vShadowOffset(0, 0)
	, m_PositionBounds(Falcon::Rectangle::InverseMax())
	, m_uTickCount(0u)
	, m_ActivePalette()
	, m_ActiveSkin(Animation2D::kDefaultSkin)
	, m_bShadowCast(false)
	, m_bVariableTimeStep(false)
#if SEOUL_HOT_LOADING
	, m_LoadDataCount(0)
	, m_LoadNetworkCount(0)
#endif // /#if SEOUL_HOT_LOADING
#if !SEOUL_SHIP
	// To workaround a potential bug in developer only builds, we conditionally
	// use the existence of files in the "source" folder instead of the "content"
	// folder for palette management, if animation data exists in the source folder.
	//
	// This is to handle the case where, on local developer machines, cooked files
	// have become stale. This can cause havoc with palettes, since they fundamentally
	// depend on whether a files exists or not to determine whether they should fall
	// back to the default animation files or not.
	, m_bCheckSourceFilesForPalettes(FileManager::Get()->Exists(pNetworkInstance->GetDataHandle().GetKey().GetAbsoluteFilenameInSource()))
#endif // /#if !SEOUL_SHIP
{
	m_pAttachments.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) BoneAttachments(*this));

	// Let our owner know.
	CheckedPtr<Movie> pOwner(GetPtr<Movie>(m_hOwner));
	if (pOwner.IsValid())
	{
		pOwner->AddActiveAnimation2D(this);
	}
}

Animation2DNetworkInstance::~Animation2DNetworkInstance()
{
	// Let our owner know.
	CheckedPtr<Movie> pOwner(GetPtr<Movie>(m_hOwner));
	if (pOwner.IsValid())
	{
		pOwner->RemoveActiveAnimation2D(this);
	}

	// Cleanup the posing cache.
	SafeDeleteTable(m_tPosed);
	Falcon::Clipper::DestroyMeshClipCache(m_pMeshClipCache);
}

Falcon::Instance* Animation2DNetworkInstance::Clone(Falcon::AddInterface& rInterface) const
{
	Animation2DNetworkInstance* pReturn = SEOUL_NEW(MemoryBudgets::UIRuntime) Animation2DNetworkInstance(
		*GetPtr(m_hOwner),
		m_pNetworkInstance);
	CloneTo(rInterface, *pReturn);
	return pReturn;
}

Bool Animation2DNetworkInstance::ComputeLocalBounds(Falcon::Rectangle& rBounds)
{
	// Check if bounds have been computed (will be an inverse, invalid bounds if not).
	if (m_PositionBounds.m_fLeft <= m_PositionBounds.m_fRight)
	{
		rBounds = m_PositionBounds;
		return true;
	}

	return false;
}

void Animation2DNetworkInstance::Skin(
	CheckedPtr<DeformData> pDeform,
	RGBA color,
	const Animation2D::MeshAttachment& mesh,
	const SkinningPalette& vPalette,
	PosedEntry& r) const
{
	// Early out if already in-sync.
	if (m_uTickCount == r.m_uPoseCount)
	{
		return;
	}

	// Sanity check deform data.
	if (pDeform.IsValid() && pDeform->GetSize() % 2u != 0u)
	{
		return;
	}

	// Now in-sync.
	r.m_uPoseCount = m_uTickCount;

	// Setup rendering feature.
	r.m_eRenderingFeature = (color != RGBA::White() ? Falcon::Render::Feature::kColorMultiply : Falcon::Render::Feature::kNone);

	auto const& vBoneCounts = mesh.GetBoneCounts();
	auto const& vTexCoords = mesh.GetTexCoords();

	// Get vertices to sample from.
	Vector2D const* pVertices = nullptr;
	UInt32 uVertices = 0u;
	if (pDeform.IsValid())
	{
		// This cast depends on the layout of Vector2D.
		SEOUL_STATIC_ASSERT(0 == offsetof(Vector2D, X));
		SEOUL_STATIC_ASSERT(4 == offsetof(Vector2D, Y));
		SEOUL_STATIC_ASSERT(8 == sizeof(Vector2D));

		pVertices = (Vector2D const*)pDeform->Data();
		uVertices = pDeform->GetSize() / 2u;
	}
	else
	{
		pVertices = mesh.GetVertices().Data();
		uVertices = mesh.GetVertices().GetSize();
	}

	r.m_vVertices.Clear();
	r.m_PositionBounds = Falcon::Rectangle::InverseMax();
	r.m_TexCoordsBounds = Falcon::Rectangle::InverseMax();

	// Rigid skinning, just apply the slot bone.
	if (vBoneCounts.IsEmpty())
	{
		UInt32 const uSize = uVertices;
		r.m_vVertices.Reserve(uSize);

		for (UInt32 i = 0u; i < uSize; ++i)
		{
			auto const& t = vTexCoords[i];
			auto const& v = pVertices[i];

			r.m_PositionBounds.AbsorbPoint(v);
			r.m_TexCoordsBounds.AbsorbPoint(t);
			r.m_vVertices.PushBack(StandardVertex2D::Create(
				v.X,
				v.Y,
				color,
				RGBA::TransparentBlack(),
				t.X,
				t.Y));
		}
	}
	// Otherwise, skinned.
	else
	{
		UInt32 const uSize = vBoneCounts.GetSize();
		r.m_vVertices.Reserve(uSize);

		auto const& vLinks = mesh.GetLinks();

		UInt32 u = 0u;
		for (UInt32 uBone = 0u; uBone < uSize; ++uBone)
		{
			Vector2D v(Vector2D::Zero());

			UInt32 const uBoneCount = vBoneCounts[uBone];
			UInt32 const uEnd = (u + uBoneCount);
			for (; u < uEnd; ++u)
			{
				auto const& link = vLinks[u];
				v += Matrix2x3::TransformPosition(
					vPalette[link.m_uIndex],
					pVertices[u]) * link.m_fWeight;
			}

			auto const& t = vTexCoords[uBone];

			r.m_PositionBounds.AbsorbPoint(v);
			r.m_TexCoordsBounds.AbsorbPoint(t);
			r.m_vVertices.PushBack(StandardVertex2D::Create(
				v.X,
				v.Y,
				color,
				RGBA::TransparentBlack(),
				t.X,
				t.Y));
		}
	}

	// Edge case, early out.
	auto const fTexCoordWidth = r.m_TexCoordsBounds.GetWidth();
	auto const fTexCoordHeight = r.m_TexCoordsBounds.GetHeight();
	if (IsZero(fTexCoordWidth) || IsZero(fTexCoordHeight))
	{
		r.m_fEffectiveWidth = 0.0f;
		r.m_fEffectiveHeight = 0.0f;
		return;
	}

	// Fast, correct if the mesh is mostly uniform.
	auto const fApproxEffectiveWidth = r.m_PositionBounds.GetWidth() / fTexCoordWidth;
	auto const fApproxEffectiveHeight = r.m_PositionBounds.GetHeight() / fTexCoordHeight;

	// Compute effective width and height for texture selection.
	// Edges has been pre-setup to provide the following data:
	// - UV separation between endpoint vertices (distance squared
	//   in UV space).
	// - 1.0 / (T1 - T0), inverse separation between UVs.
	//
	// Also, the edge list has been pruned to be small but also a representative
	// sample of triangles in the mesh.
	//
	// The algorithm here is as follows:
	// - for each unique edge, computer dot(V, 1 / T),
	//   where V is the separation between the positions and
	//   T is the separation between the UVs.
	// - distribute the factor into the local width and height
	//   by using the dot product of the endpoints.
	// - clamp the result to this factor * the rough approximate
	//   above. This prevents very small triangles from exploding
	//   the needed texture resolution for the entire mesh.
	static const Float kfMaxFactor = 3.0f;

	auto const fMaxWidth = (kfMaxFactor * fApproxEffectiveWidth);
	auto const fMaxHeight = (kfMaxFactor * fApproxEffectiveHeight);

	r.m_fEffectiveWidth = 0.0f;
	r.m_fEffectiveHeight = 0.0f;
	{
		auto const& edges = mesh.GetEdges();
		for (auto const& edge : edges)
		{
			// Vertices.
			auto const& v0 = r.m_vVertices[edge.m_u0];
			auto const& v1 = r.m_vVertices[edge.m_u1];

			// Compute the position separation.
			auto const vDiffP = (v1.m_vP - v0.m_vP);
			auto const vAbsDiffP = vDiffP.Abs();

			// Factor is the dot of one over the vector in UV space
			// and the position vector.
			auto const f = Vector2D::Dot(vAbsDiffP, edge.m_vAbsOneOverDiffT);

			// Now separate the factor into local space X and Y.
			auto const vAbsNP = Vector2D::Normalize(vAbsDiffP);
			auto const fW = Max(f * vAbsNP.X, r.m_fEffectiveWidth);
			auto const fH = Max(f * vAbsNP.Y, r.m_fEffectiveHeight);

			// Apply - if we've hit max of both, early out.
			r.m_fEffectiveWidth = Min(fMaxWidth, fW);
			r.m_fEffectiveHeight = Min(fMaxHeight, fH);
			if (r.m_fEffectiveHeight == fMaxHeight &&
				r.m_fEffectiveWidth == fMaxWidth)
			{
				break;
			}
		}
	}
}

void Animation2DNetworkInstance::Pose(
	Falcon::Render::Poser& rPoser,
	const Matrix2x3& mParent,
	const Falcon::ColorTransformWithAlpha& cxParent)
{
	if (!GetVisible())
	{
		return;
	}

	if (!m_pNetworkInstance->IsReady())
	{
		return;
	}

#if SEOUL_HOT_LOADING
	// Need to clear the clipping cache on load changes.
	if (m_LoadDataCount != m_pNetworkInstance->GetLoadDataCount() ||
		m_LoadNetworkCount != m_pNetworkInstance->GetLoadNetworkCount())
	{
		m_LoadDataCount = m_pNetworkInstance->GetLoadDataCount();
		m_LoadNetworkCount = m_pNetworkInstance->GetLoadNetworkCount();
		SafeDeleteTable(m_tPosed);
		m_tPalette.Clear();
	}
#endif // /#if SEOUL_HOT_LOADING

	auto const& pData = m_pNetworkInstance->GetData();
	auto const& state = m_pNetworkInstance->GetState();
	auto const& drawOrder = state.GetDrawOrder();
	auto const& palette = state.GetSkinningPalette();
	auto const& skins = pData->GetSkins();
	auto const& slotsData = pData->GetSlots();
	auto const& slotsState = state.GetSlots();

	auto pSkin = skins.Find(m_ActiveSkin);
	if (nullptr == pSkin)
	{
		return;
	}

	Falcon::ColorTransformWithAlpha cxWorld = (cxParent * GetColorTransformWithAlpha());
	if (cxWorld.m_fMulA == 0.0f)
	{
		return;
	}

	// TODO: Move bounds computation into Advance() and use it here to early out of drawing.

	// Apply prop lighting settings to the character.
	{
		auto v = rPoser.GetState().m_pStage3DSettings->m_Lighting.m_Props.m_vColor;
		cxWorld.m_fMulR *= v.X;
		cxWorld.m_fMulG *= v.Y;
		cxWorld.m_fMulB *= v.Z;
	}

	// Reset bounds, compute in loop.
	m_PositionBounds = Falcon::Rectangle::InverseMax();

	UInt32 uClips = 0u;
	UInt32 const uDraws = drawOrder.GetSize();
	for (UInt32 iDraw = 0u; iDraw < uDraws; ++iDraw)
	{
		UInt32 const i = (UInt32)drawOrder[iDraw];
		auto const& slotData = slotsData[i];
		auto const& slotState = slotsState[i];

		SharedPtr<Animation2D::Attachment> p;
		{
			auto pSets = pSkin->Find(slotData.m_Id);
			if (nullptr != pSets)
			{
				pSets->GetValue(slotState.m_AttachmentId, p);
			}
		}

		if (!p.IsValid())
		{
			continue;
		}

		auto& rData = ResolvePosedEntry(p);
		switch (p->GetType())
		{
		case Animation2D::AttachmentType::kBitmap:
			{
				SharedPtr<Animation2D::BitmapAttachment> pBitmap((Animation2D::BitmapAttachment*)p.GetPtr());

				// Skip if fully transparent.
				RGBA const color(pBitmap->GetColor() * slotState.m_Color);
				if (color.m_A == 0)
				{
					continue;
				}

				Float const fWidth(pBitmap->GetWidth());
				Float const fHeight(pBitmap->GetHeight());
				auto const mLocalTransform = Matrix2x3::CreateScale(1.0f, -1.0f) * // TODO: Don't perform the inversion (for +Y down) like this, simplify.
					palette[slotData.m_iBone] *
					Matrix2x3::CreateTranslation(pBitmap->GetPositionX(), pBitmap->GetPositionY()) *
					Matrix2x3::CreateRotationFromDegrees(pBitmap->GetRotationInDegrees()) *
					Matrix2x3::CreateScale(pBitmap->GetScaleX(), pBitmap->GetScaleY());
				auto const mWorld(
					mParent *
					GetTransform() *
					mLocalTransform);

				// Compute the (unclipped) bitmap shape, to use for
				// culling and bounds computation.
				Float const fTX0 = 0.0f;
				Float const fTX1 = 1.0f;
				Float const fTY0 = 0.0f;
				Float const fTY1 = 1.0f;
				Float const fPX0 = (fTX0 * fWidth) - (fWidth * 0.5f);
				Float const fPX1 = (fTX1 * fWidth) - (fWidth * 0.5f);
				Float const fPY0 = ((1.0f - fTY1) * fHeight) - (fHeight * 0.5f);
				Float const fPY1 = ((1.0f - fTY0) * fHeight) - (fHeight * 0.5f);

				rData.m_PositionBounds = Falcon::Rectangle::Create(fPX0, fPX1, fPY0, fPY1);
				rData.m_TexCoordsBounds = Falcon::Rectangle::Create(fTX0, fTX1, fTY0, fTY1);
				m_PositionBounds = Falcon::Rectangle::Merge(
					m_PositionBounds,
					TransformRectangle(mLocalTransform, rData.m_PositionBounds));

				auto const worldBounds(TransformRectangle(mWorld, rData.m_PositionBounds));
				auto const filePath(ResolveFilePath(pBitmap->GetFilePath()));
				Falcon::TextureReference reference;
				auto const eResult = rPoser.ResolveTextureReference(
					worldBounds,
					this,
					rPoser.GetRenderThreshold(fWidth, fHeight, mWorld),
					filePath,
					reference);
				if (Falcon::Render::PoserResolveResult::kSuccess == eResult)
				{
					auto const worldOcclusion(Falcon::ComputeOcclusionRectangle(mWorld, reference, rData.m_PositionBounds));
					rPoser.Pose(
						worldBounds,
						this,
						mWorld,
						cxWorld,
						reference,
						worldOcclusion,
						Falcon::Render::Feature::kColorMultiply,
						(Int32)i);
				}
			}
			break;
		case Animation2D::AttachmentType::kClipping:
		{
			using namespace Falcon;

			SharedPtr<Animation2D::ClippingAttachment> pClipping((Animation2D::ClippingAttachment*)p.GetPtr());

			Matrix2x3 mLocal;
			if (pClipping->GetBoneCounts().IsEmpty())
			{
				mLocal = (
					Matrix2x3::CreateScale(1.0f, -1.0f) * // TODO: Don't perform the inversion (for +Y down) like this, simplify.
					palette[slotData.m_iBone]);
			}
			else
			{
				mLocal = (
					Matrix2x3::CreateScale(1.0f, -1.0f)); // TODO: Don't perform the inversion (for +Y down) like this, simplify.
			}
			auto const mWorld(
				mParent *
				GetTransform() *
				mLocal);

			SEOUL_STATIC_ASSERT(sizeof(Vector2D) == sizeof(Float) * 2); // Necessary for next bit of coercion.
			SEOUL_ASSERT(pClipping->GetVertices().GetSize() % 2u == 0u);
			auto const pVertices = (Vector2D const*)pClipping->GetVertices().Data();
			auto const uVertices = pClipping->GetVertices().GetSize() / 2u;
			rPoser.ClipStackAddConvexHull(
				mWorld,
				pVertices,
				uVertices);
			if (rPoser.ClipStackPush())
			{
				++uClips;
			}
			else
			{
				goto done;
			}
		}
		break;
		case Animation2D::AttachmentType::kLinkedMesh:
			{
				SharedPtr<Animation2D::LinkedMeshAttachment> pMesh((Animation2D::LinkedMeshAttachment*)p.GetPtr());

				// Skip if fully transparent.
				RGBA const color(pMesh->GetColor() * slotState.m_Color);
				if (color.m_A == 0)
				{
					continue;
				}

				auto pParent(pMesh->GetParent());

				CheckedPtr<Animation2D::DataInstance::DeformData> pData;
				if (pMesh->GetDeform())
				{
					state.GetDeforms().GetValue(
						Animation2D::DeformKey(pMesh->GetSkinId(), slotData.m_Id, slotState.m_AttachmentId),
						pData);
				}

				Skin(
					pData,
					color,
					*pParent,
					palette,
					rData);

				Matrix2x3 mLocal;
				if (pParent->GetBoneCounts().IsEmpty())
				{
					mLocal = (
						Matrix2x3::CreateScale(1.0f, -1.0f) * // TODO: Don't perform the inversion (for +Y down) like this, simplify.
						palette[slotData.m_iBone]);
				}
				else
				{
					mLocal = Matrix2x3::CreateScale(1.0f, -1.0f); // TODO: Don't perform the inversion (for +Y down) like this, simplify.
				}
				auto const mWorld = (
					mParent *
					GetTransform() *
					mLocal);

				m_PositionBounds = Falcon::Rectangle::Merge(
					m_PositionBounds,
					TransformRectangle(mLocal, rData.m_PositionBounds));

				auto const worldBounds(TransformRectangle(mWorld, rData.m_PositionBounds));
				auto const filePath(ResolveFilePath(pMesh->GetFilePath()));
				Falcon::TextureReference reference;
				auto const eResult = rPoser.ResolveTextureReference(
					worldBounds,
					this,
					rPoser.GetRenderThreshold(rData.m_fEffectiveWidth, rData.m_fEffectiveHeight, mWorld),
					filePath,
					reference);
				if (Falcon::Render::PoserResolveResult::kSuccess == eResult)
				{
					rPoser.Pose(
						worldBounds,
						this,
						mWorld,
						cxWorld,
						reference,
						Falcon::Rectangle(),
						Falcon::Render::Feature::kColorMultiply,
						(Int32)i);
				}
			}
			break;

		case Animation2D::AttachmentType::kMesh:
			{
				SharedPtr<Animation2D::MeshAttachment> pMesh((Animation2D::MeshAttachment*)p.GetPtr());

				// Skip if fully transparent.
				RGBA const color(pMesh->GetColor() * slotState.m_Color);
				if (color.m_A == 0)
				{
					continue;
				}

				CheckedPtr<Animation2D::DataInstance::DeformData> pData;
				state.GetDeforms().GetValue(
					Animation2D::DeformKey(m_ActiveSkin, slotData.m_Id, slotState.m_AttachmentId),
					pData);

				Skin(
					pData,
					color,
					*pMesh,
					palette,
					rData);

				Matrix2x3 mLocal;
				if (pMesh->GetBoneCounts().IsEmpty())
				{
					mLocal = (
						Matrix2x3::CreateScale(1.0f, -1.0f) * // TODO: Don't perform the inversion (for +Y down) like this, simplify.
						palette[slotData.m_iBone]);
				}
				else
				{
					mLocal = (
						Matrix2x3::CreateScale(1.0f, -1.0f)); // TODO: Don't perform the inversion (for +Y down) like this, simplify.
				}
				auto const mWorld(
					mParent *
					GetTransform() *
					mLocal);

				m_PositionBounds = Falcon::Rectangle::Merge(
					m_PositionBounds,
					TransformRectangle(mLocal, rData.m_PositionBounds));

				auto const worldBounds(TransformRectangle(mWorld, rData.m_PositionBounds));
				auto const filePath(ResolveFilePath(pMesh->GetFilePath()));
				Falcon::TextureReference reference;
				auto const eResult = rPoser.ResolveTextureReference(
					worldBounds,
					this,
					rPoser.GetRenderThreshold(rData.m_fEffectiveWidth, rData.m_fEffectiveHeight, mWorld),
					filePath,
					reference);
				if (Falcon::Render::PoserResolveResult::kSuccess == eResult)
				{
					rPoser.Pose(
						worldBounds,
						this,
						mWorld,
						cxWorld,
						reference,
						Falcon::Rectangle(),
						Falcon::Render::Feature::kColorMultiply,
						(Int32)i);
				}
			}
			break;
		default:
			break;
		};
	}

done:
	for (UInt32 i = 0u; i < uClips; ++i)
	{
		rPoser.ClipStackPop();
	}

#if SEOUL_ENABLE_CHEATS
	if (m_pAttachments.IsValid() && Animation2DCommands::s_bAttachmentDebug)
	{
		using namespace Falcon;

		TextureReference solidFill;
		if (Render::PoserResolveResult::kSuccess == rPoser.ResolveTextureReference(
			TransformRectangle(mParent * GetTransform(), m_PositionBounds),
			this,
			1.0f,
			FilePath(),
			solidFill))
		{
			Int iCount = 0;
			for (auto const& a : m_pAttachments->GetAttachments())
			{
				++iCount;
				auto const v = GetWorldSpaceBonePosition(a.First);
				auto const rect = Rectangle::Create(v.X - kfDebugAttachmentRadius, v.X + kfDebugAttachmentRadius, v.X - kfDebugAttachmentRadius, v.X + kfDebugAttachmentRadius);
				rPoser.Pose(
					rect,
					this,
					Matrix2x3::CreateTranslation(v),
					ColorTransformWithAlpha::Identity(),
					solidFill,
					rect,
					Render::Feature::kColorMultiply,
					-iCount);
			}
		}
	}
#endif // /#if SEOUL_ENABLE_CHEATS
}

void Animation2DNetworkInstance::Draw(
	Falcon::Render::Drawer& rDrawer,
	const Falcon::Rectangle& worldBoundsPreClip,
	const Matrix2x3& mWorld,
	const Falcon::ColorTransformWithAlpha& cxWorld,
	const Falcon::TextureReference& textureReference,
	Int32 iSubInstanceId)
{
#if SEOUL_ENABLE_CHEATS
	// Attachment drawing.
	if (iSubInstanceId < 0)
	{
		using namespace Falcon;

		auto const outerColor = RGBA::Create(ColorARGBu8::Black());
		auto const innerColor = RGBA::Create(ColorARGBu8::Red());
		FixedArray<ShapeVertex, 8> aVertices;
		aVertices[0] = ShapeVertex::Create(-kfDebugAttachmentRadius, +kfDebugAttachmentRadius, outerColor, RGBA::TransparentBlack(), 0.0f, 0.0f);
		aVertices[1] = ShapeVertex::Create(-kfDebugAttachmentRadius, -kfDebugAttachmentRadius, outerColor, RGBA::TransparentBlack(), 0.0f, 1.0f);
		aVertices[2] = ShapeVertex::Create(+kfDebugAttachmentRadius, -kfDebugAttachmentRadius, outerColor, RGBA::TransparentBlack(), 1.0f, 1.0f);
		aVertices[3] = ShapeVertex::Create(+kfDebugAttachmentRadius, +kfDebugAttachmentRadius, outerColor, RGBA::TransparentBlack(), 1.0f, 0.0f);
		aVertices[4] = ShapeVertex::Create(-kfDebugAttachmentRadius + 2.0f, +kfDebugAttachmentRadius - 2.0f, innerColor, RGBA::TransparentBlack(), 0.0f, 0.0f);
		aVertices[5] = ShapeVertex::Create(-kfDebugAttachmentRadius + 2.0f, -kfDebugAttachmentRadius + 2.0f, innerColor, RGBA::TransparentBlack(), 0.0f, 1.0f);
		aVertices[6] = ShapeVertex::Create(+kfDebugAttachmentRadius - 2.0f, -kfDebugAttachmentRadius + 2.0f, innerColor, RGBA::TransparentBlack(), 1.0f, 1.0f);
		aVertices[7] = ShapeVertex::Create(+kfDebugAttachmentRadius - 2.0f, +kfDebugAttachmentRadius - 2.0f, innerColor, RGBA::TransparentBlack(), 1.0f, 0.0f);
		rDrawer.DrawTriangleList(
			worldBoundsPreClip,
			textureReference,
			mWorld,
			aVertices.Data(),
			aVertices.GetSize(),
			TriangleListDescription::kQuadList,
			Render::Feature::kColorMultiply);
		return;
	}
#endif // /#if SEOUL_ENABLE_CHEATS

	auto const& pData = m_pNetworkInstance->GetData();
	auto const& state = m_pNetworkInstance->GetState();
	auto const& skins = pData->GetSkins();
	auto const& slotsData = pData->GetSlots();
	auto const& slotsState = state.GetSlots();
	auto const& slotData = slotsData[iSubInstanceId];
	auto const& slotState = slotsState[iSubInstanceId];

	auto pSkin = skins.Find(m_ActiveSkin);
	if (nullptr == pSkin)
	{
		return;
	}

	SharedPtr<Animation2D::Attachment> p;
	{
		auto pSets = pSkin->Find(slotData.m_Id);
		if (nullptr != pSets)
		{
			pSets->GetValue(slotState.m_AttachmentId, p);
		}
	}

	if (!p.IsValid())
	{
		return;
	}

	auto& rData = ResolvePosedEntry(p);
	switch (p->GetType())
	{
	case Animation2D::AttachmentType::kBitmap:
		{
			using namespace Falcon;

			SharedPtr<Animation2D::BitmapAttachment> pBitmap((Animation2D::BitmapAttachment*)p.GetPtr());
			Float const fWidth = (Float)pBitmap->GetWidth();
			Float const fHeight = (Float)pBitmap->GetHeight();

			Float const fTX0 = textureReference.m_vVisibleOffset.X;
			Float const fTX1 = textureReference.m_vVisibleOffset.X + textureReference.m_vVisibleScale.X;
			Float const fTY0 = textureReference.m_vVisibleOffset.Y;
			Float const fTY1 = textureReference.m_vVisibleOffset.Y + textureReference.m_vVisibleScale.Y;
			Float const fPX0 = (fTX0 * fWidth) - (fWidth * 0.5f);
			Float const fPX1 = (fTX1 * fWidth) - (fWidth * 0.5f);
			Float const fPY0 = ((1.0f - fTY1) * fHeight) - (fHeight * 0.5f);
			Float const fPY1 = ((1.0f - fTY0) * fHeight) - (fHeight * 0.5f);

			RGBA const color(pBitmap->GetColor() * slotState.m_Color);

			// Note the different vertex order from most Falcon draw calls,
			// since animation data uses +Y up instead of +Y down.
			FixedArray<ShapeVertex, 4> aVertices;
			aVertices[0] = ShapeVertex::Create(fPX0, fPY1, color, RGBA::TransparentBlack(), fTX0, fTY0);
			aVertices[1] = ShapeVertex::Create(fPX0, fPY0, color, RGBA::TransparentBlack(), fTX0, fTY1);
			aVertices[2] = ShapeVertex::Create(fPX1, fPY0, color, RGBA::TransparentBlack(), fTX1, fTY1);
			aVertices[3] = ShapeVertex::Create(fPX1, fPY1, color, RGBA::TransparentBlack(), fTX1, fTY0);

			auto const eRenderingFeature = (color != RGBA::White() ? Render::Feature::kColorMultiply : Render::Feature::kNone);
			rDrawer.DrawTriangleList(
				worldBoundsPreClip,
				textureReference,
				mWorld,
				cxWorld,
				aVertices.Data(),
				aVertices.GetSize(),
				TriangleListDescription::kQuadList,
				eRenderingFeature);
		}
		break;
	case Animation2D::AttachmentType::kLinkedMesh:
		{
			SharedPtr<Animation2D::LinkedMeshAttachment> pMesh((Animation2D::LinkedMeshAttachment*)p.GetPtr());
			SEOUL_ASSERT(pMesh->GetParent()->GetType() == Animation2D::AttachmentType::kMesh);
			SharedPtr<Animation2D::MeshAttachment> pParent((Animation2D::MeshAttachment*)pMesh->GetParent().GetPtr());

			auto const& clipped = Clip(*pParent, textureReference, rData);

			if (!clipped.m_vIndices.IsEmpty())
			{
				rDrawer.DrawTriangleList(
					worldBoundsPreClip,
					textureReference,
					mWorld,
					cxWorld,
					clipped.m_vIndices.Data(),
					clipped.m_vIndices.GetSize(),
					clipped.m_vVertices.Data(),
					clipped.m_vVertices.GetSize(),
					Falcon::TriangleListDescription::kNotSpecific,
					rData.m_eRenderingFeature);
			}
		}
		break;
	case Animation2D::AttachmentType::kMesh:
		{
			SharedPtr<Animation2D::MeshAttachment> pMesh((Animation2D::MeshAttachment*)p.GetPtr());

			auto const& clipped = Clip(*pMesh, textureReference, rData);

			if (!clipped.m_vIndices.IsEmpty())
			{
				rDrawer.DrawTriangleList(
					worldBoundsPreClip,
					textureReference,
					mWorld,
					cxWorld,
					clipped.m_vIndices.Data(),
					clipped.m_vIndices.GetSize(),
					clipped.m_vVertices.Data(),
					clipped.m_vVertices.GetSize(),
					Falcon::TriangleListDescription::kNotSpecific,
					rData.m_eRenderingFeature);
			}
		}
		break;
	default:
		break;
	};
}

#if SEOUL_ENABLE_CHEATS
void Animation2DNetworkInstance::PoseInputVisualization(
	Falcon::Render::Poser& rPoser,
	const Matrix2x3& mParent,
	RGBA color)
{
	// Check if bounds have been computed (will be an inverse, invalid bounds if not).
	if (m_PositionBounds.m_fLeft > m_PositionBounds.m_fRight)
	{
		return;
	}

	auto const& bounds = m_PositionBounds;

	// TODO: Draw the appropriate shape for exact hit testing.
	auto const mWorld(mParent * GetTransform());
	auto const worldBounds = Falcon::TransformRectangle(mWorld, bounds);
	rPoser.PoseInputVisualization(
		worldBounds,
		bounds,
		mWorld,
		color);
}
#endif

Bool Animation2DNetworkInstance::HitTest(
	const Matrix2x3& mParent,
	Float32 fWorldX,
	Float32 fWorldY,
	Bool bIgnoreVisibility /*= false*/) const
{
	if (!bIgnoreVisibility)
	{
		if (!GetVisible())
		{
			return false;
		}
	}

	// Check if bounds have been computed (will be an inverse, invalid bounds if not).
	if (m_PositionBounds.m_fLeft > m_PositionBounds.m_fRight)
	{
		return false;
	}

	Matrix2x3 const mWorld = (mParent * GetTransform());
	Matrix2x3 const mInverseWorld = mWorld.Inverse();

	Vector2D const vObjectSpace = Matrix2x3::TransformPosition(mInverseWorld, Vector2D(fWorldX, fWorldY));
	Float32 fObjectSpaceX = vObjectSpace.X;
	Float32 fObjectSpaceY = vObjectSpace.Y;

	if (fObjectSpaceX < m_PositionBounds.m_fLeft) { return false; }
	if (fObjectSpaceY < m_PositionBounds.m_fTop) { return false; }
	if (fObjectSpaceX > m_PositionBounds.m_fRight) { return false; }
	if (fObjectSpaceY > m_PositionBounds.m_fBottom) { return false; }

	return true;
}

Falcon::InstanceType Animation2DNetworkInstance::GetType() const
{
	return Falcon::InstanceType::kAnimation2D;
}

Float Animation2DNetworkInstance::GetCurrentMaxTime() const
{
	return m_pNetworkInstance->GetCurrentMaxTime();
}

Bool Animation2DNetworkInstance::GetTimeToEvent(HString sEventName, Float& fTimeToEvent) const
{
	return m_pNetworkInstance->GetTimeToEvent(sEventName, fTimeToEvent);
}

void Animation2DNetworkInstance::SetCondition(HString name, Bool bValue)
{
	m_pNetworkInstance->SetCondition(name, bValue);
}

void Animation2DNetworkInstance::SetParameter(HString name, Float fValue)
{
	m_pNetworkInstance->SetParameter(name, fValue);
}

void Animation2DNetworkInstance::TriggerTransition(HString name)
{
	m_pNetworkInstance->TriggerTransition(name);
}

void Animation2DNetworkInstance::AddTimestepOffset(Float fTimestepOffset)
{
	m_pNetworkInstance->AddTimestepOffset(fTimestepOffset);
}

void Animation2DNetworkInstance::AddBoneAttachment(Int16 iBoneIndex, SharedPtr<Falcon::Instance> pAttachment)
{
	m_pAttachments->AddAttachment(iBoneIndex, pAttachment);
}

static inline void GetActiveStatePath(
	const SharedPtr<Animation::NodeInstance>& pInstance,
	String& rs,
	UInt32& ruId)
{
	// Convenience.
	if (!pInstance.IsValid())
	{
		return;
	}

	using namespace Animation;
	switch (pInstance->GetType())
	{
	case NodeType::kBlend:
	{
		auto pBlend((Animation::BlendInstance const*)pInstance.GetPtr());
		auto const fMix = pBlend->GetCurrentMixParameter();

		if (fMix >= 0.5f)
		{
			GetActiveStatePath(pBlend->GetChildB(), rs, ruId);
		}
		else
		{
			GetActiveStatePath(pBlend->GetChildA(), rs, ruId);
		}
	}
	break;

	case NodeType::kPlayClip:
		break;

	case NodeType::kStateMachine:
	{
		auto pState((Animation::StateMachineInstance const*)pInstance.GetPtr());
		auto const name = pState->GetNewId();
		if (!name.IsEmpty())
		{
			if (!rs.IsEmpty())
			{
				rs.Append('/');
			}
			rs.Append(name.CStr(), name.GetSizeInBytes());
		}

		IncrementalHash(ruId, pState->GetTransitionCount());
		GetActiveStatePath(pState->GetNew(), rs, ruId);
	}
	break;
	};
}

String Animation2DNetworkInstance::GetActiveStatePath(UInt32& ruId) const
{
	// Build a forward slash separate list of active states.
	// When we encounter blend nodes, we pick the branch
	// with the higher weight.
	auto pRoot(m_pNetworkInstance->GetRoot());

	String sReturn;
	ruId = 0u;
	UI::GetActiveStatePath(pRoot, sReturn, ruId);
	return sReturn;
}

Int16 Animation2DNetworkInstance::GetBoneIndex(HString id) const
{
	const SharedPtr<Animation2D::DataDefinition const>& data = m_pNetworkInstance->GetData();
	return data->GetBoneIndex(id);
}

Vector2D Animation2DNetworkInstance::GetBonePosition(Int16 boneIndex) const
{
	const Animation2D::DataInstance& state = m_pNetworkInstance->GetState();
	const Animation2D::DataInstance::SkinningPalette& vSkinningPalette = state.GetSkinningPalette();

	if(boneIndex < 0 || (UInt)boneIndex >= vSkinningPalette.GetSize())
	{
		SEOUL_FAIL(String::Printf("Invalid bone index %d in GetBonePosition", boneIndex).CStr());
		return Vector2D::Zero();
	}

	return Matrix2x3::TransformPosition(GetTransform() * Matrix2x3::CreateScale(1.0f, -1.0f), vSkinningPalette[boneIndex].GetTranslation());
}

Vector2D Animation2DNetworkInstance::GetLocalBonePosition(Int16 boneIndex) const
{
	const Animation2D::DataInstance& state = m_pNetworkInstance->GetState();
	const Animation2D::DataInstance::SkinningPalette& vSkinningPalette = state.GetSkinningPalette();

	if(boneIndex < 0 || (UInt)boneIndex >= vSkinningPalette.GetSize())
	{
		SEOUL_FAIL(String::Printf("Invalid bone index %d in GetBonePosition", boneIndex).CStr());
		return Vector2D::Zero();
	}

	auto const v(vSkinningPalette[boneIndex].GetTranslation());
	return Vector2D(v.X, -v.Y);
}

Vector2D Animation2DNetworkInstance::GetLocalBoneScale(Int16 boneIndex) const
{
	const Animation2D::DataInstance& state = m_pNetworkInstance->GetState();
	const Animation2D::DataInstance::SkinningPalette& vSkinningPalette = state.GetSkinningPalette();

	if(boneIndex < 0 || (UInt)boneIndex >= vSkinningPalette.GetSize())
	{
		SEOUL_FAIL(String::Printf("Invalid bone index %d in GetBonePosition", boneIndex).CStr());
		return Vector2D::Zero();
	}

	Matrix2D mPreRotation;
	Matrix2D mRotation;
	(void)Matrix2D::Decompose(vSkinningPalette[boneIndex].GetUpper2x2(), mPreRotation, mRotation);

	return Vector2D(mPreRotation.M00, mPreRotation.M11);
}

Vector2D Animation2DNetworkInstance::GetWorldSpaceBonePosition(Int16 boneIndex) const
{
	const Animation2D::DataInstance& state = m_pNetworkInstance->GetState();
	const Animation2D::DataInstance::SkinningPalette& vSkinningPalette = state.GetSkinningPalette();

	if(boneIndex < 0 || (UInt)boneIndex >= vSkinningPalette.GetSize())
	{
		SEOUL_FAIL(String::Printf("Invalid bone index %d in GetWorldSpaceBonePosition", boneIndex).CStr());
		return Vector2D::Zero();
	}
	// This inversion of the Y axis is because Spine's coordinate system 0,0 is in the lower left and ours is in the upper left
	return Matrix2x3::TransformPosition(ComputeWorldTransform() * Matrix2x3::CreateScale(1.0f, -1.0f), vSkinningPalette[boneIndex].GetTranslation());
}

Matrix2x3 Animation2DNetworkInstance::GetWorldSpaceBoneTransform(Int16 boneIndex) const
{
	const Animation2D::DataInstance& state = m_pNetworkInstance->GetState();
	const Animation2D::DataInstance::SkinningPalette& vSkinningPalette = state.GetSkinningPalette();

	if(boneIndex < 0 || (UInt)boneIndex >= vSkinningPalette.GetSize())
	{
		SEOUL_FAIL(String::Printf("Invalid bone index %d in GetWorldSpaceBoneTransform", boneIndex).CStr());
		return Matrix2x3::Identity();
	}

	// This inversion of the Y axis is because Spine's coordinate system 0,0 is in the lower left and ours is in the upper left
	return ComputeWorldTransform() * Matrix2x3::CreateScale(1.0f, -1.0f) * vSkinningPalette[boneIndex] * Matrix2x3::CreateScale(1.0f, -1.0f);
}

void Animation2DNetworkInstance::AllDonePlaying(Bool& rbDone, Bool& rbLooping) const
{
	m_pNetworkInstance->AllDonePlaying(rbDone, rbLooping);
}

Bool Animation2DNetworkInstance::IsInStateTransition() const
{
	return m_pNetworkInstance->IsInStateTransition();
}

Bool Animation2DNetworkInstance::IsReady() const
{
	return m_pNetworkInstance->IsReady();
}

void Animation2DNetworkInstance::Tick(Float fDeltaTimeInSeconds)
{
	// Early out if not reachable/visible.
	CheckedPtr<Movie> pOwner(GetPtr<Movie>(m_hOwner));
	if (!pOwner.IsValid() || !pOwner->IsReachableAndVisible(this))
	{
		return;
	}

	m_pNetworkInstance->Tick(fDeltaTimeInSeconds);
	m_pAttachments->Update();
	++m_uTickCount;
}

Animation2DNetworkInstance::ClippedEntry::ClippedEntry()
	: m_ClipRectangle()
	, m_vIndices()
	, m_vRemap()
	, m_vVertices()
	, m_uClippedCount(0u)
{
}

Animation2DNetworkInstance::ClippedEntry::~ClippedEntry()
{
}

Animation2DNetworkInstance::PosedEntry::PosedEntry()
	: m_vClipped()
	, m_vVertices()
	, m_PositionBounds()
	, m_TexCoordsBounds()
	, m_uPoseCount(0u)
	, m_eRenderingFeature(Falcon::Render::Feature::kNone)
	, m_fEffectiveWidth(0.0f)
	, m_fEffectiveHeight(0.0f)
{
}

Animation2DNetworkInstance::PosedEntry::~PosedEntry()
{
	SafeDeleteVector(m_vClipped);
}

/**
 * Used to clip skinned data (in m_vScratchVertices) against
 * the visible rectangle of the mesh's current texture data.
 */
const Animation2DNetworkInstance::ClippedEntry& Animation2DNetworkInstance::Clip(
	const Animation2D::MeshAttachment& mesh,
	const Falcon::TextureReference& textureReference,
	PosedEntry& rPosedEntry)
{
	using namespace Falcon;

	// Now, we use the Falcon clipping functionality with the (perhaps odd looking) trick
	// of swapping the texture/position components - we're clipping in texture
	// space, against a rectangle formed by vVisibleOffset
	Rectangle const clipRectangle(Rectangle::Create(
		textureReference.m_vVisibleOffset.X,
		textureReference.m_vVisibleOffset.X + textureReference.m_vVisibleScale.X,
		textureReference.m_vVisibleOffset.Y,
		textureReference.m_vVisibleOffset.Y + textureReference.m_vVisibleScale.Y));

	auto& r = ResolveClippedEntry(rPosedEntry, clipRectangle);

	// Early out if already up-to-date.
	if (m_uTickCount == r.m_uClippedCount)
	{
		return r;
	}

	// Now in-sync.
	r.m_uClippedCount = m_uTickCount;

	// If the clip rectangle fully enclosing the computed texture coordinates,
	// we don't need to clip.
	if (Contains(clipRectangle, rPosedEntry.m_TexCoordsBounds))
	{
		r.m_vIndices.Assign(mesh.GetIndices().Begin(), mesh.GetIndices().End());
		r.m_vVertices.Assign(rPosedEntry.m_vVertices.Begin(), rPosedEntry.m_vVertices.End());
		return r;
	}

	// Remap is empty, need to recompute.
	if (r.m_vRemap.IsEmpty())
	{
		// Populate initial entry.
		{
			// Local reference to the mesh's texture coordinates and indices.
			auto const& vI = mesh.GetIndices();
			auto const& vT = mesh.GetTexCoords();

			// Vertex count equal to texture entry count.
			UInt32 const uVertices = vT.GetSize();

			// Populate.
			r.m_vIndices.Assign(vI.Begin(), vI.End());
			r.m_vRemap.ResizeNoInitialize(uVertices);
			for (UInt32 i = 0u; i < uVertices; ++i)
			{
				auto& rOut = r.m_vRemap[i];
				auto const& vIn = vT[i];

				rOut.Reset(vIn, i);
			}
		}

		// Perform the actual clip in texture space. Note our usage of a different
		// threshold - this is important, as our "positions" are texture coordinates
		// and the default clipping threshold is in pixels, not texels.
		Clipper::MeshClip(
			*m_pMeshClipCache,
			clipRectangle,
			Falcon::TriangleListDescription::kNotSpecific,
			rPosedEntry.m_TexCoordsBounds,
			r.m_vIndices,
			r.m_vIndices.GetSize(),
			r.m_vRemap,
			r.m_vRemap.GetSize(),
			kfTextureSpaceClipTolerance);
	}

	// Apply remapping to the skinned vertices to produce clipped
	// vertices.

	// Populate the output, clipped vertices.
	{
		UInt32 const uSize = r.m_vRemap.GetSize();
		r.m_vVertices.ResizeNoInitialize(uSize);

		// Color is constant across the vertices, so just reuse the first
		// value if non-empty.
		auto const color = (rPosedEntry.m_vVertices.IsEmpty() ? RGBA::Black() : rPosedEntry.m_vVertices[0].m_ColorMultiply);
		for (UInt32 i = 0u; i < uSize; ++i)
		{
			auto const& rIn = r.m_vRemap[i];
			auto& rOut = r.m_vVertices[i];

			// Compute v - it is a weight accumulation of the unclipped
			// vertices.
			Vector2D v(Vector2D::Zero());
			for (UInt32 j = 0u; j < rIn.m_uCount; ++j)
			{
				auto const& contrib = rIn.m_a[j];
				auto const& vSource = rPosedEntry.m_vVertices[contrib.m_u].m_vP;
				v += vSource * contrib.m_f;
			}

			// Output vertex - weighted position, fixed texture coordinates
			// and color.
			rOut = StandardVertex2D::Create(
				v.X,
				v.Y,
				color,
				RGBA::TransparentBlack(),
				rIn.m_v.X,
				rIn.m_v.Y);
		}
	}

	// Done,
	return r;
}

void Animation2DNetworkInstance::CloneTo(Falcon::AddInterface& rInterface, Animation2DNetworkInstance& rClone) const
{
	Falcon::Instance::CloneTo(rInterface, rClone);
	rClone.m_hOwner = m_hOwner;
	rClone.m_pNetworkInstance.Reset(static_cast<Animation2D::NetworkInstance*>(m_pNetworkInstance->Clone()));
	rClone.m_vShadowOffset = rClone.m_vShadowOffset;
	rClone.m_ActivePalette = m_ActivePalette;
	rClone.m_ActiveSkin = m_ActiveSkin;
	rClone.m_bShadowCast = m_bShadowCast;
}

/** Create or retrieve the clipped entry data for the given posed entry. */
Animation2DNetworkInstance::ClippedEntry& Animation2DNetworkInstance::ResolveClippedEntry(PosedEntry& r, const Falcon::Rectangle& clipRectangle)
{
	for (auto i = r.m_vClipped.Begin(); r.m_vClipped.End() != i; ++i)
	{
		if ((*i)->m_ClipRectangle == clipRectangle)
		{
			return *(*i);
		}
	}

	auto pReturn = SEOUL_NEW(MemoryBudgets::UIRuntime) ClippedEntry;
	pReturn->m_ClipRectangle = clipRectangle;
	r.m_vClipped.PushBack(pReturn);

	return *pReturn;
}

/** Create and/or retrieve the FilePath resolution based on current palette. */
FilePath Animation2DNetworkInstance::ResolveFilePath(FilePath filePath)
{
	FilePath ret;
	if (!m_tPalette.GetValue(filePath, ret))
	{
		// An empty palette just resolves to a one-to-one mapping.
		if (m_ActivePalette.IsEmpty())
		{
			ret = filePath;
		}
		// Otherwise, the palette string is used at the directory
		// level sibling to the animation file.
		else
		{
			// Base is the relative path up to the folder of the Spine
			// animation file.
			String const sBase = Path::GetDirectoryName(
				m_pNetworkInstance->GetDataHandle().GetKey().GetRelativeFilenameWithoutExtension().ToString());

			// sOrig is the full Source relative filename to the original requested image
			// file.
			String const sOrig(filePath.GetRelativeFilename());

			// To constructed the palette path, we remove the base from sOrig,
			// then remove one more directory from the left (the original sub folder
			// we are replacing). +1 to remove the separator - we can be this
			// rigid since FilePaths are normalized and have a very
			// predictable structure.
			String sSuffix(sOrig.Substring(sBase.GetSize() + 1u));
			auto const uNextSeparator = sSuffix.Find(Path::GetDirectorySeparatorChar(keCurrentPlatform));
			if (String::NPos != uNextSeparator)
			{
				sSuffix = sSuffix.Substring(uNextSeparator + 1u);
			}

			// Now assemble the total path.
			ret = FilePath::CreateContentFilePath(
				Path::Combine(sBase, String(m_ActivePalette), sSuffix));

#if !SEOUL_SHIP
			// See comment on m_bCheckSourceFilesForPalettes for why
			// we check source files (conditionally) instead of cooked
			// files in developer builds.
			if (m_bCheckSourceFilesForPalettes)
			{
				if (!FileManager::Get()->ExistsInSource(ret))
				{
					ret = filePath;
				}
			}
			else
#endif // /#if !SEOUL_SHIP

			// Check if the file exists - if not, it's not overriden
			// by the palette, so fall back to the defaults.
			if (!FileManager::Get()->Exists(ret))
			{
				ret = filePath;
			}
		}

		// Done, cache the lookup.
		SEOUL_VERIFY(m_tPalette.Insert(filePath, ret).Second);
	}

	return ret;
}

/** Create or retrieve the posed entry data for the given attachment. */
Animation2DNetworkInstance::PosedEntry& Animation2DNetworkInstance::ResolvePosedEntry(const SharedPtr<Animation2D::Attachment>& pAttachment)
{
	auto p(pAttachment);

	// For linked meshes, use the target mesh for the lookup instead.
	if (p->GetType() == Animation2D::AttachmentType::kLinkedMesh)
	{
		p = ((Animation2D::LinkedMeshAttachment*)p.GetPtr())->GetParent();
	}

	auto ppPosedEntry = m_tPosed.Find(p.GetPtr());
	if (nullptr == ppPosedEntry)
	{
		auto const pPosedEntry = SEOUL_NEW(MemoryBudgets::UIRuntime) PosedEntry;
		auto const e = m_tPosed.Insert(p.GetPtr(), pPosedEntry);
		SEOUL_ASSERT(e.Second);
		ppPosedEntry = &(e.First->Second);
	}

	return *(*ppPosedEntry);
}

} // namespace UI

} // namespace Seoul

#endif // /#if SEOUL_WITH_ANIMATION_2D
