/**
 * \file UIFxRenderer.cpp
 * \brief Specialization of IFxRenderer for binding into the UI system's
 * rendering backend.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Camera.h"
#include "Engine.h"
#include "FalconClipper.h"
#include "FalconRenderDrawer.h"
#include "FalconRenderPoser.h"
#include "UIFxRenderer.h"
#include "UIManager.h"
#include "UIRenderer.h"

namespace Seoul::UI
{

/** Utility, converts a SeoulEngine Matrix4D to a Matrix2x3. */
inline static Matrix2x3 ToFalconMatrix(const Matrix4D& m)
{
	Matrix2x3 mReturn;
	mReturn.M00 = m.M00;
	mReturn.M01 = m.M01;
	mReturn.M02 = m.M03;
	mReturn.M10 = m.M10;
	mReturn.M11 = m.M11;
	mReturn.M12 = m.M13;
	return mReturn;
}

/** Given an extended Fx render mode, convert into the corresponding Falcon blend mode (as a feature). */
inline static Falcon::Render::Feature::Enum ExtendedToFeature(FxRendererMode eMode)
{
	using namespace Falcon::Render::Feature;

	// One-to-one mapping, with fallback if we get out of sync.
	switch (eMode)
	{
	case FxRendererMode::kExtended_InvSrcAlpha_One: return kExtended_InvSrcAlpha_One;
	case FxRendererMode::kExtended_InvSrcColor_One: return kExtended_InvSrcColor_One;
	case FxRendererMode::kExtended_One_InvSrcColor: return kExtended_One_InvSrcColor;
	case FxRendererMode::kExtended_One_SrcAlpha: return kExtended_One_SrcAlpha;
	case FxRendererMode::kExtended_One_SrcColor: return kExtended_One_SrcColor;
	case FxRendererMode::kExtended_SrcAlpha_InvSrcAlpha: return kExtended_SrcAlpha_InvSrcAlpha;
	case FxRendererMode::kExtended_SrcAlpha_InvSrcColor: return kExtended_SrcAlpha_InvSrcColor;
	case FxRendererMode::kExtended_SrcAlpha_One: return kExtended_SrcAlpha_One;
	case FxRendererMode::kExtended_SrcAlpha_SrcAlpha: return kExtended_SrcAlpha_SrcAlpha;
	case FxRendererMode::kExtended_SrcColor_InvSrcAlpha: return kExtended_SrcColor_InvSrcAlpha;
	case FxRendererMode::kExtended_SrcColor_InvSrcColor: return kExtended_SrcColor_InvSrcColor;
	case FxRendererMode::kExtended_SrcColor_One: return kExtended_SrcColor_One;
	case FxRendererMode::kExtended_Zero_InvSrcColor: return kExtended_Zero_InvSrcColor;
	case FxRendererMode::kExtended_Zero_SrcColor: return kExtended_Zero_SrcColor;
	default:
		SEOUL_FAIL("Out-of-sync enum or ExtendedToFeature called on non-extended mode.");
		return Falcon::Render::Feature::kNone;
	};
}

/** Utility, used by both variations of GetVertices. */
inline static Falcon::ShapeVertex Create(
	const Vector2D& vCornerPosition,
	const Vector2D& vCornerTexcoord,
	const Vector4D& vTexcoordScaleAndShift,
	RGBA cColor)
{
	// NOTE: We used to clamp the texture coordinates to [0, 1] here so they are
	// compatible with texture atlas. This is premature and unnecessary
	// (they will be clamped by the rendering backend as necessary). This also
	// potentially fights and can produce incorrect results for some vTexcoordScaleAndShift
	// values - we rely on the clipping functionality that is applied based on the scale/shift
	// value and then the final backend clamp to handling the various cases.
	Vector2D const vTexcoords = Vector2D::ComponentwiseMultiply(vCornerTexcoord, vTexcoordScaleAndShift.GetXY()) + vTexcoordScaleAndShift.GetZW();

	Falcon::ShapeVertex const ret = Falcon::ShapeVertex::Create(
		vCornerPosition.X,
		vCornerPosition.Y,
		RGBA::Create(cColor.m_R, cColor.m_G, cColor.m_B, cColor.m_A),
		RGBA::TransparentBlack(),
		vTexcoords.X,
		vTexcoords.Y);

	return ret;
}

