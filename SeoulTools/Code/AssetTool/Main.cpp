/**
 * \file Main.cpp
 *
 * \brief AssetTool is a utility for displaying UI dependencies of
 * .swf files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "CommandLineArgWrapper.h"
#include "Compress.h"
#include "CoreSettings.h"
#include "Directory.h"
#include "DiskFileSystem.h"
#include "FalconFCNFile.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "HashTable.h"
#include "Logger.h"
#include "Path.h"
#include "ReflectionCommandLineArgs.h"
#include "ReflectionDefine.h"
#include "ReflectionScriptStub.h"
#include "ScopedAction.h"
#include "SeoulUtil.h"

#if SEOUL_PLATFORM_WINDOWS
#include "Platform.h"
#include <shellapi.h>
#endif

namespace Seoul
{

#define SEOUL_ERR(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)

/**
 * Root level command-line arguments - handled by reflection, can be
 * configured via the literal command-line, environment variables, or
 * a configuration file.
 */
class AssetToolCommandLineArgs SEOUL_SEALED
{
public:
	static Platform Platform;

private:
	AssetToolCommandLineArgs() = delete; // Static class.
	SEOUL_DISABLE_COPY(AssetToolCommandLineArgs);
};

Platform AssetToolCommandLineArgs::Platform = keCurrentPlatform;

SEOUL_BEGIN_TYPE(AssetToolCommandLineArgs, TypeFlags::kDisableNew | TypeFlags::kDisableCopy)
	SEOUL_CMDLINE_PROPERTY(Platform, 0, "platform", true)
		SEOUL_ATTRIBUTE(Description, "Platform to scan for .fcn files.")
SEOUL_END_TYPE()

// Use default core virtuals
const CoreVirtuals* g_pCoreVirtuals = &g_DefaultCoreVirtuals;

typedef HashTable<FilePath, Falcon::FCNFile::FCNDependencies, MemoryBudgets::Falcon> ImageToSwf;

static Bool GetFCNFileDependencies(FilePath filePath, Falcon::FCNFile::FCNDependencies& rv)
{
	void* pC = nullptr;
	UInt32 uC = 0u;
	if (!FileManager::Get()->ReadAll(filePath, pC, uC, 0u, MemoryBudgets::Cooking))
	{
		SEOUL_ERR("%s: dependency scan, failed reading UI Movie data from disk.", filePath.CStr());
		return false;
	}

	void* pU = nullptr;
	UInt32 uU = 0u;
	auto const deferred(MakeDeferredAction([&]() { MemoryManager::Deallocate(pU); }));
	Bool const bSuccess = ZSTDDecompress(pC, uC, pU, uU);
	MemoryManager::Deallocate(pC);

	if (!bSuccess)
	{
		SEOUL_ERR("%s: dependency scan, failed decompressing UI Movie data.", filePath.CStr());
		return false;
	}

	return Falcon::FCNFile::GetFCNFileDependencies(filePath, pU, uU, rv);
}

static Bool GenerateImageToSwf(ImageToSwf& rtImageToSwf)
{
	// Iterate .fcn files and gather.
	Falcon::FCNFile::FCNDependencies v;
	Vector<String> vs;
	if (!Directory::GetDirectoryListing(GamePaths::Get()->GetContentDir(), vs, false, true, ".fcn"))
	{
		SEOUL_ERR("Failed enumerating directory '%s'", GamePaths::Get()->GetContentDir().CStr());
		return false;
	}

	ImageToSwf tImageToSwf;
	for (auto const& s : vs)
	{
		auto const fcnFilePath(FilePath::CreateContentFilePath(s));

		// Get deps.
		Falcon::FCNFile::FCNDependencies vDeps;
		if (!GetFCNFileDependencies(fcnFilePath, vDeps))
		{
			SEOUL_ERR("Failed reading '%s'", fcnFilePath.CStr());
			return false;
		}

		// Now build out deps.
		for (auto const& dep : vDeps)
		{
			if (!IsTextureFileType(dep.GetType()))
			{
				continue;
			}

			auto pv = tImageToSwf.Find(dep);
			if (nullptr == pv)
			{
				auto const e = tImageToSwf.Insert(dep, Falcon::FCNFile::FCNDependencies());
				pv = &e.First->Second;
			}
			pv->PushBack(fcnFilePath);
		}
	}

	// Sort.
	for (auto e : tImageToSwf)
	{
		auto& v = e.Second;
		QuickSort(v.Begin(), v.End(), [](const FilePath& a, const FilePath& b)
		{
			return (strcmp(a.CStr(), b.CStr()) < 0);
		});
	}

	// Done.
	rtImageToSwf.Swap(tImageToSwf);
	return true;
}

