/**
 * \file GamePatcherTest.cpp
 * \brief Test for the GamePatcher flow, makes sure that
 * all patchable types are applied correctly.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Prereqs.h"

#if SEOUL_WITH_FMOD
#define SEOUL_TEST_FMOD 1
#else
#define SEOUL_TEST_FMOD 0
#endif

#if SEOUL_WITH_ANIMATION_2D
#include "Animation2DManager.h"
#endif
#include "AnimationNetworkDefinitionManager.h"
#include "AnimationNodeDefinition.h"
#include "AnimationPlayClipDefinition.h"
#include "EffectManager.h"
#include "FalconMovieClipInstance.h"
#include "FalconTexture.h"
#if SEOUL_TEST_FMOD
#include "FMODSoundManager.h"
#endif // /SEOUL_TEST_FMOD
#include "FxManager.h"
#include "GameAuthManager.h"
#include "GamePatcher.h"
#include "GamePatcherTest.h"
#include "GamePaths.h"
#include "GameScriptManager.h"
#include "HTTPServer.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "ScriptFunctionInvoker.h"
#include "SoundManager.h"
#include "TextureManager.h"
#include "UIManager.h"
#include "UIMovie.h"
#include "UIRenderer.h"
#include "UnitTesting.h"
#include "UnitTestsGameHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

#if SEOUL_TEST_FMOD
static const UInt32 kuExpectedCountRestart = 18u;
static const HString kEventName("UI/Buttons/Close");
#else
static const UInt32 kuExpectedCountRestart = 14u;
#endif // /#if SEOUL_TEST_FMOD

SEOUL_BEGIN_TYPE(GamePatcherTest, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(UnitTest, Attributes::UnitTest::kInstantiateForEach) // Want Engine and other resources to be recreated for each test.
	SEOUL_METHOD(TestNoPatch)
	SEOUL_METHOD(TestPatchA)
	SEOUL_METHOD(TestPatchB)
	SEOUL_METHOD(TestMulti)
	SEOUL_METHOD(TestRestartingAfterContentReload)
	SEOUL_METHOD(TestRestartingAfterGameConfigManager)
	SEOUL_METHOD(TestRestartingAfterPrecacheUrls)
SEOUL_END_TYPE();

class GamePatcherTestMovie SEOUL_SEALED : public UI::Movie
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(GamePatcherTestMovie);

	GamePatcherTestMovie()
	{
	}

	~GamePatcherTestMovie()
	{
	}

private:
	SEOUL_DISABLE_COPY(GamePatcherTestMovie);
}; // class GamePatcherTestMovie
SEOUL_BEGIN_TYPE(GamePatcherTestMovie, TypeFlags::kDisableCopy)
	SEOUL_PARENT(UI::Movie)
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

static void TestStats(const Game::PatcherDisplayStats& stats, UInt32 uMinExpectedReloadCount, UInt32 uMaxExpectedReloadCount)
{
	SEOUL_UNITTESTING_ASSERT_LESS_EQUAL(uMinExpectedReloadCount, stats.m_uReloadedFiles);
	SEOUL_UNITTESTING_ASSERT_GREATER_EQUAL(uMaxExpectedReloadCount, stats.m_uReloadedFiles);
}

#if SEOUL_TEST_FMOD
static Sound::Manager* CreateFMODHeadlessSoundManager()
{
	return SEOUL_NEW(MemoryBudgets::Audio) FMODSound::Manager(FMODSound::Manager::kHeadless);
}
#endif // /SEOUL_TEST_FMOD

GamePatcherTest::GamePatcherTest()
	: m_pHelper()
{
	Sound::Manager*(*soundManagerCreate)() = nullptr;
#if SEOUL_TEST_FMOD
	soundManagerCreate = CreateFMODHeadlessSoundManager;
#endif // /SEOUL_TEST_FMOD

	// Startup game.
	Byte const* sPrefix = GetPlatformPrefix();
	m_pHelper.Reset(SEOUL_NEW(MemoryBudgets::Developer) UnitTestsGameHelper(
		"http://localhost:8057",
		String::Printf("GamePatcher/%s_Config.sar", sPrefix).CStr(),
		String::Printf("GamePatcher/%s_Content.sar", sPrefix).CStr(),
		String::Printf("GamePatcher/%s_ContentUpdate.sar" /* Intentional, does not exist on disk */, sPrefix).CStr(),
		soundManagerCreate));
}

GamePatcherTest::~GamePatcherTest()
{
	m_pServer.Reset();
	m_pHelper.Reset();
}

