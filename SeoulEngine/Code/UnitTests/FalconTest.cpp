/**
 * \file FalconTest.cpp
 * \brief Unit test for functionality in the Falcon project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Compress.h"
#include "ContentLoadManager.h"
#include "FalconBitmapDefinition.h"
#include "FalconEditTextCommon.h"
#include "FalconEditTextDefinition.h"
#include "FalconEditTextInstance.h"
#include "FalconFCNFile.h"
#include "FalconInstance.h"
#include "FalconMovieClipInstance.h"
#include "FalconRenderCommand.h"
#include "FalconRenderBatchOptimizer.h"
#include "FalconRenderOcclusionOptimizer.h"
#include "FalconTest.h"
#include "FalconTypes.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "Logger.h"
#include "PackageFileSystem.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"
#include "SeoulFileReaders.h"
#include "Texture.h"
#include "UIManager.h"
#include "UnitTesting.h"
#include "UnitTestsEngineHelper.h"
#include "UnitTestsFileManagerHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(FalconTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestInstanceTransform)
	SEOUL_METHOD(TestRenderBatchOptimizerNoIntersection)
	SEOUL_METHOD(TestRenderBatchOptimizerNoIntersectionWide)
	SEOUL_METHOD(TestRenderBatchOptimizerPartialIntersectionWide)
	SEOUL_METHOD(TestRenderBatchOptimizerInterrupt)
	SEOUL_METHOD(TestRenderBatchOptimizerIntersection)
	SEOUL_METHOD(TestRenderBatchOptimizerPartialIntersection)
	SEOUL_METHOD(TestRenderBatchOptimizerPartialIntersectionBlocked)
	SEOUL_METHOD(TestRenderOcclusionOptimizerNoOcclusion)
	SEOUL_METHOD(TestRenderOcclusionOptimizerOccluded)
	SEOUL_METHOD(TestRenderOcclusionOptimizerMixed)
	SEOUL_METHOD(TestRenderOcclusionOptimizerMultiple)
	SEOUL_METHOD(TestWriteGlyphBitmap)
	SEOUL_METHOD(TestRectangleIntersect)
	SEOUL_METHOD(TestSetTransform)
	SEOUL_METHOD(TestSetTransformTerms)
	SEOUL_METHOD(TestScaleRegressionX)
	SEOUL_METHOD(TestScaleRegressionY)
	SEOUL_METHOD(TestSkewRegression)
	SEOUL_METHOD(TestRotationUpdate)
	SEOUL_METHOD(TestScaleUpdate)
	SEOUL_METHOD(TestHtmlFormattingCharRefs)
	SEOUL_METHOD(TestHtmlFormattingRegression)
	SEOUL_METHOD(TestHtmlFormattingRobustness)
	SEOUL_METHOD(TestHtmlFormattingStrings)
	SEOUL_METHOD(TestHtmlFormattingValues)
	SEOUL_METHOD(TestProperties)
	SEOUL_METHOD(TestGetFcnDependencies)
SEOUL_END_TYPE()

static inline Byte const* GetPlatformPrefix()
{
	Byte const* sPrefix = GetCurrentPlatformName();

	// TODO: Temp until we promote Linux to a full platform.
	if (keCurrentPlatform == Platform::kLinux)
	{
		sPrefix = kaPlatformNames[(UInt32)Platform::kAndroid];
	}

	return sPrefix;
}

class FalconTestFalconInstance SEOUL_SEALED : public Falcon::Instance
{
public:
	FalconTestFalconInstance()
		: Falcon::Instance(0u)
	{
	}

	~FalconTestFalconInstance()
	{
	}

	virtual Instance* Clone(Falcon::AddInterface& rInterface) const SEOUL_OVERRIDE
	{
		return SEOUL_NEW(MemoryBudgets::Developer) FalconTestFalconInstance;
	}

	virtual Bool ComputeLocalBounds(Falcon::Rectangle& rBounds) SEOUL_OVERRIDE
	{
		return false;
	}

	virtual Falcon::InstanceType GetType() const SEOUL_OVERRIDE
	{
		return Falcon::InstanceType::kCustom;
	}

	virtual Bool HitTest(
		const Matrix2x3& mParent,
		Float32 fWorldX,
		Float32 fWorldY,
		Bool bIgnoreVisibility /*= false*/) const SEOUL_OVERRIDE
	{
		return false;
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(FalconTestFalconInstance);
}; // class FalconTestFalconInstance

class FalconTestTexture SEOUL_SEALED : public Falcon::Texture
{
public:
	FalconTestTexture(Int32 i)
		: m_i(i)
	{
	}

	~FalconTestTexture()
	{
	}

	Int32 GetId() const { return m_i; }

	// Falcon::Texture overrides
	virtual const TextureContentHandle& GetTextureContentHandle() { return m_hTexture; }
	virtual Bool HasDimensions() const SEOUL_OVERRIDE { return false; }
	virtual Bool IsAtlas() const SEOUL_OVERRIDE { return false; }
	virtual Bool IsLoading() const SEOUL_OVERRIDE { return false; }
	virtual Bool ResolveLoadingData(
		const FilePath& filePath,
		Falcon::TextureLoadingData& rData) const SEOUL_OVERRIDE { return false; }
	virtual Bool ResolveTextureMetrics(Falcon::TextureMetrics& r) const SEOUL_OVERRIDE { return false; }
	// /Falcon::Texture overrides

protected:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(FalconTestTexture);

	// Falcon::Texture overrides
	virtual bool DoResolveMemoryUsageInBytes(Int32& riMemoryUsageInBytes) const SEOUL_OVERRIDE
	{
		return false;
	}
	// /Falcon::Texture overrides

	TextureContentHandle m_hTexture;
	Int32 m_i;

private:
	SEOUL_DISABLE_COPY(FalconTestTexture);
}; // class FalconTestTexture