static Bool GetPNGDimensions(FilePath filePath, Int32& riWidth, Int32& riHeight)
{
	static const UInt32 kuOffsetToWidthAndHeight = 16;

	auto const sSourcePng(filePath.GetAbsoluteFilenameInSource());
	DiskSyncFile file(sSourcePng, File::kRead);
	if (!file.Seek(kuOffsetToWidthAndHeight, File::kSeekFromStart))
	{
		SEOUL_ERR("failed reading header of PNG '%s'", sSourcePng.CStr());
		return false;
	}

	Int32 iWidth = 0;
	Int32 iHeight = 0;
	if (sizeof(iWidth) != file.ReadRawData(&iWidth, sizeof(iWidth)))
	{
		SEOUL_ERR("failed reading width of PNG '%s'", sSourcePng.CStr());
		return false;
	}
	if (sizeof(iHeight) != file.ReadRawData(&iHeight, sizeof(iHeight)))
	{
		SEOUL_ERR("failed reading height of PNG '%s'", sSourcePng.CStr());
		return false;
	}

#if SEOUL_LITTLE_ENDIAN
	iWidth = EndianSwap32(iWidth);
	iHeight = EndianSwap32(iHeight);
#endif

	riWidth = iWidth;
	riHeight = iHeight;
	return true;
}

typedef FixedArray<UInt64, (UInt32)FileType::LAST_TEXTURE_TYPE - (UInt32)FileType::FIRST_TEXTURE_TYPE + 1u> Sizes;
struct ImageEntry SEOUL_SEALED
{
	FilePath m_FilePath;
	Int m_iWidth = 0;
	Int m_iHeight = 0;
	Sizes m_aSizes;

	UInt64 GetSize(FileType eType) const
	{
		return m_aSizes[(UInt32)eType - (UInt32)FileType::FIRST_TEXTURE_TYPE];
	}

	UInt64 GetTotalSize() const
	{
		UInt64 ret = 0;
		for (auto const u : m_aSizes)
		{
			ret += u;
		}
		return ret;
	}

	Bool operator<(const ImageEntry& b) const
	{
		auto const uSizeA = GetSize(FileType::kTexture1) + GetSize(FileType::kTexture4);
		auto const uSizeB = b.GetSize(FileType::kTexture1) + b.GetSize(FileType::kTexture4);
		if (uSizeA == uSizeB)
		{
			return (strcmp(m_FilePath.CStr(), b.m_FilePath.CStr()) < 0);
		}
		else
		{
			return uSizeB < uSizeA;
		}
	}
}; // struct ImageEntry
typedef Vector<ImageEntry, MemoryBudgets::Falcon> ImageEntries;

struct AssetStats SEOUL_SEALED
{
	UInt32 m_uTotalImages = 0u;
	Sizes m_aSizes;

	Int64 GetTotalSize() const
	{
		UInt64 ret = 0;
		for (auto const u : m_aSizes)
		{
			ret += u;
		}
		return ret;
	}
}; // struct AssetStats

static AssetStats GetStats(const ImageEntries& vEntries)
{
	AssetStats stats;

	stats.m_uTotalImages = vEntries.GetSize();
	for (auto const& entry : vEntries)
	{
		auto const uCount = stats.m_aSizes.GetSize();
		for (UInt32 i = 0u; i < uCount; ++i)
		{
			stats.m_aSizes[i] += entry.m_aSizes[i];
		}
	}

	return stats;
}

static inline void WriteLine(BufferedSyncFile& r, const String& s)
{
	r.Printf("%s\n", s.CStr());
}