void GamePatcherTest::TestNoPatchImpl(Bool bAllowRestart, UInt32 uMinExpectedReloadCount /*= 7u*/, UInt32 uMaxExpectedReloadCount /*= 9u*/)
{
	InitServer("no_patch_login.json", "no_patch_refresh.json");

	Game::PatcherDisplayStats stats;
	WaitForUIState(stats, "Screens", "TestMovie", bAllowRestart);
	TestStats(stats, uMinExpectedReloadCount, uMaxExpectedReloadCount);

	CheckAnimation2DData("Base");
	CheckAnimation2DNetwork("Base");
	CheckEffect("Base");
	CheckFx(0.999328f);
	CheckMovie("Base");
	CheckScript("Base");
	CheckScriptSetting("Base");
	CheckSound("Base");
	CheckTexture(32, 32);
}

void GamePatcherTest::TestPatchAImpl(Bool bAllowRestart, UInt32 uMinExpectedReloadCount /*= 7u*/, UInt32 uMaxExpectedReloadCount /*= 9u*/)
{
	InitServer("patch_a_login.json", "patch_a_refresh.json");

	Game::PatcherDisplayStats stats;
	WaitForUIState(stats, "Screens", "TestMovie2", bAllowRestart);
	TestStats(stats, uMinExpectedReloadCount, uMaxExpectedReloadCount);

	CheckAnimation2DData("PatchA");
	CheckAnimation2DNetwork("PatchA");
	CheckEffect("PatchA");
	CheckFx(1.999328f);
	CheckMovie("PatchA");
	CheckScript("PatchA");
	CheckScriptSetting("PatchA");
	CheckSound("PatchA");
	CheckTexture(64, 64);
}

void GamePatcherTest::TestPatchBImpl(Bool bAllowRestart, UInt32 uMinExpectedReloadCount /*= 7u*/, UInt32 uMaxExpectedReloadCount /*= 9u*/)
{
	InitServer("patch_b_login.json", "patch_b_refresh.json");

	Game::PatcherDisplayStats stats1;
	WaitForUIState(stats1, "Screens", "TestMovie3", bAllowRestart);
	TestStats(stats1, uMinExpectedReloadCount, uMaxExpectedReloadCount);
	Game::PatcherDisplayStats stats2;
	WaitForUIState(stats2, "Screens2", "TestMovie4", bAllowRestart);
	TestStats(stats2, 0u, 0u);

	CheckAnimation2DData("PatchB");
	CheckAnimation2DNetwork("PatchB");
	CheckEffect("PatchB");
	CheckFx(2.999328f);
	CheckMovie("PatchB");
	CheckScript("PatchB");
	CheckScriptSetting("PatchB");
	CheckSound("PatchB");
	CheckTexture(64, 32);
}

// This calls the other tests in a mixture of orders.
// They will all share the same game context, so
// it should properly stress "mid game" patches.
void GamePatcherTest::TestMulti()
{
	TestNoPatch();
	TestPatchA();
	TestPatchB();
	TestNoPatchImpl(false, 8u, 10u);
	TestPatchB();
	TestNoPatchImpl(false, 8u, 10u);
	TestPatchA();
	TestPatchA();
	TestNoPatch();
	TestPatchB();
	TestPatchAImpl(false, 8u, 10u);
	TestPatchB();
	TestPatchBImpl(false, 8u, 10u);
}

void GamePatcherTest::TestRestartingAfterContentReload()
{
	TestRestartingImpl((Int)Game::PatcherState::kWaitingForContentReload);
}

void GamePatcherTest::TestRestartingAfterGameConfigManager()
{
	TestRestartingImpl((Int)Game::PatcherState::kWaitingForGameConfigManager);
}

void GamePatcherTest::TestRestartingAfterPrecacheUrls()
{
	TestRestartingImpl((Int)Game::PatcherState::kWaitingForPrecacheUrls);
}

// Regression for a case where restarting mid patcher
// could prevent certain changes from being detected.
void UnitTestingHook_SetGamePatcherSimulateRestartState(Game::PatcherState eState);
void GamePatcherTest::TestRestartingImpl(Int iPatcherState)
{
	TestNoPatch();
	UnitTestingHook_SetGamePatcherSimulateRestartState((Game::PatcherState)iPatcherState);
	TestPatchAImpl(true, kuExpectedCountRestart, kuExpectedCountRestart);
	UnitTestingHook_SetGamePatcherSimulateRestartState((Game::PatcherState)iPatcherState);
	TestPatchBImpl(true, kuExpectedCountRestart, kuExpectedCountRestart);
}