void FalconTest::TestInstanceTransform()
{
	SharedPtr<FalconTestFalconInstance> pInstance(SEOUL_NEW(MemoryBudgets::Developer) FalconTestFalconInstance());

	// Identity.
	SEOUL_UNITTESTING_ASSERT_EQUAL(Matrix2x3::Identity(), pInstance->GetTransform());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetPosition().X);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetPosition().Y);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetPositionX());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetPositionY());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetRotationInDegrees());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetRotationInRadians());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetScale().X);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetScale().Y);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetScaleX());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetScaleY());

	// Manipulate position (X).
	pInstance->SetPositionX(3.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(Matrix2x3(1, 0, 3, 0, 1, 0), pInstance->GetTransform());
	SEOUL_UNITTESTING_ASSERT_EQUAL(3.0f, pInstance->GetPosition().X);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetPosition().Y);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3.0f, pInstance->GetPositionX());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetPositionY());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetRotationInDegrees());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetRotationInRadians());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetScale().X);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetScale().Y);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetScaleX());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetScaleY());

	// Manipulate position (Y).
	pInstance->SetPositionY(73.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(Matrix2x3(1, 0, 3, 0, 1, 73), pInstance->GetTransform());
	SEOUL_UNITTESTING_ASSERT_EQUAL(3.0f, pInstance->GetPosition().X);
	SEOUL_UNITTESTING_ASSERT_EQUAL(73.0f, pInstance->GetPosition().Y);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3.0f, pInstance->GetPositionX());
	SEOUL_UNITTESTING_ASSERT_EQUAL(73.0f, pInstance->GetPositionY());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetRotationInDegrees());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetRotationInRadians());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetScale().X);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetScale().Y);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetScaleX());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetScaleY());

	// Manipulate position.
	pInstance->SetPosition(5.0f, -25.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(Matrix2x3(1, 0, 5, 0, 1, -25), pInstance->GetTransform());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5.0f, pInstance->GetPosition().X);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-25.0f, pInstance->GetPosition().Y);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5.0f, pInstance->GetPositionX());
	SEOUL_UNITTESTING_ASSERT_EQUAL(-25.0f, pInstance->GetPositionY());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetRotationInDegrees());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetRotationInRadians());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetScale().X);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetScale().Y);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetScaleX());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetScaleY());

	// Manipulate rotation (many radians).
	for (Int32 i = -720; i <= 720; ++i)
	{
		Float const fDegrees = (Float)i;
		Float const fRadians = DegreesToRadians(fDegrees);

		pInstance->SetRotationInRadians(fRadians);

		auto const mA = Matrix2x3::CreateTranslation(5, -25) * Matrix2x3::CreateRotation(fRadians);
		auto const mB = pInstance->GetTransform();
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(mA, mB, 1e-4f);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5.0f, pInstance->GetPosition().X);
		SEOUL_UNITTESTING_ASSERT_EQUAL(-25.0f, pInstance->GetPosition().Y);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5.0f, pInstance->GetPositionX());
		SEOUL_UNITTESTING_ASSERT_EQUAL(-25.0f, pInstance->GetPositionY());

		Float const fTestDegrees = pInstance->GetRotationInDegrees();
		SEOUL_UNITTESTING_ASSERT(EqualDegrees(fDegrees, fTestDegrees, 1e-4f));

		Float const fTestRadians = pInstance->GetRotationInRadians();
		SEOUL_UNITTESTING_ASSERT(EqualRadians(fRadians, fTestRadians, 1e-6f));

		{
			Float const fScaleX = pInstance->GetScale().X;
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1.0f, fScaleX, 1e-4f);
		}
		{
			Float const fScaleX = pInstance->GetScaleX();
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1.0f, fScaleX, 1e-4f);
		}

		{
			Float const fScaleY = pInstance->GetScale().Y;
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1.0f, fScaleY, 1e-4f);
		}
		{
			Float const fScaleY = pInstance->GetScaleY();
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1.0f, fScaleY, 1e-4f);
		}
	}

	// Manipulate rotation (many degrees).
	for (Int32 i = -720; i <= 720; ++i)
	{
		Float const fDegrees = (Float)i;
		Float const fRadians = DegreesToRadians(fDegrees);

		pInstance->SetRotationInDegrees(fDegrees);

		auto const mA = Matrix2x3::CreateTranslation(5, -25) * Matrix2x3::CreateRotation(fRadians);
		auto const mB = pInstance->GetTransform();
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(mA, mB, 1e-4f);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5.0f, pInstance->GetPosition().X);
		SEOUL_UNITTESTING_ASSERT_EQUAL(-25.0f, pInstance->GetPosition().Y);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5.0f, pInstance->GetPositionX());
		SEOUL_UNITTESTING_ASSERT_EQUAL(-25.0f, pInstance->GetPositionY());

		Float const fTestDegrees = pInstance->GetRotationInDegrees();
		SEOUL_UNITTESTING_ASSERT(EqualDegrees(fDegrees, fTestDegrees, 1e-4f));

		Float const fTestRadians = pInstance->GetRotationInRadians();
		SEOUL_UNITTESTING_ASSERT(EqualRadians(fRadians, fTestRadians, 1e-6f));

		{
			Float const fScaleX = pInstance->GetScale().X;
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1.0f, fScaleX, 1e-4f);
		}
		{
			Float const fScaleX = pInstance->GetScaleX();
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1.0f, fScaleX, 1e-4f);
		}
		{
			Float const fScaleY = pInstance->GetScale().Y;
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1.0f, fScaleY, 1e-4f);
		}
		{
			Float const fScaleY = pInstance->GetScaleY();
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1.0f, fScaleY, 1e-4f);
		}
	}

	// Restore rotation prior to further tests.
	pInstance->SetRotationInDegrees(45.0f);

	// Manipulate scale.
	pInstance->SetScale(20.0f, 7.0f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Matrix2x3::CreateTranslation(5, -25) * Matrix2x3::CreateRotationFromDegrees(45.0f) * Matrix2x3::CreateScale(20.0f, 7.0f), pInstance->GetTransform(), 1e-4f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5.0f, pInstance->GetPosition().X);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-25.0f, pInstance->GetPosition().Y);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5.0f, pInstance->GetPositionX());
	SEOUL_UNITTESTING_ASSERT_EQUAL(-25.0f, pInstance->GetPositionY());
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(45.0f, pInstance->GetRotationInDegrees(), 1e-4f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(DegreesToRadians(45.0f), pInstance->GetRotationInRadians(), 1e-4f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(20.0f, pInstance->GetScale().X, 1e-4f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(7.0f, pInstance->GetScale().Y, 1e-4f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(20.0f, pInstance->GetScaleX(), 1e-4f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(7.0f, pInstance->GetScaleY(), 1e-4f);

	// Manipulate scale (X).
	pInstance->SetScaleX(30.0f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Matrix2x3::CreateTranslation(5, -25) * Matrix2x3::CreateRotationFromDegrees(45.0f) * Matrix2x3::CreateScale(30.0f, 7.0f), pInstance->GetTransform(), 1e-4f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5.0f, pInstance->GetPosition().X);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-25.0f, pInstance->GetPosition().Y);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5.0f, pInstance->GetPositionX());
	SEOUL_UNITTESTING_ASSERT_EQUAL(-25.0f, pInstance->GetPositionY());
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(45.0f, pInstance->GetRotationInDegrees(), 1e-4f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(DegreesToRadians(45.0f), pInstance->GetRotationInRadians(), 1e-4f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(30.0f, pInstance->GetScale().X, 1e-4f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(7.0f, pInstance->GetScale().Y, 1e-4f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(30.0f, pInstance->GetScaleX(), 1e-4f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(7.0f, pInstance->GetScaleY(), 1e-4f);

	// Manipulate scale (Y).
	pInstance->SetScaleY(-5.0f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Matrix2x3::CreateTranslation(5, -25) * Matrix2x3::CreateRotationFromDegrees(45.0f) * Matrix2x3::CreateScale(30.0f, -5.0f), pInstance->GetTransform(), 1e-4f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5.0f, pInstance->GetPosition().X);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-25.0f, pInstance->GetPosition().Y);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5.0f, pInstance->GetPositionX());
	SEOUL_UNITTESTING_ASSERT_EQUAL(-25.0f, pInstance->GetPositionY());
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(45.0f, pInstance->GetRotationInDegrees(), 1e-4f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(DegreesToRadians(45.0f), pInstance->GetRotationInRadians(), 1e-4f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(30.0f, pInstance->GetScale().X, 1e-4f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-5.0f, pInstance->GetScale().Y, 1e-4f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(30.0f, pInstance->GetScaleX(), 1e-4f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-5.0f, pInstance->GetScaleY(), 1e-4f);
}

static Falcon::Rectangle NoIntersectionRectangle(UInt32 u)
{
	return Falcon::Rectangle::Create(
		(Float)u,
		(Float)(u + 1),
		(Float)u,
		(Float)(u + 1));
}

static Falcon::Rectangle NoIntersectionRectangleWide(UInt32 u)
{
	switch (u)
	{
	case 0u:
		return Falcon::Rectangle::Create(1, 2, 1, 2);
	case 1u:
		return Falcon::Rectangle::Create(0, 1, 0, 1);
	case 2u:
		return Falcon::Rectangle::Create(2, 3, 2, 3);
	case 3u:
	case 4u:
		return Falcon::Rectangle::Create(1, 2, 1, 2);
	default:
		SEOUL_UNITTESTING_ASSERT(false);
		return Falcon::Rectangle::Max();
	};
}

static Falcon::Rectangle PartialIntersectionRectangleWide(UInt32 u)
{
	switch (u)
	{
	case 0u:
		return Falcon::Rectangle::Create(1, 2, 1, 2);
	case 1u:
		return Falcon::Rectangle::Create(0, 1, 0, 1);
	case 2u:
		return Falcon::Rectangle::Create(2, 3, 2, 3);
	case 3u:
	case 4u:
		return Falcon::Rectangle::Create(0, 1, 0, 1);
	default:
		SEOUL_UNITTESTING_ASSERT(false);
		return Falcon::Rectangle::Max();
	};
}

static Falcon::Rectangle AllIntersectionRectangle(UInt32 u)
{
	return Falcon::Rectangle::Create(0, 1, 0, 1);
}

static Falcon::Rectangle PartialIntersectionRectangle(UInt32 u)
{
	switch (u)
	{
	case 0u:
	case 1u:
		return Falcon::Rectangle::Create(0, 1, 0, 1);
	case 2u:
	case 3u:
	case 4u:
		return Falcon::Rectangle::Create(1, 2, 1, 2);
	default:
		SEOUL_UNITTESTING_ASSERT(false);
		return Falcon::Rectangle::Max();
	};
}

static Falcon::Rectangle PartialIntersectionRectangleBlocked(UInt32 u)
{
	switch (u)
	{
	case 0u:
	case 1u:
	case 2u:
	case 3u:
		return Falcon::Rectangle::Create(0, 1, 0, 1);
	case 4u:
		return Falcon::Rectangle::Create(1, 2, 1, 2);
	default:
		SEOUL_UNITTESTING_ASSERT(false);
		return Falcon::Rectangle::Max();
	};
}

static UInt32 GetTextureInterleaved(UInt32 u)
{
	return (u % 2u);
}

static UInt32 GetTexturePartial(UInt32 u)
{
	switch (u)
	{
	case 0u: return 0u;
	case 1u: return 0u;
	case 2u: return 1u;
	case 3u: return 0u;
	case 4u: return 2u;
	default:
		SEOUL_UNITTESTING_ASSERT(false);
		return 0u;
	};
}

static UInt32 GetTexturePartialBlocked(UInt32 u)
{
	switch (u)
	{
	case 0u: return 0u;
	case 1u: return 0u;
	case 2u: return 1u;
	case 3u: return 0u;
	case 4u: return 0u;
	default:
		SEOUL_UNITTESTING_ASSERT(false);
		return 0u;
	};
}

static UInt32 GetTextureNoIntersectionWide(UInt32 u)
{
	switch (u)
	{
	case 0u: return 0u;
	case 1u: return 1u;
	case 2u: return 1u;
	case 3u: return 0u;
	case 4u: return 0u;
	default:
		SEOUL_UNITTESTING_ASSERT(false);
		return 0u;
	};
}

static Bool NoInterrupt(UInt32 u)
{
	return false;
}

static Bool Interrupt(UInt32 u)
{
	return (4u == u);
}

static void TestBatchOptimizerCommon(
	Falcon::Rectangle (*getRectangle)(UInt32 u),
	UInt32 (*getTextureId)(UInt32 u),
	Bool (*interrupt)(UInt32 u),
	UInt32 uTestTextures,
	UInt32 uSize,
	UInt32 const* puExpected)
{
	using namespace Falcon;
	using namespace Falcon::Render;

	CommandBuffer buffer;

	Vector< SharedPtr<FalconTestTexture> > vpTextures(uTestTextures);
	for (UInt32 i = 0u; i < uTestTextures; ++i)
	{
		vpTextures[i].Reset(
			SEOUL_NEW(MemoryBudgets::Developer) FalconTestTexture(i));
	};

	Vector<CommandPose> vPoses(uSize);
	for (UInt32 i = 0; i < uSize; ++i)
	{
		if (interrupt(i))
		{
			buffer.IssueWorldCullChange(
				Falcon::Rectangle::Max(),
				1.0f,
				1.0f);
		}

		auto& r = buffer.IssuePose();
		r = CommandPose();
		r.m_TextureReference.m_pTexture = vpTextures[getTextureId(i)];
		r.m_WorldRectangle = getRectangle(i);

		vPoses[i] = r;
	}

	BatchOptimizer optimizer;
	optimizer.Optimize(buffer);

	UInt32 iIndex = 0u;
	for (auto i = buffer.Begin(); buffer.End() != i; ++i)
	{
		if (interrupt(iIndex))
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL((UInt16)CommandType::kWorldCullChange, i->m_uType);
			++i;
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt16)CommandType::kPose, i->m_uType);
		++iIndex;
	}

	UInt32 uInterruptIndex = 0u;
	UInt32 uPoseIndex = 0u;
	for (auto i = buffer.Begin(); buffer.End() != i; ++i)
	{
		if (interrupt(uPoseIndex))
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(uInterruptIndex, i->m_u);
			++uInterruptIndex;
			++i;
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(puExpected[uPoseIndex], i->m_u);
		++uPoseIndex;
	}

	for (UInt32 i = 0u; i < uSize; ++i)
	{
		auto const& pose = buffer.GetPose(i);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vPoses[i].m_cxWorld, pose.m_cxWorld);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vPoses[i].m_fDepth3D, pose.m_fDepth3D);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vPoses[i].m_iClip, pose.m_iClip);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vPoses[i].m_iSubRenderableId, pose.m_iSubRenderableId);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vPoses[i].m_mWorld, pose.m_mWorld);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vPoses[i].m_pRenderable, pose.m_pRenderable);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vPoses[i].m_TextureReference.m_pTexture, pose.m_TextureReference.m_pTexture);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vPoses[i].m_vShadowPlaneWorldPosition, pose.m_vShadowPlaneWorldPosition);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vPoses[i].m_WorldRectangle, pose.m_WorldRectangle);
	}
}

void FalconTest::TestRenderBatchOptimizerNoIntersection()
{
	static const UInt32 kuCount = 5u;

	static const UInt32 kaExpected[] =
	{
		0, 2, 4, 1, 3,
	};

	TestBatchOptimizerCommon(NoIntersectionRectangle, GetTextureInterleaved, NoInterrupt, 2u, kuCount, kaExpected);
}

void FalconTest::TestRenderBatchOptimizerNoIntersectionWide()
{
	static const UInt32 kuCount = 5u;

	static const UInt32 kaExpected[] =
	{
		0, 3, 4, 1, 2,
	};

	TestBatchOptimizerCommon(NoIntersectionRectangleWide, GetTextureNoIntersectionWide, NoInterrupt, 2u, kuCount, kaExpected);
}

void FalconTest::TestRenderBatchOptimizerPartialIntersectionWide()
{
	static const UInt32 kuCount = 5u;

	static const UInt32 kaExpected[] =
	{
		1, 2, 0, 3, 4,
	};

	TestBatchOptimizerCommon(PartialIntersectionRectangleWide, GetTextureNoIntersectionWide, NoInterrupt, 2u, kuCount, kaExpected);
}

void FalconTest::TestRenderBatchOptimizerInterrupt()
{
	static const UInt32 kuCount = 5u;

	static const UInt32 kaExpected[] =
	{
		0, 2, 1, 3, 4,
	};

	TestBatchOptimizerCommon(NoIntersectionRectangle, GetTextureInterleaved, Interrupt, 2u, kuCount, kaExpected);
}

void FalconTest::TestRenderBatchOptimizerIntersection()
{
	static const UInt32 kuCount = 5u;

	static const UInt32 kaExpected[] =
	{
		0, 1, 2, 3, 4,
	};

	TestBatchOptimizerCommon(AllIntersectionRectangle, GetTextureInterleaved, NoInterrupt, 2u, kuCount, kaExpected);
}

void FalconTest::TestRenderBatchOptimizerPartialIntersection()
{
	static const UInt32 kuCount = 5u;

	static const UInt32 kaExpected[] =
	{
		2, 0, 1, 3, 4,
	};

	TestBatchOptimizerCommon(PartialIntersectionRectangle, GetTexturePartial, NoInterrupt, 3u, kuCount, kaExpected);
}

void FalconTest::TestRenderBatchOptimizerPartialIntersectionBlocked()
{
	static const UInt32 kuCount = 5u;

	static const UInt32 kaExpected[] =
	{
		0, 1, 2, 3, 4,
	};

	TestBatchOptimizerCommon(PartialIntersectionRectangleBlocked, GetTexturePartialBlocked, NoInterrupt, 3u, kuCount, kaExpected);
}

static void TestOcclusionCommon(
	const Vector<Falcon::Render::CommandPose>& vExpected,
	Falcon::Render::CommandBuffer& buffer)
{
	using namespace Falcon;
	using namespace Falcon::Render;

	OcclusionOptimizer optimizer;
	optimizer.Optimize(buffer);

	SEOUL_UNITTESTING_ASSERT_EQUAL(vExpected.GetSize(), buffer.GetBufferSize());
	for (UInt32 i = 0u; i < vExpected.GetSize(); ++i)
	{
		auto const& pose = buffer.GetPose((buffer.Begin() + i)->m_u);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vExpected[i].m_cxWorld, pose.m_cxWorld);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vExpected[i].m_fDepth3D, pose.m_fDepth3D);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vExpected[i].m_iClip, pose.m_iClip);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vExpected[i].m_iSubRenderableId, pose.m_iSubRenderableId);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vExpected[i].m_mWorld, pose.m_mWorld);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vExpected[i].m_pRenderable, pose.m_pRenderable);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vExpected[i].m_TextureReference.m_pTexture, pose.m_TextureReference.m_pTexture);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vExpected[i].m_vShadowPlaneWorldPosition, pose.m_vShadowPlaneWorldPosition);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vExpected[i].m_WorldRectangle, pose.m_WorldRectangle);
	}
}

