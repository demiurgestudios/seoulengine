/**
 * \file FalconFlaChecker.cpp
 * \brief For non-ship only, implements validation of Adobe Animate
 * (.FLA) files, which are .zip archives that contain Adobe Animate
 * data in .xml, image (.png and .jpg), and proprietary binary files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconFlaChecker.h"

#if !SEOUL_SHIP
#include "DiskFileSystem.h"
#include "FileManager.h"
#include "Logger.h"
#include "Path.h"
#include "ScopedAction.h"
#include "SeoulPugiXml.h"
#include "ZipFile.h"

namespace Seoul::Falcon
{

#define SEOUL_ERR(fmt, ...) SEOUL_WARN("%s: " fmt, Path::GetFileName(sFilename).CStr(), ##__VA_ARGS__)

static const String kDomDocFilename("DOMDocument.xml");
static const String kPublishSettings("PublishSettings.xml");

static const String kDOMBitmapItemXpath("/DOMDocument/media/DOMBitmapItem");
static const String kDOMDocumentXpath("/DOMDocument");
static const String kDOMFontItemXpath("/DOMDocument/fonts/DOMFontItem");
static const String kDOMFrameXpath("frames/DOMFrame"); // Relative to a DOMLayer element, not root anchored.
static const String kDOMLayerXpath("layers/DOMLayer"); // Relative to a DOMTimeline element, not root anchored.
static const String kDOMSymbolInstanceXpath("elements/DOMSymbolInstance"); // Relative to a DOMFrame element, not root anchored.
static const String kDOMSymbolItemXpath("/DOMSymbolItem");
static const String kDOMSymbolItemTimelineXpath("/DOMSymbolItem/timeline/DOMTimeline");
static const String kDOMTimelineXpath("/DOMDocument/timelines/DOMTimeline");
static const String kFlashFilenameXpath("/flash_profiles/flash_profile/PublishFormatProperties/flashFileName");

static const String kSharedLibraryUrl("sharedLibraryURL");
static const String kSymbolRtImportAttribute("linkageImportForRS");
static const String kSymbolRtExportAttribute("linkageExportForRS");
static const String kSymbolRtLinkageUrlAttribute("linkageURL");
static const String kLibraryPrefix("LIBRARY");
static const String kLibrarySymbolNameAttribute("libraryItemName");
static const String kLayerTypeAttribute("layerType");

static const String kBadFilenameCharacters(R"(:;#&*?"|<>[],)");
static const String kDuplicateFontCopySuffix(" copy");
static const String kXml(".xml");

static Bool ReadFile(const String& sFilename, void*& rp, UInt32& ru)
{
	if (FileManager::Get())
	{
		if (!FileManager::Get()->ReadAll(sFilename, rp, ru, 0u, MemoryBudgets::Falcon))
		{
			SEOUL_ERR("file not found or error reading file.");
			return false;
		}
	}
	else
	{
		if (!DiskSyncFile::ReadAll(sFilename, rp, ru, 0u, MemoryBudgets::Falcon))
		{
			SEOUL_ERR("file not found or error reading file.");
			return false;
		}
	}

	return true;
}

static Bool LoadXml(const String& sFilename, const ZipFileReader& zipFile, const String& sName, pugi::xml_document& r)
{
	void* p = nullptr;
	UInt32 u = 0u;
	if (!zipFile.ReadAll(sName, p, u, 0u, MemoryBudgets::Falcon))
	{
		return false;
	}

	// TODO: See load_buffer_inplace_own() source - some results
	// mean pugi did *not* take ownership of the buffer yet and we
	// need to deallocate it. But I don't expect us to ever
	// actually hit those errors.
	auto const result = r.load_buffer_inplace_own(p, (size_t)u);
	if (!result)
	{
		SEOUL_ERR("failed reading XML '%s': %s", sName.CStr(), result.description());
		return false;
	}

	return true;
}

static Bool GetFlashFileName(const String& sFilename, const ZipFileReader& reader, String& rs)
{
	pugi::xml_document publishSettings;
	if (!LoadXml(sFilename, reader, kPublishSettings, publishSettings))
	{
		return false;
	}

	auto const flashFileName(publishSettings.select_node(kFlashFilenameXpath.CStr()).node());
	if (!flashFileName || !flashFileName.text())
	{
		SEOUL_ERR("%s does not contain '%s'", kPublishSettings.CStr(), kFlashFilenameXpath.CStr());
		return false;
	}

	rs.Assign(flashFileName.text().as_string());
	return true;
}

/**
 * Gets the name of the given font element without any path specifiers
 * or automatically generated name parts, such as "copy"
 */