void GamePatcherTest::CheckAnimation2DData(Byte const* sName)
{
#if SEOUL_WITH_ANIMATION_2D
	auto h(Animation2D::Manager::Get()->GetData(FilePath::CreateContentFilePath("Authored/Animation2Ds/Test/Test.son")));
	Content::LoadManager::Get()->WaitUntilLoadIsFinished(h);
	auto p(h.GetPtr());

	auto pEvents = p->GetEvents().Find(HString("Test"));
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(6.1f, pEvents->m_f, 1e-6f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-10, pEvents->m_i);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String(sName), pEvents->m_s);
#endif
}

void GamePatcherTest::CheckAnimation2DNetwork(Byte const* sName)
{
#if SEOUL_WITH_ANIMATION_2D
	auto h(Animation::NetworkDefinitionManager::Get()->GetNetwork(FilePath::CreateConfigFilePath("Animation2Ds/Test.json")));
	Content::LoadManager::Get()->WaitUntilLoadIsFinished(h);
	auto p(h.GetPtr());

	SEOUL_UNITTESTING_ASSERT_EQUAL(Animation::NodeType::kPlayClip, p->GetRoot()->GetType());
	SharedPtr<Animation::PlayClipDefinition> pPlayClip((Animation::PlayClipDefinition*)p->GetRoot().GetPtr());
	SEOUL_UNITTESTING_ASSERT_EQUAL(HString(sName), pPlayClip->GetName());
#endif
}

void GamePatcherTest::CheckEffect(Byte const* sName, Double fTimeOutSeconds /* = 1.0 */)
{
	auto h(EffectManager::Get()->GetEffect(FilePath::CreateContentFilePath("Authored/Effects/Test.fx")));
	Content::LoadManager::Get()->WaitUntilLoadIsFinished(h);
	auto p(h.GetPtr());

	Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
	while (p->GetState() != BaseGraphicsObject::kReset)
	{
		// Simulate a 60 FPS frame so we're not starving devices with not many cores.
		auto const iBegin = SeoulTime::GetGameTimeInTicks();
		m_pHelper->Tick();

		if (SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks) > fTimeOutSeconds)
		{
			SEOUL_UNITTESTING_ASSERT(false);
		}

		// Simulate a 60 FPS frame so we're not starving devices with not many cores.
		auto const iEnd = SeoulTime::GetGameTimeInTicks();
		auto const uSleep = (UInt32)Floor(Clamp(SeoulTime::ConvertTicksToMilliseconds(iEnd - iBegin), 0.0, 17.0));
		Thread::Sleep(uSleep);
	}

	SEOUL_UNITTESTING_ASSERT(p->HasParameterWithSemantic(HString(String::Printf("seoul_TestParameter%s", sName))));
}

void GamePatcherTest::CheckFx(Float fExpectedDuration, Double fTimeOutSeconds /* = 1.0 */)
{
#if SEOUL_WITH_FX_STUDIO
	Fx* p = nullptr;
	SEOUL_VERIFY(FxManager::Get()->GetFx(
		FilePath::CreateContentFilePath("Authored/Fx/TestFx.xfx"),
		p));
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, p);
	Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
	while (p->IsLoading())
	{
		// Simulate a 60 FPS frame so we're not starving devices with not many cores.
		auto const iBegin = SeoulTime::GetGameTimeInTicks();
		m_pHelper->Tick();

		if (SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks) > fTimeOutSeconds)
		{
			SEOUL_UNITTESTING_ASSERT(false);
		}

		// Simulate a 60 FPS frame so we're not starving devices with not many cores.
		auto const iEnd = SeoulTime::GetGameTimeInTicks();
		auto const uSleep = (UInt32)Floor(Clamp(SeoulTime::ConvertTicksToMilliseconds(iEnd - iBegin), 0.0, 17.0));
		Thread::Sleep(uSleep);
	}
	SEOUL_UNITTESTING_ASSERT(p->Start(Matrix4D(), 0u));
	FxProperties props;
	SEOUL_UNITTESTING_ASSERT(p->GetProperties(props));
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(fExpectedDuration, props.m_fDuration, 1e-6f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(false, props.m_bHasLoops);
	SafeDelete(p);
#endif // /#if SEOUL_WITH_FX_STUDIO
}