void FalconTest::TestRenderOcclusionOptimizerNoOcclusion()
{
	using namespace Falcon;
	using namespace Falcon::Render;

	CommandBuffer buffer;

	SharedPtr<Texture> pTestTexture(SEOUL_NEW(MemoryBudgets::Developer) FalconTestTexture(0));

	Vector<CommandPose> vPoses;
	{
		auto& r = buffer.IssuePose();
		r = CommandPose();
		r.m_cxWorld = Falcon::ColorTransformWithAlpha::Identity();
		r.m_TextureReference.m_pTexture = pTestTexture;
		r.m_WorldRectangle = Rectangle::Create(1, 2, 1, 2);
		vPoses.PushBack(r);
	}
	{
		auto& r = buffer.IssuePose();
		r = CommandPose();
		r.m_cxWorld = Falcon::ColorTransformWithAlpha::Identity();
		r.m_TextureReference.m_pTexture = pTestTexture;
		r.m_WorldOcclusionRectangle = Rectangle::Create(1.1f, 2, 1, 2);
		r.m_WorldRectangle = Rectangle::Create(1, 2, 1, 2);
		vPoses.PushBack(r);
	}

	TestOcclusionCommon(vPoses, buffer);
}

void FalconTest::TestRenderOcclusionOptimizerOccluded()
{
	using namespace Falcon;
	using namespace Falcon::Render;

	CommandBuffer buffer;

	SharedPtr<Texture> pTestTexture(SEOUL_NEW(MemoryBudgets::Developer) FalconTestTexture(0));

	Vector<CommandPose> vPoses;
	{
		auto& r = buffer.IssuePose();
		r = CommandPose();
		r.m_cxWorld = Falcon::ColorTransformWithAlpha::Identity();
		r.m_TextureReference.m_pTexture = pTestTexture;
		r.m_WorldRectangle = Rectangle::Create(1, 2, 1, 2);
	}
	{
		auto& r = buffer.IssuePose();
		r = CommandPose();
		r.m_cxWorld = Falcon::ColorTransformWithAlpha::Identity();
		r.m_TextureReference.m_pTexture = pTestTexture;
		r.m_WorldOcclusionRectangle = Rectangle::Create(1, 2, 1, 2);
		r.m_WorldRectangle = Rectangle::Create(1, 2, 1, 2);
		vPoses.PushBack(r);
	}

	TestOcclusionCommon(vPoses, buffer);
}

void FalconTest::TestRenderOcclusionOptimizerMixed()
{
	using namespace Falcon;
	using namespace Falcon::Render;

	CommandBuffer buffer;

	SharedPtr<Texture> pTestTexture(SEOUL_NEW(MemoryBudgets::Developer) FalconTestTexture(0));

	Vector<CommandPose> vPoses;
	{
		auto& r = buffer.IssuePose();
		r = CommandPose();
		r.m_cxWorld = Falcon::ColorTransformWithAlpha::Identity();
		r.m_TextureReference.m_pTexture = pTestTexture;
		r.m_WorldRectangle = Rectangle::Create(0.9f, 2, 1, 2);
		vPoses.PushBack(r);
	}
	{
		auto& r = buffer.IssuePose();
		r = CommandPose();
		r.m_cxWorld = Falcon::ColorTransformWithAlpha::Identity();
		r.m_TextureReference.m_pTexture = pTestTexture;
		r.m_WorldRectangle = Rectangle::Create(1, 2, 1, 2);
	}
	{
		auto& r = buffer.IssuePose();
		r = CommandPose();
		r.m_cxWorld = Falcon::ColorTransformWithAlpha::Identity();
		r.m_TextureReference.m_pTexture = pTestTexture;
		r.m_WorldOcclusionRectangle = Rectangle::Create(1, 2, 1, 2);
		r.m_WorldRectangle = Rectangle::Create(1, 2, 1, 2);
		vPoses.PushBack(r);
	}

	TestOcclusionCommon(vPoses, buffer);
}

void FalconTest::TestRenderOcclusionOptimizerMultiple()
{
	using namespace Falcon;
	using namespace Falcon::Render;

	CommandBuffer buffer;

	SharedPtr<Texture> pTestTexture(SEOUL_NEW(MemoryBudgets::Developer) FalconTestTexture(0));

	Vector<CommandPose> vPoses;
	{
		auto& r = buffer.IssuePose();
		r = CommandPose();
		r.m_cxWorld = Falcon::ColorTransformWithAlpha::Identity();
		r.m_TextureReference.m_pTexture = pTestTexture;
		r.m_WorldRectangle = Rectangle::Create(1.5, 2, 1.5, 2);
	}
	{
		auto& r = buffer.IssuePose();
		r = CommandPose();
		r.m_cxWorld = Falcon::ColorTransformWithAlpha::Identity();
		r.m_TextureReference.m_pTexture = pTestTexture;
		r.m_WorldRectangle = Rectangle::Create(0.9f, 2, 1, 2);
		vPoses.PushBack(r);
	}
	{
		auto& r = buffer.IssuePose();
		r = CommandPose();
		r.m_cxWorld = Falcon::ColorTransformWithAlpha::Identity();
		r.m_TextureReference.m_pTexture = pTestTexture;
		r.m_WorldRectangle = Rectangle::Create(1, 2, 1, 2);
	}
	{
		auto& r = buffer.IssuePose();
		r = CommandPose();
		r.m_cxWorld = Falcon::ColorTransformWithAlpha::Identity();
		r.m_TextureReference.m_pTexture = pTestTexture;
		r.m_WorldOcclusionRectangle = Rectangle::Create(1, 2, 1, 2);
		r.m_WorldRectangle = Rectangle::Create(1, 2, 1, 2);
		vPoses.PushBack(r);
	}

	TestOcclusionCommon(vPoses, buffer);
}

static const Float kfGlyphScale = 0.024000f;

struct FalconTestGlyphEntry SEOUL_SEALED
{
	UniChar const m_Char;
	Int32 const m_iWidth;
	Int32 const m_iHeight;
	UInt8 const* const m_pGlyphData;
	size_t const m_zGlyphData;
}; // struct FalconTestGlyphEntry

// #define SEOUL_FALCON_TEST_GENERATE 1

#ifndef SEOUL_FALCON_TEST_GENERATE
#include "FalconTestData.h"
#endif

void FalconTest::TestWriteGlyphBitmap()
{
	using namespace Falcon;

	UnitTestsFileManagerHelper helper;

	void* pData = nullptr;
	UInt32 uSize = 0u;
	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(
		Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/Falcon/Roboto-Regular.ttf"),
		pData,
		uSize,
		0u,
		MemoryBudgets::Developer));

	TrueTypeFontData data(HString("Roboto-Regular"), pData, uSize);

#ifdef SEOUL_FALCON_TEST_GENERATE
	{
		ScopedPtr<SyncFile> pFile;
		SEOUL_UNITTESTING_ASSERT(FileManager::Get()->OpenFile(
			Path::Combine(Path::GetDirectoryName(__FILE__), "FalconTestData.h"),
			File::kWriteTruncate,
			pFile));
		BufferedSyncFile file(pFile.Get(), false);

		for (Int i = 33; i <= 126; ++i)
		{
			Int iX0, iX1, iY0, iY1;
			SEOUL_UNITTESTING_ASSERT(data.GetGlyphBitmapBox((UniChar)i, kfGlyphScale, iX0, iY0, iX1, iY1));

			Int const iBaseWidth = (iX1 - iX0) + 1;
			Int const iBaseHeight = (iY1 - iY0) + 1;
			Int const iFullWidth = (iBaseWidth + kiDiameterSDF);
			Int const iFullHeight = (iBaseHeight + kiDiameterSDF);

			UInt8* pGlyph = (UInt8*)MemoryManager::Allocate(iFullWidth * iFullHeight, MemoryBudgets::Developer);
			SEOUL_UNITTESTING_ASSERT(data.WriteGlyphBitmap(
				(UniChar)i,
				pGlyph,
				iFullWidth,
				iFullHeight,
				iFullWidth,
				kfGlyphScale,
				true));

			file.Printf("static const Int32 s_kiGlyph%dWidth = %d;\n", (Int)i, iFullWidth);
			file.Printf("static const Int32 s_kiGlyph%dHeight = %d;\n", (Int)i, iFullHeight);
			file.Printf("static const UInt8 s_kaGlyph%d[] = \n{", (Int)i);
			Int iOut = 0;
			for (Int y = 0; y < iFullHeight; ++y)
			{
				file.Printf("\n\t");
				for (Int x = 0; x < iFullWidth; ++x)
				{
					file.Printf("0x%02X, ", (UInt)pGlyph[iOut++]);
				}
			}
			file.Printf("\n};\n");

			MemoryManager::Deallocate(pGlyph);
		}

		file.Printf("\nstatic const FalconTestGlyphEntry s_kaGlyphEntries[] = \n{");

		for (Int i = 33; i <= 126; ++i)
		{
			file.Printf("\n\t{ ");
			if (i == '\'' || i == '\\')
			{
				file.Printf("'\\%c', ", (Byte)i);
			}
			else
			{
				file.Printf("'%c', ", (Byte)i);
			}
			file.Printf("s_kiGlyph%dWidth, ", (Int)i);
			file.Printf("s_kiGlyph%dHeight, ", (Int)i);
			file.Printf("s_kaGlyph%d, ", (Int)i);
			file.Printf("sizeof(s_kaGlyph%d), },", (Int)i);
		}

		file.Printf("\n};\n");
	}
#else // #ifndef SEOUL_FALCON_TEST_GENERATE
	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(s_kaGlyphEntries); ++i)
	{
		auto const& e = s_kaGlyphEntries[i];

		SEOUL_UNITTESTING_ASSERT(e.m_zGlyphData == (size_t)(e.m_iHeight * e.m_iWidth));
		UInt8* pGlyph = (UInt8*)MemoryManager::Allocate(e.m_zGlyphData, MemoryBudgets::Developer);
		SEOUL_UNITTESTING_ASSERT(data.WriteGlyphBitmap(
			e.m_Char,
			pGlyph,
			e.m_iWidth,
			e.m_iHeight,
			e.m_iWidth,
			kfGlyphScale,
			true));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(pGlyph, e.m_pGlyphData, e.m_zGlyphData));
		MemoryManager::Deallocate(pGlyph);
	}
#endif // /#ifndef SEOUL_FALCON_TEST_GENERATE
}

void FalconTest::TestRectangleIntersect()
{
	auto a = Falcon::Rectangle::Create(0.0f, 100.0f, 0.0f, 100.0f);
	auto b = Falcon::Rectangle::Create(0.0f, 25.0f, 0.0f, 100.0f);

	SEOUL_UNITTESTING_ASSERT(Falcon::Intersects(a, Matrix2x3::Identity(), b));
	for (auto i = 0; i < 360; ++i)
	{
		SEOUL_UNITTESTING_ASSERT(Falcon::Intersects(
			a,
			Matrix2x3::CreateTranslation(0.01f, 0.01f) * Matrix2x3::CreateRotationFromDegrees((Float)i),
			b));
	}

	SEOUL_UNITTESTING_ASSERT(!Falcon::Intersects(a, Matrix2x3::CreateTranslation( 100.01f,    0.00f), b));
	SEOUL_UNITTESTING_ASSERT(!Falcon::Intersects(a, Matrix2x3::CreateTranslation(   0.00f,  100.01f), b));
	SEOUL_UNITTESTING_ASSERT(!Falcon::Intersects(a, Matrix2x3::CreateTranslation( -25.01f,    0.01f), b));
	SEOUL_UNITTESTING_ASSERT(!Falcon::Intersects(a, Matrix2x3::CreateTranslation(   0.00f, -100.01f), b));

	for (auto i = 0; i < 360; ++i)
	{
		SEOUL_UNITTESTING_ASSERT(!Falcon::Intersects(
			a,
			Matrix2x3::CreateTranslation(Vector2D(a.m_fRight, a.m_fBottom).Length()+ Vector2D(b.m_fRight, b.m_fBottom).Length() + 0.01f, 0.0f) * Matrix2x3::CreateRotationFromDegrees((Float)i),
			b));
	}
}

