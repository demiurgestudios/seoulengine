/**
 * \file Animation2DTest.cpp
 * \brief Unit test header file for the Animation2D project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnimationBlendDefinition.h"
#include "AnimationBlendInstance.h"
#include "AnimationEventInterface.h"
#include "AnimationNetworkDefinition.h"
#include "AnimationNetworkDefinitionManager.h"
#include "AnimationNodeDefinition.h"
#include "AnimationNodeInstance.h"
#include "AnimationNodeType.h"
#include "AnimationPlayClipDefinition.h"
#include "AnimationStateMachineDefinition.h"
#include "AnimationStateMachineInstance.h"
#if SEOUL_WITH_ANIMATION_2D
#include "Animation2DClipInstance.h"
#include "Animation2DDataInstance.h"
#include "Animation2DManager.h"
#include "Animation2DNetworkInstance.h"
#include "Animation2DPlayClipInstance.h"
#include "Animation2DTest.h"
#endif
#include "ContentLoadManager.h"
#include "Engine.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "Logger.h"
#include "Matrix2x3.h"
#include "PackageFileSystem.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "UnitTestsEngineHelper.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

#if SEOUL_WITH_ANIMATION_2D

SEOUL_BEGIN_TYPE(Animation2DTestExpectedValues)
	SEOUL_PROPERTY_N("DrawOrder", m_vDrawOrder)
	SEOUL_PROPERTY_N("Skinning", m_vSkinning)
	SEOUL_PROPERTY_N("Slots", m_vSlots)
	SEOUL_PROPERTY_N("Vertices", m_vvVertices)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2DTest, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(UnitTest, Attributes::UnitTest::kInstantiateForEach)
	SEOUL_METHOD(TestData)
	SEOUL_METHOD(TestDrawOrder)
	SEOUL_METHOD(TestDrawOrderAttackFrame0)
	SEOUL_METHOD(TestDrawOrderAttackFramePoint25)
	SEOUL_METHOD(TestDrawOrderAttackFramePoint5)
	SEOUL_METHOD(TestDrawOrderIdleFrame0)
	SEOUL_METHOD(TestDrawOrderIdleFramePoint25)
	SEOUL_METHOD(TestDrawOrderIdleFramePoint5)
	SEOUL_METHOD(TestFrame0Loop)
	SEOUL_METHOD(TestEvents)
	SEOUL_METHOD(TestGetTimeToEvent)
	SEOUL_METHOD(TestFramePoint25SecondsLoop)
	SEOUL_METHOD(TestFramePoint5SecondsLoop)
	SEOUL_METHOD(TestFrame1SecondLoop)
	SEOUL_METHOD(TestFrame1Point5SecondsLoop)
	SEOUL_METHOD(Test3Frame0Loop)
	SEOUL_METHOD(Test3FramePoint25SecondsLoop)
	SEOUL_METHOD(Test3FramePoint5SecondsLoop)
	SEOUL_METHOD(Test3Frame1SecondLoop)
	SEOUL_METHOD(Test3Frame1Point5SecondsLoop)
	SEOUL_METHOD(Test3Frame0NoLoop)
	SEOUL_METHOD(Test3FramePoint25SecondsNoLoop)
	SEOUL_METHOD(Test3FramePoint5SecondsNoLoop)
	SEOUL_METHOD(Test3Frame1SecondNoLoop)
	SEOUL_METHOD(Test3Frame1Point5SecondsNoLoop)
	SEOUL_METHOD(TestFrame0NoLoop)
	SEOUL_METHOD(TestFramePoint25SecondsNoLoop)
	SEOUL_METHOD(TestFramePoint5SecondsNoLoop)
	SEOUL_METHOD(TestFrame1SecondNoLoop)
	SEOUL_METHOD(TestFrame1Point5SecondsNoLoop)
	SEOUL_METHOD(TestFrame0Path)
	SEOUL_METHOD(TestFramePoint25SecondsPath)
	SEOUL_METHOD(TestFramePoint5SecondsPath)
	SEOUL_METHOD(TestFrame1SecondPath)
	SEOUL_METHOD(TestFrame1Point5SecondsPath)
	SEOUL_METHOD(TestFrame0Path2)
	SEOUL_METHOD(TestFramePoint25SecondsPath2)
	SEOUL_METHOD(TestFramePoint5SecondsPath2)
	SEOUL_METHOD(TestFrame1SecondPath2)
	SEOUL_METHOD(TestFrame1Point5SecondsPath2)
	SEOUL_METHOD(TestFrame0TransformConstraint)
	SEOUL_METHOD(TestFramePoint25SecondsTransformConstraint)
	SEOUL_METHOD(TestFramePoint5SecondsTransformConstraint)
	SEOUL_METHOD(TestFrame1SecondTransformConstraint)
	SEOUL_METHOD(TestFrame1Point5SecondsTransformConstraint)
	SEOUL_METHOD(TestHeadTurn)
	SEOUL_METHOD(TestHeadTurnFrame0)
	SEOUL_METHOD(TestNetwork)
	SEOUL_METHOD(TestNetworkEval)
	SEOUL_METHOD(TestRotation)
	SEOUL_METHOD(TestSynchronizeTime)
	SEOUL_METHOD(TestTPose)

	SEOUL_METHOD(TestTCRegressionFrame0)
	SEOUL_METHOD(TestTCRegressionFramePoint25)
	SEOUL_METHOD(TestTCRegressionFramePoint5)
	SEOUL_METHOD(TestTCRegressionFrame1)
	SEOUL_METHOD(TestTCRegressionFrame1Point5)

	SEOUL_METHOD(TestTCHibanaFrame0)
	SEOUL_METHOD(TestTCHibanaFramePoint25)
	SEOUL_METHOD(TestTCHibanaFramePoint5)
	SEOUL_METHOD(TestTCHibanaFrame1)
	SEOUL_METHOD(TestTCHibanaFrame1Point5)

	SEOUL_METHOD(TestChuihFrame0)
	SEOUL_METHOD(TestChuihFramePoint25)
	SEOUL_METHOD(TestChuihFramePoint5)
	SEOUL_METHOD(TestChuihFrame1)
	SEOUL_METHOD(TestChuihFrame1Point5)

	SEOUL_METHOD(TestComprehensive)
SEOUL_END_TYPE()

static void CheckBoneStates(
	const SharedPtr<Animation2D::NetworkInstance>& p,
	HString a,
	Float fTimeA,
	HString b,
	Float fTimeB,
	Float fBlend)
{
	using namespace Animation;
	using namespace Animation2D;

	auto const& pData = p->GetData();

	auto const& pClipA = pData->GetClip(a);
	auto const& pClipB = pData->GetClip(b);

	DataInstance stateA(pData, SharedPtr<EventInterface>());
	DataInstance stateB(pData, SharedPtr<EventInterface>());

	ClipInstance clipInstanceA(
		stateA,
		pClipA,
		ClipSettings());
	ClipInstance clipInstanceB(
		stateB,
		pClipB,
		ClipSettings());

	// Advance each clip to the desired time.
	clipInstanceA.EvaluateRange(0.0f, fTimeA, 1.0f);
	clipInstanceA.Evaluate(fTimeA, 1.0f, true);
	clipInstanceB.EvaluateRange(0.0f, fTimeB, 1.0f);
	clipInstanceB.Evaluate(fTimeB, 1.0f, true);

	// Tick each state.
	stateA.ApplyCache();
	stateA.PoseSkinningPalette();
	stateB.ApplyCache();
	stateB.PoseSkinningPalette();

	// Now check that the result is as expected.
	auto const& state = p->GetState();

	auto const& vBonesT = state.GetBones();
	auto const& vBonesA = stateA.GetBones();
	auto const& vBonesB = stateB.GetBones();

	UInt32 const uBones = vBonesT.GetSize();
	for (UInt32 i = 0u; i < uBones; ++i)
	{
		auto const t = vBonesT[i];
		auto const a = vBonesA[i];
		auto const b = vBonesB[i];

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(t.m_fPositionX, Lerp(a.m_fPositionX, b.m_fPositionX, fBlend), 1e-3f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(t.m_fPositionY, Lerp(a.m_fPositionY, b.m_fPositionY, fBlend), 1e-3f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(t.m_fRotationInDegrees, LerpDegrees(a.m_fRotationInDegrees, b.m_fRotationInDegrees, fBlend), 1e-3f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(t.m_fScaleX, Lerp(a.m_fScaleX, b.m_fScaleX, fBlend), 1e-3f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(t.m_fScaleY, Lerp(a.m_fScaleY, b.m_fScaleY, fBlend), 1e-3f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(t.m_fShearX, Lerp(a.m_fShearX, b.m_fShearX, fBlend), 1e-3f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(t.m_fShearY, Lerp(a.m_fShearY, b.m_fShearY, fBlend), 1e-3f);
	}
}

Animation2DTest::Animation2DTest()
{
	m_pHelper.Reset(SEOUL_NEW(MemoryBudgets::Developer) UnitTestsEngineHelper);
	m_pNetworkDefinitionManager.Reset(SEOUL_NEW(MemoryBudgets::Developer) Animation::NetworkDefinitionManager);
	m_pManager.Reset(SEOUL_NEW(MemoryBudgets::Developer) Animation2D::Manager);
}

Animation2DTest::~Animation2DTest()
{
	m_pManager.Reset();
	m_pNetworkDefinitionManager.Reset();
	m_pHelper.Reset();
}

void Animation2DTest::TestData()
{
	auto p(Animation2D::Manager::Get()->CreateInstance(
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestNetworkNoLoop.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test1/TestAnimation2D.son"),
		SharedPtr<Animation::EventInterface>()));
	WaitForReady(p);

	auto const& pData = p->GetData();

	using namespace Animation2D;

	// Metadata
	{
		auto const& metadata = pData->GetMetaData();
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1112.91f, metadata.m_fHeight, 1e-3f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1333.78f, metadata.m_fWidth, 1e-3f);
	}

	// Bones
	{
		auto const& bones = pData->GetBones();
		SEOUL_UNITTESTING_ASSERT_EQUAL(13, bones.GetSize());

		{
			auto const& bone = bones[0];
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, bone.m_bSkinRequired);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Animation2D::TransformMode::kNormal, bone.m_eTransformMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fLength);
			SEOUL_UNITTESTING_ASSERT_EQUAL(28.68f, bone.m_fPositionX);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-105.71f, bone.m_fPositionY, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fRotationInDegrees);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("root"), bone.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(-1, bone.m_iParent);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString(), bone.m_ParentId);
		}
		{
			auto const& bone = bones[1];
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, bone.m_bSkinRequired);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Animation2D::TransformMode::kNormal, bone.m_eTransformMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fLength);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-267.71f, bone.m_fPositionX, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-366.62f, bone.m_fPositionY, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fRotationInDegrees);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("attachment"), bone.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, bone.m_iParent);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("root"), bone.m_ParentId);
		}
		{
			auto const& bone = bones[2];
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, bone.m_bSkinRequired);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Animation2D::TransformMode::kNormal, bone.m_eTransformMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fLength);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-298.29999f, bone.m_fPositionX, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(44.1f, bone.m_fPositionY, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fRotationInDegrees);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("color"), bone.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, bone.m_iParent);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("root"), bone.m_ParentId);
		}
		{
			auto const& bone = bones[3];
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, bone.m_bSkinRequired);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Animation2D::TransformMode::kNormal, bone.m_eTransformMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fLength);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(283.53f, bone.m_fPositionX, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-356.27f, bone.m_fPositionY, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fRotationInDegrees);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("draworder"), bone.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, bone.m_iParent);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("root"), bone.m_ParentId);
		}
		{
			auto const& bone = bones[4];
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, bone.m_bSkinRequired);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Animation2D::TransformMode::kNormal, bone.m_eTransformMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fLength);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-320.74f, bone.m_fPositionX, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(413.7f, bone.m_fPositionY, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fRotationInDegrees);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("mesh"), bone.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, bone.m_iParent);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("root"), bone.m_ParentId);
		}
		{
			auto const& bone = bones[5];
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, bone.m_bSkinRequired);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Animation2D::TransformMode::kNormal, bone.m_eTransformMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fLength);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-353.28f, bone.m_fPositionX, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(627.53f, bone.m_fPositionY, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-11.22f, bone.m_fRotationInDegrees, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("meshweighted"), bone.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, bone.m_iParent);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("root"), bone.m_ParentId);
		}
		{
			auto const& bone = bones[6];
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, bone.m_bSkinRequired);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Animation2D::TransformMode::kNormal, bone.m_eTransformMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fLength);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(177.52f, bone.m_fPositionX, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(56.62f, bone.m_fPositionY, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fRotationInDegrees);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("pathfollower"), bone.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, bone.m_iParent);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("root"), bone.m_ParentId);
		}
		{
			auto const& bone = bones[7];
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, bone.m_bSkinRequired);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Animation2D::TransformMode::kNormal, bone.m_eTransformMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fLength);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-305.0f, bone.m_fPositionX, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(201.0f, bone.m_fPositionY, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fRotationInDegrees);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("rotate"), bone.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, bone.m_iParent);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("root"), bone.m_ParentId);
		}
		{
			auto const& bone = bones[8];
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, bone.m_bSkinRequired);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Animation2D::TransformMode::kNormal, bone.m_eTransformMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fLength);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(183.99f, bone.m_fPositionX, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_EQUAL(-138.0f, bone.m_fPositionY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fRotationInDegrees);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("scale"), bone.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, bone.m_iParent);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("root"), bone.m_ParentId);
		}
		{
			auto const& bone = bones[9];
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, bone.m_bSkinRequired);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Animation2D::TransformMode::kNormal, bone.m_eTransformMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fLength);
			SEOUL_UNITTESTING_ASSERT_EQUAL(-304.0f, bone.m_fPositionX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(-146.0f, bone.m_fPositionY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fRotationInDegrees);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("shear"), bone.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, bone.m_iParent);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("root"), bone.m_ParentId);
		}
		{
			auto const& bone = bones[10];
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, bone.m_bSkinRequired);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Animation2D::TransformMode::kNormal, bone.m_eTransformMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fLength);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(169.6f, bone.m_fPositionX, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(420.7f, bone.m_fPositionY, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fRotationInDegrees);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("transformconstrained"), bone.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, bone.m_iParent);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("root"), bone.m_ParentId);
		}
		{
			auto const& bone = bones[11];
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, bone.m_bSkinRequired);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Animation2D::TransformMode::kNormal, bone.m_eTransformMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fLength);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(604.27f, bone.m_fPositionX, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(420.7f, bone.m_fPositionY, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fRotationInDegrees);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("transformconstrainttarget"), bone.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, bone.m_iParent);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("root"), bone.m_ParentId);
		}
		{
			auto const& bone = bones[12];
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, bone.m_bSkinRequired);
			SEOUL_UNITTESTING_ASSERT_EQUAL(Animation2D::TransformMode::kNormal, bone.m_eTransformMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fLength);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(190.99f, bone.m_fPositionX, 1e-3f);
			SEOUL_UNITTESTING_ASSERT_EQUAL(201.0f, bone.m_fPositionY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fRotationInDegrees);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bone.m_fScaleY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bone.m_fShearY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("translate"), bone.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, bone.m_iParent);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("root"), bone.m_ParentId);
		}
	}

	// Slots
	{
		static const HString kLogo("images/logo");
		static const HString kSpine("images/spine");

		auto const& slots = pData->GetSlots();
		SEOUL_UNITTESTING_ASSERT_EQUAL(14, slots.GetSize());

		{
			auto const& slot = slots[0];
			SEOUL_UNITTESTING_ASSERT_EQUAL(kSpine, slot.m_AttachmentId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("translate"), slot.m_BoneId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), slot.m_Color);
			SEOUL_UNITTESTING_ASSERT_EQUAL(SlotBlendMode::kAlpha, slot.m_eBlendMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(12, slot.m_iBone);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("images/spine"), slot.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Black(), slot.m_SecondaryColor);
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, slot.m_bHasSecondaryColor);
		}
		{
			auto const& slot = slots[1];
			SEOUL_UNITTESTING_ASSERT_EQUAL(kSpine, slot.m_AttachmentId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("rotate"), slot.m_BoneId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), slot.m_Color);
			SEOUL_UNITTESTING_ASSERT_EQUAL(SlotBlendMode::kAlpha, slot.m_eBlendMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(7, slot.m_iBone);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("images/spine2"), slot.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Black(), slot.m_SecondaryColor);
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, slot.m_bHasSecondaryColor);
		}
		{
			auto const& slot = slots[2];
			SEOUL_UNITTESTING_ASSERT_EQUAL(kSpine, slot.m_AttachmentId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("scale"), slot.m_BoneId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), slot.m_Color);
			SEOUL_UNITTESTING_ASSERT_EQUAL(SlotBlendMode::kAlpha, slot.m_eBlendMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(8, slot.m_iBone);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("images/spine3"), slot.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Black(), slot.m_SecondaryColor);
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, slot.m_bHasSecondaryColor);
		}
		{
			auto const& slot = slots[3];
			SEOUL_UNITTESTING_ASSERT_EQUAL(kSpine, slot.m_AttachmentId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("shear"), slot.m_BoneId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), slot.m_Color);
			SEOUL_UNITTESTING_ASSERT_EQUAL(SlotBlendMode::kAlpha, slot.m_eBlendMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(9, slot.m_iBone);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("images/spine4"), slot.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Black(), slot.m_SecondaryColor);
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, slot.m_bHasSecondaryColor);
		}
		{
			auto const& slot = slots[4];
			SEOUL_UNITTESTING_ASSERT_EQUAL(kSpine, slot.m_AttachmentId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("color"), slot.m_BoneId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), slot.m_Color);
			SEOUL_UNITTESTING_ASSERT_EQUAL(SlotBlendMode::kAlpha, slot.m_eBlendMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(2, slot.m_iBone);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("images/spine5"), slot.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Black(), slot.m_SecondaryColor);
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, slot.m_bHasSecondaryColor);
		}
		{
			auto const& slot = slots[5];
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("path"), slot.m_AttachmentId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("root"), slot.m_BoneId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), slot.m_Color);
			SEOUL_UNITTESTING_ASSERT_EQUAL(SlotBlendMode::kAlpha, slot.m_eBlendMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, slot.m_iBone);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("path2"), slot.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Black(), slot.m_SecondaryColor);
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, slot.m_bHasSecondaryColor);
		}
		{
			auto const& slot = slots[6];
			SEOUL_UNITTESTING_ASSERT_EQUAL(kSpine, slot.m_AttachmentId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("pathfollower"), slot.m_BoneId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), slot.m_Color);
			SEOUL_UNITTESTING_ASSERT_EQUAL(SlotBlendMode::kAlpha, slot.m_eBlendMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(6, slot.m_iBone);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("images/spine6"), slot.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Black(), slot.m_SecondaryColor);
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, slot.m_bHasSecondaryColor);
		}
		{
			auto const& slot = slots[7];
			SEOUL_UNITTESTING_ASSERT_EQUAL(kSpine, slot.m_AttachmentId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("transformconstrained"), slot.m_BoneId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), slot.m_Color);
			SEOUL_UNITTESTING_ASSERT_EQUAL(SlotBlendMode::kAlpha, slot.m_eBlendMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(10, slot.m_iBone);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("images/spine7"), slot.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Black(), slot.m_SecondaryColor);
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, slot.m_bHasSecondaryColor);
		}
		{
			auto const& slot = slots[8];
			SEOUL_UNITTESTING_ASSERT_EQUAL(kSpine, slot.m_AttachmentId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("transformconstrainttarget"), slot.m_BoneId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), slot.m_Color);
			SEOUL_UNITTESTING_ASSERT_EQUAL(SlotBlendMode::kAlpha, slot.m_eBlendMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(11, slot.m_iBone);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("images/spine8"), slot.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Black(), slot.m_SecondaryColor);
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, slot.m_bHasSecondaryColor);
		}
		{
			auto const& slot = slots[9];
			SEOUL_UNITTESTING_ASSERT_EQUAL(kSpine, slot.m_AttachmentId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("mesh"), slot.m_BoneId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), slot.m_Color);
			SEOUL_UNITTESTING_ASSERT_EQUAL(SlotBlendMode::kAlpha, slot.m_eBlendMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(4, slot.m_iBone);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("images/spine9"), slot.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Black(), slot.m_SecondaryColor);
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, slot.m_bHasSecondaryColor);
		}
		{
			auto const& slot = slots[10];
			SEOUL_UNITTESTING_ASSERT_EQUAL(kLogo, slot.m_AttachmentId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("attachment"), slot.m_BoneId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), slot.m_Color);
			SEOUL_UNITTESTING_ASSERT_EQUAL(SlotBlendMode::kAlpha, slot.m_eBlendMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, slot.m_iBone);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("images/spine10"), slot.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Black(), slot.m_SecondaryColor);
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, slot.m_bHasSecondaryColor);
		}
		{
			auto const& slot = slots[11];
			SEOUL_UNITTESTING_ASSERT_EQUAL(kLogo, slot.m_AttachmentId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("draworder"), slot.m_BoneId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), slot.m_Color);
			SEOUL_UNITTESTING_ASSERT_EQUAL(SlotBlendMode::kAlpha, slot.m_eBlendMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(3, slot.m_iBone);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("draworder2"), slot.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Black(), slot.m_SecondaryColor);
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, slot.m_bHasSecondaryColor);
		}
		{
			auto const& slot = slots[12];
			SEOUL_UNITTESTING_ASSERT_EQUAL(kSpine, slot.m_AttachmentId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("draworder"), slot.m_BoneId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), slot.m_Color);
			SEOUL_UNITTESTING_ASSERT_EQUAL(SlotBlendMode::kAlpha, slot.m_eBlendMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(3, slot.m_iBone);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("draworder1"), slot.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Black(), slot.m_SecondaryColor);
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, slot.m_bHasSecondaryColor);
		}
		{
			auto const& slot = slots[13];
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("linkedmesh"), slot.m_AttachmentId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("meshweighted"), slot.m_BoneId);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), slot.m_Color);
			SEOUL_UNITTESTING_ASSERT_EQUAL(SlotBlendMode::kAlpha, slot.m_eBlendMode);
			SEOUL_UNITTESTING_ASSERT_EQUAL(5, slot.m_iBone);
			SEOUL_UNITTESTING_ASSERT_EQUAL(HString("images/spine12"), slot.m_Id);
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Black(), slot.m_SecondaryColor);
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, slot.m_bHasSecondaryColor);
		}
	}

	// Transforms
	{
		auto const& transforms = pData->GetTransforms();
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, transforms.GetSize());
		auto const& t = transforms.Front();

		SEOUL_UNITTESTING_ASSERT_EQUAL(-400.0f, t.m_fDeltaPositionX);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, t.m_fDeltaPositionY);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, t.m_fDeltaRotationInDegrees);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, t.m_fDeltaScaleX);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, t.m_fDeltaScaleY);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, t.m_fDeltaShearY);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.509f, t.m_fPositionMix, 1e-3f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.509f, t.m_fRotationMix, 1e-3f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.509f, t.m_fScaleMix, 1e-3f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.509f, t.m_fShearMix, 1e-3f);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("transformconstraint"), t.m_Id);
		SEOUL_UNITTESTING_ASSERT_EQUAL(11, t.m_iTarget);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("transformconstrainttarget"), t.m_TargetId);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, t.m_vBoneIds.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("transformconstrained"), t.m_vBoneIds[0]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, t.m_viBones.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(10, t.m_viBones[0]);
	}

	// Paths
	{
		auto const& paths = pData->GetPaths();
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, paths.GetSize());
		auto const& path = paths.Front();

		SEOUL_UNITTESTING_ASSERT_EQUAL(PathPositionMode::kPercent, path.m_ePositionMode);
		SEOUL_UNITTESTING_ASSERT_EQUAL(PathRotationMode::kTangent, path.m_eRotationMode);
		SEOUL_UNITTESTING_ASSERT_EQUAL(PathSpacingMode::kLength, path.m_eSpacingMode);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, path.m_fPosition);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, path.m_fPositionMix);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(34.2f, path.m_fRotationInDegrees, 1e-3f);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, path.m_fRotationMix);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, path.m_fSpacing);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("pathconstraint"), path.m_Id);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, path.m_iTarget);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("path2"), path.m_TargetId);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, path.m_vBoneIds.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("pathfollower"), path.m_vBoneIds[0]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, path.m_viBones.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, path.m_viBones[0]);
	}

	// Skins
	{
		FilePath const logoPath(FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test1/images/logo.png"));
		FilePath const spinePath(FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test1/images/spine.png"));

		auto const& skins = pData->GetSkins();
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, skins.GetSize());

		auto const& skin = *skins.Find(HString("default"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(14u, skin.GetSize());

		{
			auto const& slot = *skin.Find(HString("draworder1"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, slot.GetSize());
			auto const& attach = **slot.Find(HString("images/spine"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(AttachmentType::kBitmap, attach.GetType());
			auto const& bitmap = (BitmapAttachment const&)attach;
			SEOUL_UNITTESTING_ASSERT_EQUAL(spinePath, bitmap.GetFilePath());
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), bitmap.GetColor());
			SEOUL_UNITTESTING_ASSERT_EQUAL(120, bitmap.GetHeight());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionX());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionY());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetRotationInDegrees());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleX());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleY());
			SEOUL_UNITTESTING_ASSERT_EQUAL(377, bitmap.GetWidth());
		}
		{
			auto const& slot = *skin.Find(HString("draworder2"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, slot.GetSize());
			auto const& attach = **slot.Find(HString("images/logo"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(AttachmentType::kBitmap, attach.GetType());
			auto const& bitmap = (BitmapAttachment const&)attach;
			SEOUL_UNITTESTING_ASSERT_EQUAL(logoPath, bitmap.GetFilePath());
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), bitmap.GetColor());
			SEOUL_UNITTESTING_ASSERT_EQUAL(120, bitmap.GetHeight());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionX());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionY());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetRotationInDegrees());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleX());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleY());
			SEOUL_UNITTESTING_ASSERT_EQUAL(120, bitmap.GetWidth());
		}
		{
			auto const& slot = *skin.Find(HString("images/spine"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, slot.GetSize());
			auto const& attach = **slot.Find(HString("images/spine"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(AttachmentType::kBitmap, attach.GetType());
			auto const& bitmap = (BitmapAttachment const&)attach;
			SEOUL_UNITTESTING_ASSERT_EQUAL(spinePath, bitmap.GetFilePath());
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), bitmap.GetColor());
			SEOUL_UNITTESTING_ASSERT_EQUAL(120, bitmap.GetHeight());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionX());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionY());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetRotationInDegrees());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleX());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleY());
			SEOUL_UNITTESTING_ASSERT_EQUAL(377, bitmap.GetWidth());
		}
		{
			auto const& slot = *skin.Find(HString("images/spine10"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(2u, slot.GetSize());
			{
				auto const& attach = **slot.Find(HString("images/logo"));
				SEOUL_UNITTESTING_ASSERT_EQUAL(AttachmentType::kBitmap, attach.GetType());
				auto const& bitmap = (BitmapAttachment const&)attach;
				SEOUL_UNITTESTING_ASSERT_EQUAL(logoPath, bitmap.GetFilePath());
				SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), bitmap.GetColor());
				SEOUL_UNITTESTING_ASSERT_EQUAL(120, bitmap.GetHeight());
				SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionX());
				SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionY());
				SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetRotationInDegrees());
				SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleX());
				SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleY());
				SEOUL_UNITTESTING_ASSERT_EQUAL(120, bitmap.GetWidth());
			}
			{
				auto const& attach = **slot.Find(HString("images/spine"));
				SEOUL_UNITTESTING_ASSERT_EQUAL(AttachmentType::kBitmap, attach.GetType());
				auto const& bitmap = (BitmapAttachment const&)attach;
				SEOUL_UNITTESTING_ASSERT_EQUAL(spinePath, bitmap.GetFilePath());
				SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), bitmap.GetColor());
				SEOUL_UNITTESTING_ASSERT_EQUAL(120, bitmap.GetHeight());
				SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionX());
				SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionY());
				SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetRotationInDegrees());
				SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleX());
				SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleY());
				SEOUL_UNITTESTING_ASSERT_EQUAL(377, bitmap.GetWidth());
			}
		}
		{
			auto const& slot = *skin.Find(HString("images/spine12"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(2u, slot.GetSize());
			{
				auto const& attach = **slot.Find(HString("images/spine"));
				SEOUL_UNITTESTING_ASSERT_EQUAL(AttachmentType::kMesh, attach.GetType());
				auto const& mesh = (MeshAttachment const&)attach;
				SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), mesh.GetColor());
				SEOUL_UNITTESTING_ASSERT_EQUAL(spinePath, mesh.GetFilePath());
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(188.37f, mesh.GetHeight(), 1e-3f);
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(392.7797f, mesh.GetWidth(), 1e-3f);
			}
			{
				auto const& attach = **slot.Find(HString("linkedmesh"));
				SEOUL_UNITTESTING_ASSERT_EQUAL(AttachmentType::kLinkedMesh, attach.GetType());
				auto const& mesh = (LinkedMeshAttachment const&)attach;
				SEOUL_UNITTESTING_ASSERT_EQUAL(true, mesh.GetDeform());
				SEOUL_UNITTESTING_ASSERT_EQUAL(logoPath, mesh.GetFilePath());
				SEOUL_UNITTESTING_ASSERT_EQUAL(120.0f, mesh.GetHeight());
				SEOUL_UNITTESTING_ASSERT_EQUAL(*slot.Find(HString("images/spine")), mesh.GetParent());
				SEOUL_UNITTESTING_ASSERT_EQUAL(HString("images/spine"), mesh.GetParentId());
				SEOUL_UNITTESTING_ASSERT_EQUAL(120.0f, mesh.GetWidth());
			}
		}
		{
			auto const& slot = *skin.Find(HString("images/spine2"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, slot.GetSize());
			auto const& attach = **slot.Find(HString("images/spine"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(AttachmentType::kBitmap, attach.GetType());
			auto const& bitmap = (BitmapAttachment const&)attach;
			SEOUL_UNITTESTING_ASSERT_EQUAL(spinePath, bitmap.GetFilePath());
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), bitmap.GetColor());
			SEOUL_UNITTESTING_ASSERT_EQUAL(120, bitmap.GetHeight());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionX());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionY());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetRotationInDegrees());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleX());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleY());
			SEOUL_UNITTESTING_ASSERT_EQUAL(377, bitmap.GetWidth());
		}
		{
			auto const& slot = *skin.Find(HString("images/spine3"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, slot.GetSize());
			auto const& attach = **slot.Find(HString("images/spine"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(AttachmentType::kBitmap, attach.GetType());
			auto const& bitmap = (BitmapAttachment const&)attach;
			SEOUL_UNITTESTING_ASSERT_EQUAL(spinePath, bitmap.GetFilePath());
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), bitmap.GetColor());
			SEOUL_UNITTESTING_ASSERT_EQUAL(120, bitmap.GetHeight());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionX());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionY());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetRotationInDegrees());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleX());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleY());
			SEOUL_UNITTESTING_ASSERT_EQUAL(377, bitmap.GetWidth());
		}
		{
			auto const& slot = *skin.Find(HString("images/spine4"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, slot.GetSize());
			auto const& attach = **slot.Find(HString("images/spine"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(AttachmentType::kBitmap, attach.GetType());
			auto const& bitmap = (BitmapAttachment const&)attach;
			SEOUL_UNITTESTING_ASSERT_EQUAL(spinePath, bitmap.GetFilePath());
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), bitmap.GetColor());
			SEOUL_UNITTESTING_ASSERT_EQUAL(120, bitmap.GetHeight());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionX());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionY());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.15f, bitmap.GetRotationInDegrees());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleX());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleY());
			SEOUL_UNITTESTING_ASSERT_EQUAL(377, bitmap.GetWidth());
		}
		{
			auto const& slot = *skin.Find(HString("images/spine5"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, slot.GetSize());
			auto const& attach = **slot.Find(HString("images/spine"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(AttachmentType::kBitmap, attach.GetType());
			auto const& bitmap = (BitmapAttachment const&)attach;
			SEOUL_UNITTESTING_ASSERT_EQUAL(spinePath, bitmap.GetFilePath());
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), bitmap.GetColor());
			SEOUL_UNITTESTING_ASSERT_EQUAL(120, bitmap.GetHeight());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionX());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionY());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetRotationInDegrees());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleX());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleY());
			SEOUL_UNITTESTING_ASSERT_EQUAL(377, bitmap.GetWidth());
		}
		{
			auto const& slot = *skin.Find(HString("images/spine6"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, slot.GetSize());
			auto const& attach = **slot.Find(HString("images/spine"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(AttachmentType::kBitmap, attach.GetType());
			auto const& bitmap = (BitmapAttachment const&)attach;
			SEOUL_UNITTESTING_ASSERT_EQUAL(spinePath, bitmap.GetFilePath());
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), bitmap.GetColor());
			SEOUL_UNITTESTING_ASSERT_EQUAL(120, bitmap.GetHeight());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionX());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionY());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetRotationInDegrees());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleX());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleY());
			SEOUL_UNITTESTING_ASSERT_EQUAL(377, bitmap.GetWidth());
		}
		{
			auto const& slot = *skin.Find(HString("images/spine7"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, slot.GetSize());
			auto const& attach = **slot.Find(HString("images/spine"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(AttachmentType::kBitmap, attach.GetType());
			auto const& bitmap = (BitmapAttachment const&)attach;
			SEOUL_UNITTESTING_ASSERT_EQUAL(spinePath, bitmap.GetFilePath());
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), bitmap.GetColor());
			SEOUL_UNITTESTING_ASSERT_EQUAL(120, bitmap.GetHeight());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionX());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionY());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetRotationInDegrees());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleX());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleY());
			SEOUL_UNITTESTING_ASSERT_EQUAL(377, bitmap.GetWidth());
		}
		{
			auto const& slot = *skin.Find(HString("images/spine8"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, slot.GetSize());
			auto const& attach = **slot.Find(HString("images/spine"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(AttachmentType::kBitmap, attach.GetType());
			auto const& bitmap = (BitmapAttachment const&)attach;
			SEOUL_UNITTESTING_ASSERT_EQUAL(spinePath, bitmap.GetFilePath());
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), bitmap.GetColor());
			SEOUL_UNITTESTING_ASSERT_EQUAL(120, bitmap.GetHeight());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionX());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetPositionY());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, bitmap.GetRotationInDegrees());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleX());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, bitmap.GetScaleY());
			SEOUL_UNITTESTING_ASSERT_EQUAL(377, bitmap.GetWidth());
		}
		{
			auto const& slot = *skin.Find(HString("images/spine9"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, slot.GetSize());
			auto const& attach = **slot.Find(HString("images/spine"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(AttachmentType::kMesh, attach.GetType());
			auto const& mesh = (MeshAttachment const&)attach;
			SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::White(), mesh.GetColor());
			SEOUL_UNITTESTING_ASSERT_EQUAL(spinePath, mesh.GetFilePath());
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(119.99f, mesh.GetHeight(), 1e-3f);
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(376.98999f, mesh.GetWidth(), 1e-3f);
		}
		{
			auto const& slot = *skin.Find(HString("path2"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1u, slot.GetSize());
			auto const& attach = **slot.Find(HString("path"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(AttachmentType::kPath, attach.GetType());
			auto const& path = (PathAttachment const&)attach;
			SEOUL_UNITTESTING_ASSERT_EQUAL(0u, path.GetBoneCounts().GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, path.GetClosed());
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, path.GetConstantSpeed());
			SEOUL_UNITTESTING_ASSERT_EQUAL(5u, path.GetLengths().GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(30u, path.GetVertexCount());
			SEOUL_UNITTESTING_ASSERT_EQUAL(30u, path.GetVertices().GetSize());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0u, path.GetWeights().GetSize());
		}
	}
}

// Regression for a bug in draw order processing.
void Animation2DTest::TestDrawOrder()
{
	using namespace Animation;
	using namespace Animation2D;

	auto p(Animation2D::Manager::Get()->CreateInstance(
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestDrawOrder.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/DrawOrderTest/DrawOrderTest.son"),
		SharedPtr<Animation::EventInterface>()));
	WaitForReady(p);

	// Fire the attack trigger.
	p->TriggerTransition(HString("Attack"));
	p->Tick(0.0f);

	SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kStateMachine, p->GetRoot()->GetType());
	SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Attack"), ((StateMachineInstance*)p->GetRoot().GetPtr())->GetNewId());

	// Let it play out. This should succeed. Prior to the fix, this was a crash.
	Float f = 0.0f;
	while (f < 0.5f)
	{
		p->Tick(1.0f / 60.0f);
		f += 1.0f / 60.0f;
	}
}


// Regression for a bug in draw order processing.
void Animation2DTest::TestDrawOrderAttackFrame0()
{
	TestFrameCommon(
		0.0f,
		"Frame0DrawOrderAttack",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestDrawOrderAttack.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/DrawOrderTest/DrawOrderTest.son"));
}

void Animation2DTest::TestDrawOrderAttackFramePoint25()
{
	TestFrameCommon(
		0.25f,
		"FramePoint25DrawOrderAttack",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestDrawOrderAttack.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/DrawOrderTest/DrawOrderTest.son"));
}

void Animation2DTest::TestDrawOrderAttackFramePoint5()
{
	TestFrameCommon(
		0.5f,
		"FramePoint5DrawOrderAttack",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestDrawOrderAttack.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/DrawOrderTest/DrawOrderTest.son"));
}

// Regression test for deformation bug in the draw order test.
void Animation2DTest::TestDrawOrderIdleFrame0()
{
	TestFrameCommon(
		0.0f,
		"Frame0DrawOrderIdle",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestDrawOrderIdle.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/DrawOrderTest/DrawOrderTest.son"));
}

void Animation2DTest::TestDrawOrderIdleFramePoint25()
{
	TestFrameCommon(
		0.25f,
		"FramePoint25DrawOrderIdle",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestDrawOrderIdle.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/DrawOrderTest/DrawOrderTest.son"));
}

void Animation2DTest::TestDrawOrderIdleFramePoint5()
{
	TestFrameCommon(
		0.5f,
		"FramePoint5DrawOrderIdle",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestDrawOrderIdle.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/DrawOrderTest/DrawOrderTest.son"));
}

class Animation2DTestEventInterface SEOUL_SEALED : public Animation::EventInterface
{
public:
	Animation2DTestEventInterface()
	{
	}

	virtual void DispatchEvent(HString name, Int32 i, Float32 f, const String& s) SEOUL_OVERRIDE
	{
		Entry entry;
		entry.m_Def.m_f = f;
		entry.m_Def.m_i = i;
		entry.m_Def.m_s = s;
		entry.m_Id = name;
		m_vEvents.PushBack(entry);
	}

	struct Entry
	{
		Animation2D::EventDefinition m_Def;
		HString m_Id;
	};
	Vector<Entry, MemoryBudgets::Animation2D> m_vEvents;

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(Animation2DTestEventInterface);

	SEOUL_DISABLE_COPY(Animation2DTestEventInterface);
}; // class Animation2DTestEventInterface

void Animation2DTest::TestEvents()
{
	SharedPtr<Animation2DTestEventInterface> pInterface(SEOUL_NEW(MemoryBudgets::Animation2D) Animation2DTestEventInterface);
	auto p(Animation2D::Manager::Get()->CreateInstance(
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestNetworkComplex.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test2/TestAnimation2D.son"),
		pInterface));
	WaitForReady(p);

	using namespace Animation2D;

	// Set condition to moving - we should be walking.
	p->SetCondition(HString("Moving"), true);

	// Advance 0 to enter the moving state.
	p->Tick(0.0f);

	// Verify that no events were fired (none should be fired with a 0 delta).
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, pInterface->m_vEvents.GetSize());

	// Now tick by 2 * max time to get fully into the moving state, and evaluate event behavior.
	// This should generate 6 events (events at 0, 0.5333 and 1.0666, all should be triggered twice).
	p->Tick(2.0f * p->GetCurrentMaxTime());
	{
		auto const& v = pInterface->m_vEvents;
		SEOUL_UNITTESTING_ASSERT_EQUAL(6u, v.GetSize());

		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Footstep"), v[0].m_Id);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4.5f, v[0].m_Def.m_f);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, v[0].m_Def.m_i);
		SEOUL_UNITTESTING_ASSERT_EQUAL("Test4", v[0].m_Def.m_s);

		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Footstep"), v[1].m_Id);
		SEOUL_UNITTESTING_ASSERT_EQUAL(8.5f, v[1].m_Def.m_f);
		SEOUL_UNITTESTING_ASSERT_EQUAL(8, v[1].m_Def.m_i);
		SEOUL_UNITTESTING_ASSERT_EQUAL("Test3", v[1].m_Def.m_s);

		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Footstep"), v[2].m_Id);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5.5f, v[2].m_Def.m_f);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v[2].m_Def.m_i);
		SEOUL_UNITTESTING_ASSERT_EQUAL("Test", v[2].m_Def.m_s);

		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Footstep"), v[3].m_Id);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4.5f, v[3].m_Def.m_f);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, v[3].m_Def.m_i);
		SEOUL_UNITTESTING_ASSERT_EQUAL("Test4", v[3].m_Def.m_s);

		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Footstep"), v[4].m_Id);
		SEOUL_UNITTESTING_ASSERT_EQUAL(8.5f, v[4].m_Def.m_f);
		SEOUL_UNITTESTING_ASSERT_EQUAL(8, v[4].m_Def.m_i);
		SEOUL_UNITTESTING_ASSERT_EQUAL("Test3", v[4].m_Def.m_s);

		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Footstep"), v[5].m_Id);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5.5f, v[5].m_Def.m_f);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, v[5].m_Def.m_i);
		SEOUL_UNITTESTING_ASSERT_EQUAL("Test", v[5].m_Def.m_s);
	}
}

void Animation2DTest::TestGetTimeToEvent()
{
	auto p(Animation2D::Manager::Get()->CreateInstance(
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestNetworkComplex.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test2/TestAnimation2D.son"),
		SharedPtr<Animation::EventInterface>()));

	WaitForReady(p);

	// Advance 0 to enter the idle state.
	p->Tick(0.0f);

	// invalid event name test
	Float fTimeToEvent = 6.0f;
	Bool bEventFound = p->GetTimeToEvent(HString("Invalid"), fTimeToEvent);
	SEOUL_UNITTESTING_ASSERT_EQUAL(bEventFound, false);
	SEOUL_UNITTESTING_ASSERT_EQUAL(fTimeToEvent, 6.0f); // if we don't find a result we shouldn't reset the value

	// valid event name test
	bEventFound = p->GetTimeToEvent(HString("Fidget"), fTimeToEvent);
	SEOUL_UNITTESTING_ASSERT_EQUAL(bEventFound, true);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(fTimeToEvent, 0.5f, fEpsilon);

	// Now tick to 100ms before the end of the animation and measure again
	// This is a looping animation, so we should measure to the next cycle
	p->Tick(p->GetCurrentMaxTime() - 0.1f);

	bEventFound = p->GetTimeToEvent(HString("Fidget"), fTimeToEvent);
	SEOUL_UNITTESTING_ASSERT_EQUAL(bEventFound, true);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(fTimeToEvent, 0.6f, fEpsilon);
}

void Animation2DTest::TestFrame0Loop()
{
	TestFrameCommon(0.0f, "Frame0", true);
}

void Animation2DTest::TestFramePoint25SecondsLoop()
{
	TestFrameCommon(0.25f, "FramePoint25", true);
}

void Animation2DTest::TestFramePoint5SecondsLoop()
{
	TestFrameCommon(0.5f, "FramePoint5", true);
}

void Animation2DTest::TestFrame1SecondLoop()
{
	TestFrameCommon(1.0f, "Frame1", true);
}

void Animation2DTest::TestFrame1Point5SecondsLoop()
{
	TestFrameCommon(1.5f, "Frame1Point5Loop", true);
}

void Animation2DTest::TestFrame0NoLoop()
{
	TestFrameCommon(0.0f, "Frame0");
}

void Animation2DTest::TestFramePoint25SecondsNoLoop()
{
	TestFrameCommon(0.25f, "FramePoint25", false);
}

void Animation2DTest::TestFramePoint5SecondsNoLoop()
{
	TestFrameCommon(0.5f, "FramePoint5");
}

void Animation2DTest::TestFrame1SecondNoLoop()
{
	TestFrameCommon(1.0f, "Frame1");
}

void Animation2DTest::TestFrame1Point5SecondsNoLoop()
{
	TestFrameCommon(1.5f, "Frame1Point5NoLoop");
}

void Animation2DTest::Test3Frame0Loop()
{
	Test3FrameCommon(0.0f, "Frame0Test3", true);
}

void Animation2DTest::Test3FramePoint25SecondsLoop()
{
	Test3FrameCommon(0.25f, "FramePoint25Test3", true);
}

void Animation2DTest::Test3FramePoint5SecondsLoop()
{
	Test3FrameCommon(0.5f, "FramePoint5Test3", true);
}

void Animation2DTest::Test3Frame1SecondLoop()
{
	Test3FrameCommon(1.0f, "Frame1Test3", true);
}

void Animation2DTest::Test3Frame1Point5SecondsLoop()
{
	Test3FrameCommon(1.5f, "Frame1Point5Test3", true);
}

void Animation2DTest::Test3Frame0NoLoop()
{
	Test3FrameCommon(0.0f, "Frame0Test3NoLoop");
}

void Animation2DTest::Test3FramePoint25SecondsNoLoop()
{
	Test3FrameCommon(0.25f, "FramePoint25Test3NoLoop");
}

void Animation2DTest::Test3FramePoint5SecondsNoLoop()
{
	Test3FrameCommon(0.5f, "FramePoint5Test3NoLoop");
}

void Animation2DTest::Test3Frame1SecondNoLoop()
{
	Test3FrameCommon(1.0f, "Frame1Test3NoLoop");
}

void Animation2DTest::Test3Frame1Point5SecondsNoLoop()
{
	Test3FrameCommon(1.5f, "Frame1Point5Test3NoLoop");
}

void Animation2DTest::TestFrame0Path()
{
	TestFrameCommon(
		0.0f,
		"Frame0PathTest",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestPath.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/PathTest/PathTest.son"));
}

void Animation2DTest::TestFramePoint25SecondsPath()
{
	TestFrameCommon(
		0.25f,
		"FramePoint25PathTest",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestPath.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/PathTest/PathTest.son"));
}

void Animation2DTest::TestFramePoint5SecondsPath()
{
	TestFrameCommon(
		0.5f,
		"FramePoint5PathTest",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestPath.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/PathTest/PathTest.son"));
}

void Animation2DTest::TestFrame1SecondPath()
{
	TestFrameCommon(
		1.0f,
		"Frame1PathTest",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestPath.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/PathTest/PathTest.son"));
}

void Animation2DTest::TestFrame1Point5SecondsPath()
{
	TestFrameCommon(
		1.5f,
		"Frame1Point5PathTest",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestPath.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/PathTest/PathTest.son"));
}

void Animation2DTest::TestFrame0Path2()
{
	TestFrameCommon(
		0.0f,
		"Frame0Path2Test",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestPath.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/PathTest2/PathTest2.son"));
}

void Animation2DTest::TestFramePoint25SecondsPath2()
{
	TestFrameCommon(
		0.25f,
		"FramePoint25Path2Test",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestPath.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/PathTest2/PathTest2.son"));
}

void Animation2DTest::TestFramePoint5SecondsPath2()
{
	TestFrameCommon(
		0.5f,
		"FramePoint5Path2Test",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestPath.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/PathTest2/PathTest2.son"));
}

void Animation2DTest::TestFrame1SecondPath2()
{
	TestFrameCommon(
		1.0f,
		"Frame1Path2Test",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestPath.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/PathTest2/PathTest2.son"));
}

void Animation2DTest::TestFrame1Point5SecondsPath2()
{
	TestFrameCommon(
		1.5f,
		"Frame1Point5Path2Test",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestPath.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/PathTest2/PathTest2.son"));
}

void Animation2DTest::TestFrame0TransformConstraint()
{
	TestFrameCommon(
		0.0f,
		"Frame0TransformConstraintTest",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestTransformConstraint.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/TransformConstraintTest/TransformConstraintTest.son"));
}

void Animation2DTest::TestFramePoint25SecondsTransformConstraint()
{
	TestFrameCommon(
		0.25f,
		"FramePoint25TransformConstraintTest",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestTransformConstraint.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/TransformConstraintTest/TransformConstraintTest.son"));
}

void Animation2DTest::TestFramePoint5SecondsTransformConstraint()
{
	TestFrameCommon(
		0.5f,
		"FramePoint5TransformConstraintTest",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestTransformConstraint.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/TransformConstraintTest/TransformConstraintTest.son"));
}

void Animation2DTest::TestFrame1SecondTransformConstraint()
{
	TestFrameCommon(
		1.0f,
		"Frame1TransformConstraintTest",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestTransformConstraint.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/TransformConstraintTest/TransformConstraintTest.son"));
}

void Animation2DTest::TestFrame1Point5SecondsTransformConstraint()
{
	TestFrameCommon(
		1.5f,
		"Frame1Point5TransformConstraintTest",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestTransformConstraint.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/TransformConstraintTest/TransformConstraintTest.son"));
}

// Regression for a bug in the cache accumulator that caused a loss of tpose scale.
void Animation2DTest::TestHeadTurn()
{
	using namespace Animation;
	using namespace Animation2D;

	auto p(Animation2D::Manager::Get()->CreateInstance(
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestNetworkComplex.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test2/TestAnimation2D.son"),
		SharedPtr<Animation::EventInterface>()));
	WaitForReady(p);

	// Make sure we're in the idle state.
	p->Tick(0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Idle"), SharedPtr<StateMachineInstance>((StateMachineInstance*)p->GetRoot().GetPtr())->GetNewId());

	// Wait a bit.
	for (UInt32 i = 0u; i < 120u; ++i)
	{
		p->Tick(1 / 60.0f);
	}

	// Go to the head turn state.
	p->TriggerTransition(HString("HeadTurn"));
	p->Tick(0.0f);

	SEOUL_UNITTESTING_ASSERT_EQUAL(HString("HeadTurn"), SharedPtr<StateMachineInstance>((StateMachineInstance*)p->GetRoot().GetPtr())->GetNewId());

	// Finish the head turn animation, and check states.
	{
		Float fAccum = 0.0f;
		for (UInt32 i = 0u; i < 121u; ++i)
		{
			p->Tick(1 / 60.0f);
			fAccum += (1 / 60.0f);
			CheckBoneStates(p, HString("Headturn"), fAccum, HString("Headturn"), fAccum, 0.5f);
		}
	}

	// Now make sure we return to the Idle state.
	p->Tick(0.0f);

	SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Idle"), SharedPtr<StateMachineInstance>((StateMachineInstance*)p->GetRoot().GetPtr())->GetNewId());

	// Wait 0.5 seconds for Idle to complete, then check state.
	for (UInt32 i = 0u; i < 30u; ++i)
	{
		p->Tick(1 / 60.0f);
	}

	// Check states.
	CheckBoneStates(p, HString("Idle"), 0.5f, HString("Idle"), 0.5f, 0.5f);
}

// Regression for bug caused by allowing animation curves to apply their state at
// their start time, when the evaluation time was before their start time.
void Animation2DTest::TestHeadTurnFrame0()
{
	TestFrameCommon(
		0.0f,
		"Frame0HeadTurn",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestNetworkHeadTurn.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test2/TestAnimation2D.son"));
}

void Animation2DTest::TestNetwork()
{
	auto p(Animation2D::Manager::Get()->CreateInstance(
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestNetworkComplex.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test2/TestAnimation2D.son"),
		SharedPtr<Animation::EventInterface>()));
	WaitForReady(p);

	using namespace Animation;
	using namespace Animation2D;

	// Verify that the network is configured as expected.
	auto const& pNetwork = p->GetNetwork();

	// Conditions
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, pNetwork->GetConditions().GetSize());
		Bool bValue = true;
		SEOUL_UNITTESTING_ASSERT(pNetwork->GetConditions().GetValue(HString("Moving"), bValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(false, bValue);
	}

	// Params
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, pNetwork->GetParameters().GetSize());
		Animation::NetworkDefinitionParameter parameter;
		SEOUL_UNITTESTING_ASSERT(pNetwork->GetParameters().GetValue(HString("MoveMix"), parameter));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, parameter.m_fDefault);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, parameter.m_fMin);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, parameter.m_fMax);
	}

	// Root
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kStateMachine, pNetwork->GetRoot()->GetType());
		SharedPtr<StateMachineDefinition> pRoot((StateMachineDefinition*)pNetwork->GetRoot().GetPtr());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Idle"), pRoot->GetDefaultState());
		SEOUL_UNITTESTING_ASSERT_EQUAL(4u, pRoot->GetStates().GetSize());

		// Idle
		{
			auto pState = pRoot->GetStates().Find(HString("Idle"));
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, pState);

			// Transitions
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(3u, pState->m_vTransitions.GetSize());
				{
					auto const& t = pState->m_vTransitions[0];
					SEOUL_UNITTESTING_ASSERT_EQUAL(1u, t.m_Triggers.GetSize());
					SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Attack"), *t.m_Triggers.Begin());
					SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, t.m_fTimeInSeconds);
					SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Attack"), t.m_Target);
					SEOUL_UNITTESTING_ASSERT(t.m_vConditions.IsEmpty());
					SEOUL_UNITTESTING_ASSERT(t.m_vNegativeConditions.IsEmpty());
				}
				{
					auto const& t = pState->m_vTransitions[1];
					SEOUL_UNITTESTING_ASSERT_EQUAL(0u, t.m_Triggers.GetSize());
					SEOUL_UNITTESTING_ASSERT_EQUAL(2.0f, t.m_fTimeInSeconds);
					SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Move"), t.m_Target);
					SEOUL_UNITTESTING_ASSERT_EQUAL(1u, t.m_vConditions.GetSize());
					SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Moving"), t.m_vConditions[0]);
					SEOUL_UNITTESTING_ASSERT(t.m_vNegativeConditions.IsEmpty());
				}
				{
					auto const& t = pState->m_vTransitions[2];
					SEOUL_UNITTESTING_ASSERT_EQUAL(1u, t.m_Triggers.GetSize());
					SEOUL_UNITTESTING_ASSERT_EQUAL(HString("HeadTurn"), *t.m_Triggers.Begin());
					SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, t.m_fTimeInSeconds);
					SEOUL_UNITTESTING_ASSERT_EQUAL(HString("HeadTurn"), t.m_Target);
					SEOUL_UNITTESTING_ASSERT(t.m_vConditions.IsEmpty());
					SEOUL_UNITTESTING_ASSERT(t.m_vNegativeConditions.IsEmpty());
				}
			}

			// Child
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kPlayClip, pState->m_pChild->GetType());
				auto pChild = SharedPtr<PlayClipDefinition>((PlayClipDefinition*)pState->m_pChild.GetPtr());
				SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Idle"), pChild->GetName());
				SEOUL_UNITTESTING_ASSERT_EQUAL(true, pChild->GetLoop());
				SEOUL_UNITTESTING_ASSERT_EQUAL(HString(), pChild->GetOnComplete());
			}
		}

		// Move
		{
			auto pState = pRoot->GetStates().Find(HString("Move"));
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, pState);

			// Transitions
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(2u, pState->m_vTransitions.GetSize());
				{
					auto const& t = pState->m_vTransitions[0];
					SEOUL_UNITTESTING_ASSERT_EQUAL(1u, t.m_Triggers.GetSize());
					SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Attack"), *t.m_Triggers.Begin());
					SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, t.m_fTimeInSeconds);
					SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Attack"), t.m_Target);
					SEOUL_UNITTESTING_ASSERT(t.m_vConditions.IsEmpty());
					SEOUL_UNITTESTING_ASSERT(t.m_vNegativeConditions.IsEmpty());
				}
				{
					auto const& t = pState->m_vTransitions[1];
					SEOUL_UNITTESTING_ASSERT_EQUAL(0u, t.m_Triggers.GetSize());
					SEOUL_UNITTESTING_ASSERT_EQUAL(2.0f, t.m_fTimeInSeconds);
					SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Idle"), t.m_Target);
					SEOUL_UNITTESTING_ASSERT(t.m_vConditions.IsEmpty());
					SEOUL_UNITTESTING_ASSERT_EQUAL(1u, t.m_vNegativeConditions.GetSize());
					SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Moving"), t.m_vNegativeConditions[0]);
				}
			}

			// Child
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kBlend, pState->m_pChild->GetType());
				auto pChild = SharedPtr<BlendDefinition>((BlendDefinition*)pState->m_pChild.GetPtr());
				SEOUL_UNITTESTING_ASSERT_EQUAL(HString("MoveMix"), pChild->GetMixParameterId());

				// ChildA
				{
					SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kPlayClip, pChild->GetChildA()->GetType());
					auto pChildA = SharedPtr<PlayClipDefinition>((PlayClipDefinition*)pChild->GetChildA().GetPtr());
					SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Walk"), pChildA->GetName());
					SEOUL_UNITTESTING_ASSERT_EQUAL(true, pChildA->GetLoop());
					SEOUL_UNITTESTING_ASSERT_EQUAL(HString(), pChildA->GetOnComplete());
				}

				// ChildB
				{
					SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kPlayClip, pChild->GetChildB()->GetType());
					auto pChildB = SharedPtr<PlayClipDefinition>((PlayClipDefinition*)pChild->GetChildB().GetPtr());
					SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Run"), pChildB->GetName());
					SEOUL_UNITTESTING_ASSERT_EQUAL(true, pChildB->GetLoop());
					SEOUL_UNITTESTING_ASSERT_EQUAL(HString(), pChildB->GetOnComplete());
				}
			}
		}

		// Attack
		{
			auto pState = pRoot->GetStates().Find(HString("Attack"));
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, pState);

			// Transitions
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(2u, pState->m_vTransitions.GetSize());
				{
					auto const& t = pState->m_vTransitions[0];
					SEOUL_UNITTESTING_ASSERT_EQUAL(1u, t.m_Triggers.GetSize());
					SEOUL_UNITTESTING_ASSERT_EQUAL(HString("OnAnimationComplete"), *t.m_Triggers.Begin());
					SEOUL_UNITTESTING_ASSERT_EQUAL(0.5f, t.m_fTimeInSeconds);
					SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Move"), t.m_Target);
					SEOUL_UNITTESTING_ASSERT_EQUAL(1u, t.m_vConditions.GetSize());
					SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Moving"), t.m_vConditions[0]);
					SEOUL_UNITTESTING_ASSERT(t.m_vNegativeConditions.IsEmpty());
				}
				{
					auto const& t = pState->m_vTransitions[1];
					SEOUL_UNITTESTING_ASSERT_EQUAL(1u, t.m_Triggers.GetSize());
					SEOUL_UNITTESTING_ASSERT_EQUAL(HString("OnAnimationComplete"), *t.m_Triggers.Begin());
					SEOUL_UNITTESTING_ASSERT_EQUAL(0.5f, t.m_fTimeInSeconds);
					SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Idle"), t.m_Target);
					SEOUL_UNITTESTING_ASSERT(t.m_vConditions.IsEmpty());
					SEOUL_UNITTESTING_ASSERT(t.m_vNegativeConditions.IsEmpty());
				}
			}

			// Child
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kPlayClip, pState->m_pChild->GetType());
				auto pChild = SharedPtr<PlayClipDefinition>((PlayClipDefinition*)pState->m_pChild.GetPtr());
				SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Attack"), pChild->GetName());
				SEOUL_UNITTESTING_ASSERT_EQUAL(false, pChild->GetLoop());
				SEOUL_UNITTESTING_ASSERT_EQUAL(HString("OnAnimationComplete"), pChild->GetOnComplete());
			}
		}

		// HeadTurn
		{
			auto pState = pRoot->GetStates().Find(HString("HeadTurn"));
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, pState);

			// Transitions
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(1u, pState->m_vTransitions.GetSize());
				{
					auto const& t = pState->m_vTransitions[0];
					SEOUL_UNITTESTING_ASSERT_EQUAL(1u, t.m_Triggers.GetSize());
					SEOUL_UNITTESTING_ASSERT_EQUAL(HString("OnAnimationComplete"), *t.m_Triggers.Begin());
					SEOUL_UNITTESTING_ASSERT_EQUAL(0.5f, t.m_fTimeInSeconds);
					SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Idle"), t.m_Target);
					SEOUL_UNITTESTING_ASSERT(t.m_vConditions.IsEmpty());
					SEOUL_UNITTESTING_ASSERT(t.m_vNegativeConditions.IsEmpty());
				}
			}

			// Child
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kPlayClip, pState->m_pChild->GetType());
				auto pChild = SharedPtr<PlayClipDefinition>((PlayClipDefinition*)pState->m_pChild.GetPtr());
				SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Headturn"), pChild->GetName());
				SEOUL_UNITTESTING_ASSERT_EQUAL(false, pChild->GetLoop());
				SEOUL_UNITTESTING_ASSERT_EQUAL(HString("OnAnimationComplete"), pChild->GetOnComplete());
			}
		}
	}
}

void Animation2DTest::TestNetworkEval()
{
	auto p(Animation2D::Manager::Get()->CreateInstance(
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestNetworkComplex.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test2/TestAnimation2D.son"),
		SharedPtr<Animation::EventInterface>()));
	WaitForReady(p);

	using namespace Animation;
	using namespace Animation2D;

	// Conditions
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, p->GetConditions().GetSize());
		Bool bValue = true;
		SEOUL_UNITTESTING_ASSERT(p->GetConditions().GetValue(HString("Moving"), bValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(false, bValue);
	}

	// Params
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, p->GetParameters().GetSize());
		Float fValue = 1.0f;
		SEOUL_UNITTESTING_ASSERT(p->GetParameters().GetValue(HString("MoveMix"), fValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, fValue);
	}

	// State at time 0.
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, p->GetCurrentMaxTime());
	SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kStateMachine, p->GetRoot()->GetType());
	{
		auto pRoot(SharedPtr<StateMachineInstance>((StateMachineInstance*)p->GetRoot().GetPtr()));
		SEOUL_UNITTESTING_ASSERT(pRoot->GetNew().IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Idle"), pRoot->GetNewId());
		SEOUL_UNITTESTING_ASSERT(!pRoot->GetOld().IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString(), pRoot->GetOldId());

		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kPlayClip, pRoot->GetNew()->GetType());
			auto pChild(SharedPtr<PlayClipInstance>((PlayClipInstance*)pRoot->GetNew().GetPtr()));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pChild->GetCurrentTime());
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, pChild->IsDone());
		}

		StateMachineInstance::ViableTriggers t;
		pRoot->GetViableTriggers(t);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2u, t.GetSize());
		SEOUL_UNITTESTING_ASSERT(t.HasKey(HString("Attack")));
		SEOUL_UNITTESTING_ASSERT(t.HasKey(HString("HeadTurn")));
	}

	// Fire a transition.
	p->TriggerTransition(HString("Attack"));

	// Zero time advance, make sure we end up in the attack state.
	p->Tick(0.0f);

	// New state at time 0.
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.4f, p->GetCurrentMaxTime(), 1e-3f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kStateMachine, p->GetRoot()->GetType());
	{
		auto pRoot(SharedPtr<StateMachineInstance>((StateMachineInstance*)p->GetRoot().GetPtr()));
		SEOUL_UNITTESTING_ASSERT(pRoot->GetNew().IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Attack"), pRoot->GetNewId());
		SEOUL_UNITTESTING_ASSERT(!pRoot->GetOld().IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString(), pRoot->GetOldId());

		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kPlayClip, pRoot->GetNew()->GetType());
			auto pChild(SharedPtr<PlayClipInstance>((PlayClipInstance*)pRoot->GetNew().GetPtr()));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pChild->GetCurrentTime());
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, pChild->IsDone());
		}

		StateMachineInstance::ViableTriggers t;
		pRoot->GetViableTriggers(t);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, t.GetSize());
		SEOUL_UNITTESTING_ASSERT(t.HasKey(HString("OnAnimationComplete")));
	}

	// 0.4 second advance, should trigger the end of the attack animation.
	p->Tick(0.4f);

	// New state at time 0.4.
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.4f, p->GetCurrentMaxTime(), 1e-3f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kStateMachine, p->GetRoot()->GetType());
	{
		auto pRoot(SharedPtr<StateMachineInstance>((StateMachineInstance*)p->GetRoot().GetPtr()));
		SEOUL_UNITTESTING_ASSERT(pRoot->GetNew().IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Attack"), pRoot->GetNewId());
		SEOUL_UNITTESTING_ASSERT(!pRoot->GetOld().IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString(), pRoot->GetOldId());

		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kPlayClip, pRoot->GetNew()->GetType());
			auto pChild(SharedPtr<PlayClipInstance>((PlayClipInstance*)pRoot->GetNew().GetPtr()));
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.4f, pChild->GetCurrentTime(), 1e-3f);
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, pChild->IsDone());
		}

		StateMachineInstance::ViableTriggers t;
		pRoot->GetViableTriggers(t);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, t.GetSize());
		SEOUL_UNITTESTING_ASSERT(t.HasKey(HString("OnAnimationComplete")));
	}

	// Zero time tick, should evaluate the queued OnAnimationComplete trigger and
	// start transitioning to the idle state.
	p->Tick(0.0f);

	// New state is in the transition between the idle and attack states.
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, p->GetCurrentMaxTime());
	SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kStateMachine, p->GetRoot()->GetType());
	{
		auto pRoot(SharedPtr<StateMachineInstance>((StateMachineInstance*)p->GetRoot().GetPtr()));
		SEOUL_UNITTESTING_ASSERT(pRoot->GetNew().IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Idle"), pRoot->GetNewId());
		SEOUL_UNITTESTING_ASSERT(pRoot->GetOld().IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Attack"), pRoot->GetOldId());

		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kPlayClip, pRoot->GetNew()->GetType());
			auto pChild(SharedPtr<PlayClipInstance>((PlayClipInstance*)pRoot->GetNew().GetPtr()));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pChild->GetCurrentTime());
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, pChild->IsDone());
		}
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kPlayClip, pRoot->GetOld()->GetType());
			auto pChild(SharedPtr<PlayClipInstance>((PlayClipInstance*)pRoot->GetOld().GetPtr()));
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.4f, pChild->GetCurrentTime(), 1e-3f);
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, pChild->IsDone());
		}

		StateMachineInstance::ViableTriggers t;
		pRoot->GetViableTriggers(t);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2u, t.GetSize());
		SEOUL_UNITTESTING_ASSERT(t.HasKey(HString("Attack")));
		SEOUL_UNITTESTING_ASSERT(t.HasKey(HString("HeadTurn")));
	}

	// Now move through half of the transition time.
	p->Tick(0.25f);

	// New state remains in the transition between the idle and attack states.
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, p->GetCurrentMaxTime());
	SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kStateMachine, p->GetRoot()->GetType());
	{
		auto pRoot(SharedPtr<StateMachineInstance>((StateMachineInstance*)p->GetRoot().GetPtr()));
		SEOUL_UNITTESTING_ASSERT(pRoot->GetNew().IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Idle"), pRoot->GetNewId());
		SEOUL_UNITTESTING_ASSERT(pRoot->GetOld().IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Attack"), pRoot->GetOldId());

		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kPlayClip, pRoot->GetNew()->GetType());
			auto pChild(SharedPtr<PlayClipInstance>((PlayClipInstance*)pRoot->GetNew().GetPtr()));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0.25f, pChild->GetCurrentTime());
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, pChild->IsDone());
		}
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kPlayClip, pRoot->GetOld()->GetType());
			auto pChild(SharedPtr<PlayClipInstance>((PlayClipInstance*)pRoot->GetOld().GetPtr()));
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.4f, pChild->GetCurrentTime(), 1e-3f);
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, pChild->IsDone());
		}

		StateMachineInstance::ViableTriggers t;
		pRoot->GetViableTriggers(t);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2u, t.GetSize());
		SEOUL_UNITTESTING_ASSERT(t.HasKey(HString("Attack")));
		SEOUL_UNITTESTING_ASSERT(t.HasKey(HString("HeadTurn")));
	}

	// Check that the blend has blended bones as expected.
	CheckBoneStates(p, HString("Attack"), 0.4f, HString("Idle"), 0.25f, 0.5f);

	// Interrupt the transition to proceed to the move state.
	p->SetCondition(HString("Moving"), true);

	// Zero time tick to make sure we've ended up in the moving state,
	// and that the old state is "idle", since we were at 50% of the way
	// through the transition to it.
	p->Tick(0.0f);

	// New state remains in the transition between the idle and moving states.
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1.0666f, p->GetCurrentMaxTime(), 1e-3f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kStateMachine, p->GetRoot()->GetType());
	{
		auto pRoot(SharedPtr<StateMachineInstance>((StateMachineInstance*)p->GetRoot().GetPtr()));
		SEOUL_UNITTESTING_ASSERT(pRoot->GetNew().IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Move"), pRoot->GetNewId());
		SEOUL_UNITTESTING_ASSERT(pRoot->GetOld().IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Idle"), pRoot->GetOldId());

		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kBlend, pRoot->GetNew()->GetType());
			auto pChild(SharedPtr<BlendInstance>((BlendInstance*)pRoot->GetNew().GetPtr()));
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kPlayClip, pChild->GetChildA()->GetType());
				auto pChildA(SharedPtr<PlayClipInstance>((PlayClipInstance*)pChild->GetChildA().GetPtr()));
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.0f, pChildA->GetCurrentTime(), 1e-3f);
				SEOUL_UNITTESTING_ASSERT_EQUAL(false, pChildA->IsDone());
			}
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kPlayClip, pChild->GetChildB()->GetType());
				auto pChildB(SharedPtr<PlayClipInstance>((PlayClipInstance*)pChild->GetChildB().GetPtr()));
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.0f, pChildB->GetCurrentTime(), 1e-3f);
				SEOUL_UNITTESTING_ASSERT_EQUAL(false, pChildB->IsDone());
			}
		}
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kPlayClip, pRoot->GetOld()->GetType());
			auto pChild(SharedPtr<PlayClipInstance>((PlayClipInstance*)pRoot->GetOld().GetPtr()));
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.25f, pChild->GetCurrentTime(), 1e-3f);
			SEOUL_UNITTESTING_ASSERT_EQUAL(false, pChild->IsDone());
		}

		StateMachineInstance::ViableTriggers t;
		pRoot->GetViableTriggers(t);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, t.GetSize());
		SEOUL_UNITTESTING_ASSERT(t.HasKey(HString("Attack")));
	}

	// Check that the blend has blended bones as expected.
	CheckBoneStates(p, HString("Idle"), 0.25f, HString("Walk"), 0.0f, 0.0f);

	// Now set the blend value in preparation.
	p->SetParameter(HString("MoveMix"), 0.25f);

	// Tick to finish the transition.
	p->Tick(2.0f);

	// New state remains in the transition between the walk and run states.
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1.0666f, p->GetCurrentMaxTime(), 1e-3f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kStateMachine, p->GetRoot()->GetType());
	{
		auto pRoot(SharedPtr<StateMachineInstance>((StateMachineInstance*)p->GetRoot().GetPtr()));
		SEOUL_UNITTESTING_ASSERT(pRoot->GetNew().IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Move"), pRoot->GetNewId());
		SEOUL_UNITTESTING_ASSERT(!pRoot->GetOld().IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString(), pRoot->GetOldId());

		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kBlend, pRoot->GetNew()->GetType());
			auto pChild(SharedPtr<BlendInstance>((BlendInstance*)pRoot->GetNew().GetPtr()));
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kPlayClip, pChild->GetChildA()->GetType());
				auto pChildA(SharedPtr<PlayClipInstance>((PlayClipInstance*)pChild->GetChildA().GetPtr()));
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Fmod(2.0f, 1.0666f), pChildA->GetCurrentTime(), 1e-3f);
				SEOUL_UNITTESTING_ASSERT_EQUAL(false, pChildA->IsDone());
			}
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kPlayClip, pChild->GetChildB()->GetType());
				auto pChildB(SharedPtr<PlayClipInstance>((PlayClipInstance*)pChild->GetChildB().GetPtr()));
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Fmod(2.0f, 0.5333f), pChildB->GetCurrentTime(), 1e-3f);
				SEOUL_UNITTESTING_ASSERT_EQUAL(false, pChildB->IsDone());
			}
		}

		StateMachineInstance::ViableTriggers t;
		pRoot->GetViableTriggers(t);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, t.GetSize());
		SEOUL_UNITTESTING_ASSERT(t.HasKey(HString("Attack")));
	}

	// Check states.
	CheckBoneStates(p, HString("Walk"), Fmod(2.0f, 1.0667f), HString("Run"), Fmod(2.0f, 0.5333f), 0.25f);
}

// Regression for a bug introduced in a recent checkin. Rotation was broken
// and this was missed with other animation tests.
void Animation2DTest::TestRotation()
{
	TestFrameCommon(
		0.0f,
		"Frame0Complex",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestNetworkComplex.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test2/TestAnimation2D.son"));
	TestFrameCommon(
		0.25f,
		"FramePoint25Complex",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestNetworkComplex.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test2/TestAnimation2D.son"));
	TestFrameCommon(
		0.5f,
		"FramePoint5Complex",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestNetworkComplex.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test2/TestAnimation2D.son"));
	TestFrameCommon(
		1.0f,
		"Frame1Complex",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestNetworkComplex.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test2/TestAnimation2D.son"));
}

void Animation2DTest::TestSynchronizeTime()
{
	auto p(Animation2D::Manager::Get()->CreateInstance(
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestNetworkSynchronizeTime.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test2/TestAnimation2D.son"),
		SharedPtr<Animation::EventInterface>()));
	WaitForReady(p);

	static const Float kfStep = 1.0f / 60.0f;

	using namespace Animation;
	using namespace Animation2D;

	// Advance by a 1 / 60, make sure the full time
	// step goes to BlendClipA.
	p->Tick(kfStep);

	auto pRoot(p->GetRoot());
	SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kBlend, pRoot->GetType());

	{
		SharedPtr<BlendInstance> pBlend((BlendInstance*)pRoot.GetPtr());
		SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kPlayClip, pBlend->GetChildA()->GetType());
		{
			SharedPtr<PlayClipInstance> pA((PlayClipInstance*)pBlend->GetChildA().GetPtr());
			SEOUL_UNITTESTING_ASSERT_EQUAL(kfStep, pA->GetCurrentTime());
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kPlayClip, pBlend->GetChildB()->GetType());
		{
			SharedPtr<PlayClipInstance> pB((PlayClipInstance*)pBlend->GetChildB().GetPtr());
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(kfStep * 0.5f, pB->GetCurrentTime(), 1e-4f);
		}

		// Change blend mode, and advance again.
		p->SetParameter(HString("MoveMix"), 1.0f);
		p->Tick(kfStep);

		SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kPlayClip, pBlend->GetChildA()->GetType());
		{
			SharedPtr<PlayClipInstance> pA((PlayClipInstance*)pBlend->GetChildA().GetPtr());
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(kfStep + kfStep * 2.0f, pA->GetCurrentTime(), 1e-4f);
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(NodeType::kPlayClip, pBlend->GetChildB()->GetType());
		{
			SharedPtr<PlayClipInstance> pB((PlayClipInstance*)pBlend->GetChildB().GetPtr());
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(kfStep * 0.5f + kfStep, pB->GetCurrentTime(), 1e-4f);
		}
	}
}

static void Decompose(
	const Matrix2x3& m,
	Matrix2D& rmPreRotation,
	Matrix2D& rmRotation,
	Vector2D& rvTranslation)
{
	if (!Matrix2x3::Decompose(m, rmPreRotation, rmRotation, rvTranslation))
	{
		rmPreRotation = Matrix2D::Zero();
		rmRotation = Matrix2D::Zero();
		rvTranslation = m.GetTranslation();
	}
}

void Animation2DTest::TestTPose()
{
	auto p(Animation2D::Manager::Get()->CreateInstance(
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestNetworkNoLoop.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test1/TestAnimation2D.son"),
		SharedPtr<Animation::EventInterface>()));
	WaitForReady(p);

	auto const& state = p->GetState();
	auto const& deforms = state.GetDeforms();
	auto const& drawOrder = state.GetDrawOrder();
	auto const& palette = state.GetSkinningPalette();
	auto const& slots = state.GetSlots();
	auto const& slotsData = p->GetData()->GetSlots();

	// Load the expected data.
	Animation2DTestExpectedValues values;
	String sExpected;
	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/Expected/TPose.json"),
		sExpected));
	SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeFromString(
		sExpected,
		&values));

	SEOUL_UNITTESTING_ASSERT_EQUAL(14u, drawOrder.GetSize());
	for (UInt32 i = 0u; i < slots.GetSize(); ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(values.m_vDrawOrder[i], drawOrder[i]);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(13u, palette.GetSize());
	for (UInt32 i = 0u; i < palette.GetSize(); ++i)
	{
		Matrix2D mPreRotation0, mPreRotation1;
		Matrix2D mRotation0, mRotation1;
		Vector2D vTranslation0, vTranslation1;

		Decompose(values.m_vSkinning[i], mPreRotation0, mRotation0, vTranslation0);
		Decompose(palette[i], mPreRotation1, mRotation1, vTranslation1);

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(mPreRotation0, mPreRotation1, 1e-2f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(mRotation0, mRotation1, 1e-1f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(vTranslation0, vTranslation1, 0.9f);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(14u, slots.GetSize());
	for (UInt32 i = 0u; i < slots.GetSize(); ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(values.m_vSlots[i], slots[i]);
	}

	for (UInt32 i = 0u; i < slots.GetSize(); ++i)
	{
		auto const key(Animation2D::DeformKey(HString("default"), slotsData[i].m_Id, slots[i].m_AttachmentId));

		CheckedPtr<Animation2D::DataInstance::DeformData> p;
		if (!deforms.GetValue(key, p))
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, values.m_vvVertices[i].GetSize());
		}
		else
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(values.m_vvVertices[i].GetSize() * 2u, p->GetSize());
			for (UInt32 j = 0u; j < values.m_vvVertices[i].GetSize(); ++j)
			{
				Vector2D const vActual((*p)[j * 2u + 0u], (*p)[j * 2u + 1u]);
				SEOUL_UNITTESTING_ASSERT_EQUAL(values.m_vvVertices[i][j], vActual);
			}
		}
	}
}

void Animation2DTest::TestTCRegressionFrame0()
{
	TestFrameCommon(
		0.0f,
		"Frame0TCRegression",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TCRegression.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test4/TestAnimation2D.son"));
}

void Animation2DTest::TestTCRegressionFramePoint25()
{
	TestFrameCommon(
		0.25f,
		"FramePoint25TCRegression",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TCRegression.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test4/TestAnimation2D.son"));
}

void Animation2DTest::TestTCRegressionFramePoint5()
{
	TestFrameCommon(
		0.5f,
		"FramePoint5TCRegression",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TCRegression.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test4/TestAnimation2D.son"));
}

void Animation2DTest::TestTCRegressionFrame1()
{
	TestFrameCommon(
		1.0f,
		"Frame1TCRegression",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TCRegression.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test4/TestAnimation2D.son"));
}

void Animation2DTest::TestTCRegressionFrame1Point5()
{
	TestFrameCommon(
		1.5f,
		"Frame1Point5TCRegression",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TCRegression.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test4/TestAnimation2D.son"));
}

void Animation2DTest::TestTCHibanaFrame0()
{
	TestFrameCommon(
		0.0f,
		"Frame0TCHibana",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestTransformConstraint.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test5/TestAnimation2D.son"));
}

void Animation2DTest::TestTCHibanaFramePoint25()
{
	TestFrameCommon(
		0.25f,
		"FramePoint25TCHibana",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestTransformConstraint.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test5/TestAnimation2D.son"));
}

void Animation2DTest::TestTCHibanaFramePoint5()
{
	TestFrameCommon(
		0.5f,
		"FramePoint5TCHibana",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestTransformConstraint.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test5/TestAnimation2D.son"));
}

void Animation2DTest::TestTCHibanaFrame1()
{
	TestFrameCommon(
		1.0f,
		"Frame1TCHibana",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestTransformConstraint.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test5/TestAnimation2D.son"));
}

void Animation2DTest::TestTCHibanaFrame1Point5()
{
	TestFrameCommon(
		1.5f,
		"Frame1Point5TCHibana",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/TestTransformConstraint.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test5/TestAnimation2D.son"));
}

void Animation2DTest::TestChuihFrame0()
{
	TestFrameCommon(
		0.0f,
		"ChuihAnimProblem/Frame0",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/ChuihBrokenAnim.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test6/TestAnimation2D.son"));
}

void Animation2DTest::TestChuihFramePoint25()
{
	TestFrameCommon(
		0.25f,
		"ChuihAnimProblem/FramePoint25",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/ChuihBrokenAnim.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test6/TestAnimation2D.son"));
}

void Animation2DTest::TestChuihFramePoint5()
{
	TestFrameCommon(
		0.5f,
		"ChuihAnimProblem/FramePoint5",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/ChuihBrokenAnim.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test6/TestAnimation2D.son"));
}

void Animation2DTest::TestChuihFrame1()
{
	TestFrameCommon(
		1.0f,
		"ChuihAnimProblem/Frame1",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/ChuihBrokenAnim.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test6/TestAnimation2D.son"));
}

void Animation2DTest::TestChuihFrame1Point5()
{
	TestFrameCommon(
		1.5f,
		"ChuihAnimProblem/Frame1Point5Loop",
		FilePath::CreateConfigFilePath("UnitTests/Animation2D/ChuihBrokenAnim.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test6/TestAnimation2D.son"));
}

static HString ResolveAttachmentId(
	const Animation2D::DataDefinition::Skins& tSkins,
	HString skinId,
	HString slotId,
	HString parentAttachmentId)
{
	if (parentAttachmentId.IsEmpty())
	{
		return parentAttachmentId;
	}

	auto pSkin = tSkins.Find(skinId);
	SEOUL_UNITTESTING_ASSERT(nullptr != pSkin);
	auto pSlot = pSkin->Find(slotId);
	if (nullptr == pSlot)
	{
		return parentAttachmentId;
	}
	auto ppAttachment = pSlot->Find(parentAttachmentId);
	if (nullptr == ppAttachment)
	{
		return parentAttachmentId;
	}
	SEOUL_UNITTESTING_ASSERT(ppAttachment->IsValid());

	if ((*ppAttachment)->GetType() == Animation2D::AttachmentType::kLinkedMesh)
	{
		SharedPtr<Animation2D::LinkedMeshAttachment> p((Animation2D::LinkedMeshAttachment*)ppAttachment->GetPtr());
		return p->GetParentId();
	}
	else
	{
		return parentAttachmentId;
	}
}

// Runs tests against a large number of samples. Due to the brittleness of
// the data (and due to the expected data being generated from the "before" state),
// this test is off by default, it only checks it if has been run before an update.
//
// To run, first enable SEOUL_GEN_TEST_DATA with the old state of the code. This will
// generate an expected database. Then run SEOUL_RUN_TEST_DATA with the new state of
// the code. Once you're satisfied with the results and verification of new code,
// unset both macros and update the expected version to the new target.
#define SEOUL_GEN_TEST_DATA 0
#define SEOUL_RUN_TEST_DATA 0
#if !SEOUL_GEN_TEST_DATA && !SEOUL_RUN_TEST_DATA
static Byte const* skLastTestSpineVersion = "3.8.79";
#endif

#if SEOUL_GEN_TEST_DATA || SEOUL_RUN_TEST_DATA
static void ComprehensiveFileSystems()
{
	FileManager::Get()->RegisterFileSystem<PackageFileSystem>(
		Path::Combine(GamePaths::Get()->GetBaseDir(), "Data/PC_BaseContent.sar"));
}

static void PlayTo(Animation2D::ClipInstance& inst, Float& fLastTime, Float fNextTime, Bool bLoop)
{
	auto const fMaxTime = inst.GetMaxTime();
	if (bLoop && fMaxTime > 1e-4f)
	{
		while (fNextTime > fMaxTime)
		{
			SEOUL_ASSERT(fLastTime <= fMaxTime);
			if (fMaxTime > fLastTime)
			{
				inst.EvaluateRange(fLastTime, fMaxTime, 1.0f);
			}

			fNextTime -= fMaxTime;
			fLastTime = 0.0f;
		}
	}
	else
	{
		fNextTime = Min(fNextTime, fMaxTime);
	}

	if (fNextTime > fLastTime)
	{
		inst.EvaluateRange(fLastTime, fNextTime, 1.0f);
	}

	inst.Evaluate(fNextTime, 1.0f, false);
}

namespace Reflection
{

namespace TableDetail
{

template<>
struct ConstructTableKey<FilePath, false> SEOUL_SEALED
{
	static inline Bool FromHString(HString key, FilePath& rOut)
	{
		rOut.SetDirectory(GameDirectory::kContent);
		rOut.SetRelativeFilenameWithoutExtension(key);
		rOut.SetType(FileType::kAnimation2D);
		return true;
	}

	static inline Bool ToHString(const FilePath& key, HString& rOut)
	{
		rOut = key.GetRelativeFilenameWithoutExtension();
		return true;
	}
};

}

}

SEOUL_SPEC_TEMPLATE_TYPE(DefaultHashTableKeyTraits<FilePath>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2DTestExpectedValues>)
SEOUL_SPEC_TEMPLATE_TYPE(HashTable<HString, Vector<Animation2DTestExpectedValues>>)
SEOUL_SPEC_TEMPLATE_TYPE(HashTable<FilePath, HashTable<HString, Vector<Animation2DTestExpectedValues>>>)
#endif

void Animation2DTest::TestComprehensive()
{
#if SEOUL_GEN_TEST_DATA
	m_pHelper.Reset();
	m_pHelper.Reset(SEOUL_NEW(MemoryBudgets::Developer) UnitTestsEngineHelper(ComprehensiveFileSystems));

	HashTable<FilePath, HashTable<HString, Vector<Animation2DTestExpectedValues> > > tData;
	{
		FilePath dirPath;
		dirPath.SetDirectory(GameDirectory::kContent);
		Vector<String> vs;
		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			GamePaths::Get()->GetSourceDir(),
			vs,
			false,
			true,
			FileTypeToSourceExtension(FileType::kAnimation2D)));
		for (auto const& s : vs)
		{
			auto const filePath = FilePath::CreateContentFilePath(s);

			auto hData(Animation2D::Manager::Get()->GetData(filePath));
			Content::LoadManager::Get()->WaitUntilLoadIsFinished(hData);
			auto pData(hData.GetPtr());

			SharedPtr<Animation2DTestEventInterface> pInterf(SEOUL_NEW(MemoryBudgets::Animation2D) Animation2DTestEventInterface);
			auto const& tClips(pData->GetClips());

			HashTable<HString, Vector<Animation2DTestExpectedValues> > tEntry;
			for (auto const& pair : tClips)
			{
				auto const pClip(pair.Second);
				Float fMaxTime = 0.0f;
				{
					Animation2D::DataInstance instance(pData, pInterf);
					Animation2D::ClipInstance clipInst(instance, pClip, Animation::ClipSettings());
					fMaxTime = clipInst.GetMaxTime();
				}

				Vector<Animation2DTestExpectedValues> vEntries;
				static const Int kiSteps = 10;
				for (Int iStep = 0; iStep <= kiSteps; ++iStep)
				{
					Animation2D::DataInstance instance(pData, pInterf);
					Animation2D::ClipInstance clipInst(instance, pClip, Animation::ClipSettings());

					// Now advance fDeltaTimeInSeconds seconds into the animation.
					Float fLastTime = 0.0f;
					Float fTargetTime = Lerp(0.0f, fMaxTime, Clamp((Float)iStep / (Float)kiSteps, 0.0f, 1.0f));
					PlayTo(clipInst, fLastTime, fTargetTime, true);
					instance.ApplyCache();
					instance.PoseSkinningPalette();

					auto const& deforms = instance.GetDeforms();
					auto const& drawOrder = instance.GetDrawOrder();
					auto const& palette = instance.GetSkinningPalette();
					auto const& slots = instance.GetSlots();
					auto const& slotsData = pData->GetSlots();

					Animation2DTestExpectedValues entry;
					for (UInt32 i = 0u; i < slots.GetSize(); ++i)
					{
						entry.m_vDrawOrder.PushBack(drawOrder[i]);
					}

					for (UInt32 i = 0u; i < palette.GetSize(); ++i)
					{
						entry.m_vSkinning.PushBack(palette[i]);
					}

					for (UInt32 i = 0u; i < slots.GetSize(); ++i)
					{
						entry.m_vSlots.PushBack(slots[i]);
					}

					entry.m_vvVertices.Resize(slots.GetSize());
					for (UInt32 i = 0u; i < slots.GetSize(); ++i)
					{
						HString const skinId("default");
						HString const slotId(slotsData[i].m_Id);
						HString const attachmentId(ResolveAttachmentId(
							pData->GetSkins(),
							skinId,
							slotId,
							slots[i].m_AttachmentId));

						auto const key(Animation2D::DeformKey(skinId, slotId, attachmentId));

						CheckedPtr<Animation2D::DataInstance::DeformData> p;
						if (deforms.GetValue(key, p))
						{
							for (UInt32 j = 1u; j < p->GetSize(); j += 2u)
							{
								entry.m_vvVertices[i].PushBack(Vector2D((*p)[j - 1], (*p)[j]));
							}
						}
					}

					vEntries.PushBack(entry);
				}

				tEntry.Insert(pair.First, vEntries);
			}

			tData.Insert(filePath, tEntry);
		}
	}

	DataStore ds;
	SEOUL_UNITTESTING_ASSERT(Reflection::SerializeToDataStore(&tData, ds));
	auto const sFile(FilePath::CreateConfigFilePath(R"(UnitTests\Animation2D\Comprehensive\Expected.dat)").GetAbsoluteFilename());
	Directory::CreateDirPath(Path::GetDirectoryName(sFile));
	DiskSyncFile file(
		sFile,
		File::kWriteTruncate);
	SEOUL_UNITTESTING_ASSERT(ds.Save(file, keCurrentPlatform));
#elif SEOUL_RUN_TEST_DATA
	m_pHelper.Reset();
	m_pHelper.Reset(SEOUL_NEW(MemoryBudgets::Developer) UnitTestsEngineHelper(ComprehensiveFileSystems));

	HashTable<FilePath, HashTable<HString, Vector<Animation2DTestExpectedValues> > > tData;
	{
		{
			DiskSyncFile file(
				FilePath::CreateConfigFilePath(R"(UnitTests\Animation2D\Comprehensive\Expected.dat)").GetAbsoluteFilename(),
				File::kRead);
			DataStore ds;
			SEOUL_UNITTESTING_ASSERT(ds.Load(file));
			SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(ContentKey(), ds, ds.GetRootNode(), &tData));
		}

		for (auto const& pair : tData)
		{
			auto const filePath = pair.First;

			auto hData(Animation2D::Manager::Get()->GetData(filePath));
			Content::LoadManager::Get()->WaitUntilLoadIsFinished(hData);
			auto pData(hData.GetPtr());

			SharedPtr<Animation2DTestEventInterface> pInterf(SEOUL_NEW(MemoryBudgets::Animation2D) Animation2DTestEventInterface);
			auto const& tClips(pData->GetClips());

			auto const& tEntry = pair.Second;
			for (auto const& pair : tClips)
			{
				auto const pClip(pair.Second);
				Float fMaxTime = 0.0f;
				{
					Animation2D::DataInstance instance(pData, pInterf);
					Animation2D::ClipInstance clipInst(instance, pClip, Animation::ClipSettings());
					fMaxTime = clipInst.GetMaxTime();
				}

				auto const& vEntries = *tEntry.Find(pair.First);
				UInt32 uEntry = 0u;
				static const Int kiSteps = 10;
				for (Int iStep = 0; iStep <= kiSteps; ++iStep)
				{
					auto const& values = vEntries[uEntry];
					++uEntry;

					Animation2D::DataInstance instance(pData, pInterf);
					Animation2D::ClipInstance clipInst(instance, pClip, Animation::ClipSettings());

					// Now advance fDeltaTimeInSeconds seconds into the animation.
					Float fLastTime = 0.0f;
					Float fTargetTime = Lerp(0.0f, fMaxTime, Clamp((Float)iStep / (Float)kiSteps, 0.0f, 1.0f));
					PlayTo(clipInst, fLastTime, fTargetTime, true);
					instance.ApplyCache();
					instance.PoseSkinningPalette();

					auto const& deforms = instance.GetDeforms();
					auto const& drawOrder = instance.GetDrawOrder();
					auto const& palette = instance.GetSkinningPalette();
					auto const& slots = instance.GetSlots();
					auto const& slotsData = pData->GetSlots();

					SEOUL_UNITTESTING_ASSERT_EQUAL(values.m_vDrawOrder.GetSize(), drawOrder.GetSize());
					for (UInt32 i = 0u; i < slots.GetSize(); ++i)
					{
						SEOUL_UNITTESTING_ASSERT_EQUAL(values.m_vDrawOrder[i], drawOrder[i]);
					}

					SEOUL_UNITTESTING_ASSERT_EQUAL(values.m_vSkinning.GetSize(), palette.GetSize());
					for (UInt32 i = 0u; i < palette.GetSize(); ++i)
					{
						Matrix2D mPreRotation0, mPreRotation1;
						Matrix2D mRotation0, mRotation1;
						Vector2D vTranslation0, vTranslation1;

						Decompose(values.m_vSkinning[i], mPreRotation0, mRotation0, vTranslation0);
						Decompose(palette[i], mPreRotation1, mRotation1, vTranslation1);

						auto const fDegrees0 = RadiansToDegrees(mRotation0.DecomposeRotation());
						auto const fDegrees1 = RadiansToDegrees(mRotation1.DecomposeRotation());

						// Threshold here is pretty large, as the stock spine runtime uses approximations
						// for rotation (sin/cos), sqrt, and a few other things that we do not use.
						SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(mPreRotation0, mPreRotation1, 1e-1f);
						SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(fDegrees0, fDegrees1, 0.2f);
						SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(vTranslation0, vTranslation1, 0.9f);
					}

					SEOUL_UNITTESTING_ASSERT_EQUAL(values.m_vSlots.GetSize(), slots.GetSize());
					for (UInt32 i = 0u; i < slots.GetSize(); ++i)
					{
						// TODO: Always ok? Newer spine seems to prune some attachments
						// that are invalid/no-ops.
						if (!(values.m_vSlots[i] == slots[i]))
						{
							SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(HString(), values.m_vSlots[i].m_AttachmentId);
							SEOUL_UNITTESTING_ASSERT_EQUAL(HString(), slots[i].m_AttachmentId);
							SEOUL_UNITTESTING_ASSERT_EQUAL(values.m_vSlots[i].m_Color, slots[i].m_Color);
						}
						else
						{
							SEOUL_UNITTESTING_ASSERT_EQUAL(values.m_vSlots[i], slots[i]);
						}
					}

					for (UInt32 i = 0u; i < slots.GetSize(); ++i)
					{
						HString const skinId("default");
						HString const slotId(slotsData[i].m_Id);
						HString const attachmentId(ResolveAttachmentId(
							pData->GetSkins(),
							skinId,
							slotId,
							slots[i].m_AttachmentId));

						auto const key(Animation2D::DeformKey(skinId, slotId, attachmentId));

						CheckedPtr<Animation2D::DataInstance::DeformData> p;
						if (!deforms.GetValue(key, p))
						{
							SEOUL_UNITTESTING_ASSERT_EQUAL(0, values.m_vvVertices[i].GetSize());
						}
						else
						{
							SEOUL_UNITTESTING_ASSERT_EQUAL(values.m_vvVertices[i].GetSize() * 2u, p->GetSize());
							for (UInt32 j = 0u; j < values.m_vvVertices[i].GetSize(); ++j)
							{
								Vector2D const vExpected(values.m_vvVertices[i][j]);
								Vector2D const vActual((*p)[j * 2u + 0u], (*p)[j * 2u + 1u]);
								SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(vExpected, vActual, 1e-1f);
						}
					}
				}
			}
		}
	}
}
#else
	SEOUL_UNITTESTING_ASSERT_MESSAGE(
		0 == strcmp(skLastTestSpineVersion, Animation2D::ksExpectedSpineVersion.CStr()),
		"Spine has been updated to version %s, but Animation2DTest::TestComprehensive is still tagged with version %s"
		"See comment on SEOUL_GEN_TEST_DATA in Animation2DTest.cpp for the steps to regenerate and run this test prior to "
		"an update to Animation2D test code.", Animation2D::ksExpectedSpineVersion.CStr(), skLastTestSpineVersion);
#endif
}

void Animation2DTest::TestFrameCommon(
	Float fDeltaTimeInSeconds,
	Byte const* sName,
	Bool bLoop /*= false*/)
{
	TestFrameCommon(
		fDeltaTimeInSeconds,
		sName,
		FilePath::CreateConfigFilePath(bLoop ? "UnitTests/Animation2D/TestNetworkLoop.json" : "UnitTests/Animation2D/TestNetworkNoLoop.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test1/TestAnimation2D.son"));
}