/** Utility, used to finalize vertices created by both variations of GetVertices(). */
inline static void FinishVertices(
	FxRendererMode eMode,
	UInt8 uAlphaClampMin,
	UInt8 uAlphaClampMax,
	Falcon::ShapeVertex aVertices[4])
{
	switch (eMode)
	{
	case FxRendererMode::kAdditive:
		aVertices[0].m_ColorAdd.m_BlendingFactor = 255;
		aVertices[1].m_ColorAdd.m_BlendingFactor = 255;
		aVertices[2].m_ColorAdd.m_BlendingFactor = 255;
		aVertices[3].m_ColorAdd.m_BlendingFactor = 255;
		break;
	case FxRendererMode::kAlphaClamp:
	case FxRendererMode::kColorAlphaClamp:
		{
			auto const c = ColorAdd::Create(
				uAlphaClampMin,
				uAlphaClampMax,
				0,
				128); /* Special value that indicates alpha clamp rendering. */

			aVertices[0].m_ColorAdd = c;
			aVertices[1].m_ColorAdd = c;
			aVertices[2].m_ColorAdd = c;
			aVertices[3].m_ColorAdd = c;
		}
		break;
	case FxRendererMode::kNormal: // fall-through
	default: // also includes any extended blend modes.
		break;
	}
}

/**
 * Utility, generates vertices ready for Falcon UI from FX particle data.
 *
 * Vertex construction here differs from stock Falcon code
 * (e.g. see FalconBitmapInstance), due to the FX system being a relic
 * from early SeoulEngine days, using a coordinate system with (0, 0) in the
 * lower-left corner, with +Y pointing up. Falcon uses a coordinate system
 * with (0, 0) in the upper-left corner, with +Y pointing down. As a result,
 * vertex order differs here, and the texture V component must be flipped
 * relative to the position Y.
 */
inline static void GetVertices(
	const FxParticle& renderableParticle,
	FxRendererMode eMode,
	Falcon::ShapeVertex aVertices[4])
{
	Float const fU = 1.0f;
	Float const fV = 1.0f;

	aVertices[0] = Create(Vector2D(-0.5f, -0.5f), Vector2D(0 , fV), renderableParticle.m_vTexcoordScaleAndShift, renderableParticle.m_Color);
	aVertices[1] = Create(Vector2D( 0.5f, -0.5f), Vector2D(fU, fV), renderableParticle.m_vTexcoordScaleAndShift, renderableParticle.m_Color);
	aVertices[2] = Create(Vector2D( 0.5f,  0.5f), Vector2D(fU,  0), renderableParticle.m_vTexcoordScaleAndShift, renderableParticle.m_Color);
	aVertices[3] = Create(Vector2D(-0.5f,  0.5f), Vector2D(0 ,  0), renderableParticle.m_vTexcoordScaleAndShift, renderableParticle.m_Color);

	FinishVertices(eMode, renderableParticle.m_uAlphaClampMin, renderableParticle.m_uAlphaClampMax, aVertices);
}

/**
 * Utility, generates vertices ready for Falcon UI from FX particle data.
 * Apply the visible rectangle of the texture that will be used to draw,
 * to optimize rendering and reduce overdraw with areas of the texture
 * that are completely transparent.
 *
 * Vertex construction here differs from stock Falcon code
 * (e.g. see FalconBitmapInstance), due to the FX system being a relic
 * from early SeoulEngine days, using a coordinate system with (0, 0) in the
 * lower-left corner, with +Y pointing up. Falcon uses a coordinate system
 * with (0, 0) in the upper-left corner, with +Y pointing down. As a result,
 * vertex order differs here, and the texture V component must be flipped
 * relative to the position Y.
 *
 * Can fail for some texture scale/shift values that shift the entire
 * visible area outside the particle quad.
 */