void FalconTest::TestSetTransform()
{
	for (Int s = -10; s <= 10; ++s)
	{
		for (Int r = -179; r <= 180; ++r)
		{
			for (Int t = -10; t <= 10; ++t)
			{
				auto const m =
					Matrix2x3::CreateTranslation((Float)t, (Float)-t) *
					Matrix2x3::CreateRotationFromDegrees((Float)r) *
					Matrix2x3::CreateScale((Float)s, (Float)s);

				SharedPtr<FalconTestFalconInstance> p(SEOUL_NEW(MemoryBudgets::Developer) FalconTestFalconInstance());
				p->SetTransform(m);

				// SetTransform does not have isolated X scale/Y scale mirror tracking, so
				// it must assume (based on the determinant) where the negative or not
				// scale is. With both axes negative or positive in tandum, the determinant
				// will be positive, so it will assume neither axis has scale, which result
				// in a positive scale along both with an adjusted rotation to account.
				auto const testS = (s < 0 ? -s : s);

				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)testS, p->GetScale().X, 1e-4f);
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)testS, p->GetScaleX(), 1e-4f);
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)testS, p->GetScale().Y, 1e-4f);
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)testS, p->GetScaleY(), 1e-4f);
				if (0 != s)
				{
					auto const testR = (s < 0
						? (r <= 0 ? (180 + r) : (r - 180))
						: (180 == r ? -180 : r));
					SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)testR, p->GetRotationInDegrees(), 1e-4f);
					SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(DegreesToRadians((Float)testR), p->GetRotationInRadians(), 1e-4f);
				}
				SEOUL_UNITTESTING_ASSERT_EQUAL((Float)t, p->GetPosition().X);
				SEOUL_UNITTESTING_ASSERT_EQUAL((Float)t, p->GetPositionX());
				SEOUL_UNITTESTING_ASSERT_EQUAL((Float)-t, p->GetPosition().Y);
				SEOUL_UNITTESTING_ASSERT_EQUAL((Float)-t, p->GetPositionY());
			}
		}
	}

	for (Int s = -10; s <= 10; ++s)
	{
		for (Int r = -179; r <= 180; ++r)
		{
			for (Int t = -10; t <= 10; ++t)
			{
				auto const m =
					Matrix2x3::CreateTranslation((Float)t, (Float)-t) *
					Matrix2x3::CreateRotationFromDegrees((Float)r) *
					Matrix2x3::CreateScale(-(Float)s, (Float)s);

				SharedPtr<FalconTestFalconInstance> p(SEOUL_NEW(MemoryBudgets::Developer) FalconTestFalconInstance());
				p->SetTransform(m);

				// SetTransform does not have isolated X scale/Y scale mirror tracking, so
				// it must assume (based on the determinant) where the negative or not
				// scale is. With both axes negative opposed, the determinant will always
				// be negative, which will always be tracked as a negative scale along x.
				auto const testS = (s < 0 ? -s : s);

				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-(Float)testS, p->GetScale().X, 1e-4f);
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-(Float)testS, p->GetScaleX(), 1e-4f);
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)testS, p->GetScale().Y, 1e-4f);
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)testS, p->GetScaleY(), 1e-4f);
				if (0 != s)
				{
					auto const testR = (s < 0
						? (r < 0 ? (180 + r) : (r - 180))
						: (180 == r ? -180 : r));
					SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)testR, p->GetRotationInDegrees(), 1e-4f);
					SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(DegreesToRadians((Float)testR), p->GetRotationInRadians(), 1e-4f);
				}
				SEOUL_UNITTESTING_ASSERT_EQUAL((Float)t, p->GetPosition().X);
				SEOUL_UNITTESTING_ASSERT_EQUAL((Float)t, p->GetPositionX());
				SEOUL_UNITTESTING_ASSERT_EQUAL((Float)-t, p->GetPosition().Y);
				SEOUL_UNITTESTING_ASSERT_EQUAL((Float)-t, p->GetPositionY());
			}
		}
	}
}

void FalconTest::TestSetTransformTerms()
{
	for (Int s = -10; s <= 10; ++s)
	{
		for (Int r = -179; r <= 180; ++r)
		{
			for (Int t = -10; t <= 10; ++t)
			{
				SharedPtr<FalconTestFalconInstance> p(SEOUL_NEW(MemoryBudgets::Developer) FalconTestFalconInstance());

				p->SetRotationInDegrees((Float)r);
				p->SetScale((Float)s, (Float)s);
				p->SetPosition((Float)t, (Float)-t);

				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)s, p->GetScale().X, 1e-4f);
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)s, p->GetScaleX(), 1e-4f);
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)s, p->GetScale().Y, 1e-4f);
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)s, p->GetScaleY(), 1e-4f);
				if (0 != s)
				{
					auto const fTestR = (Float)(180 == r ? -180 : r);
					SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(fTestR, p->GetRotationInDegrees(), 1e-4f);
					SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(DegreesToRadians(fTestR), p->GetRotationInRadians(), 1e-4f);
				}
				SEOUL_UNITTESTING_ASSERT_EQUAL((Float)t, p->GetPosition().X);
				SEOUL_UNITTESTING_ASSERT_EQUAL((Float)t, p->GetPositionX());
				SEOUL_UNITTESTING_ASSERT_EQUAL((Float)-t, p->GetPosition().Y);
				SEOUL_UNITTESTING_ASSERT_EQUAL((Float)-t, p->GetPositionY());
			}
		}
	}

	for (Int sx = -3; sx <= 2; ++sx)
	{
		for (Int sy = -2; sy <= 3; ++sy)
		{
			for (Int r = -179; r <= 180; ++r)
			{
				for (Int t = -2; t <= 2; ++t)
				{
					SharedPtr<FalconTestFalconInstance> p(SEOUL_NEW(MemoryBudgets::Developer) FalconTestFalconInstance());

					p->SetRotationInDegrees((Float)r);
					p->SetScale((Float)sx, (Float)sy);
					p->SetPosition((Float)t, (Float)-t);

					SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)sx, p->GetScale().X, 1e-4f);
					SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)sx, p->GetScaleX(), 1e-4f);
					SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)sy, p->GetScale().Y, 1e-4f);
					SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)sy, p->GetScaleY(), 1e-4f);
					if (0 != sx && 0 != sy)
					{
						auto const fTestR = (Float)(180 == r ? -180 : r);
						SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(fTestR, p->GetRotationInDegrees(), 1e-4f);
						SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(DegreesToRadians(fTestR), p->GetRotationInRadians(), 1e-4f);
					}
					SEOUL_UNITTESTING_ASSERT_EQUAL((Float)t, p->GetPosition().X);
					SEOUL_UNITTESTING_ASSERT_EQUAL((Float)t, p->GetPositionX());
					SEOUL_UNITTESTING_ASSERT_EQUAL((Float)-t, p->GetPosition().Y);
					SEOUL_UNITTESTING_ASSERT_EQUAL((Float)-t, p->GetPositionY());
				}
			}
		}
	}

	for (Int s = -10; s <= 10; ++s)
	{
		for (Int r = -179; r <= 180; ++r)
		{
			for (Int t = -10; t <= 10; ++t)
			{
				SharedPtr<FalconTestFalconInstance> p(SEOUL_NEW(MemoryBudgets::Developer) FalconTestFalconInstance());

				p->SetRotationInDegrees((Float)r);
				p->SetScale(-(Float)s, (Float)s);
				p->SetPosition((Float)t, (Float)-t);

				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-(Float)s, p->GetScale().X, 1e-4f);
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-(Float)s, p->GetScaleX(), 1e-4f);
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)s, p->GetScale().Y, 1e-4f);
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)s, p->GetScaleY(), 1e-4f);
				if (0 != s)
				{
					auto const fTestR = (Float)(180 == r ? -180 : r);
					SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(fTestR, p->GetRotationInDegrees(), 1e-4f);
					SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(DegreesToRadians(fTestR), p->GetRotationInRadians(), 1e-4f);
				}
				SEOUL_UNITTESTING_ASSERT_EQUAL((Float)t, p->GetPosition().X);
				SEOUL_UNITTESTING_ASSERT_EQUAL((Float)t, p->GetPositionX());
				SEOUL_UNITTESTING_ASSERT_EQUAL((Float)-t, p->GetPosition().Y);
				SEOUL_UNITTESTING_ASSERT_EQUAL((Float)-t, p->GetPositionY());
			}
		}
	}
}

// Regression test for a case where setting the X scale to 0
// would cause the X scale to be set permanently to 0.
void FalconTest::TestScaleRegressionX()
{
	SharedPtr<FalconTestFalconInstance> p(SEOUL_NEW(MemoryBudgets::Developer) FalconTestFalconInstance());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, p->GetScaleX());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, p->GetScaleY());
	p->SetScaleY(10.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, p->GetScaleX());
	SEOUL_UNITTESTING_ASSERT_EQUAL(10.0f, p->GetScaleY());
	p->SetScaleX(0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, p->GetScaleX());
	SEOUL_UNITTESTING_ASSERT_EQUAL(10.0f, p->GetScaleY());

	for (Int i = 0; i < 15; ++i)
	{
		auto const vOrigScale = p->GetScale();
		for (Int iRot = -179; iRot <= 180; ++iRot)
		{
			auto const vScale = p->GetScale();
			p->SetRotationInDegrees((Float)iRot);
			// With scale of 0, rotation is lost.
			auto const fTest = (IsZero(vScale.X) || IsZero(vScale.Y)) ? 0.0f : (Float)iRot;
			if (180 == iRot)
			{
				SEOUL_UNITTESTING_ASSERT(EqualDegrees(fTest, p->GetRotationInDegrees(), 1e-4f));
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(fTest, p->GetRotationInDegrees(), 1e-4f);
			}
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(vScale, p->GetScale(), 1e-4f);
		}
		p->SetRotationInDegrees(0);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(vOrigScale, p->GetScale(), 1e-4f);

		auto const f = (Float)i / 14.0f;
		p->SetScaleX(f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(f, p->GetScaleX(), 1e-4f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(10.0f, p->GetScaleY(), 1e-3f);
	}
}

// Regression test for a case where setting the Y scale to 0
// would cause the Y scale to be set permanently to 0.
void FalconTest::TestScaleRegressionY()
{
	SharedPtr<FalconTestFalconInstance> p(SEOUL_NEW(MemoryBudgets::Developer) FalconTestFalconInstance());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, p->GetScaleX());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, p->GetScaleY());
	p->SetScaleX(10.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(10.0f, p->GetScaleX());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, p->GetScaleY());
	p->SetScaleY(0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(10.0f, p->GetScaleX());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, p->GetScaleY());

	for (Int i = 0; i < 15; ++i)
	{
		// With an Y of 0, rotation is lost.
		auto const vOrigScale = p->GetScale();
		for (Int iRot = -179; iRot <= 180; ++iRot)
		{
			auto const vScale = p->GetScale();
			p->SetRotationInDegrees((Float)iRot);
			// With scale of 0, rotation is lost.
			auto const fTest = (IsZero(vScale.X) || IsZero(vScale.Y)) ? 0.0f : (Float)iRot;
			if (180 == iRot)
			{
				SEOUL_UNITTESTING_ASSERT(EqualDegrees(fTest, p->GetRotationInDegrees(), 1e-4f));
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(fTest, p->GetRotationInDegrees(), 1e-4f);
			}
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(vScale, p->GetScale(), 1e-4f);
		}
		p->SetRotationInDegrees(0);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(vOrigScale, p->GetScale(), 1e-4f);

		auto const f = (Float)i / 14.0f;
		p->SetScaleY(f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(10.0f, p->GetScaleX(), 1e-3f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(f, p->GetScaleY(), 1e-4f);
	}
}

// Regression for a case where an instance with skew needed to
// maintain that skew across scaling.
void FalconTest::TestSkewRegression()
{
	SharedPtr<FalconTestFalconInstance> p(SEOUL_NEW(MemoryBudgets::Developer) FalconTestFalconInstance());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, p->GetRotationInDegrees());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, p->GetScaleX());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, p->GetScaleY());

	Matrix2x3 m;
	m.M00 = 0.999984741f;
	m.M10 = 0.0f;
	m.M01 = -0.755447388f;
	m.M11 = 1.37194824f;
	m.TX = 0.349999994f;
	m.TY = -0.250000000f;
	p->SetTransform(m);

	for (Int i = 0; i < 15; ++i)
	{
		auto const f = (Float)i / 14.0f;
		p->SetScaleX(f);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, p->GetRotationInDegrees());
	}
}

