/**
 * \file EngineCommands.cpp
 * \brief Cheat commands for Engine level functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ColorBlindViz.h"
#include "Engine.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "ReflectionDefine.h"
#include "Renderer.h"

namespace Seoul
{

#if SEOUL_ENABLE_CHEATS

enum class EngineCommandsTimeScale
{
	k0 = -5,
	k005 = -4,
	k025 = -3,
	k05 = -2,
	k09 = -1,
	k1 = 0,
	k2,
	k4,

	MIN = k0,
	MAX = k4,
};

SEOUL_BEGIN_ENUM(EngineCommandsTimeScale)
	SEOUL_ENUM_N("0", EngineCommandsTimeScale::k0)
	SEOUL_ENUM_N("0.05", EngineCommandsTimeScale::k005)
	SEOUL_ENUM_N("0.25", EngineCommandsTimeScale::k025)
	SEOUL_ENUM_N("0.5", EngineCommandsTimeScale::k05)
	SEOUL_ENUM_N("0.9", EngineCommandsTimeScale::k09)
	SEOUL_ENUM_N("1.0", EngineCommandsTimeScale::k1)
	SEOUL_ENUM_N("2.0", EngineCommandsTimeScale::k2)
	SEOUL_ENUM_N("4.0", EngineCommandsTimeScale::k4)
SEOUL_END_ENUM()

static inline EngineCommandsTimeScale DeriveScale(Double f)
{
	if (f <= 0.0) { return EngineCommandsTimeScale::k0; }
	else if (f <= 0.05) { return EngineCommandsTimeScale::k005; }
	else if (f <= 0.25) { return EngineCommandsTimeScale::k025; }
	else if (f <= 0.5) { return EngineCommandsTimeScale::k05; }
	else if (f <= 0.9) { return EngineCommandsTimeScale::k09; }
	else if (f <= 1.0) { return EngineCommandsTimeScale::k1; }
	else if (f <= 2.0) { return EngineCommandsTimeScale::k2; }
	else if (f <= 4.0) { return EngineCommandsTimeScale::k4; }
	else { return EngineCommandsTimeScale::k4; }
}

static inline Double ToScale(EngineCommandsTimeScale e)
{
	switch (e)
	{
	case EngineCommandsTimeScale::k0: return 0.0;
	case EngineCommandsTimeScale::k005: return 0.05;
	case EngineCommandsTimeScale::k025: return 0.25;
	case EngineCommandsTimeScale::k05: return 0.5;
	case EngineCommandsTimeScale::k09: return 0.9;
	case EngineCommandsTimeScale::k1: return 1.0;
	case EngineCommandsTimeScale::k2: return 2.0;
	case EngineCommandsTimeScale::k4: return 4.0;
	default:
		return 1.0;
	};
}

static Reflection::Any GetCurrentTimeScaleValue()
{
	return (Int)DeriveScale(Engine::Get()->GetDevOnlyGlobalTickScale());
}

static Reflection::Any GetCurrentColorBlindVizMode()
{
	return (Int)ColorBlindViz::GetMode();
}

#if SEOUL_ENABLE_MEMORY_TOOLING
static void LogMemoryDetailsPrintfLike(void* userData, const Byte* sFormat, ...)
{
	auto& rFile = *((BufferedSyncFile*)userData);
	va_list args;
	va_start(args, sFormat);
	rFile.VPrintf(sFormat, args);
	va_end(args);
}
#endif // SEOUL_ENABLE_MEMORY_TOOLING

/**
 * Cheat commands for Engine functionality.
 */
class EngineCommands SEOUL_SEALED
{
public:
	EngineCommands()
	{
		m_eLastNonZeroTimeScale = EngineCommandsTimeScale::k1;
	}

	~EngineCommands()
	{
	}

#if SEOUL_ENABLE_MEMORY_TOOLING
	void LogAllMemory()
	{
		String const sLogDirGetLogDir(GamePaths::Get()->GetLogDir());
		String const sFileName(String::Printf("MemoryInfo_All_%s.txt",
			WorldTime::GetUTCTime().ToLocalTimeString(true).CStr()));

		ScopedPtr<SyncFile> pFile;
		if (FileManager::Get()->OpenFile(
			Path::Combine(sLogDirGetLogDir, sFileName),
			File::kWriteTruncate,
			pFile) && pFile->CanWrite())
		{
			BufferedSyncFile file(pFile.Get(), false);
			MemoryManager::PrintMemoryDetails(MemoryBudgets::Unknown, LogMemoryDetailsPrintfLike, &file);
		}
	}