inline static Bool TryGetVertices(
	const FxParticle& renderableParticle,
	FxRendererMode eMode,
	Falcon::ShapeVertex aVertices[4],
	const Vector2D& vVisibleOffset,
	const Vector2D& vVisibleScale)
{
	// Two possibilities - if the particle's texture scale and shift
	// is the identity (scale of (1, 1), shift of (0, 0)), we can
	// use simple recomputations based on the visible offset and scale.
	if (Vector4D(1, 1, 0, 0) == renderableParticle.m_vTexcoordScaleAndShift)
	{
		// Texture coordinates are exactly equal to the
		// visible rectangle, since the base is on [0, 1].
		float const fTU0 = vVisibleOffset.X;
		float const fTV0 = vVisibleOffset.Y;
		float const fTU1 = vVisibleOffset.X + vVisibleScale.X;
		float const fTV1 = vVisibleOffset.Y + vVisibleScale.Y;

		// Position is just the texture coordinates offset, since
		// the base is on [0, 1].
		float const fX0 = (fTU0 - 0.5f);
		float const fY0 = (1.0f - fTV0 - 0.5f);
		float const fX1 = (fTU1 - 0.5f);
		float const fY1 = (1.0f - fTV1 - 0.5f);

		aVertices[0] = Create(Vector2D(fX0, fY0), Vector2D(fTU0, fTV0), renderableParticle.m_vTexcoordScaleAndShift, renderableParticle.m_Color);
		aVertices[1] = Create(Vector2D(fX1, fY0), Vector2D(fTU1, fTV0), renderableParticle.m_vTexcoordScaleAndShift, renderableParticle.m_Color);
		aVertices[2] = Create(Vector2D(fX1, fY1), Vector2D(fTU1, fTV1), renderableParticle.m_vTexcoordScaleAndShift, renderableParticle.m_Color);
		aVertices[3] = Create(Vector2D(fX0, fY1), Vector2D(fTU0, fTV1), renderableParticle.m_vTexcoordScaleAndShift, renderableParticle.m_Color);

		FinishVertices(eMode, renderableParticle.m_uAlphaClampMin, renderableParticle.m_uAlphaClampMax, aVertices);
		return true;
	}
	// Otherwise, we need to use a more complex method that treats the
	// rectangle formed by vVisibleOffset and vVisibleScale as a clipping
	// rectangle.
	else
	{
		// Use the initial vertex generation with no visible rectangle.
		GetVertices(renderableParticle, eMode, aVertices);

		// Now, we use the Falcon clipping functionality with the (perhaps odd looking) trick
		// of swapping the texture/position components - we're clipping in texture
		// space, against a rectangle formed by vVisibleOffset
		Falcon::Rectangle const clipRectangle(Falcon::Rectangle::Create(
			vVisibleOffset.X,
			vVisibleOffset.X + vVisibleScale.X,
			vVisibleOffset.Y,
			vVisibleOffset.Y + vVisibleScale.Y));

		// Swap texture coordinates into the positions to use for clipping.
		for (Int32 i = 0; i < 4; ++i)
		{
			Seoul::Swap(aVertices[i].m_vP.X, aVertices[i].m_vT.X);
			Seoul::Swap(aVertices[i].m_vP.Y, aVertices[i].m_vT.Y);
		}

		// Clip - can fully clip if texture coordinates completely
		// moved the image outside its quad, otherwise expected to
		// be 0 (no clip) or 4 (clipped, but kept the same number
		// of vertices).
		Int32 const iResult = Falcon::Clipper::ConvexClip(
			clipRectangle,
			aVertices,
			4,
			aVertices,
			1e-4f);
		(void)iResult;
		SEOUL_ASSERT(iResult < 0 || 0 == iResult || 4 == iResult);

		// Fully culled.
		if (iResult < 0)
		{
			return false;
		}

		// Swap the texture coordinates and positions back.
		for (Int32 i = 0; i < 4; ++i)
		{
			Seoul::Swap(aVertices[i].m_vP.X, aVertices[i].m_vT.X);
			Seoul::Swap(aVertices[i].m_vP.Y, aVertices[i].m_vT.Y);
		}

		return true;
	}
}