void GamePatcherTest::CheckMovie(Byte const* sName)
{
	m_pHelper->Tick();

	auto pMovie = (UI::Manager::Get()->GetStack()[0].m_pMachine->GetActiveState()->GetMovieStackHead());

	SharedPtr<Falcon::MovieClipInstance> pRoot;
	SEOUL_UNITTESTING_ASSERT(pMovie->GetRootMovieClip(pRoot));

	SharedPtr<Falcon::Instance> p;
	SEOUL_UNITTESTING_ASSERT(pRoot->GetChildByName(HString(String::Printf("mcTop%s", sName)), p));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Falcon::InstanceType::kMovieClip, p->GetType());
}

void GamePatcherTest::CheckScript(Byte const* sName)
{
	auto p(Game::ScriptManager::Get()->GetVm());

	HString const name(String("Global") + String(sName));
	Script::FunctionInvoker invoker(*p, name);
	SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

	String sValue;
	SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(sName, sValue);
}

void GamePatcherTest::CheckScriptSetting(Byte const* sName)
{
	auto p(Game::ScriptManager::Get()->GetVm());

	Script::FunctionInvoker invoker(*p, HString("GlobalSetting"));
	SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

	String sValue;
	SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(sName, sValue);
}

template <typename F>
static void WaitFor(UnitTestsGameHelper& helper, const F& func, Double fTimeOutSeconds = 30.0)
{
	Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
	while (!func())
	{
		helper.Tick();

		if (SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks) > fTimeOutSeconds)
		{
			SEOUL_UNITTESTING_FAIL("Ran out of time waiting for completion.");
		}
	}
}

void GamePatcherTest::CheckSound(Byte const* sName)
{
#if SEOUL_TEST_FMOD
	auto p(Sound::Manager::Get()->NewSoundEvent());
	Sound::Manager::Get()->AssociateSoundEvent(ContentKey(
		FilePath::CreateContentFilePath("Authored/Sound/App.fspro"),
		kEventName), *p);

	WaitFor(*m_pHelper, [&]() { return !p->IsLoading(); });
	SEOUL_UNITTESTING_ASSERT(p->Start(Vector3D::Zero(), Vector3D::Zero(), false));

	Int32 iExpectedMs = 0;
	if (0 == strcmp("Base", sName))
	{
		iExpectedMs = 311;
	}
	else if (0 == strcmp("PatchA", sName))
	{
		iExpectedMs = 205;
	}
	else
	{
		iExpectedMs = 105;
	}

	Int32 iLengthMs = 0;
	SEOUL_UNITTESTING_ASSERT(p->GetLengthInMilliseconds(iLengthMs));
	SEOUL_UNITTESTING_ASSERT_EQUAL(iExpectedMs, iLengthMs);

	SafeDelete(p);
#endif // /SEOUL_TEST_FMOD
}

void GamePatcherTest::CheckTexture(UInt32 uExpectedWidth, UInt32 uExpectedHeight)
{
	auto const filePath(FilePath::CreateContentFilePath("Authored/Textures/TestTexture.png"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(FileType::kTexture0, filePath.GetType());

	// Check via TextureManager
	auto h(TextureManager::Get()->GetTexture(filePath));
	Content::LoadManager::Get()->WaitUntilLoadIsFinished(h);
	auto p(h.GetPtr());
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(p, TextureManager::Get()->GetErrorTexture());
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(p, TextureManager::Get()->GetPlaceholderTexture());
	SEOUL_UNITTESTING_ASSERT(p.IsValid());
	SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectedWidth, p->GetWidth());
	SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectedHeight, p->GetHeight());

	// Check via UI::Renderer - note that this test depends on internal
	// knowledge of Falcon:
	// - the first resolve will usually fail, since the texture
	//   hasn't been loaded into the Falcon cache yet.
	FilePath largestMip(filePath);
	largestMip.SetType(FileType::LAST_TEXTURE_TYPE);
	auto hLargest(TextureManager::Get()->GetTexture(largestMip));
	Content::LoadManager::Get()->WaitUntilLoadIsFinished(hLargest);
	auto pLargest(hLargest.GetPtr());
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(pLargest, TextureManager::Get()->GetErrorTexture());
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(pLargest, TextureManager::Get()->GetPlaceholderTexture());
	SEOUL_UNITTESTING_ASSERT(pLargest.IsValid());
	SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectedWidth >> 4, pLargest->GetWidth());
	SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectedHeight >> 4, pLargest->GetHeight());

	Falcon::TextureReference ref;
	(void)UI::Manager::Get()->GetRenderer().ResolveTextureReference(
		(Float)Max(uExpectedHeight, uExpectedWidth),
		filePath,
		ref);
	(void)m_pHelper->Tick();
	SEOUL_UNITTESTING_ASSERT(UI::Manager::Get()->GetRenderer().ResolveTextureReference(
			(Float)Max(uExpectedHeight, uExpectedWidth),
			filePath,
			ref));

	Falcon::TextureMetrics metrics;
	SEOUL_UNITTESTING_ASSERT(ref.m_pTexture->ResolveTextureMetrics(metrics));

	UInt32 const uWidth = (UInt32)Round((Float)metrics.m_iWidth * ref.m_vAtlasScale.X);
	UInt32 const uHeight = (UInt32)Round((Float)metrics.m_iHeight * ref.m_vAtlasScale.Y);
	SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectedWidth, uWidth);
	SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectedHeight, uHeight);
}

