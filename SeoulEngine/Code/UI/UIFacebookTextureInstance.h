/**
 * \file UIFacebookTextureInstance.h
 * \brief SeoulEngine subclass/extension of Falcon::Instance for Facebook profile images.
 *
 * UI::FacebookTextureInstance is a subclass of Falcon::Instance that is mostly
 * similar to UI::TextureSubstitutionInstance, except textures are sourced
 * from FacebookProfileImageManager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_FACEBOOK_TEXTURE_INSTANCE_H
#define UI_FACEBOOK_TEXTURE_INSTANCE_H

#include "FalconInstance.h"
#include "FilePath.h"
#include "Prereqs.h"
#include "ReflectionDeclare.h"
#include "SeoulHString.h"
namespace Seoul { namespace Falcon { class BitmapDefinition; } }

namespace Seoul::UI
{

/** Custom subclass of Falcon::Instance, implements texture substitution logic. */
class FacebookTextureInstance SEOUL_SEALED : public Falcon::Instance
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(FacebookTextureInstance);

	FacebookTextureInstance(
		String& facebookUserGUID,
		FilePath defaultImageFilePath,
		Int32 iTextureWidth,
		Int32 iTextureHeight);

	~FacebookTextureInstance();

	virtual Instance* Clone(Falcon::AddInterface& rInterface) const SEOUL_OVERRIDE
	{
		FacebookTextureInstance* pReturn = SEOUL_NEW(MemoryBudgets::UIRuntime) FacebookTextureInstance;
		CloneTo(rInterface, *pReturn);
		return pReturn;
	}

	virtual Bool ComputeLocalBounds(Falcon::Rectangle& rBounds) SEOUL_OVERRIDE;

	virtual void Pose(
		Falcon::Render::Poser& rPoser,
		const Matrix2x3& mParent,
		const Falcon::ColorTransformWithAlpha& cxParent) SEOUL_OVERRIDE;

#if SEOUL_ENABLE_CHEATS
	virtual void PoseInputVisualization(
		Falcon::Render::Poser& rPoser,
		const Matrix2x3& mParent,
		RGBA color) SEOUL_OVERRIDE;
#endif

	virtual void Draw(
		Falcon::Render::Drawer& rDrawer,
		const Falcon::Rectangle& worldBoundsPreClip,
		const Matrix2x3& mWorld,
		const Falcon::ColorTransformWithAlpha& cxWorld,
		const Falcon::TextureReference& textureReference,
		Int32 iSubInstanceId) SEOUL_OVERRIDE;

	virtual Bool HitTest(
		const Matrix2x3& mParent,
		Float32 fWorldX,
		Float32 fWorldY,
		Bool bIgnoreVisibility = false) const SEOUL_OVERRIDE;

	virtual Falcon::InstanceType GetType() const SEOUL_OVERRIDE;

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(FacebookTextureInstance);

	FacebookTextureInstance();

	void CloneTo(Falcon::AddInterface& rInterface, FacebookTextureInstance& rClone) const
	{
		Falcon::Instance::CloneTo(rInterface, rClone);

		// todo: add missing members to clone.
		rClone.m_vTextureCoordinates = m_vTextureCoordinates;
		rClone.m_iTextureWidth = m_iTextureWidth;
		rClone.m_iTextureHeight = m_iTextureHeight;
		rClone.m_CachedFilePath = m_CachedFilePath;
		rClone.m_CachedDefaultFilePath = m_CachedDefaultFilePath;
		rClone.m_sUserFacebookGuid = m_sUserFacebookGuid;
	}

	SharedPtr<Falcon::BitmapDefinition> m_pBitmap;
	SharedPtr<Falcon::BitmapDefinition> m_pDefaultBitmap;
	Vector4D m_vTextureCoordinates;
	FilePath m_CachedFilePath;
	FilePath m_CachedDefaultFilePath;
	Int32    m_iTextureWidth;
	Int32    m_iTextureHeight;
	String   m_sUserFacebookGuid;

	SEOUL_DISABLE_COPY(FacebookTextureInstance);
}; // class UI::FacebookTextureInstance

} // namespace Seoul::UI

#endif // include guard