static String GetBaseFontName(const String& s)
{
	return Path::GetFileNameWithoutExtension(s).ReplaceAll(kDuplicateFontCopySuffix, String());
}

typedef HashSet<String, MemoryBudgets::Falcon> Set;
typedef HashTable<String, String, MemoryBudgets::Falcon> Table;
static Bool CheckDuplicateFonts(const String& sFilename, const pugi::xml_document& domDoc)
{
	Set baseFontNames;
	Set knownDuplicates;

	Bool bOk = true;
	for (auto const& fontRes : domDoc.select_nodes(kDOMFontItemXpath.CStr()))
	{
		auto const font = fontRes.node();
		auto const sName(GetBaseFontName(font.attribute("name").value()));
		if (baseFontNames.HasKey(sName) && !knownDuplicates.HasKey(sName))
		{
			SEOUL_ERR("Found duplicate font in library: %s", sName.CStr());
			bOk = false;

			knownDuplicates.Insert(sName);
		}
		else
		{
			baseFontNames.Insert(sName);
		}
	}

	return bOk;
}

/**
 * Adobe Animate will sometimes add a sharedLibraryURL = attribute
 * to the root dom. When present, if it is set to a value other
 * than the SWF export path, this can cause hard to diagnose
 * errors with asset import.
 *
 * For example, attempting to import a MovieClip from Leaderboards.swf
 * into Leaderboards2.swf will silently reset the import linkage URL if
 * Leaderboards2.fla has a sharedLibraryURL = attribute that was (erroneously)
 * set to Leaderboards.swf (this can happen if Leaderboards2.swf was
 * started as a copy of Leaderboards.swf).
 */
static Bool CheckSharedLibraryUrlMismatch(const String& sFilename, const pugi::xml_document& domDoc, const String& sFlaFilename)
{
	auto const sBaseFilename(Path::GetFileName(sFlaFilename));
	auto const url(domDoc.select_node(kDOMDocumentXpath.CStr()).node().attribute(kSharedLibraryUrl.CStr()));

	// No attribute means implicitly valid.
	if (!url)
	{
		return true;
	}

	auto const sUrl(url.value());
	if (sBaseFilename != Path::GetFileName(sUrl))
	{
		SEOUL_ERR(R"(sharedLibraryURL="%s" but SWF publish path is "%s". )"
			R"(If the publish path is correct, you can fix the value of )"
			R"(sharedLibraryURL by marking a MovieClip to )"
			R"("Export for runtime sharing" and correcting the export URL )"
			R"(to match the publish path )"
			R"("%s".)", sUrl, sFlaFilename.CStr(), sBaseFilename.CStr());

		return false;
	}

	return true;
}

namespace
{

struct LayerData
{
	String m_sName{};
	UInt32 m_uNumNamedSymbols{};
	Int m_iFrameLength{};
};

}

/**
 * Checks to make sure that the symbol list for both frames is consistent.
 *
 * @return false if it finds an inconsistency.
 */
static Bool AreFramesConsistent(
	const String& sFilename,
	Byte const* sTimelineName,
	Byte const* sLayerName,
	Int frame1,
	const Table& namedSymbols1,
	Int frame2,
	const Table& namedSymbols2,
	Bool bFatalOnly)
{
    // First check all symbol instances in one frame,
    // and see if it is or isn't in the other.
	Bool bOk = true;
	for (auto const& pair : namedSymbols1)
	{
		auto const& instanceName1 = pair.First;
		auto const& symbolName1 = pair.Second;
		auto const psSymbolName2 = namedSymbols2.Find(instanceName1);
		if (nullptr != psSymbolName2)
		{
			// If this instance is in both dictionaries,
			// make sure it is using the same library symbol on both frames.
			if (!bFatalOnly && symbolName1 != *psSymbolName2)
			{
				SEOUL_ERR(
					"In the timeline of '%s', the layer '%s' has an instance "
					"named '%s', which on frame %d, is using the library symbol "
					"'%s', but on frame %d, is using the library symbol '%s'.",
					sTimelineName, sLayerName, instanceName1.CStr(), frame1, symbolName1.CStr(), frame2, psSymbolName2->CStr());
				bOk = false;
			}
		}
		else if (!bFatalOnly)
		{
			// This instance isn't in the other frame.
			SEOUL_ERR(
				"In the timeline of '%s', the layer '%s' has an instance named '%s' "
				"on frame %d, but it doesn't exist in frame %d.",
				sTimelineName, sLayerName, instanceName1.CStr(), frame1, frame2);
			bOk = false;
		}
	}

    // Then check all symbols on the other frame,
    // and see if there are any that are not in the first frame.
	if (!bFatalOnly)
	{
		for (auto const& pair : namedSymbols2)
		{
			auto const& instanceName2 = pair.First;
			if (!namedSymbols1.HasValue(instanceName2))
			{
				SEOUL_ERR(
					"In the timeline of '%s', the layer '%s' has an instance named '%s' "
					"on frame %d, but it doesn't exist in frame %d.",
					sTimelineName, sLayerName, instanceName2.CStr(), frame2, frame1);
				bOk = false;
			}
		}
	}

	return bOk;
}