FxRenderer::FxRenderer()
	: m_pPoser()
	, m_vModes()
	, m_vFxBuffer()
	, m_uLastPoseFrame(0u)

{
}

FxRenderer::~FxRenderer()
{
}

void FxRenderer::BeginPose(Falcon::Render::Poser& rPoser)
{
	m_pPoser = &rPoser;

	UInt32 const uFrameCount = Engine::Get()->GetFrameCount();
	if (m_uLastPoseFrame != uFrameCount)
	{
		m_uLastPoseFrame = uFrameCount;
		m_vModes.Clear();
		m_vFxBuffer.Clear();
	}
}

void FxRenderer::EndPose()
{
	m_pPoser.Reset();
}

void FxRenderer::Draw(
	Falcon::Render::Drawer& rDrawer,
	const Falcon::Rectangle& worldBoundsPreClip,
	const Matrix2x3& mWorld,
	const Falcon::ColorTransformWithAlpha& /*cxWorld*/,
	const Falcon::TextureReference& textureReference,
	Int32 iSubInstanceId)
{
	auto const& renderableParticle = m_vFxBuffer[iSubInstanceId];
	auto const eMode = m_vModes[iSubInstanceId];

	Falcon::ShapeVertex aVertices[4];

	// Generate vertices initially without visible scale/offset, to establish render size.
	GetVertices(renderableParticle, eMode, aVertices);

	// If visible offset or scale are defined, regenerate vertices
	// to keep the quads tightly fitting. This is a fill rate optimization.
	if (textureReference.m_vVisibleOffset != Vector2D::Zero() ||
		textureReference.m_vVisibleScale != Vector2D::One())
	{
		// Now regenerate with visible scale/offset.
		if (!TryGetVertices(
			renderableParticle,
			eMode,
			aVertices,
			textureReference.m_vVisibleOffset,
			textureReference.m_vVisibleScale))
		{
			return;
		}
	}

	// Make sure we signal the need for alpha shape if alpha clamp is enabled.
	if (FxRendererMode::kAlphaClamp == eMode || FxRendererMode::kColorAlphaClamp == eMode)
	{
		rDrawer.DrawTriangleList(
			worldBoundsPreClip,
			textureReference,
			mWorld,
			aVertices,
			4,
			Falcon::TriangleListDescription::kQuadList,
			(FxRendererMode::kColorAlphaClamp == eMode) ? Falcon::Render::Feature::kExtended_ColorAlphaShape : Falcon::Render::Feature::kAlphaShape);
	}
	// Extended blend mode handling.
	else if (FxRendererModeIsExtended(eMode))
	{
		rDrawer.DrawTriangleList(
			worldBoundsPreClip,
			textureReference,
			mWorld,
			aVertices,
			4,
			Falcon::TriangleListDescription::kQuadList,
			ExtendedToFeature(eMode));
	}
	// Fallback - simple/common case.
	else
	{
		auto const eFeature = (renderableParticle.m_Color != RGBA::White()
			? Falcon::Render::Feature::kColorMultiply
			: Falcon::Render::Feature::kNone);

		rDrawer.DrawTriangleList(
			worldBoundsPreClip,
			textureReference,
			mWorld,
			aVertices,
			4,
			Falcon::TriangleListDescription::kQuadList,
			eFeature);
	}
}

Bool FxRenderer::CastShadow() const
{
	return false;
}

Vector2D FxRenderer::GetShadowPlaneWorldPosition() const
{
	return Vector2D::Zero();
}