void Animation2DTest::Test3FrameCommon(
	Float fDeltaTimeInSeconds,
	Byte const* sName,
	Bool bLoop /*= false*/)
{
	TestFrameCommon(
		fDeltaTimeInSeconds,
		sName,
		FilePath::CreateConfigFilePath(bLoop ? "UnitTests/Animation2D/TestNetworkLoop.json" : "UnitTests/Animation2D/TestNetworkNoLoop.json"),
		FilePath::CreateContentFilePath("Authored/UnitTests/Animation2D/Test3/TestAnimation2D.son"));
}

void Animation2DTest::TestFrameCommon(
	Float fDeltaTimeInSeconds,
	Byte const* sName,
	FilePath networkFilePath,
	FilePath dataFilePath)
{
	// Load the expected data.
	Animation2DTestExpectedValues values;
	String sExpected;
	SEOUL_UNITTESTING_ASSERT(FileManager::Get()->ReadAll(
		FilePath::CreateConfigFilePath(String::Printf("UnitTests/Animation2D/Expected/%s.json", sName)),
		sExpected));
	SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeFromString(
		sExpected,
		&values));

	// Evaluation in spine is non-deterministic prior to the start of a timeline. e.g.
	// if draw order changes start at time 0.25, that curve is not applied at all
	// prior to the start of the curve. As a result, the exact state of an instance
	// is dependent on timesteps and whether the animation loops or not. For unit tests,
	// we need to advance using the same pattern that was used in our harness for
	// generating test data.
	static const Float32 kafSteps[] = { 0.0f, 0.25f, 0.25f, 0.5f, 0.5f };

	auto p(Animation2D::Manager::Get()->CreateInstance(
		networkFilePath,
		dataFilePath,
		SharedPtr<Animation::EventInterface>()));
	WaitForReady(p);

	// Now advance fDeltaTimeInSeconds seconds into the animation.
	m_pHelper->Tick();
	{
		Float fAccum = 0.0f;
		for (UInt32 i = 0u; i < SEOUL_ARRAY_COUNT(kafSteps); ++i)
		{
			Float const fStep = Min(kafSteps[i], fDeltaTimeInSeconds - fAccum);

			m_pManager->Tick(fStep);
			p->Tick(fStep);
			fAccum += fStep;

			if (fAccum >= fDeltaTimeInSeconds)
			{
				break;
			}
		}
	}

	auto const& state = p->GetState();
	auto const& deforms = state.GetDeforms();
	auto const& drawOrder = state.GetDrawOrder();
	auto const& palette = state.GetSkinningPalette();
	auto const& slots = state.GetSlots();
	auto const& slotsData = p->GetData()->GetSlots();

	SEOUL_UNITTESTING_ASSERT_EQUAL(values.m_vDrawOrder.GetSize(), drawOrder.GetSize());
	for (UInt32 i = 0u; i < slots.GetSize(); ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(values.m_vDrawOrder[i], drawOrder[i]);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(values.m_vSkinning.GetSize(), palette.GetSize());
	for (UInt32 i = 0u; i < palette.GetSize(); ++i)
	{
		Matrix2D mPreRotation0, mPreRotation1;
		Matrix2D mRotation0, mRotation1;
		Vector2D vTranslation0, vTranslation1;

		Decompose(values.m_vSkinning[i], mPreRotation0, mRotation0, vTranslation0);
		Decompose(palette[i], mPreRotation1, mRotation1, vTranslation1);

		// Threshold here is pretty large, as the stock spine runtime uses approximations
		// for rotation (sin/cos), sqrt, and a few other things that we do not use.
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(mPreRotation0, mPreRotation1, 1e-1f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(mRotation0, mRotation1, 0.2f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(vTranslation0, vTranslation1, 0.9f);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(values.m_vSlots.GetSize(), slots.GetSize());
	for (UInt32 i = 0u; i < slots.GetSize(); ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(values.m_vSlots[i], slots[i]);
	}

	for (UInt32 i = 0u; i < slots.GetSize(); ++i)
	{
		HString const skinId("default");
		HString const slotId(slotsData[i].m_Id);
		HString const attachmentId(ResolveAttachmentId(
			p->GetData()->GetSkins(),
			skinId,
			slotId,
			slots[i].m_AttachmentId));

		auto const key(Animation2D::DeformKey(skinId, slotId, attachmentId));

		CheckedPtr<Animation2D::DataInstance::DeformData> p;
		if (!deforms.GetValue(key, p))
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, values.m_vvVertices[i].GetSize());
		}
		else
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(values.m_vvVertices[i].GetSize() * 2u, p->GetSize());
			for (UInt32 j = 0u; j < values.m_vvVertices[i].GetSize(); ++j)
			{
				Vector2D const vExpected(values.m_vvVertices[i][j]);
				Vector2D const vActual((*p)[j * 2u + 0u], (*p)[j * 2u + 1u]);
				SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(vExpected, vActual, 1e-1f);
			}
		}
	}
}

void Animation2DTest::WaitForReady(const SharedPtr<Animation2D::NetworkInstance>& p)
{
	Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
	while (!p->IsReady())
	{
		SEOUL_UNITTESTING_ASSERT_GREATER_EQUAL(10.0f, SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks));

		// Simulate a 60 FPS frame so we're not starving devices with not many cores.
		auto const iBegin = SeoulTime::GetGameTimeInTicks();
		m_pHelper->Tick();
		m_pManager->Tick(0.0f);
		p->CheckState();
		auto const iEnd = SeoulTime::GetGameTimeInTicks();
		auto const uSleep = (UInt32)Floor(Clamp(SeoulTime::ConvertTicksToMilliseconds(iEnd - iBegin), 0.0, 17.0));
		Thread::Sleep(uSleep);
	}
}

#endif // /#if SEOUL_WITH_ANIMATION_2D

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