	void LogMemoryType(MemoryBudgets::Type eType)
	{
		String const sLogDirGetLogDir(GamePaths::Get()->GetLogDir());
		String const sFileName(String::Printf("MemoryInfo_%s_%s.txt",
			MemoryBudgets::ToString(eType),
			WorldTime::GetUTCTime().ToLocalTimeString(true).CStr()));

		ScopedPtr<SyncFile> pFile;
		if (FileManager::Get()->OpenFile(
			Path::Combine(sLogDirGetLogDir, sFileName),
			File::kWriteTruncate,
			pFile) && pFile->CanWrite())
		{
			BufferedSyncFile file(pFile.Get(), false);
			MemoryManager::PrintMemoryDetails(eType, LogMemoryDetailsPrintfLike, &file);
		}
	}
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

	void LogHStringTable()
	{
		HString::LogAllHStrings();
	}

	void SetColorBlindVizMode(ColorBlindVizMode eMode)
	{
		static const HString kConfig("ColorBlindConfig");
		static const HString kDefaultConfig("DefaultConfig");
		if (ColorBlindVizMode::kOff == eMode)
		{
			if (kConfig == Renderer::Get()->GetRendererConfigurationName())
			{
				Renderer::Get()->ReadConfiguration(
					Renderer::Get()->GetRendererConfigurationFilePath(),
					kDefaultConfig);
			}
		}
		else
		{
			if (kConfig != Renderer::Get()->GetRendererConfigurationName())
			{
				Renderer::Get()->ReadConfiguration(
					Renderer::Get()->GetRendererConfigurationFilePath(),
					kConfig);
			}
		}

		ColorBlindViz::SetMode(eMode);
	}

	void StepDownColorBlindVizMode()
	{
		auto e = ColorBlindViz::GetMode();
		e = (ColorBlindVizMode)Clamp((Int)e - 1, (Int)ColorBlindVizMode::MIN, (Int)ColorBlindVizMode::MAX);
		SetColorBlindVizMode(e);
	}

	void StepUpColorBlindVizMode()
	{
		auto e = ColorBlindViz::GetMode();
		e = (ColorBlindVizMode)Clamp((Int)e + 1, (Int)ColorBlindVizMode::MIN, (Int)ColorBlindVizMode::MAX);
		SetColorBlindVizMode(e);
	}

	void StepDownTimeScale()
	{
		auto e = DeriveScale(Engine::Get()->GetDevOnlyGlobalTickScale());
		e = (EngineCommandsTimeScale)Clamp((Int)e - 1, (Int)EngineCommandsTimeScale::MIN, (Int)EngineCommandsTimeScale::MAX);
		TimeScale(e);
	}

	void StepUpTimeScale()
	{
		auto e = DeriveScale(Engine::Get()->GetDevOnlyGlobalTickScale());
		e = (EngineCommandsTimeScale)Clamp((Int)e + 1, (Int)EngineCommandsTimeScale::MIN, (Int)EngineCommandsTimeScale::MAX);
		TimeScale(e);
	}

	void TimeScale(EngineCommandsTimeScale e)
	{
		if (e != EngineCommandsTimeScale::k0)
		{
			m_eLastNonZeroTimeScale = e;
		}
		Engine::Get()->SetDevOnlyGlobalTickScale(ToScale(e));
	}

	void TogglePause()
	{
		auto e = DeriveScale(Engine::Get()->GetDevOnlyGlobalTickScale());
		if (e == EngineCommandsTimeScale::k0)
		{
			e = m_eLastNonZeroTimeScale;
		}
		else
		{
			e = EngineCommandsTimeScale::k0;
		}
		TimeScale(e);
	}

private:
	EngineCommandsTimeScale m_eLastNonZeroTimeScale;

	SEOUL_DISABLE_COPY(EngineCommands);
}; // class EngineCommands

#if SEOUL_ENABLE_MEMORY_TOOLING
static const HString kVerboseMemoryLeakDetectionDisabled(
	"Verbose memory tooling is disabled. To enable, pass\n"
	"-verbose_memory_tooling on the command-line or set\n"
	"SEOUL_ENV_VERBOSE_MEMORY_TOOLING=true to your\n"
	"environment variables. Note that verbose tooling adds\n"
	"memory and runtime overhead and should generally be\n"
	"left disabled unless you are specifically investigating\n"
	"memory profiling.");
static HString VerboseMemoryLeakDetectionEnabled()
{
	return MemoryManager::GetVerboseMemoryLeakDetectionEnabled() ? HString() : kVerboseMemoryLeakDetectionDisabled;
}
#endif

