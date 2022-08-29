/**
 * \file FalconInstance.h
 * \brief The base class of all node instances in a Falcon scene graph.
 *
 * The Falcon scene graph is directly analagous to the Flash scene
 * graph. All subclasses of Falcon::Instance are leaf nodes, except
 * for Falcon::MovieClipInstance, which is the one and only interior
 * node (it can have child nodes via its DisplayList).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_INSTANCE_H
#define FALCON_INSTANCE_H

#include "FalconAdvanceInterface.h"
#include "FalconRenderable.h"
#include "FalconTypes.h"
#include "ReflectionDeclare.h"
#include "SharedPtr.h"
#include "VmStats.h"
namespace Seoul { namespace Falcon { class ClipStack; } }
namespace Seoul { namespace Falcon { class FCNFile; } }
namespace Seoul { namespace Falcon { class MovieClipInstance; } }
namespace Seoul { namespace Falcon { namespace Render { class Drawer; } } }
namespace Seoul { namespace Falcon { namespace Render { class Poser; } } }
namespace Seoul { namespace Falcon { struct TextureReference; } }

namespace Seoul::Falcon
{

enum class InstanceType
{
	kUnknown,
	// TODO: Custom type used outside of Falcon, this
	// is a hack.
	kAnimation2D,
	kBitmap,
	kCustom,
	kEditText,
	// TODO: Custom type used outside of Falcon, this
	// is a hack.
	kFx,
	kMovieClip,
	kShape,
};

class Instance SEOUL_ABSTRACT : public Renderable
{
public:
	SEOUL_REFLECTION_POLYMORPHIC_BASE(Instance);

	/** Defined by bits allocated to clip depth in private members. */
	static const UInt32 kuMaxClipDepth = (1 << 16) - 1;

	virtual ~Instance()
	{
#if !SEOUL_SHIP
		--g_VmStats.m_UINodes;
#endif
	}

	void AddWatcher()
	{
		SEOUL_ASSERT(m_uWatcherCount < 255u); // Catch overflow.
		++m_uWatcherCount;
	}

	virtual void Advance(AdvanceInterface& rInterface)
	{
		// Nop
	}

	virtual void AdvanceToFrame0(AddInterface& rInterface)
	{
	}

	virtual Instance* Clone(AddInterface& rInterface) const = 0;

	Bool ComputeBounds(Rectangle& rBounds)
	{
		Rectangle localBounds;
		if (ComputeLocalBounds(localBounds))
		{
			rBounds = TransformRectangle(GetTransform(), localBounds);
			return true;
		}

		return false;
	}

	virtual Bool ComputeLocalBounds(Rectangle& rBounds) = 0;

	virtual void ComputeMask(
		const Matrix2x3& mParent,
		const ColorTransformWithAlpha& cxParent,
		Render::Poser& rPoser)
	{
		// No contribution by default.
	}

	Float ComputeWorldDepth3D() const;

	Vector2D ComputeWorldPosition() const;

	Matrix2x3 ComputeWorldTransform() const;

	virtual void Pose(
		Render::Poser& /*rPoser*/,
		const Matrix2x3& /*mParent*/,
		const ColorTransformWithAlpha& /*cxParent*/)
	{
		// Nop
	}

	// Developer only feature, traversal for rendering hit testable areas.
#if SEOUL_ENABLE_CHEATS
	virtual void PoseInputVisualization(
		Render::Poser& /*rPoser*/,
		const Matrix2x3& /*mParent*/,
		RGBA /*color*/)
	{
		// Nop
	}