/**
 * Checks for any errors within the given layer.
 *
 * @return false on errors, true otherwise. Outputs resultant data.
 */
static Bool CheckLayer(
	const String& sFilename,
	const pugi::xml_document& domDoc,
	const pugi::xml_node& layer,
	Byte const* sTimelineName,
	Bool bFatalOnly,
	Table& rt,
	Int& riFrameLength)
{
    // Table of instance names to their symbol's name
	Table frame1NamedSymbols;
	auto frame1NamedSymbolsIsPopulated = false;

	auto const sLayerName = layer.attribute("name").value();

    // number of frames on this layer
	Int iFrameLength = 0;

    // For each keyframe...
	bool bOk = true;
	Table currentFrameNamedSymbols;
	for (auto const& frameRes : layer.select_nodes(kDOMFrameXpath.CStr()))
	{
		auto const frame = frameRes.node();
		currentFrameNamedSymbols.Clear();
		auto const iFrameIndex = frame.attribute("index").as_int();

        // For each symbol in this frame...
		for (auto const& symbolRes : frame.select_nodes(kDOMSymbolInstanceXpath.CStr()))
		{
			auto const symbol = symbolRes.node();
			auto const instanceName = symbol.attribute("name");

			// We only care about symbols with instance names
			if (instanceName)
			{
				auto const symbolName = symbol.attribute(kLibrarySymbolNameAttribute.CStr());
				auto& symbolTableToPopulate = (frame1NamedSymbolsIsPopulated
					? currentFrameNamedSymbols
					: frame1NamedSymbols);

                // Check for duplicate instance names in the same keyframe.
				if (!bFatalOnly && symbolTableToPopulate.HasValue(instanceName.value()))
				{
					SEOUL_ERR(
						"In the timeline of '%s', the layer '%s' has multiple "
						"instances with the name '%s' on frame %d.", sTimelineName, sLayerName, instanceName.value(), iFrameIndex + 1);
					bOk = false;
				}
                else
				{
                    // Save off the symbols in this frame
					symbolTableToPopulate.Overwrite(instanceName.value(), symbolName.value());
				}
			}
		}

        // Compare the first keyframe to this one
        if (frame1NamedSymbolsIsPopulated)
		{
			bOk = AreFramesConsistent(
				sFilename,
				sTimelineName,
				sLayerName,
				1,
				frame1NamedSymbols,
				iFrameIndex + 1,
				currentFrameNamedSymbols,
				bFatalOnly) && bOk;
		}

        // Checked one frame, so namedSymbols has now been populated
		frame1NamedSymbolsIsPopulated = true;

        // Update the frame length
		auto const keyframeDuration = frame.attribute("duration");
        if (keyframeDuration)
		{
			iFrameLength = Max(iFrameLength, iFrameIndex + keyframeDuration.as_int() - 1);
		}
		else
		{
			iFrameLength = Max(iFrameLength, iFrameIndex);
		}
	}

	rt.Swap(frame1NamedSymbols);
	riFrameLength = iFrameLength;
	return bOk;
}