static String ToHumanFriendlyFileSizeString(UInt64 uFileSize)
{
	if (uFileSize > 1024 * 1024)
	{
		return ToString(uFileSize / (1024 * 1024)) + " MBs";
	}
	else if (uFileSize > 1024)
	{
		return ToString(uFileSize / 1024) + " KBs";
	}
	else
	{
		return ToString(uFileSize) + " Bs";
	}
}

static String ToUrlFileListString(const Falcon::FCNFile::FCNDependencies& v)
{
	String s;
	for (UInt32 i = 0u; i < v.GetSize(); ++i)
	{
		if (0u != i)
		{
			s += "<br/>";
		}

		s += "<a href=\"";
		s += "file://";
		s += v[i].GetAbsoluteFilenameInSource().ReplaceAll("\\", "/");
		s += "\">";
		s += v[i].GetRelativeFilenameInSource();
		s += "</a>";
	}

	return s;
}

static String ToImageHtml(const ImageEntry& entry)
{
	static const Int kiTargetHeight = 64;
	static const Int kiMaxWidth = 512;

	auto fFactor = (Float32)entry.m_iHeight / (Float)kiTargetHeight;
	auto iImageHeight = kiTargetHeight;
	auto iImageWidth = (0.0f == fFactor ? kiTargetHeight : (Int)(entry.m_iWidth / fFactor));
	if (iImageWidth > kiMaxWidth)
	{
		fFactor = (entry.m_iWidth > 0 ? ((Float32)entry.m_iHeight / (Float32)entry.m_iWidth) : 1.0f);
		iImageWidth = kiMaxWidth;
		iImageHeight = (Int)(iImageWidth * fFactor);
	}

	auto const sUrl = entry.m_FilePath.GetAbsoluteFilenameInSource().ReplaceAll("\\", "/");
	return "<a href=\"file://" + sUrl + "\">"
		"<img src=\"file://" + sUrl + "\""
		" width=\"" + ToString(iImageWidth) + "\""
		" height=\"" + ToString(iImageHeight) + "\""
		"/></a>";
}

static Bool GenerateHtml(const ImageToSwf& tImageToSwf, const String& sOutFile)
{
	// Collect.
	ImageEntries vEntries;
	for (auto const& e : tImageToSwf)
	{
		ImageEntry entry;
		entry.m_FilePath = e.First;

		auto const uCount = entry.m_aSizes.GetSize();
		for (UInt32 i = 0u; i < uCount; ++i)
		{
			FilePath filePath = e.First;
			filePath.SetType((FileType)((Int32)i + (Int32)FileType::FIRST_TEXTURE_TYPE));
			entry.m_aSizes[i] = FileManager::Get()->GetFileSize(filePath);
		}

		if (!GetPNGDimensions(entry.m_FilePath, entry.m_iWidth, entry.m_iHeight))
		{
			return false;
		}

		vEntries.PushBack(entry);
	}

	// Gather.
	QuickSort(vEntries.Begin(), vEntries.End());
	auto const stats = GetStats(vEntries);

	// Generate HTML.
	DiskSyncFile file(sOutFile, File::kWriteTruncate);
	BufferedSyncFile writer(&file, false);
	{
		WriteLine(writer, "<html>");
		WriteLine(writer, "<head>");
		WriteLine(writer, "<title>Asset Summary</title>");
		WriteLine(writer,
			"<link rel=\"stylesheet\" type=\"text/css\" href=\"file://" +
			Path::Combine(Path::GetProcessDirectory(), "AssetTool.css").ReplaceAll("\\", "/") +
			"\"/>");
		WriteLine(writer, "</head>");
		WriteLine(writer, "<body>");
		WriteLine(writer, "<center>");
		WriteLine(writer, "<table border=\"1\" cellpadding=\"2\">");
		WriteLine(writer,
			"<tr>"
			"<th colspan=\"6\"><center>Summary</center></th>"
			"</tr>");
		WriteLine(writer, "<tr>");
		WriteLine(writer, "<td colspan=\"6\">");
		WriteLine(writer, "<p>");
		WriteLine(writer, "<b>Path:</b> " + GamePaths::Get()->GetContentDir() + "<br/>");
		WriteLine(writer, "<b>Total UI Images:</b> " + ToString(stats.m_uTotalImages) + "<br/>");
		WriteLine(writer, "<b>Total UI Images Size:</b> " + ToHumanFriendlyFileSizeString(stats.GetTotalSize()) + "<br/>");
		WriteLine(writer, "</p>");
		WriteLine(writer, "</td>");
		WriteLine(writer, "</tr>");
		WriteLine(writer,
			"<tr>"
			"<th>#</th>"
			"<th>Image</th>"
			"<th>Dimensions</th>"
			"<th bgcolor=\"#EEEEEE\">Size</th>"
			"<th>Used By</th>"
			"</tr>");

		Int32 i = 1;
		for (auto const& entry : vEntries)
		{
			auto pvSwfFiles = tImageToSwf.Find(entry.m_FilePath);

			WriteLine(writer,
				"<tr>"
				"<td>" + ToString(i) + "</td>"
				"<td>" + ToImageHtml(entry) + "</td>"
				"<td>" + ToString(entry.m_iWidth) + " x " + ToString(entry.m_iHeight) + "</td>"
				"<td bgcolor=\"#EEEEEE\">" + ToHumanFriendlyFileSizeString(entry.GetTotalSize()) + "</td>"
				"<td>" + ToUrlFileListString(*pvSwfFiles) + "</td>"
				"</tr>");
			++i;
		}
		WriteLine(writer, "</table>");
		WriteLine(writer, "</center>");
		WriteLine(writer, "</body>");
		WriteLine(writer, "</html>");
	}

	return true;
}