void GamePatcherTest::InitServer(const String& sLoginRoot, const String& sRefreshRoot)
{
	Bool const bExists = m_pServer.IsValid();
	m_pServer.Reset();

	HTTP::ServerSettings settings;

	HTTP::ServerRewritePattern pattern;
	pattern.m_sFrom = "/v1/auth/login";
	pattern.m_sTo = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/GamePatcher", sLoginRoot);
	settings.m_vRewritePatterns.PushBack(pattern);
	pattern.m_sFrom = "/v1/auth/refresh";
	pattern.m_sTo = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/GamePatcher", sRefreshRoot);
	settings.m_vRewritePatterns.PushBack(pattern);

	Byte const* sPrefix = GetPlatformPrefix();
	static Byte const* s_kaFormats[] =
	{
		"ConfigUpdateA.sar",
		"ContentUpdateA.sar",
		"ConfigUpdateB.sar",
		"ContentUpdateB.sar",
	};

	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(s_kaFormats); ++i)
	{
		pattern.m_sFrom = String("/") + s_kaFormats[i];
		pattern.m_sTo = Path::Combine(
			GamePaths::Get()->GetConfigDir(),
			"UnitTests/GamePatcher",
			String::Printf("%s_%s", sPrefix, s_kaFormats[i]));
		settings.m_vRewritePatterns.PushBack(pattern);
	}

	settings.m_sRootDirectory = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/GamePatcher");
	settings.m_iPort = 8057;
	settings.m_iThreadCount = 1;
	m_pServer.Reset(SEOUL_NEW(MemoryBudgets::Developer) HTTP::Server(settings));

	// Trigger a restart if a server already existed.
	if (bExists)
	{
		UI::Manager::Get()->TriggerRestart(true);

		Game::PatcherDisplayStats stats;
		WaitForUIState(stats, "Screens", "Startup", false);
		TestStats(stats, 0u, 0u);
	}
}

void GamePatcherTest::WaitForUIState(
	Game::PatcherDisplayStats& rStats,
	Byte const* sMachine,
	Byte const* sState,
	Bool bAllowRestart,
	Double fTimeOutSeconds /* = 10.0 */)
{
	HString const machine(sMachine);
	HString const state(sState);

	rStats = Game::PatcherDisplayStats{};
	if (Game::Patcher::Get()) { rStats = Game::Patcher::Get()->GetStats(); }

	Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
	while (UI::Manager::Get()->GetStateMachineCurrentStateId(machine) != state)
	{
		// Simulate a 60 FPS frame so we're not starving devices with not many cores.
		auto const iBegin = SeoulTime::GetGameTimeInTicks();
		m_pHelper->Tick();

		// Refresh stats.
		if (Game::Patcher::Get()) { rStats = Game::Patcher::Get()->GetStats(); }

		// Sanity check the game patcher - should never enter the kRestarting state
		// unless explicitly expected.
		if (Game::Patcher::Get() && !bAllowRestart)
		{
			SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(
				Game::PatcherState::kRestarting,
				Game::Patcher::Get()->GetState());
		}

		if (SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks) > fTimeOutSeconds)
		{
			SEOUL_UNITTESTING_ASSERT(false);
		}

		// Simulate a 60 FPS frame so we're not starving devices with not many cores.
		auto const iEnd = SeoulTime::GetGameTimeInTicks();
		auto const uSleep = (UInt32)Floor(Clamp(SeoulTime::ConvertTicksToMilliseconds(iEnd - iBegin), 0.0, 17.0));
		Thread::Sleep(uSleep);
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