void FalconTest::TestRotationUpdate()
{
	// Basic.
	{
		SharedPtr<FalconTestFalconInstance> p(SEOUL_NEW(MemoryBudgets::Developer) FalconTestFalconInstance());
		for (Int i = -179; i <= 180; ++i)
		{
			p->SetRotationInDegrees((Float)i);
			SEOUL_UNITTESTING_ASSERT(EqualDegrees((Float)i, p->GetRotationInDegrees(), 1e-4f));
		}
	}

	// Negative X scale.
	{
		SharedPtr<FalconTestFalconInstance> p(SEOUL_NEW(MemoryBudgets::Developer) FalconTestFalconInstance());
		p->SetScaleX(-1.5f);
		for (Int i = -179; i <= 180; ++i)
		{
			p->SetRotationInDegrees((Float)i);
			SEOUL_UNITTESTING_ASSERT(EqualDegrees((Float)i, p->GetRotationInDegrees(), 1e-4f));
		}
	}

	// Negative Y scale.
	{
		SharedPtr<FalconTestFalconInstance> p(SEOUL_NEW(MemoryBudgets::Developer) FalconTestFalconInstance());
		p->SetScaleY(-1.5f);
		for (Int i = -179; i <= 180; ++i)
		{
			p->SetRotationInDegrees((Float)i);
			SEOUL_UNITTESTING_ASSERT(EqualDegrees((Float)i, p->GetRotationInDegrees(), 1e-4f));
		}
	}

	// Positive X scale.
	{
		SharedPtr<FalconTestFalconInstance> p(SEOUL_NEW(MemoryBudgets::Developer) FalconTestFalconInstance());
		p->SetScaleX(1.5f);
		for (Int i = -179; i <= 180; ++i)
		{
			p->SetRotationInDegrees((Float)i);
			SEOUL_UNITTESTING_ASSERT(EqualDegrees((Float)i, p->GetRotationInDegrees(), 1e-4f));
		}
	}

	// Positive Y scale.
	{
		SharedPtr<FalconTestFalconInstance> p(SEOUL_NEW(MemoryBudgets::Developer) FalconTestFalconInstance());
		p->SetScaleY(1.5f);
		for (Int i = -179; i <= 180; ++i)
		{
			p->SetRotationInDegrees((Float)i);
			SEOUL_UNITTESTING_ASSERT(EqualDegrees((Float)i, p->GetRotationInDegrees(), 1e-4f));
		}
	}

	// Negative dual scale.
	{
		SharedPtr<FalconTestFalconInstance> p(SEOUL_NEW(MemoryBudgets::Developer) FalconTestFalconInstance());
		p->SetScale(-1.5f, -1.5f);
		for (Int i = -179; i <= 180; ++i)
		{
			p->SetRotationInDegrees((Float)i);
			SEOUL_UNITTESTING_ASSERT(EqualDegrees((Float)i, p->GetRotationInDegrees(), 1e-4f));
		}
	}

	// Positive dual scale.
	{
		SharedPtr<FalconTestFalconInstance> p(SEOUL_NEW(MemoryBudgets::Developer) FalconTestFalconInstance());
		p->SetScale(1.5f, 1.5f);
		for (Int i = -179; i <= 180; ++i)
		{
			p->SetRotationInDegrees((Float)i);
			SEOUL_UNITTESTING_ASSERT(EqualDegrees((Float)i, p->GetRotationInDegrees(), 1e-4f));
		}
	}
}

void FalconTest::TestScaleUpdate()
{
	// X
	{
		SharedPtr<FalconTestFalconInstance> p(SEOUL_NEW(MemoryBudgets::Developer) FalconTestFalconInstance());
		for (Int i = -15; i <= 15; ++i)
		{
			p->SetScaleX((Float)i);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)i, p->GetScaleX(), 1e-4f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)i, p->GetScale().X, 1e-4f);
		}
	}

	// Y
	{
		SharedPtr<FalconTestFalconInstance> p(SEOUL_NEW(MemoryBudgets::Developer) FalconTestFalconInstance());
		for (Int i = -15; i <= 15; ++i)
		{
			p->SetScaleY((Float)i);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)i, p->GetScaleY(), 1e-4f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Float)i, p->GetScale().Y, 1e-4f);
		}
	}
}

namespace
{

void TestInitialize()
{
	Byte const* sPlatform = GetPlatformPrefix();
	FileManager::Get()->RegisterFileSystem<PackageFileSystem>(
		Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf(R"(UnitTests\GamePatcher\%s_Config.sar)", sPlatform)));
	FileManager::Get()->RegisterFileSystem<PackageFileSystem>(
		Path::Combine(GamePaths::Get()->GetConfigDir(), String::Printf(R"(UnitTests\GamePatcher\%s_Content.sar)", sPlatform)));
}

static Vector< Pair<void*, UInt32> > s_vLocBodies;

static void CleanupBodies()
{
	for (Int32 i = (Int32)s_vLocBodies.GetSize() - 1; i >= 0; --i)
	{
		MemoryManager::Deallocate(s_vLocBodies[i].First);
		s_vLocBodies[i].First = nullptr;
	}
	s_vLocBodies.Clear();
}

void TestInitializeWithLocLoad()
{
	// Before regular initialize, read loc.json. This
	// is the app's version, we want to grab it before
	// we override the lookup.
	CleanupBodies();
	for (auto const& s : { "English", "Spanish", "French", "Korean", "Japanese", "Russian", "German", "Italian" })
	{
		auto const filePath = FilePath::CreateConfigFilePath(String::Printf("Loc/%s/locale.json", s));
		if (FileManager::Get()->Exists(filePath))
		{
			s_vLocBodies.PushBack( Pair<void*, UInt32>() );
			SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(filePath, s_vLocBodies.Back().First, s_vLocBodies.Back().Second, 0u, MemoryBudgets::Developer));
		}
	}

	// Now perform regular initialize.
	TestInitialize();
}

class TestInterface : public Falcon::AddInterface
{
public:
	virtual void FalconOnAddToParent(
		Falcon::MovieClipInstance* pParent,
		Falcon::Instance* pInstance,
		const HString& sClassName) SEOUL_OVERRIDE
	{
	}

	virtual void FalconOnClone(
		Falcon::Instance const* pFromInstance,
		Falcon::Instance* pToInstance) SEOUL_OVERRIDE
	{
	}
};

} // namespace anonymous

void FalconTest::TestHtmlFormattingCharRefs()
{
	using namespace Falcon;

	UnitTestsEngineHelper helper(TestInitialize);
	UI::Manager man(FilePath::CreateConfigFilePath("gui.json"), UI::StackFilter::kAlways);
	TestInterface testinterf;

	auto hFCN = man.GetFCNFileData(FilePath::CreateContentFilePath("Authored/UI/TestMovie.swf"));
	Content::LoadManager::Get()->WaitUntilLoadIsFinished(hFCN);
	auto pFCN(hFCN.GetPtr()->GetFCNFile());

	SharedPtr<EditTextInstance> pInstance;
	{
		SharedPtr<MovieClipDefinition> pMovieClip;
		SEOUL_UNITTESTING_ASSERT(pFCN->GetExportedDefinition(HString("TestSymbol"), pMovieClip));
		SharedPtr<MovieClipInstance> pMcInstance;
		pMovieClip->CreateInstance(pMcInstance);
		pMcInstance->AdvanceToFrame0(testinterf);
		pMcInstance->GetChildByName(HString("txtTest"), pInstance);
	}

	for (auto const& s : {
		"The quick brown fox&amp;Jumped over the lazy dog",
		"The quick brown fox&Jumped over the lazy dog",
		"The quick brown fox&#38;Jumped over the lazy dog",
		"The quick brown fox&#0x26;Jumped over the lazy dog",
		})
	{
		pInstance->SetXhtmlText(s);
		pInstance->CommitFormatting();
		SEOUL_UNITTESTING_ASSERT_EQUAL("The quick brown fox&Jumped over the lazy dog", pInstance->GetText());
	}

	for (auto const& s : {
		"The quick brown fox&nbsp;Jumped over the lazy dog",
		"The quick brown fox&#160;Jumped over the lazy dog",
		"The quick brown fox&#0xA0;Jumped over the lazy dog",
		})
	{
		pInstance->SetXhtmlText(s);
		pInstance->CommitFormatting();

		DataStore ds;
		DataStoreParser::FromString(R"(["The quick brown fox\u00A0Jumped over the lazy dog"])", ds);

		DataNode val;
		String sExpected;
		SEOUL_UNITTESTING_ASSERT(ds.GetValueFromArray(ds.GetRootNode(), 0u, val));
		SEOUL_UNITTESTING_ASSERT(ds.AsString(val, sExpected));
		SEOUL_UNITTESTING_ASSERT_EQUAL(sExpected, pInstance->GetText());
	}
}

void FalconTest::TestHtmlFormattingRegression()
{
	using namespace Falcon;

	UnitTestsEngineHelper helper(TestInitialize);
	UI::Manager man(FilePath::CreateConfigFilePath("gui.json"), UI::StackFilter::kAlways);
	TestInterface testinterf;

	auto hFCN = man.GetFCNFileData(FilePath::CreateContentFilePath("Authored/UI/TestMovie.swf"));
	Content::LoadManager::Get()->WaitUntilLoadIsFinished(hFCN);
	auto pFCN(hFCN.GetPtr()->GetFCNFile());

	SharedPtr<EditTextInstance> pInstance;
	{
		SharedPtr<MovieClipDefinition> pMovieClip;
		SEOUL_UNITTESTING_ASSERT(pFCN->GetExportedDefinition(HString("TestSymbol"), pMovieClip));
		SharedPtr<MovieClipInstance> pMcInstance;
		pMovieClip->CreateInstance(pMcInstance);
		pMcInstance->AdvanceToFrame0(testinterf);
		pMcInstance->GetChildByName(HString("txtTest"), pInstance);
	}

	// Cache - reference will not change.
	auto const& v = pInstance->UnitTesting_GetTextChunks();

	// Case
	{
		// Newline is intentional and is the source of the bug.
		auto const s = "<vertical_centered/><font color=\"#eeeee7\" effect=\"BlackOutline\"><img src=\"content://Authored/Textures/TestTexture.png\"\r\nwidth=\"27\" height=\"27\" voffset=\"7\"/> ${Amount}</font>";

		pInstance->SetXhtmlText(s);
		pInstance->CommitFormatting();
		SEOUL_UNITTESTING_ASSERT_EQUAL(R"( ${Amount})", pInstance->GetText());
	}

	// Case
	{
		auto const s = "<1m";

		pInstance->SetXhtmlText(s);
		pInstance->CommitFormatting();
		SEOUL_UNITTESTING_ASSERT_EQUAL("<1m", pInstance->GetText());
	}

	// Case
	{
		auto const s = "<font size=55><b>BONUS HEROES!</b></font><font size=8>\r\n</font><font size=20><b>GET MORE COVERS OF YOUR FAVORITE CHARACTERS!</b></font";

		pInstance->SetXhtmlText(s);
		pInstance->CommitFormatting();
		SEOUL_UNITTESTING_ASSERT_EQUAL("BONUS HEROES!\r\nGET MORE COVERS OF YOUR FAVORITE CHARACTERS!", pInstance->GetText());

		SEOUL_UNITTESTING_ASSERT_EQUAL("BONUS HEROES!", String(v[0].m_iBegin.GetPtr(), v[0].m_uNumberOfCharacters));
		SEOUL_UNITTESTING_ASSERT_EQUAL("\r\n", String(v[1].m_iBegin.GetPtr(), v[1].m_uNumberOfCharacters));
		SEOUL_UNITTESTING_ASSERT_EQUAL("GET MORE COVERS OF YOUR FAVORITE CHARACTERS!", String(v[2].m_iBegin.GetPtr(), v[2].m_uNumberOfCharacters));

		SEOUL_UNITTESTING_ASSERT_EQUAL(55, v[0].m_Format.GetUnscaledTextHeight());
		SEOUL_UNITTESTING_ASSERT_EQUAL(8,  v[1].m_Format.GetUnscaledTextHeight());
		SEOUL_UNITTESTING_ASSERT_EQUAL(20, v[2].m_Format.GetUnscaledTextHeight());

		SEOUL_UNITTESTING_ASSERT_EQUAL(true, v[0].m_Format.m_Font.m_bBold);
		SEOUL_UNITTESTING_ASSERT_EQUAL(false, v[1].m_Format.m_Font.m_bBold);
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, v[2].m_Format.m_Font.m_bBold);
	}

	// Case
	{
		auto const s = "<b>JUGGERNAUT: HEROIC</b>\r\n<font size-35>DON'T STOP</font>";

		pInstance->SetXhtmlText(s);
		pInstance->CommitFormatting();
		SEOUL_UNITTESTING_ASSERT_EQUAL("JUGGERNAUT: HEROIC\r\n-35>DON'T STOP", pInstance->GetText());
	}
}

