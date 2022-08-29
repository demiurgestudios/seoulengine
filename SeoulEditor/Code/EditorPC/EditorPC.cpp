/**
 * \file EditorPC.cpp
 * \brief Defines the entry point for the PC Editor
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "CrashManager.h"
#include "D3DCommonDeviceSettings.h"
#include "DiskFileSystem.h"
#include "EditorMain.h"
#include "EditorPCResource.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "PCEngineDefault.h"
#include "Prereqs.h"
#include "ScopedAction.h"
#include "SeoulTime.h"
#include "StringUtil.h"
#include "Thread.h"

namespace Seoul
{

D3DDeviceEntry GetD3D11DeviceWindowEntry();

/** Get the Editor's base directory - the folder that contains the Editor executable. */
static String GetEditorBaseDirectoryPath()
{
	WCHAR aBuffer[MAX_PATH];
	SEOUL_VERIFY(0u != ::GetModuleFileNameW(nullptr, aBuffer, SEOUL_ARRAY_COUNT(aBuffer)));

	// Resolve the exact path to the editor binaries directory.
	auto const sEditorPath = Path::GetExactPathName(Path::GetDirectoryName(WCharTToUTF8(aBuffer)));

	return sEditorPath;
}

/** Get the App's base directory - we use the app's base directory for GamePaths, and override a minimal set of editor specific settings. */
static String GetBaseDirectoryPath()
{
	auto const sEditorPath = GetEditorBaseDirectoryPath();

	// Now resolve the App directory using assumed directory structure.
	auto const sPath = Path::GetExactPathName(Path::Combine(
		Path::GetDirectoryName(sEditorPath, 5),
		Path::Combine("App", "Binaries", "PC", "Developer", "x64")));
	return sPath;
}

/**
 * \brief Global hook, called by FileManager as early as possible during initialization,
 * to give us a chance to hook up our file systems before any file requests are made.
 */
static void OnInitializeFileSystems()
{
	// Standard disk access.
	FileManager::Get()->RegisterFileSystem<DiskFileSystem>();

	// Remap of Data/Config into the Editor folder for specific overrides.
	auto const sEditorPath = GetEditorBaseDirectoryPath();

	// Now resolve the Editor directory - this is where the Editor's Data
	// folder is located.
	auto const sBaseDir = Path::GetDirectoryName(sEditorPath, 4);

	// Config remap.
	{
		auto const sPath = Path::GetExactPathName(Path::Combine(
			sBaseDir,
			Path::Combine("Data", "Config")));

		// Setup the remap filesystem.
		FilePath dirPath;
		dirPath.SetDirectory(GameDirectory::kConfig);
		FileManager::Get()->RegisterFileSystem<RemapDiskFileSystem>(
			dirPath,
			sPath,
			true);
	}
}

/**
 * \brief Windows main wrapper
 *
 * Initialize the editor.
 *
 * @param[in] hInstance Current instance
 * @param[in] hPrevInstance Previous instance
 * @param[in] lpCmdLine Input command line
 * @param[in] nShowCmd Specifies how the window should appear (maximized, minimized, etc)
 *
 * @return    Returns an integer exit code
 */
int RealWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR lpCmdLine,
	int nShowCmd)
{
	// Set abort behavior - fully enabled in non-ship builds,
	// fully disabled in ship builds.
#if SEOUL_SHIP
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#else
	_set_abort_behavior(0xFFFFFFFF, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif

	// Hook up a callback that will be invoked when the FileSystem is starting up,
	// so we can configure the game's packages before any file requests are made.
	g_pInitializeFileSystemsCallback = OnInitializeFileSystems;

	// Initialize SeoulTime
	SeoulTime::MarkGameStartTick();

	// Mark that we're now in the main function.
	auto const inMain(MakeScopedAction(&BeginMainFunction, &EndMainFunction));

	// Setup some game specific paths before initializing Engine and Core.
	GamePaths::SetUserConfigJsonFileName("editor_config.json");
	GamePaths::SetRelativeSaveDirPath(SOEUL_APP_SAVE_COMPANY_DIR "\\SeoulEditor\\");

	// Enable run-time memory check for debug builds.
#if SEOUL_DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	// Set the main thread to the current thread.
	SetMainThreadId(Thread::GetThisThreadId());

	PCEngineSettings settings;
	settings.m_RenderDeviceSettings.m_hInstance = hInstance;
	settings.m_sBaseDirectoryPath = GetBaseDirectoryPath();

	// The editor is expecting to use COM functionality for PCEngine,
	// so we want it warm started on the main thread.
	settings.m_bWarmStartCOM = true;

	// Ordered list of devices we support. Highest priority first.
	settings.m_RenderDeviceSettings.m_vEntries.PushBack(GetD3D11DeviceWindowEntry());
	settings.m_RenderDeviceSettings.m_aMouseCursors[(UInt32)MouseCursor::kArrow] = (HCURSOR)::LoadImage(hInstance, MAKEINTRESOURCE(IDC_CURSOR_ARROW), IMAGE_CURSOR, 0, 0, 0);
	settings.m_RenderDeviceSettings.m_aMouseCursors[(UInt32)MouseCursor::kArrowLeftBottomRightTop] = (HCURSOR)::LoadImage(hInstance, MAKEINTRESOURCE(IDC_CURSOR_ARROW_LBRT), IMAGE_CURSOR, 0, 0, 0);
	settings.m_RenderDeviceSettings.m_aMouseCursors[(UInt32)MouseCursor::kArrowLeftRight] = (HCURSOR)::LoadImage(hInstance, MAKEINTRESOURCE(IDC_CURSOR_ARROW_LR), IMAGE_CURSOR, 0, 0, 0);
	settings.m_RenderDeviceSettings.m_aMouseCursors[(UInt32)MouseCursor::kArrowLeftTopRightBottom] = (HCURSOR)::LoadImage(hInstance, MAKEINTRESOURCE(IDC_CURSOR_ARROW_LTRB), IMAGE_CURSOR, 0, 0, 0);
	settings.m_RenderDeviceSettings.m_aMouseCursors[(UInt32)MouseCursor::kArrowUpDown] = (HCURSOR)::LoadImage(hInstance, MAKEINTRESOURCE(IDC_CURSOR_ARROW_UD), IMAGE_CURSOR, 0, 0, 0);
	settings.m_RenderDeviceSettings.m_aMouseCursors[(UInt32)MouseCursor::kIbeam] = (HCURSOR)::LoadImage(hInstance, MAKEINTRESOURCE(IDC_CURSOR_IBEAM), IMAGE_CURSOR, 0, 0, 0);
	settings.m_RenderDeviceSettings.m_aMouseCursors[(UInt32)MouseCursor::kMove] = (HCURSOR)::LoadImage(hInstance, MAKEINTRESOURCE(IDC_CURSOR_MOVE), IMAGE_CURSOR, 0, 0, 0);
	settings.m_RenderDeviceSettings.m_iApplicationIcon = IDI_PCLAUNCH;

	// Graphics minimum requirements
	settings.m_RenderDeviceSettings.m_uMinimumPixelShaderVersion = 2u;
	settings.m_RenderDeviceSettings.m_uMinimumVertexShaderVersion = 2u;

	// Startup, run, and shutdown.
	{
		NullCrashManager crashManager;
		PCEngineDefault engine(settings);
		engine.SetIPCPipeName("\\\\.\\Pipe\\SeoulEditor");
		engine.Initialize();

		// Multiple copy handling may trigger a quit
		// during initialize, so just skip everything else.
		if (!engine.WantsQuit())
		{
			Editor::Main main;
			main.Run();
		}

		engine.SetIPCMessageCallback(Seoul::ReceiveIPCMessageDelegate());
		engine.Shutdown();
	}

	return 0;
}

} // namespace Seoul

/**
 * Windows program entry point
 */
INT WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR lpCmdLine,
	int nShowCmd)
{
	return Seoul::RealWinMain(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
}