#endif

	virtual void Draw(
		Render::Drawer& /*rDrawer*/,
		const Rectangle& /*worldBoundsPreClip*/,
		const Matrix2x3& /*mParent*/,
		const ColorTransformWithAlpha& /*cxParent*/,
		const TextureReference& /*textureReference*/,
		Int32 /*iSubInstanceId*/) SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual Bool ExactHitTest(
		const Matrix2x3& mParent,
		Float32 fWorldX,
		Float32 fWorldY,
		Bool bIgnoreVisibility = false) const
	{
		return HitTest(mParent, fWorldX, fWorldY, bIgnoreVisibility);
	}

	Float GetAlpha() const
	{
		return m_ColorTransform.m_fMulA;
	}

	Float GetBlendingFactor() const
	{
		return (Float)m_ColorTransform.m_uBlendingFactor / 255.0f;
	}

	UInt16 GetClipDepth() const
	{
		return m_uClipDepth;
	}

	ColorTransform GetColorTransform() const
	{
		return m_ColorTransform.GetTransform();
	}

	const ColorTransformWithAlpha& GetColorTransformWithAlpha() const
	{
		return m_ColorTransform;
	}

	UInt16 GetDefinitionID() const
	{
		return m_uDefinitionID;
	}

	// TODO: Likely, selectively implementing
	// Depth3D will result in unexpected behavior.
	//
	// We should just push the 3D depth down into Falcon::Instance.
	virtual Float GetDepth3D() const { return 0.0f; }
	virtual void SetDepth3D(Float fDepth3D) {}

	Float GetWorldDepth3D() const;

	Bool GetIgnoreDepthProjection() const { return m_bIgnoreDepthProjection; }
	void SetIgnoreDepthProjection(Bool bIgnoreDepthProjection) { m_bIgnoreDepthProjection = bIgnoreDepthProjection; }

	UInt16 GetDepthInParent() const
	{
		return m_uDepthInParent;
	}

	HString GetName() const
	{
		return m_Name;
	}

	void GatherFullName(String& rs) const;

	MovieClipInstance* GetParent() const
	{
		return m_pParent;
	}

	Vector2D GetPosition() const
	{
		return m_mTransform.GetTranslation();
	}

	Float GetPositionX() const
	{
		return m_mTransform.TX;
	}

	Float GetPositionY() const
	{
		return m_mTransform.TY;
	}

	Float GetRotationInDegrees() const
	{
		return RadiansToDegrees(GetRotationInRadians());
	}

	Float GetRotationInRadians() const
	{
		// NOTE: Relies on the assumption that scale/skew
		// can be approximated as uniform magnitude
		// along an axis (x or y), such that the Atan2
		// of those values is unaffected by the magnitude
		// (since it is a uniform scaling factor on both
		// terms).
		auto const fDet = m_mTransform.DeterminantUpper2x2();
		if (IsZero(fDet))
		{
			return 0.0f; // For consistency - a negative determinant means we've lost rotation information.
		}
		else
		{
			if (m_bNegativeScaleX)
			{
				return Atan2(-m_mTransform.M10, -m_mTransform.M00);
			}
			else
			{
				return Atan2(m_mTransform.M10, m_mTransform.M00);
			}
		}
	}

	Vector2D GetScale() const
	{
		return Vector2D(GetScaleX(), GetScaleY());
	}

	Float GetScaleX() const
	{
		// See also: gameswf_types.cpp, line 304. Adapted with x/y negative scale tracking.

		// Length of column 0.
		auto const fAbsX = Sqrt(m_mTransform.M00 * m_mTransform.M00 + m_mTransform.M10 * m_mTransform.M10);
		return (m_bNegativeScaleX ? -fAbsX : fAbsX);
	}

	Float GetScaleY() const
	{
		// See also: gameswf_types.cpp, line 319. Adapted with x/y negative scale tracking.

		// Length of column 1.
		auto const fAbsY = Sqrt(m_mTransform.M11 * m_mTransform.M11 + m_mTransform.M01 * m_mTransform.M01);
		return (m_bNegativeScaleY ? -fAbsY : fAbsY);
	}

	Bool GetScissorClip() const
	{
		return m_bScissorClip;
	}

	const Matrix2x3& GetTransform() const
	{
		return m_mTransform;
	}

	virtual InstanceType GetType() const = 0;

	Bool GetVisible() const
	{
		return m_bVisible;
	}

	Bool GetVisibleAndNotAlphaZero() const
	{
		return (m_bVisible && 0.0f != m_ColorTransform.m_fMulA);
	}

	UInt8 GetWatcherCount() const
	{
		return m_uWatcherCount;
	}

	virtual Bool HitTest(
		const Matrix2x3& mParent,
		Float32 fWorldX,
		Float32 fWorldY,
		Bool bIgnoreVisibility = false) const = 0;

	void RemoveWatcher()
	{
		SEOUL_ASSERT(m_uWatcherCount > 0);
		--m_uWatcherCount;
	}

	void SetAlpha(Float fAlpha)
	{
		m_ColorTransform.m_fMulA = fAlpha;
	}

	void SetBlendingFactor(Float fBlendingFactor)
	{
		m_ColorTransform.m_uBlendingFactor = (UInt8)Clamp(fBlendingFactor * 255.0f + 0.5f, 0.0f, 255.0f);
	}

	void SetClipDepth(UInt16 uClipDepth)
	{
		m_uClipDepth = uClipDepth;
	}

	void SetColorTransform(const ColorTransform& cx)
	{
		m_ColorTransform.SetTransform(cx);
	}

	void SetColorTransformWithAlpha(const ColorTransformWithAlpha& cx)
	{
		m_ColorTransform = cx;
	}

	void SetName(HString name);

	void SetPosition(Float fX, Float fY)
	{
		m_mTransform.TX = fX;
		m_mTransform.TY = fY;
	}

	void SetPosition(const Vector2D& v)
	{
		SetPosition(v.X, v.Y);
	}

	void SetPositionX(Float fX)
	{
		m_mTransform.TX = fX;
	}

	void SetPositionY(Float fY)
	{
		m_mTransform.TY = fY;
	}

	void SetRotationInDegrees(Float fAngle)
	{
		SetRotationInRadians(DegreesToRadians(fAngle));
	}

	void SetRotationInRadians(Float fAngle)
	{
		// Compute delta rotation.
		auto const fDelta = (fAngle - GetRotationInRadians());

		// Apply.
		m_mTransform.SetUpper2x2(
			Matrix2D::CreateRotation(fDelta) * m_mTransform.GetUpper2x2());
	}

	void SetScale(Float fX, Float fY)
	{
		SetScaleX(fX);
		SetScaleY(fY);
	}

	void SetScaleX(Float fX)
	{
		// See also: gameswf_types.cpp, line 304. Adapted for setting X individually.

		// Length of column 0.
		auto const fAbsX = Sqrt(m_mTransform.M00 * m_mTransform.M00 + m_mTransform.M10 * m_mTransform.M10);
		if (fAbsX <= fEpsilon) // Zero.
		{
			m_mTransform.M00 = fX;
			m_mTransform.M10 = 0.0f;
		}
		else
		{
			// Rescale existing scale by multiplier - necessary to maintain skew
			// in light of new scale value.
			auto fFactor = (fX / fAbsX);
			if (m_bNegativeScaleX)
			{
				fFactor = -fFactor;
			}

			m_mTransform.M00 *= fFactor;
			m_mTransform.M10 *= fFactor;
		}

		// Track whether scale is negative or not.
		m_bNegativeScaleX = (fX < 0.0f);
	}

	void SetScaleY(Float fY)
	{
		// See also: gameswf_types.cpp, line 319. Adapted for setting Y individually.

		// Length of column 0.
		auto const fAbsY = Sqrt(m_mTransform.M11 * m_mTransform.M11 + m_mTransform.M01 * m_mTransform.M01);
		if (fAbsY <= fEpsilon) // Zero.
		{
			m_mTransform.M11 = fY;
			m_mTransform.M01 = 0.0f;
		}
		else
		{
			// Rescale existing scale by multiplier - necessary to maintain skew
			// in light of new scale value.
			auto fFactor = (fY / fAbsY);
			if (m_bNegativeScaleY)
			{
				fFactor = -fFactor;
			}

			m_mTransform.M11 *= fFactor;
			m_mTransform.M01 *= fFactor;
		}

		// Track whether scale is negative or not.
		m_bNegativeScaleY = (fY < 0.0f);
	}

	void SetScissorClip(Bool bEnable)
	{
		m_bScissorClip = bEnable;
	}

	void SetTransform(const Matrix2x3& m)
	{
		m_mTransform = m;

		// Negative determinant, mark X scale as negative.
		m_bNegativeScaleX = (m.DeterminantUpper2x2() < 0.0f);
		m_bNegativeScaleY = false;
	}

	void SetVisible(Bool bVisible)
	{
		m_bVisible = bVisible;
	}

	void SetWorldPosition(Float fX, Float fY);

	void SetWorldTransform(const Matrix2x3& m);

	virtual Bool CastShadow() const SEOUL_OVERRIDE { return false; }
	virtual Vector2D GetShadowPlaneWorldPosition() const SEOUL_OVERRIDE { return Vector2D::Zero(); }