void FalconTest::TestHtmlFormattingRobustness()
{
	using namespace Falcon;

	UnitTestsEngineHelper helper(TestInitialize);
	UI::Manager man(FilePath::CreateConfigFilePath("gui.json"), UI::StackFilter::kAlways);
	TestInterface testinterf;

	auto hFCN = man.GetFCNFileData(FilePath::CreateContentFilePath("Authored/UI/TestMovie.swf"));
	Content::LoadManager::Get()->WaitUntilLoadIsFinished(hFCN);
	auto pFCN(hFCN.GetPtr()->GetFCNFile());

	SharedPtr<EditTextInstance> pInstance;
	{
		SharedPtr<MovieClipDefinition> pMovieClip;
		SEOUL_UNITTESTING_ASSERT(pFCN->GetExportedDefinition(HString("TestSymbol"), pMovieClip));
		SharedPtr<MovieClipInstance> pMcInstance;
		pMovieClip->CreateInstance(pMcInstance);
		pMcInstance->AdvanceToFrame0(testinterf);
		pMcInstance->GetChildByName(HString("txtTest"), pInstance);
	}

	// Cache - reference will not change.
	auto const& v = pInstance->UnitTesting_GetTextChunks();

	// Basic.
	for (auto const& s : {
		"The quick brown fox<br/>Jumped over the lazy dog",
		"The quick brown fox<br>Jumped over the lazy dog",
		"The quick brown fox<p/>Jumped over the lazy dog"
		})
	{
		pInstance->SetXhtmlText(s);
		pInstance->CommitFormatting();
		SEOUL_UNITTESTING_ASSERT_EQUAL("The quick brown foxJumped over the lazy dog", pInstance->GetText());
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(19, v.Front().m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(24, v.Back().m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(28.0f, v.Front().m_Format.GetUnscaledTextHeight());
		SEOUL_UNITTESTING_ASSERT_EQUAL(28.0f, v.Back().m_Format.GetUnscaledTextHeight());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.Front().m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.Back().m_iLine);
	}

	// Robust attribute handling (typos)
	for (auto const& s : {
		"The quick brown fox<font size=10>Jumped over the lazy dog</font>",
		"The quick brown fox<font size= 10>Jumped over the lazy dog</font>",
		"The quick brown fox<font size= 10 >Jumped over the lazy dog</font>",
		"The quick brown fox<font size=10 >Jumped over the lazy dog</font>",
		"The quick brown fox<font size=  10 >Jumped over the lazy dog</font>",
		"The quick brown fox<font size=  10  >Jumped over the lazy dog</font>",
		"The quick brown fox<font size=		 10		 >Jumped over the lazy dog</font>",

		"The quick brown fox<font size=\"10\">Jumped over the lazy dog</font>",
		"The quick brown fox<font size= \"10\">Jumped over the lazy dog</font>",
		"The quick brown fox<font size= \"10\" >Jumped over the lazy dog</font>",
		"The quick brown fox<font size=\"10\" >Jumped over the lazy dog</font>",

		"The quick brown fox<font size='10'>Jumped over the lazy dog</font>",
		"The quick brown fox<font size= '10'>Jumped over the lazy dog</font>",
		"The quick brown fox<font size= '10' >Jumped over the lazy dog</font>",
		"The quick brown fox<font size=10'>Jumped over the lazy dog</font>",
		"The quick brown fox<font size= 10'>Jumped over the lazy dog</font>",
		"The quick brown fox<font size=10' >Jumped over the lazy dog</font>",
		"The quick brown fox<font size= 10' >Jumped over the lazy dog</font>",

		"The quick brown fox<font size=\"10'>Jumped over the lazy dog</font>",
		"The quick brown fox<font size= \"10'>Jumped over the lazy dog</font>",
		"The quick brown fox<font size= \"10' >Jumped over the lazy dog</font>",
		"The quick brown fox<font size=10'>Jumped over the lazy dog</font>",
		"The quick brown fox<font size= 10'>Jumped over the lazy dog</font>",
		"The quick brown fox<font size=10' >Jumped over the lazy dog</font>",
		"The quick brown fox<font size= 10' >Jumped over the lazy dog</font>",

		"The quick brown fox<font size='10\">Jumped over the lazy dog</font>",
		"The quick brown fox<font size= '10\">Jumped over the lazy dog</font>",
		"The quick brown fox<font size= '10\" >Jumped over the lazy dog</font>",
		"The quick brown fox<font size=10\">Jumped over the lazy dog</font>",
		"The quick brown fox<font size= 10\">Jumped over the lazy dog</font>",
		"The quick brown fox<font size=10\" >Jumped over the lazy dog</font>",
		"The quick brown fox<font size= 10\" >Jumped over the lazy dog</font>",
		})
	{
		pInstance->SetXhtmlText(s);
		pInstance->CommitFormatting();
		SEOUL_UNITTESTING_ASSERT_EQUAL("The quick brown foxJumped over the lazy dog", pInstance->GetText());
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(19, v.Front().m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(24, v.Back().m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(28.0f, v.Front().m_Format.GetUnscaledTextHeight());
		SEOUL_UNITTESTING_ASSERT_EQUAL(10.0f, v.Back().m_Format.GetUnscaledTextHeight());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.Front().m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.Back().m_iLine);
	}

	// Robust termination handling (missing close tag).
	for (auto const& s : {
		"The quick brown fox<p align=center><font size=10>Jumped over the lazy dog</font></p>",
		"The quick brown fox<p align=center><font size=10>Jumped over the lazy dog</p></font>",
		"The quick brown fox<p align=center><font size=10>Jumped over the lazy dog</p>",
		"The quick brown fox<p align=center><font size=10>Jumped over the lazy dog</font>",
		"The quick brown fox<p align=center><font size=10>Jumped over the lazy dog",
		})
	{
		pInstance->SetXhtmlText(s);
		pInstance->CommitFormatting();
		SEOUL_UNITTESTING_ASSERT_EQUAL("The quick brown foxJumped over the lazy dog", pInstance->GetText());
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(19, v.Front().m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(24, v.Back().m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(28.0f, v.Front().m_Format.GetUnscaledTextHeight());
		SEOUL_UNITTESTING_ASSERT_EQUAL(10.0f, v.Back().m_Format.GetUnscaledTextHeight());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.Front().m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.Back().m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HtmlAlign::kLeft, v.Front().m_Format.GetAlignmentEnum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HtmlAlign::kCenter, v.Back().m_Format.GetAlignmentEnum());
	}

	// Robust termination handling (incorrect slash or missing slash).
	for (auto const& s : {
		"The quick brown fox<p align=center><font size=10>Jumped over the lazy dog</font></p>",
		"The quick brown fox<p align=center><font size=10>Jumped over the lazy dog</font><\\p>",
		"The quick brown fox<p align=center><font size=10>Jumped over the lazy dog<\\font><\\p>",
		"The quick brown fox<p align=center><font size=10>Jumped over the lazy dog<\\font>",
		})
	{
		pInstance->SetXhtmlText(s);
		pInstance->CommitFormatting();
		SEOUL_UNITTESTING_ASSERT_EQUAL("The quick brown foxJumped over the lazy dog", pInstance->GetText());
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(19, v.Front().m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(24, v.Back().m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(28.0f, v.Front().m_Format.GetUnscaledTextHeight());
		SEOUL_UNITTESTING_ASSERT_EQUAL(10.0f, v.Back().m_Format.GetUnscaledTextHeight());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.Front().m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.Back().m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HtmlAlign::kLeft, v.Front().m_Format.GetAlignmentEnum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HtmlAlign::kCenter, v.Back().m_Format.GetAlignmentEnum());
	}

	// Robust value handling (missing delimiter on colors).
	for (auto const& s : {
		"The quick brown fox<p align=center><font color=#FF05F7>Jumped over the lazy dog</font></p>",
		"The quick brown fox<p align=center><font color=FF05F7>Jumped over the lazy dog</font><\\p>",
		"The quick brown fox<p align=center><font color=\"#FF05F7\">Jumped over the lazy dog<\\font><\\p>",
		"The quick brown fox<p align=center><font color=\"FF05F7\">Jumped over the lazy dog<\\font>",
		})
	{
		pInstance->SetXhtmlText(s);
		pInstance->CommitFormatting();
		SEOUL_UNITTESTING_ASSERT_EQUAL("The quick brown foxJumped over the lazy dog", pInstance->GetText());
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(19, v.Front().m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(24, v.Back().m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Create(51, 51, 51, 255), v.Front().m_Format.m_TextColor);
		SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Create(255, 0x05, 0xF7, 255), v.Back().m_Format.m_TextColor);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.Front().m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v.Back().m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HtmlAlign::kLeft, v.Front().m_Format.GetAlignmentEnum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HtmlAlign::kCenter, v.Back().m_Format.GetAlignmentEnum());
	}
}

void FalconTest::TestHtmlFormattingStrings()
{
	using namespace Falcon;

	UnitTestsEngineHelper helper(TestInitializeWithLocLoad);
	auto const action(MakeScopedAction([]() {}, [&]() { CleanupBodies(); }));

	UI::Manager man(FilePath::CreateConfigFilePath("gui.json"), UI::StackFilter::kAlways);
	TestInterface testinterf;

	auto hFCN = man.GetFCNFileData(FilePath::CreateContentFilePath("Authored/UI/TestMovie.swf"));
	Content::LoadManager::Get()->WaitUntilLoadIsFinished(hFCN);
	auto pFCN(hFCN.GetPtr()->GetFCNFile());

	SharedPtr<EditTextInstance> pInstance;
	{
		SharedPtr<MovieClipDefinition> pMovieClip;
		SEOUL_UNITTESTING_ASSERT(pFCN->GetExportedDefinition(HString("TestSymbol"), pMovieClip));
		SharedPtr<MovieClipInstance> pMcInstance;
		pMovieClip->CreateInstance(pMcInstance);
		pMcInstance->AdvanceToFrame0(testinterf);
		pMcInstance->GetChildByName(HString("txtTest"), pInstance);
	}

	for (auto const& pair : s_vLocBodies)
	{
		// Parse.
		DataStore ds;
		SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString((Byte const*)pair.First, pair.Second, ds));

		// Iterate and test.
		for (auto i = ds.TableBegin(ds.GetRootNode()), iEnd = ds.TableEnd(ds.GetRootNode()); iEnd != i; ++i)
		{
			String s;
			SEOUL_UNITTESTING_ASSERT(ds.AsString(i->Second, s));

			// Just set - we're testing for warning generation and crashes, we don't
			// know the contents otherwise.
			pInstance->SetXhtmlText(s);
			pInstance->CommitFormatting();
		}
	}
}

void FalconTest::TestHtmlFormattingValues()
{
	using namespace Falcon;

	UnitTestsEngineHelper helper(TestInitialize);
	UI::Manager man(FilePath::CreateConfigFilePath("gui.json"), UI::StackFilter::kAlways);
	TestInterface testinterf;

	auto hFCN = man.GetFCNFileData(FilePath::CreateContentFilePath("Authored/UI/TestMovie.swf"));
	Content::LoadManager::Get()->WaitUntilLoadIsFinished(hFCN);
	auto pFCN(hFCN.GetPtr()->GetFCNFile());

	SharedPtr<EditTextInstance> pInstance;
	{
		SharedPtr<MovieClipDefinition> pMovieClip;
		SEOUL_UNITTESTING_ASSERT(pFCN->GetExportedDefinition(HString("TestSymbol"), pMovieClip));
		SharedPtr<MovieClipInstance> pMcInstance;
		pMovieClip->CreateInstance(pMcInstance);
		pMcInstance->AdvanceToFrame0(testinterf);
		pMcInstance->GetChildByName(HString("txtTest"), pInstance);
	}

	// Cache - reference will not change.
	auto const& v = pInstance->UnitTesting_GetTextChunks();
	auto const& vImg = pInstance->UnitTesting_GetImages();
	auto const& vLinks = pInstance->UnitTesting_GetLinks();

	// Bold.
	{
		pInstance->SetXhtmlText("The quick brown fox<b>Jumped over</b> the lazy dog");
		pInstance->CommitFormatting();
		SEOUL_UNITTESTING_ASSERT_EQUAL("The quick brown foxJumped over the lazy dog", pInstance->GetText());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(19, v[0].m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(11, v[1].m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(13, v[2].m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v[0].m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v[1].m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v[2].m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Roboto Medium"), v[0].m_Format.m_Font.m_sName);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Roboto Medium"), v[1].m_Format.m_Font.m_sName);
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, v[1].m_Format.m_Font.m_bBold);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Roboto Medium"), v[2].m_Format.m_Font.m_sName);
	}
	// Br.
	{
		pInstance->SetXhtmlText("The quick brown fox<br>Jumped over<br/> the lazy dog");
		pInstance->CommitFormatting();
		SEOUL_UNITTESTING_ASSERT_EQUAL("The quick brown foxJumped over the lazy dog", pInstance->GetText());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(19, v[0].m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(11, v[1].m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(13, v[2].m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v[0].m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, v[1].m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, v[2].m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Roboto Medium"), v[0].m_Format.m_Font.m_sName);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Roboto Medium"), v[1].m_Format.m_Font.m_sName);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Roboto Medium"), v[2].m_Format.m_Font.m_sName);
	}
	// Font.
	{
		pInstance->SetXhtmlText("The quick brown fox<font letterSpacing=2 size=10 color=#FF05F7>Jumped over</font> the lazy dog");
		pInstance->CommitFormatting();
		SEOUL_UNITTESTING_ASSERT_EQUAL("The quick brown foxJumped over the lazy dog", pInstance->GetText());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(19, v[0].m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(11, v[1].m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(13, v[2].m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v[0].m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v[1].m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v[2].m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v[0].m_Format.GetUnscaledLetterSpacing());
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, v[1].m_Format.GetUnscaledLetterSpacing());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v[2].m_Format.GetUnscaledLetterSpacing());
		SEOUL_UNITTESTING_ASSERT_EQUAL(28, v[0].m_Format.GetUnscaledTextHeight());
		SEOUL_UNITTESTING_ASSERT_EQUAL(10, v[1].m_Format.GetUnscaledTextHeight());
		SEOUL_UNITTESTING_ASSERT_EQUAL(28, v[2].m_Format.GetUnscaledTextHeight());
		SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Create(51, 51, 51, 255), v[0].m_Format.m_TextColor);
		SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Create(255, 0x05, 0xF7, 255), v[1].m_Format.m_TextColor);
		SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Create(51, 51, 51, 255), v[2].m_Format.m_TextColor);
	}
	// Italic.
	{
		pInstance->SetXhtmlText("The quick brown fox<i>Jumped over</i> the lazy dog");
		pInstance->CommitFormatting();
		SEOUL_UNITTESTING_ASSERT_EQUAL("The quick brown foxJumped over the lazy dog", pInstance->GetText());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(19, v[0].m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(11, v[1].m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(13, v[2].m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v[0].m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v[1].m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v[2].m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Roboto Medium"), v[0].m_Format.m_Font.m_sName);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Roboto Medium"), v[1].m_Format.m_Font.m_sName);
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, v[1].m_Format.m_Font.m_bItalic);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Roboto Medium"), v[2].m_Format.m_Font.m_sName);
	}
	// Image.
	{
		{
			pInstance->SetXhtmlText("The quick brown fox<br>Jumped over<img hspace='7' vspace=3 src='content://Authored/Textures/TestTexture.png' width=93 height='97' hoffset=-5 voffset=\"7\"> the lazy dog");
			pInstance->CommitFormatting();
			SEOUL_UNITTESTING_ASSERT_EQUAL("The quick brown foxJumped over the lazy dog", pInstance->GetText());
			SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(19, v[0].m_uNumberOfCharacters);
			SEOUL_UNITTESTING_ASSERT_EQUAL(11, v[1].m_uNumberOfCharacters);
			SEOUL_UNITTESTING_ASSERT_EQUAL(13, v[2].m_uNumberOfCharacters);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, v[0].m_iLine);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, v[1].m_iLine);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, v[2].m_iLine);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Roboto Medium"), v[0].m_Format.m_Font.m_sName);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, vImg.GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(HtmlAlign::kLeft, vImg[0].m_eAlignment);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HtmlImageAlign::kMiddle, vImg[0].m_eImageAlignment);
			SEOUL_UNITTESTING_ASSERT_EQUAL(7, vImg[0].m_fXMargin);
			SEOUL_UNITTESTING_ASSERT_EQUAL(3, vImg[0].m_fYMargin);
			SEOUL_UNITTESTING_ASSERT_EQUAL(-1, vImg[0].m_iLinkIndex);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, vImg[0].m_iStartingTextLine);
			SEOUL_UNITTESTING_ASSERT(vImg[0].m_pBitmap.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(97, vImg[0].m_pBitmap->GetHeight());
			SEOUL_UNITTESTING_ASSERT_EQUAL(93, vImg[0].m_pBitmap->GetWidth());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(0, 0, 1, 1), vImg[0].m_vTextureCoordinates);
		}
	}
	// Link
	{
		pInstance->SetXhtmlText("The quick brown fox<a href='Foo' type=Bar>Jumped over</a> the lazy dog");
		pInstance->CommitFormatting();
		SEOUL_UNITTESTING_ASSERT_EQUAL("The quick brown foxJumped over the lazy dog", pInstance->GetText());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(19, v[0].m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(11, v[1].m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(13, v[2].m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(-1, v[0].m_Format.m_iLinkIndex);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v[1].m_Format.m_iLinkIndex);
		SEOUL_UNITTESTING_ASSERT_EQUAL(-1, v[2].m_Format.m_iLinkIndex);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v[0].m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v[1].m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v[2].m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, vLinks.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL("Foo", vLinks[0]->m_sLinkString);
		SEOUL_UNITTESTING_ASSERT_EQUAL("Bar", vLinks[0]->m_sType);
	}
	// p
	{
		pInstance->SetXhtmlText("The quick brown fox<p align=right>Jumped over</p> the lazy dog");
		pInstance->CommitFormatting();
		SEOUL_UNITTESTING_ASSERT_EQUAL("The quick brown foxJumped over the lazy dog", pInstance->GetText());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(19, v[0].m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(11, v[1].m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(13, v[2].m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(-1, v[0].m_Format.m_iLinkIndex);
		SEOUL_UNITTESTING_ASSERT_EQUAL(-1, v[1].m_Format.m_iLinkIndex);
		SEOUL_UNITTESTING_ASSERT_EQUAL(-1, v[2].m_Format.m_iLinkIndex);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v[0].m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v[1].m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, v[2].m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HtmlAlign::kLeft, v[0].m_Format.GetAlignmentEnum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HtmlAlign::kRight, v[1].m_Format.GetAlignmentEnum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HtmlAlign::kLeft, v[2].m_Format.GetAlignmentEnum());
	}
	// vertical_centered
	{
		pInstance->SetXhtmlText("The quick brown fox<vertical_centered/>Jumped over the lazy dog");
		pInstance->CommitFormatting();
		SEOUL_UNITTESTING_ASSERT_EQUAL("The quick brown foxJumped over the lazy dog", pInstance->GetText());
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(19, v[0].m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(24, v[1].m_uNumberOfCharacters);
		SEOUL_UNITTESTING_ASSERT_EQUAL(-1, v[0].m_Format.m_iLinkIndex);
		SEOUL_UNITTESTING_ASSERT_EQUAL(-1, v[1].m_Format.m_iLinkIndex);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v[0].m_iLine);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, v[1].m_iLine);

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
			5.25f,
			v[0].m_fYOffset,
			1e-3f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(
			5.25f,
			v[1].m_fYOffset,
			1e-3f);
	}
}

void FalconTest::TestProperties()
{
	SharedPtr<FalconTestFalconInstance> pInstance(SEOUL_NEW(MemoryBudgets::Developer) FalconTestFalconInstance());

	pInstance->SetAlpha(0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetAlpha());
	pInstance->SetAlpha(1.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetAlpha());

	pInstance->SetBlendingFactor(0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetBlendingFactor());
	pInstance->SetBlendingFactor(1.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetBlendingFactor());

	pInstance->SetClipDepth(0u);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, pInstance->GetClipDepth());
	pInstance->SetClipDepth(255u);
	SEOUL_UNITTESTING_ASSERT_EQUAL(255u, pInstance->GetClipDepth());

	pInstance->SetColorTransform(Falcon::ColorTransform());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Falcon::ColorTransform(), pInstance->GetColorTransform());
	pInstance->SetColorTransform(Falcon::ColorTransform::Identity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Falcon::ColorTransform::Identity(), pInstance->GetColorTransform());

	pInstance->SetColorTransformWithAlpha(Falcon::ColorTransformWithAlpha());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Falcon::ColorTransformWithAlpha(), pInstance->GetColorTransformWithAlpha());
	pInstance->SetColorTransformWithAlpha(Falcon::ColorTransformWithAlpha::Identity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Falcon::ColorTransformWithAlpha::Identity(), pInstance->GetColorTransformWithAlpha());

