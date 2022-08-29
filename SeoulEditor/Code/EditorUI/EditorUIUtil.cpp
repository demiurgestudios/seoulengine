/**
 * \file EditorUIUtil.cpp
 * \brief Miscellaneous utility functions for the EditorUI project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Camera.h"
#include "EditorUIUtil.h"
#include "Matrix4D.h"
#include "ReflectionAttributes.h"
#include "StackOrHeapArray.h"
#include "Viewport.h"

namespace Seoul::EditorUI
{

static const Float kfTooltipTime = 0.5f;

Float32 ComputeGizmoScale(
	Float fDesiredSizeInPixels,
	const Camera& camera,
	const Viewport& viewport,
	const Vector3D& vGizmoWorldPosition)
{
	if (camera.GetProjectionMatrix().IsPerspective())
	{
		auto const v(Matrix4D::Transform(
			camera.GetViewProjectionMatrix(),
			Vector4D(vGizmoWorldPosition, 1.0f)));

		// TODO: If we ever change our definition of FOV, this
		// may need to switch to width instead of height.
		return (v.W * Min(fDesiredSizeInPixels / (Float)viewport.m_iViewportHeight, 1.0f));
	}
	else
	{
		const Frustum& frustum = camera.GetFrustum();
		Float const fHeight = (frustum.GetTopPlane().D + frustum.GetBottomPlane().D);

		return (fHeight * (fDesiredSizeInPixels / (Float)viewport.m_iViewportHeight));
	}
}

Bool InputText(
	Byte const* sLabel,
	String& rs,
	ImGuiInputTextFlags eFlags /*= 0*/,
	ImGuiInputTextCallback pCallback /*= nullptr*/,
	void* pUserData /*= nullptr*/)
{
	static const UInt32 kuOversize = 64u;
	static const UInt32 kuStackSize = 128u;

	// Space for additional characters, plus the null terminator.
	StackOrHeapArray<Byte, kuStackSize, MemoryBudgets::Editor> aBuffer(rs.GetSize() + 1u + kuOversize);
	if (!rs.IsEmpty())
	{
		memcpy(aBuffer.Data(), rs.CStr(), rs.GetSize());
	}
	aBuffer[rs.GetSize()] = '\0';

	// Handle input.
	if (ImGui::InputText(sLabel, aBuffer.Data(), aBuffer.GetSize(), eFlags, pCallback, pUserData))
	{
		rs = String(aBuffer.Data());
		return true;
	}

	return false;
}

void SetTooltipEx(Byte const* sLabel)
{
	using namespace ImGui;
	using namespace Reflection;
	using namespace Reflection::Attributes;

	if (!IsItemHovered())
	{
		return;
	}

	auto const fHoveredTime = GetHoveredTime();
	if (fHoveredTime < kfTooltipTime)
	{
		return;
	}

	SetTooltip("%s", sLabel);
}

void SetTooltipEx(const Reflection::AttributeCollection& attributes)
{
	using namespace ImGui;
	using namespace Reflection;
	using namespace Reflection::Attributes;

	if (!IsItemHovered())
	{
		return;
	}

	auto const fHoveredTime = GetHoveredTime();
	if (fHoveredTime < kfTooltipTime)
	{
		return;
	}

	if (auto pDescription = attributes.GetAttribute<Description>())
	{
		SetTooltip(pDescription->m_DescriptionText.CStr());
	}
}

} // namespace Seoul::EditorUI
