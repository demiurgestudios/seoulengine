/**
 * \file FalconInstance.cpp
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

#include "FalconInstance.h"
#include "FalconMovieClipInstance.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_BEGIN_ENUM(Falcon::InstanceType)
	SEOUL_ENUM_N("Unknown", Falcon::InstanceType::kUnknown)
	SEOUL_ENUM_N("Animation2D", Falcon::InstanceType::kAnimation2D)
	SEOUL_ENUM_N("Bitmap", Falcon::InstanceType::kBitmap)
	SEOUL_ENUM_N("Custom", Falcon::InstanceType::kCustom)
	SEOUL_ENUM_N("EditText", Falcon::InstanceType::kEditText)
	SEOUL_ENUM_N("Fx", Falcon::InstanceType::kFx)
	SEOUL_ENUM_N("MovieClip", Falcon::InstanceType::kMovieClip)
	SEOUL_ENUM_N("Shape", Falcon::InstanceType::kShape)
SEOUL_END_ENUM()

namespace
{

Float GetInstanceHeight(const Falcon::Instance& instance)
{
	Falcon::Rectangle rect;
	(void)const_cast<Falcon::Instance&>(instance).ComputeLocalBounds(rect);
	return rect.GetHeight();
}

Bool GetVisibleToRoot(const Falcon::Instance& instance)
{
	auto p = &instance;
	while (nullptr != p)
	{
		if (!p->GetVisibleAndNotAlphaZero()) { return false; }
		p = p->GetParent();
	}

	return true;
}

Float GetInstanceWidth(const Falcon::Instance& instance)
{
	Falcon::Rectangle rect;
	(void)const_cast<Falcon::Instance&>(instance).ComputeLocalBounds(rect);
	return rect.GetWidth();
}

void SetInstancePosition(Falcon::Instance &instance, Vector2D position)
{
	instance.SetPosition(position);
}

Vector2D GetInstancePosition(const Falcon::Instance &instance)
{
	return instance.GetPosition();
}

Vector2D GetInstanceScale(const Falcon::Instance &instance)
{
	return instance.GetScale();
}

} // namespace anonymous

SEOUL_BEGIN_TYPE(Falcon::Instance, TypeFlags::kDisableNew)
	SEOUL_PROPERTY_PAIR_N("Alpha", GetAlpha, SetAlpha)
	SEOUL_PROPERTY_PAIR_N("BlendingFactor", GetBlendingFactor, SetBlendingFactor)
	SEOUL_PROPERTY_PAIR_N("ClipDepth", GetClipDepth, SetClipDepth)
	SEOUL_PROPERTY_N_EXT("Depth", GetDepthInParent)
	SEOUL_PROPERTY_N_Q("Height", GetInstanceHeight)
	SEOUL_PROPERTY_PAIR_N_Q("Position", GetInstancePosition, SetInstancePosition)
	SEOUL_PROPERTY_PAIR_N("Rotation", GetRotationInDegrees, SetRotationInDegrees)
	SEOUL_PROPERTY_N_Q("Scale", GetInstanceScale)
	SEOUL_PROPERTY_PAIR_N("ScissorClip", GetScissorClip, SetScissorClip)
	SEOUL_PROPERTY_N_EXT("Type", GetType)
	SEOUL_PROPERTY_PAIR_N("Visible", GetVisible, SetVisible)
	SEOUL_PROPERTY_N_Q("VisibleToRoot", GetVisibleToRoot)
	SEOUL_PROPERTY_N_Q("Width", GetInstanceWidth)
SEOUL_END_TYPE()

namespace Falcon
{

Float Instance::ComputeWorldDepth3D() const
{
	if (nullptr != m_pParent)
	{
		return (m_pParent->ComputeWorldDepth3D() + GetDepth3D());
	}
	else
	{
		return GetDepth3D();
	}
}

Vector2D Instance::ComputeWorldPosition() const
{
	Matrix2x3 const mWorldTransform = ComputeWorldTransform();
	return Vector2D(mWorldTransform.TX, mWorldTransform.TY);
}

Matrix2x3 Instance::ComputeWorldTransform() const
{
	if (nullptr != m_pParent)
	{
		return (m_pParent->ComputeWorldTransform() * m_mTransform);
	}
	else
	{
		return m_mTransform;
	}
}

void Instance::SetName(HString name)
{
	if (name != m_Name)
	{
		m_Name = name;
		if (m_pParent)
		{
			m_pParent->m_DisplayList.UpdateName(name, m_uDepthInParent);
		}
	}
}

void Instance::SetWorldPosition(Float fX, Float fY)
{
	Vector2D const vWorldPosition = Vector2D(fX, fY);
	Vector2D const vObjectPosition = (m_pParent
		? Matrix2x3::TransformPosition(m_pParent->ComputeWorldTransform().Inverse(), vWorldPosition)
		: vWorldPosition);

	SetPosition(vObjectPosition.X, vObjectPosition.Y);
}

void Instance::SetWorldTransform(const Matrix2x3& m)
{
	if (m_pParent)
	{
		SetTransform(m_pParent->ComputeWorldTransform().Inverse() * m);
	}
	else
	{
		SetTransform(m);
	}
}

Float Instance::GetWorldDepth3D() const
{
	Float depth = GetDepth3D();
	if (GetParent() != nullptr)
	{
		depth += GetParent()->GetWorldDepth3D();
	}

	return depth;
}

void Instance::GatherFullName(String& rs) const
{
	auto pParent(GetParent());
	if (pParent != nullptr)
	{
		pParent->GatherFullName(rs);
		rs.Append('.');
	}

	if (GetName().IsEmpty())
	{
#if !SEOUL_SHIP
		if (!GetDebugName().IsEmpty())
		{
			rs.Append(GetDebugName());
			return;
		}
#endif
		rs.Append("<no-name>");
	}
	else
	{
		rs.Append(GetName().CStr());
	}
}

#if SEOUL_LOGGING_ENABLED
String GetPath(Instance const* pInstance)
{
	if (nullptr == pInstance) { return String(); }

	auto const sParent(GetPath(pInstance->GetParent()));
	String const sChild(pInstance->GetName());

	if (sParent.IsEmpty()) { return sChild; }
	else { return (sParent + "." + sChild); }
}
#endif

} // namespace Falcon

} // namespace Seoul