#if !SEOUL_SHIP
	pInstance->SetDebugName(String());
	SEOUL_UNITTESTING_ASSERT_EQUAL(String(), pInstance->GetDebugName());
	pInstance->SetDebugName("Test");
	SEOUL_UNITTESTING_ASSERT_EQUAL("Test", pInstance->GetDebugName());
#endif

	// TODO: These can/should be enabled if Depth3D is
	// made universal. It is currently a bit of a hack - the API
	// is available in Falcon::Instance but the implementation is
	// limited to certain node types.
	// pInstance->SetDepth3D(0.0f);
	// SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetDepth3D());
	// pInstance->SetDepth3D(1.0f);
	// SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetDepth3D());

	pInstance->SetIgnoreDepthProjection(false);
	SEOUL_UNITTESTING_ASSERT_EQUAL(false, pInstance->GetIgnoreDepthProjection());
	pInstance->SetIgnoreDepthProjection(true);
	SEOUL_UNITTESTING_ASSERT_EQUAL(true, pInstance->GetIgnoreDepthProjection());

	pInstance->SetName(HString());
	SEOUL_UNITTESTING_ASSERT_EQUAL(HString(), pInstance->GetName());
	pInstance->SetName(HString("TestName"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(HString("TestName"), pInstance->GetName());

	pInstance->SetPosition(0.0f, 0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D::Zero(), pInstance->GetPosition());
	pInstance->SetPosition(0.0f, 1.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(0, 1), pInstance->GetPosition());
	pInstance->SetPosition(1.0f, 0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(1, 0), pInstance->GetPosition());
	pInstance->SetPosition(Vector2D(0.0f, 0.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D::Zero(), pInstance->GetPosition());
	pInstance->SetPosition(Vector2D(0.0f, 1.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(0, 1), pInstance->GetPosition());
	pInstance->SetPosition(Vector2D(1.0f, 0.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(1, 0), pInstance->GetPosition());

	pInstance->SetPositionX(0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetPositionX());
	pInstance->SetPositionX(1.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetPositionX());

	pInstance->SetPositionY(0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetPositionY());
	pInstance->SetPositionY(1.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetPositionY());

	pInstance->SetRotationInDegrees(0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetRotationInDegrees());
	pInstance->SetRotationInDegrees(45.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(45.0f, pInstance->GetRotationInDegrees());

	pInstance->SetRotationInRadians(0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetRotationInRadians());
	pInstance->SetRotationInRadians(fPiOverTwo);
	SEOUL_UNITTESTING_ASSERT_EQUAL(fPiOverTwo, pInstance->GetRotationInRadians());

	pInstance->SetScale(0.0f, 0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D::Zero(), pInstance->GetScale());
	pInstance->SetScale(0.0f, 1.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(0, 1), pInstance->GetScale());
	pInstance->SetScale(1.0f, 0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(1, 0), pInstance->GetScale());

	pInstance->SetScaleX(0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetScaleX());
	pInstance->SetScaleX(1.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetScaleX());
	pInstance->SetScaleX(-1.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0f, pInstance->GetScaleX());

	pInstance->SetScaleY(0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetScaleY());
	pInstance->SetScaleY(1.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, pInstance->GetScaleY());
	pInstance->SetScaleY(-1.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0f, pInstance->GetScaleY());

	pInstance->SetScissorClip(false);
	SEOUL_UNITTESTING_ASSERT_EQUAL(false, pInstance->GetScissorClip());
	pInstance->SetScissorClip(true);
	SEOUL_UNITTESTING_ASSERT_EQUAL(true, pInstance->GetScissorClip());

	pInstance->SetTransform(Matrix2x3());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Matrix2x3(), pInstance->GetTransform());
	pInstance->SetTransform(Matrix2x3::Identity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Matrix2x3::Identity(), pInstance->GetTransform());

	pInstance->SetVisible(false);
	SEOUL_UNITTESTING_ASSERT_EQUAL(false, pInstance->GetVisible());
	pInstance->SetVisible(true);
	SEOUL_UNITTESTING_ASSERT_EQUAL(true, pInstance->GetVisible());

	pInstance->SetWorldPosition(0.0f, 0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D::Zero(), pInstance->ComputeWorldPosition());
	pInstance->SetWorldPosition(1.0f, 0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(1, 0), pInstance->ComputeWorldPosition());
	pInstance->SetWorldPosition(0.0f, 1.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(0, 1), pInstance->ComputeWorldPosition());

	pInstance->SetWorldTransform(Matrix2x3());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Matrix2x3(), pInstance->ComputeWorldTransform());
	pInstance->SetWorldTransform(Matrix2x3::Identity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(Matrix2x3::Identity(), pInstance->ComputeWorldTransform());
}

static Bool SkipRectangleInSwf(SyncFile& r)
{
	UInt8 uNextByte = 0;
	if (!ReadUInt8(r, uNextByte))
	{
		SEOUL_LOG("%s: dependency scan, failed reading first byte to skip reactangle of cooked SWF.", r.GetAbsoluteFilename().CStr());
		return false;
	}

	// The rectangle record is a little complex - the first 5 bits
	// are the # of bits used for each of the next 4 components of
	// the rectangle record, and the result is round up to be byte aligned.
	Int32 iShift = 7;
	Int32 iBit = 4;
	Int32 iBits = 0;
	while (iBit >= 0)
	{
		Bool bBit = ((((UInt32)uNextByte) >> iShift) & 1) == 1;
		if (bBit)
		{
			iBits |= (1 << iBit);
		}

		--iBit;
		--iShift;
	}

	// The size of the rectangle is 5 bits + 4 elements, which are
	// each the size of the # value in the first 5 bits. We've
	// read 1 byte, so the remaining is that total minus 8.
	Int32 iRemainingBits = (5 + (4 * iBits)) - 8;

	// Now skip the remaining bytes.
	while (iRemainingBits > 0)
	{
		UInt8 uUnused = 0;
		if (!ReadUInt8(r, uUnused))
		{
			SEOUL_LOG("%s: dependency scan, failed reading additional bytes to skip reactangle of cooked SWF.", r.GetAbsoluteFilename().CStr());
			return false;
		}
		iRemainingBits -= 8;
	}

	return true;
}

static Bool TestGetFCNFileDependencies(
	FilePath filePath,
	void const* p,
	UInt32 u,
	Falcon::FCNFile::FCNDependencies& rv)
{
	FullyBufferedSyncFile file((void*)p, u, false);

	// Header - starts with version and signature.
	UInt32 uVersion = 0u;
	if (!ReadUInt32(file, uVersion))
	{
		SEOUL_LOG("%s: dependency scan, failed reading UI Movie data version.", filePath.CStr());
		return false;
	}
	if (uVersion != Falcon::kFCNVersion)
	{
		SEOUL_LOG("%s: dependency scan, invalid UI movie version '%u', expected '%u'.", filePath.CStr(), uVersion, Falcon::kFCNVersion);
		return false;
	}

	UInt32 uSignature = 0u;
	if (!ReadUInt32(file, uSignature))
	{
		SEOUL_LOG("%s: dependency scan, failed reading UI Movie data signature.", filePath.CStr());
		return false;
	}
	if (uSignature != Falcon::kFCNSignature)
	{
		SEOUL_LOG("%s: dependency scan, invalid UI movie signature '%u', expected '%u'.", filePath.CStr(), uSignature, Falcon::kFCNSignature);
		return false;
	}

	// Skip the rectangle record
	if (!SkipRectangleInSwf(file))
	{
		return false;
	}

	// Next three records are the frame rate and frame count - after that
	// are tag entries.
	UInt8 uFrameRateMinor = 0u;
	if (!ReadUInt8(file, uFrameRateMinor))
	{
		SEOUL_LOG("%s: dependency scan, failed reading UI Movie data frame rate minor part.", filePath.CStr());
		return false;
	}

	UInt8 uFrameRateMajor = 0u;
	if (!ReadUInt8(file, uFrameRateMajor))
	{
		SEOUL_LOG("%s: dependency scan, failed reading UI Movie data frame rate major part.", filePath.CStr());
		return false;
	}

	UInt16 uFrameCount = 0u;
	if (!ReadUInt16(file, uFrameCount))
	{
		SEOUL_LOG("%s: dependency scan, failed reading UI Movie data frame count part.", filePath.CStr());
		return false;
	}
	// /Header

	// Library dependencies and texture dependencies are in UTF8.
	Vector<UInt8, MemoryBudgets::Cooking> v;

	// Tags
	auto const sBase(Path::GetDirectoryName(filePath.GetAbsoluteFilenameInSource()));
	Int32 iTagId = 0;
	do
	{
		// Tag header is an id and a size in bytes
		UInt16 uRawTagData = 0;
		if (!ReadUInt16(file, uRawTagData))
		{
			SEOUL_LOG("%s: dependency scan, failed reading UI Movie data tag length.", filePath.CStr());
			return false;
		}

		Int32 iTagData = (Int32)uRawTagData;
		iTagId = (Int32)(iTagData >> 6);
		Int32 zTagLengthInBytes = (Int32)(iTagData & 0x3F);

		// If the size is 0x3F, then there's an additional 32-bit
		// entry describing a "long" tag.
		if (zTagLengthInBytes == 0x3F)
		{
			UInt32 uRawTagLength = 0u;
			if (!ReadUInt32(file, uRawTagLength))
			{
				SEOUL_LOG("%s: dependency scan, failed reading UI Movie data tag extended length.", filePath.CStr());
				return false;
			}
			zTagLengthInBytes = (Int32)uRawTagLength;
		}

		Int64 iNextOffsetInBytes = 0;
		if (!file.GetCurrentPositionIndicator(iNextOffsetInBytes))
		{
			SEOUL_LOG("%s: dependency scan, failed getting position indicator for tag skip.", filePath.CStr());
			return false;
		}
		iNextOffsetInBytes += (Int64)zTagLengthInBytes;

		// Tag Id 57 = ImportAssets
		// Tag Id 71 = ImportAssets2
		if (iTagId == 57 || iTagId == 71)
		{
			// The next chunk of data is a UTF8 encoded string
			v.Clear();

			UInt8 c = 0;
			if (!ReadUInt8(file, c))
			{
				SEOUL_LOG("%s: dependency scan, failed reading UI Movie dependency string first byte.", filePath.CStr());
				return false;
			}
			zTagLengthInBytes--;

			// Append bytes that are part of the string, excluding the null terminator.
			while (c != 0)
			{
				v.PushBack(c);

				if (!ReadUInt8(file, c))
				{
					SEOUL_LOG("%s: dependency scan, failed reading UI Movie dependency string additional byte.", filePath.CStr());
					return false;
				}
				zTagLengthInBytes--;
			}

			// Process the library as a dependency.
			if (!v.IsEmpty())
			{
				auto const sFilename(Path::Combine(sBase, String((Byte const*)v.Data(), v.GetSize())));
				rv.PushBack(FilePath::CreateContentFilePath(sFilename));
			}
		}
		// Tag Id 92 = DefineExternalBitmap
		else if (iTagId == 92)
		{
			// Skip the UInt16 definition ID.
			UInt16 uUnusedDefinitionID = 0;
			if (!ReadUInt16(file, uUnusedDefinitionID))
			{
				SEOUL_LOG("%s: dependency scan, failed reading UI Movie dependency bitmap definition ID.", filePath.CStr());
				return false;
			}
			zTagLengthInBytes -= 2;

			// Now read the sized string - length of the string is ahead of the string as a single byte.
			UInt8 uRawStringLength = 0;
			if (!ReadUInt8(file, uRawStringLength))
			{
				SEOUL_LOG("%s: dependency scan, failed reading UI Movie dependency bitmap string first byte.", filePath.CStr());
				return false;
			}
			Int32 iLength = (Int32)uRawStringLength;
			zTagLengthInBytes--;

			// Length includes the null terminator.
			v.Clear();
			UInt8 c = 0;
			for (int i = 0; i < iLength - 1; ++i)
			{
				if (!ReadUInt8(file, c))
				{
					SEOUL_LOG("%s: dependency scan, failed reading UI Movie dependency bitmap string additional byte.", filePath.CStr());
					return false;
				}
				v.PushBack(c);
				zTagLengthInBytes--;
			}

			// Process the texture as a dependency.
			if (!v.IsEmpty())
			{
				rv.PushBack(FilePath::CreateContentFilePath(String((Byte const*)v.Data(), v.GetSize())));
			}
		}

		// Skip any remaining bytes.
		if (!file.Seek(iNextOffsetInBytes, File::kSeekFromStart))
		{
			SEOUL_LOG("%s: dependency scan, failed skipping UI Movie tag.", filePath.CStr());
			return false;
		}
	} while (0 != iTagId);

	return true;
}

void FalconTest::TestGetFcnDependencies()
{
	using namespace Falcon;

	UnitTestsFileManagerHelper helper;

	auto const filePath = FilePath::CreateContentFilePath("Authored/UnitTests/Falcon/Test.swf");
	void* p = nullptr;
	UInt32 u = 0u;
	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(
		filePath,
		p,
		u,
		0u,
		MemoryBudgets::Developer));
	auto const deferred(MakeDeferredAction([&]() { MemoryManager::Deallocate(p); }));
	void* pU = nullptr;
	UInt32 uU = 0u;
	SEOUL_UNITTESTING_ASSERT(ZSTDDecompress(p, u, pU, uU));
	auto const deferred2(MakeDeferredAction([&]() { MemoryManager::Deallocate(pU); }));

	FCNFile::FCNDependencies vExpected;
	SEOUL_UNITTESTING_ASSERT(TestGetFCNFileDependencies(filePath, pU, uU, vExpected));

	FCNFile::FCNDependencies v;
	SEOUL_UNITTESTING_ASSERT(FCNFile::GetFCNFileDependencies(filePath, pU, uU, v));

	SEOUL_UNITTESTING_ASSERT(vExpected == v);
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