static Bool CheckTimeline(const String& sFilename, const pugi::xml_document& domDoc, const pugi::xml_node& timeline, Bool bFatalOnly)
{
	Bool bOk = true;

	Table namedSymbols;
	Table namedSymbolsInLayer;
	Vector<LayerData, MemoryBudgets::Falcon> vLayerData;
	Int iMaxFrameLength = 0;
	auto const sTimelineName = timeline.attribute("name").value();
	for (auto const& layerRes : timeline.select_nodes(kDOMLayerXpath.CStr()))
	{
		auto const layer = layerRes.node();
		auto const sLayerType = layer.attribute(kLayerTypeAttribute.CStr()).value();

		// Skip.
		if (0 == strcmp(sLayerType, "folder") || 0 == strcmp(sLayerType, "guide"))
		{
			continue;
		}

		// Check the layer for any errors,
		// but also save off the symbols in it
		Int iFrameLength = 0;
		namedSymbolsInLayer.Clear();
		bOk = CheckLayer(sFilename, domDoc, layer, sTimelineName, bFatalOnly, namedSymbolsInLayer, iFrameLength) && bOk;
		auto const sLayerName = layer.attribute("name").value();

		LayerData data;
		data.m_sName = sLayerName;
		data.m_uNumNamedSymbols = namedSymbolsInLayer.GetSize();
		data.m_iFrameLength = iFrameLength;
		vLayerData.PushBack(data);

		iMaxFrameLength = Max(iFrameLength, iMaxFrameLength);

		// Check to make sure there is not an instance in one layer,
		// with the same name as an instance which is also in another layer.
		// (This will technically be OK as long as they don't exist on the
		// same frame, but it is bad practice which can easily lead to
		// errors either way).
		for (auto const& pair : namedSymbolsInLayer)
		{
			auto const& sInstanceName = pair.First;

			String sNamedSymbol;
			if (namedSymbols.GetValue(sInstanceName, sNamedSymbol) && !bFatalOnly)
			{
				SEOUL_ERR("In the timeline of '%s', the instance '%s' exists "
					"both in the layer '%s' as well as in the layer '%s'.",
					sTimelineName, sInstanceName.CStr(), sNamedSymbol.CStr(), sLayerName);
				bOk = false;
			}
			else
			{
				namedSymbols.Overwrite(sInstanceName, sLayerName);
			}
		}
	}

	if (!bFatalOnly)
	{
		for (auto const& data : vLayerData)
		{
			if (data.m_uNumNamedSymbols > 0u && data.m_iFrameLength < iMaxFrameLength)
			{
				SEOUL_ERR("In the timeline of '%s', the layer '%s' has a named symbol, "
					"but doesn\'t have enough frames to fill the whole timeline.", sTimelineName, data.m_sName.CStr());
				bOk = false;
			}
		}
	}

	return bOk;
}

static Bool CheckTimelines(const String& sFilename, const pugi::xml_document& domDoc, Bool bFatalOnly)
{
	Bool bOk = true;
	for (auto const& timeline : domDoc.select_nodes(kDOMTimelineXpath.CStr()))
	{
		bOk = CheckTimeline(sFilename, domDoc, timeline.node(), bFatalOnly) && bOk;
	}

	return bOk;
}

static Bool CheckJpgCompression(const String& sFilename, const pugi::xml_document& domDoc)
{
	Bool bOk = true;
	for (auto const& mediaRes : domDoc.select_nodes(kDOMBitmapItemXpath.CStr()))
	{
		auto const media = mediaRes.node();

		// Input as JPEG, disallow.
		if (0 != strcmp(media.attribute("originalCompressionType").value(), "lossless"))
		{
			SEOUL_ERR("Image imported as JPG originally, must be a lossless format (PNG): %s", media.attribute("name").value());
			bOk = false;
			continue;
		}

		// Baked as JPEG, disallow.
		if (media.attribute("useImportedJPEGData").as_bool("true") /* default is true */ ||
			0 != strcmp(media.attribute("compressionType").value(), "lossless"))
		{
			SEOUL_ERR("Check image compression settings on image, must be set to lossless: %s", media.attribute("name").value());
			bOk = false;
			continue;
		}
	}

	return bOk;
}

static Bool CheckDomDocument(const String& sFilename, const pugi::xml_document& domDoc, Bool bFatalOnly, const String& sFlaFilename)
{
	Bool bOk = true;
	bOk = CheckDuplicateFonts(sFilename, domDoc) && bOk;
	bOk = CheckSharedLibraryUrlMismatch(sFilename, domDoc, sFlaFilename) && bOk;
	bOk = CheckTimelines(sFilename, domDoc, bFatalOnly) && bOk;
	bOk = CheckJpgCompression(sFilename, domDoc) && bOk;
	return bOk;
}