#if !SEOUL_SHIP
	/**
	 * As suggested, this is debug-only identifier. It can be used by
	 * developer code to identify nodes that do not have useful or
	 * meaningful instance names.
	 */
	const String& GetDebugName() const
	{
		return m_sDebugName;
	}

	/**
	 * As suggested, this is debug-only identifier. It can be used by
	 * developer code to identify nodes that do not have useful or
	 * meaningful instance names.
	 */
	void SetDebugName(const String& sName)
	{
		m_sDebugName = sName;
	}
#endif // /#if !SEOUL_SHIP

protected:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(Instance);

	Instance(UInt16 uDefinitionID)
		: m_pParent(nullptr)
		, m_ColorTransform(ColorTransformWithAlpha::Identity())
		, m_mTransform(Matrix2x3::Identity())
		, m_Name()
		, m_uDefinitionID(uDefinitionID)
		, m_uClipDepth(0)
		, m_uDepthInParent(0)
		, m_uWatcherCount(0)
		, m_bVisible(true)
		, m_bScissorClip(false)
		, m_bIgnoreDepthProjection(false)
		, m_bNegativeScaleX(false)
		, m_bNegativeScaleY(false)
		, m_uUnusedReserved(0u)
	{
#if !SEOUL_SHIP
		++g_VmStats.m_UINodes;
#endif
	}

	void CloneTo(AddInterface& rInterface, Instance& rClone) const
	{
		rClone.m_ColorTransform = m_ColorTransform;
		rClone.m_mTransform = m_mTransform;
		rClone.m_Name = m_Name;
		rClone.m_uClipDepth = m_uClipDepth;
		rClone.m_uFlags = m_uFlags;

		// Ping the interface if we have a watcher.
		if (m_uWatcherCount != 0)
		{
			rInterface.FalconOnClone(this, &rClone);
		}
	}

private:
	friend class DisplayList;
#if !SEOUL_SHIP
	String m_sDebugName;
#endif // /#if !SEOUL_SHIP
	MovieClipInstance* m_pParent;
	ColorTransformWithAlpha m_ColorTransform;
	Matrix2x3 m_mTransform;
	HString m_Name;
	UInt16 const m_uDefinitionID;
	UInt16 m_uClipDepth;
	UInt16 m_uDepthInParent;
	UInt8 m_uWatcherCount;
	union
	{
		struct
		{
			UInt8 m_bVisible : 1;
			UInt8 m_bScissorClip : 1;
			UInt8 m_bIgnoreDepthProjection : 1;
			UInt8 m_bNegativeScaleX : 1;
			UInt8 m_bNegativeScaleY : 1;
			UInt8 m_uUnusedReserved : 3;
		};
		UInt8 m_uFlags;
	};

	SEOUL_DISABLE_COPY(Instance);
}; // class Instance
template <typename T> struct InstanceTypeOf;

#if SEOUL_LOGGING_ENABLED
String GetPath(Instance const* pInstance);
#endif

} // namespace Seoul::Falcon

#endif // include guard