SEOUL_BEGIN_TYPE(EngineCommands, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(CommandsInstance)

#if SEOUL_ENABLE_MEMORY_TOOLING
	SEOUL_METHOD(LogAllMemory)
		SEOUL_ATTRIBUTE(Category, "Engine")
		SEOUL_ATTRIBUTE(Description,
			"Write summary of all memory to a file.\n"
			"File will be named MemoryInfo_All_<timestamp>.txt\n"
			"and will be located in the Log directory.")
		SEOUL_ATTRIBUTE(DisplayName, "Log All Memory")
		SEOUL_ATTRIBUTE(CommandNeedsButton)
		SEOUL_ATTRIBUTE(CommandIsDisabled, VerboseMemoryLeakDetectionEnabled)
	SEOUL_METHOD(LogMemoryType)
		SEOUL_ATTRIBUTE(Category, "Engine")
		SEOUL_ATTRIBUTE(Description,
			"Write summary of a particular memory type to a file.\n"
			"File will be named MemoryInfo_<type>_<timestamp>.txt\n"
			"and will be located in the Log directory.")
			SEOUL_ATTRIBUTE(DisplayName, "Log Memory of Type")
		SEOUL_ATTRIBUTE(CommandNeedsButton)
		SEOUL_ATTRIBUTE(CommandIsDisabled, VerboseMemoryLeakDetectionEnabled)
#endif // /#if SEOUL_ENABLE_MEMORY_TOOLING
	SEOUL_METHOD(LogHStringTable)
		SEOUL_ATTRIBUTE(Category, "Engine")
		SEOUL_ATTRIBUTE(Description,
			"Logs out the HStringTable")
		SEOUL_ATTRIBUTE(DisplayName, "Log HString Table")
		SEOUL_ATTRIBUTE(CommandNeedsButton)

	SEOUL_METHOD(SetColorBlindVizMode)
		SEOUL_ATTRIBUTE(Category, "Rendering")
		SEOUL_ATTRIBUTE(Description,
			"Set visualization for various forms of color blindness.\n"
			"Deutanopia - Common\n"
			"Protanopia - Rare\n"
			"Tritanopia - Very Rare\n"
			"Achromatopsia - Extremely Rare\n")
		SEOUL_ATTRIBUTE(DisplayName, "Color Blind Visualization")
		SEOUL_ARG_ATTRIBUTE(0, GetCurrentValue, GetCurrentColorBlindVizMode)
	SEOUL_METHOD(StepDownColorBlindVizMode)
		SEOUL_ATTRIBUTE(Category, "Rendering")
		SEOUL_ATTRIBUTE(Description,
			"Advance the color viz mode to the previous mode.\n")
		SEOUL_ATTRIBUTE(DisplayName, "Previous Color Blind Mode")
	SEOUL_METHOD(StepUpColorBlindVizMode)
		SEOUL_ATTRIBUTE(Category, "Rendering")
		SEOUL_ATTRIBUTE(Description,
			"Advance the color viz mode to the next mode.\n")
		SEOUL_ATTRIBUTE(DisplayName, "Next Color Blind Mode")
	SEOUL_METHOD(StepDownTimeScale)
		SEOUL_ATTRIBUTE(Category, "Simulation")
		SEOUL_ATTRIBUTE(Description,
			"Decrease the time scale value by 1 step.\n")
		SEOUL_ATTRIBUTE(DisplayName, "Step Down Time Scale")
	SEOUL_METHOD(StepUpTimeScale)
		SEOUL_ATTRIBUTE(Category, "Simulation")
		SEOUL_ATTRIBUTE(Description,
			"Increase the time scale value by 1 step.\n")
		SEOUL_ATTRIBUTE(DisplayName, "Step Up Time Scale")
	SEOUL_METHOD(TimeScale)
		SEOUL_ATTRIBUTE(Category, "Simulation")
		SEOUL_ATTRIBUTE(Description,
			"Set slow-mo or high-speed simulation. This scales\n"
			"all simulation and rendering.")
		SEOUL_ATTRIBUTE(DisplayName, "Time Scale")
		SEOUL_ARG_ATTRIBUTE(0, GetCurrentValue, GetCurrentTimeScaleValue)
	SEOUL_METHOD(TogglePause)
		SEOUL_ATTRIBUTE(Category, "Simulation")
		SEOUL_ATTRIBUTE(Description,
			"Toggles the time scale between 0 and the last non-zero value.\n")
		SEOUL_ATTRIBUTE(DisplayName, "Toggle Pause")
SEOUL_END_TYPE()
#endif // /#if SEOUL_ENABLE_CHEATS

} // namespace Seoul