static Bool CheckSymbol(
	const String& sFilename,
	const ZipFileReader& reader,
	Bool bFatalOnly,
	const String& sBaseFlaName,
	const String& sName)
{
	pugi::xml_document symbolDoc;
	if (!LoadXml(sFilename, reader, sName, symbolDoc))
	{
		return false;
	}

	// Check if there is an inconsistency in the import settings for this symbol
	// (Flash will clear the URL if you specify the file itself as the import URL)
	auto const symbolItem = symbolDoc.select_node(kDOMSymbolItemXpath.CStr()).node();
	auto const hasRTImport = symbolItem.attribute(kSymbolRtImportAttribute.CStr()).as_bool();
	auto const linkageUrl = symbolItem.attribute(kSymbolRtLinkageUrlAttribute.CStr());
	Bool const hasLinkageUrl = (linkageUrl);
	Bool bOk = true;
	if (hasRTImport && !hasLinkageUrl)
	{
		SEOUL_ERR(
			"Symbol '%s' is marked as being imported, but no linkage URL is "
			"specified. Did you mean to export it?", symbolItem.attribute("name").value());
		bOk = false;
	}

	auto const hasRTExport = symbolItem.attribute(kSymbolRtExportAttribute.CStr()).as_bool();
	if (hasRTExport && !hasLinkageUrl && linkageUrl.value() != sBaseFlaName)
	{
		SEOUL_ERR(
			"Symbol '%s' is marked as being exported, but linkage URL '%s' is "
			"not equal to the SWF publish path '%s'", symbolItem.attribute("name").value(), linkageUrl.value(), sBaseFlaName.CStr());
		bOk = false;
	}

	// Don't further process imported symbols as they will get processed by the source file
	if (hasRTImport)
	{
		return bOk;
	}

	// Grab the timeline element and check that for errors
	bOk = CheckTimeline(sFilename, symbolDoc, symbolDoc.select_node(kDOMSymbolItemTimelineXpath.CStr()).node(), bFatalOnly) && bOk;
	return bOk;
}

static Bool CheckFlaFilenames(const String& sFilename, const ZipFileReader& reader, Bool bFatalOnly, const String& sFlaFilename)
{
	Bool bOk = true;

	auto const sBaseFlaName(Path::GetFileName(sFlaFilename));
	String sName;
	auto const uEntries = reader.GetEntryCount();
	for (UInt32 i = 0u; i < uEntries; ++i)
	{
		if (!reader.GetEntryName(i, sName))
		{
			continue;
		}

		// Not a library entry, skip.
		if (!sName.StartsWith(kLibraryPrefix))
		{
			continue;
		}

		// Directory, skip.
		if (reader.IsDirectory(sName))
		{
			continue;
		}

		auto const uBadChar = sName.FindFirstOf(kBadFilenameCharacters);
		if (String::NPos != uBadChar)
		{
			SEOUL_ERR("'%s' is a badly formatted item name in the Library, it contains "
				"character '%c'", sName.CStr(), sName[uBadChar]);
			bOk = false;
			continue;
		}

		// Only further processing of .xml files.
		if (kXml != Path::GetExtension(sName))
		{
			continue;
		}

		bOk = CheckSymbol(sFilename, reader, bFatalOnly, sBaseFlaName, sName) && bOk;
	}

	return bOk;
}

Bool CheckFla(const String& sFilename, String* psRelativeSwfFilename /*= nullptr*/, Bool bFatalOnly /*= false*/)
{
	void* p = nullptr;
	UInt32 u = 0u;
	if (!ReadFile(sFilename, p, u))
	{
		return false;
	}

	FullyBufferedSyncFile file(p, u, true, sFilename);
	ZipFileReader reader(ZipFileReader::kAcceptRecoverableCorruption);
	if (!reader.Init(file))
	{
		SEOUL_ERR(".fla does not exist or file is corrupt.");
		return false;
	}

	pugi::xml_document domDoc;
	if (!LoadXml(sFilename, reader, kDomDocFilename, domDoc))
	{
		return false;
	}

	Bool bOk = true;
	String sFlashFileName;
	bOk = GetFlashFileName(sFilename, reader, sFlashFileName) && bOk;

	// If we successfully acquired the filename, even if the rest of validation
	// fails, output it.
	if (bOk && nullptr != psRelativeSwfFilename)
	{
		psRelativeSwfFilename->Assign(sFlashFileName);
	}

	bOk = CheckDomDocument(sFilename, domDoc, bFatalOnly, sFlashFileName) && bOk;
	bOk = CheckFlaFilenames(sFilename, reader, bFatalOnly, sFlashFileName) && bOk;
	return bOk;
}

} // namespace Seoul::Falcon

#endif // /!SEOUL_SHIP