/**
 * Custom Camera that maps a fixed psuedo 3D world space for rendering Fx
 * as part of the UI system. Used to map 3D world space Fx into UI space.
 */
Camera& FxRenderer::GetCamera() const
{
	return Manager::Get()->GetRenderer().GetCamera();
}

IFxRenderer::Buffer& FxRenderer::LockFxBuffer()
{
	return m_vFxBuffer;
}

void FxRenderer::UnlockFxBuffer(
	UInt32 uParticles,
	FilePath textureFilePath,
	FxRendererMode eMode,
	Bool /*bNeedsScreenAlign*/)
{
	// Early out if no instances drawn.
	if (0u == uParticles)
	{
		return;
	}

	// Fill in the color transform based on mode.
	Falcon::ColorTransformWithAlpha cxWorld;
	cxWorld.m_uBlendingFactor = (FxRendererMode::kAdditive == eMode
		? 255u
		: ((FxRendererMode::kAlphaClamp == eMode || FxRendererMode::kColorAlphaClamp == eMode) ? 127u : 0u));
	Falcon::Render::Feature::Enum eFeature = Falcon::Render::Feature::kNone;
	switch (eMode)
	{
	case FxRendererMode::kAdditive: eFeature = Falcon::Render::Feature::kColorAdd; break;
	case FxRendererMode::kAlphaClamp: eFeature = Falcon::Render::Feature::kAlphaShape; break;
	case FxRendererMode::kColorAlphaClamp: eFeature = Falcon::Render::Feature::kExtended_ColorAlphaShape; break;
	case FxRendererMode::kNormal: eFeature = Falcon::Render::Feature::kColorMultiply; break;
	default:
		// Addition handling if an extended mode.
		if (FxRendererModeIsExtended(eMode))
		{
			eFeature = ExtendedToFeature(eMode);
		}
		break;
	}

	UInt32 const uSize = m_vFxBuffer.GetSize();
	UInt32 const uStart = (uSize - uParticles);
	m_vModes.Resize(uSize, eMode);

	// Convert FX "world space" into Falcon's movie "world space".
	auto const& renderer = Manager::Get()->GetRenderer();
	const Camera& camera = renderer.GetCamera();
	Vector4D const v(renderer.GetViewProjectionTransform());
	Matrix4D const mWorldToUIWorldSpace =
		(Matrix4D::CreateTranslation(Vector3D(v.GetZW(), 0.0f)) * Matrix4D::CreateScale(Vector3D(v.GetXY(), 1.0f))).Inverse() *
		camera.GetViewProjectionMatrix();

	for (UInt32 i = uStart; i < uSize; ++i)
	{
		const FxParticle& renderableParticle = m_vFxBuffer[i];
		Matrix2x3 const mWorld = ToFalconMatrix(
			mWorldToUIWorldSpace * renderableParticle.m_mTransform);
		auto const bounds(Falcon::Rectangle::Create(-0.5f, 0.5f, -0.5f, 0.5f));
		auto const worldBounds(TransformRectangle(mWorld, bounds));

		// Replace depth prior to texture resolution so projection calculation will
		// be correct.
		auto const old = m_pPoser->ReplaceDepth3D(renderableParticle.m_mTransform.M23);

		// Now resolve texture.
		Falcon::TextureReference reference;
		if (Falcon::Render::PoserResolveResult::kSuccess == m_pPoser->ResolveTextureReference(
			worldBounds,
			this,
			m_pPoser->GetRenderThreshold(1.0f, 1.0f, mWorld),
			textureFilePath,
			reference))
		{
			// Issue the pose on success.
			auto const worldOcclusion(Falcon::ComputeOcclusionRectangle(mWorld, reference, bounds));
			m_pPoser->Pose(
				worldBounds,
				this,
				mWorld,
				cxWorld,
				reference,
				worldOcclusion,
				eFeature,
				(Int32)i);
		}

		// Restore depth.
		(void)m_pPoser->ReplaceDepth3D(old.First, old.Second);
	}
}

} // namespace Seoul::UI