/** Get the App's base directory - we use the app's base directory for GamePaths. */
static String GetBaseDirectoryPath()
{
	// AssetTool root path.
	auto const sProcessPath = Path::GetProcessDirectory();

	// Now resolve the App directory using assumed directory structure.
	auto const sPath = Path::GetExactPathName(Path::Combine(
		Path::GetDirectoryName(sProcessPath, 5),
		Path::Combine(SEOUL_APP_ROOT_NAME, "Binaries", "PC", "Developer", "x64")));

	return sPath;
}

struct ScopedCore SEOUL_SEALED
{
	ScopedCore()
	{
		// Silence all log channels except for Assertion.
		Logger::GetSingleton().EnableAllChannels(false);
		Logger::GetSingleton().EnableChannel(LoggerChannel::Assertion, true);

		// Initialize Core support.
		CoreSettings settings;
		settings.m_bLoadLoggerConfigurationFile = false;
		settings.m_bOpenLogFile = false;
		settings.m_GamePathsSettings.m_sBaseDirectoryPath = GetBaseDirectoryPath();
		Core::Initialize(settings);

		// Specify content dir.
		m_sOriginalContentDir = GamePaths::Get()->GetContentDir();
		GamePaths::Get()->SetContentDir(
			GamePaths::Get()->GetContentDirForPlatform(AssetToolCommandLineArgs::Platform));
	}

	~ScopedCore()
	{
		// Restore content dir prior to shutdown.
		GamePaths::Get()->SetContentDir(m_sOriginalContentDir);

		// Shutdown Core handling.
		Core::ShutDown();
	}

private:
	String m_sOriginalContentDir;
}; // class ScopedCore

} // namespace Seoul

int main(int iArgC, char** ppArgV)
{
	using namespace Seoul;
	using namespace Seoul::Reflection;

	// Parse command-line args.
	if (!CommandLineArgs::Parse(ppArgV + 1, ppArgV + iArgC))
	{
		return 1;
	}

	// Core necessary beyond this point.
	ScopedCore core;

	// Gather deps.
	ImageToSwf tImageToSwf;
	if (!GenerateImageToSwf(tImageToSwf))
	{
		return 1;
	}

	// Produce output.
	auto const sOutFile(Path::ReplaceExtension(Path::GetTempFileAbsoluteFilename(), ".html"));
	if (!GenerateHtml(tImageToSwf, sOutFile))
	{
		return 1;
	}

	// Show the web page.
	//
	// TODO: Need to escalate OpenURL() into a core function
	// (currently depends on Engine).
#if SEOUL_PLATFORM_WINDOWS
	::ShellExecuteW(
		nullptr,
		L"open",
		sOutFile.WStr(),
		nullptr,
		nullptr,
		SW_SHOWNORMAL);
#else
#	error "Define for this platform."
#endif

	return 0;
}
