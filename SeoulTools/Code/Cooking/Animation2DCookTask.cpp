/**
 * \file Animation2DCookTask.cpp
 * \brief Cooking tasks for cooking Spine exported .son files into runtime
 * SeoulEngine .saf files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BaseCookTask.h"
#include "Compress.h"
#include "CookPriority.h"
#include "DataStore.h"
#include "FileManager.h"
#include "ICookContext.h"
#include "Logger.h"
#include "Prereqs.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"

#if SEOUL_WITH_ANIMATION_2D
#include "Animation2DDataDefinition.h"
#include "Animation2DReadWriteUtil.h"

namespace Seoul
{

static const HString kAttachments("attachments");

namespace Reflection
{

/**
 * Context for deserializing animation data.
 */
class Animation2DContext SEOUL_SEALED : public DefaultSerializeContext
{
public:
	Animation2DContext(
		const ContentKey& contentKey,
		const DataStore& dataStore,
		const DataNode& table,
		const TypeInfo& typeInfo)
		: DefaultSerializeContext(contentKey, dataStore, table, typeInfo)
	{
	}

	virtual Bool HandleError(SerializeError eError, HString additionalData = HString()) SEOUL_OVERRIDE
	{
		using namespace Reflection;

		// Required and similar errors are always (silently) ignored, no properties
		// in animation data are considered required.
		if (SerializeError::kRequiredPropertyHasNoCorrespondingValue != eError &&
			SerializeError::kDataStoreContainsUndefinedProperty != eError)
		{
			// Use the default handling to issue a warning.
			return DefaultSerializeContext::HandleError(eError, additionalData);
		}

		return true;
	}
}; // class Animation2DContext

} // namespace Reflection

namespace Cooking
{

static const HString kFilePath("FilePath");
static const HString kHash("hash");
static const HString kImages("images");
static const HString kMesh("mesh");
static const HString kName("name");
static const HString kPath("path");
static const HString kRegion("region");
static const HString kSpine("spine");
static const HString kSkeleton("skeleton");
static const HString kSkins("skins");
static const HString kType("type");

class Animation2DCookTask SEOUL_SEALED : public BaseCookTask
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(Animation2DCookTask);

	Animation2DCookTask()
	{
	}

	virtual Bool CanCook(FilePath filePath) const SEOUL_OVERRIDE
	{
		return (filePath.GetType() == FileType::kAnimation2D);
	}

	virtual Bool CookAllOutOfDateContent(ICookContext& rContext) SEOUL_OVERRIDE
	{
		ContentFiles v;
		if (!DefaultOutOfDateCook(rContext, FileType::kAnimation2D, v, true))
		{
			return false;
		}

		return true;
	}

	virtual Int32 GetPriority() const SEOUL_OVERRIDE
	{
		return CookPriority::kAnimation2D;
	}

private:
	SEOUL_DISABLE_COPY(Animation2DCookTask);

	Bool PostProcessAnimation2DAttachment(
		FilePath filePath,
		const String& sDirectory,
		const String& sImages,
		DataStore& r,
		HString attachmentName,
		const DataNode& attachmentNode) const
	{
		// Check the type of the attachment. Default to "region" if not defined.
		HString type = kRegion;
		DataNode typeNode;
		(void)(r.GetValueFromTable(attachmentNode, kType, typeNode) && r.AsString(typeNode, type));

		// File dependencies for the attachment can be defined with a 'name'
		// field, a 'path' field, or by the name of the dependency itself.
		// In all cases, that names defines a relative path to a .png that
		// is formed by <directory-of-animation.json>/<images>/<relative-name>.png
		String s;
		DataNode value;
		Bool bImplicit = false;
		if (r.GetValueFromTable(attachmentNode, kPath, value))
		{
			if (!r.AsString(value, s))
			{
				SEOUL_LOG_COOKING("%s: attachment '%s' has a path that is not a string.\n", filePath.CStr(), attachmentName.CStr());
				return false;
			}
		}
		else if (r.GetValueFromTable(attachmentNode, kName, value))
		{
			if (!r.AsString(value, s))
			{
				SEOUL_LOG_COOKING("%s: attachment '%s' has a name that is not a string.\n", filePath.CStr(), attachmentName.CStr());
				return false;
			}
		}
		else
		{
			s = String(attachmentName);
			bImplicit = true;
		}

		// Now check if the file exists. If so, erase both path and name fields and replace with a FilePath field constructed
		// from the path.
		String sResourcePath;
		if (!Path::CombineAndSimplify(
			sDirectory,
			Path::Combine(sImages, s + ".png"),
			sResourcePath))
		{
			SEOUL_LOG_COOKING("%s: attachment '%s' has path '%s/%s/%s', but this is an invalid path.\n",
				filePath.CStr(),
				attachmentName.CStr(),
				sDirectory.CStr(),
				sImages.CStr(),
				s.CStr());
			return false;
		}

		FilePath const resourceFilePath(FilePath::CreateContentFilePath(sResourcePath));
		if (!resourceFilePath.IsValid())
		{
			SEOUL_LOG_COOKING("%s: attachment '%s' has path '%s/%s/%s', which forms an invalid resource path.\n",
				filePath.CStr(),
				attachmentName.CStr(),
				sDirectory.CStr(),
				sImages.CStr(),
				s.CStr());
			return false;
		}

		// Check if the file exists. If it does not, fail if the dependency was explicit, otherwise just ignore the
		// dependency.
		if (!FileManager::Get()->Exists(sResourcePath))
		{
			if (bImplicit)
			{
				// Check the attachment type - if it's not a mesh attachment or
				// a bitmap attachment, ignore the lack of a resource on disk.
				//
				// Unfortunately, Spine references are implied so we
				// can't guarantee that they *must* exist. We assume this is the
				// case for mesh and bitmap based on our decision to always require
				// these references for those attachment types.
				if (kMesh != type && kRegion != type)
				{
					// Just continue, ignore.
					return true;
				}
			}

			SEOUL_LOG_COOKING("%s: '%s' attachment '%s' references resources '%s', this file does not exist.\n",
				filePath.CStr(),
				type.CStr(),
				attachmentName.CStr(),
				sResourcePath.CStr());
			return false;
		}
		// Fixup, remove "path" and "name" fields, replace with a "FilePath" field.
		else
		{
			(void)r.EraseValueFromTable(attachmentNode, kName);
			(void)r.EraseValueFromTable(attachmentNode, kPath);
			if (!r.SetFilePathToTable(attachmentNode, kFilePath, resourceFilePath))
			{
				SEOUL_LOG_COOKING("%s: attachment '%s', failed commiting resource file path '%s'.\n",
					filePath.CStr(),
					attachmentName.CStr(),
					resourceFilePath.CStr());
				return false;
			}
		}

		return true;
	}

	Bool PostProcessAnimation2DSkins(
		FilePath filePath,
		DataStore& r) const
	{
		String const sDirectory(Path::GetDirectoryName(filePath.GetAbsoluteFilenameInSource()));

		auto const root = r.GetRootNode();

		// First, get the "images" attribute from the skeleton metadata.
		// If defined, this is the relative folder from the base filename
		// where images are stored.
		DataNode node;
		(void)r.GetValueFromTable(root, kSkeleton, node);
		(void)r.GetValueFromTable(node, kImages, node);

		// Get the images field - if not present, we default to relative "images".
		String sImages;
		if (!r.AsString(node, sImages))
		{
			sImages = "images";
		}

		// Now get the skins table and process it.
		if (!r.GetValueFromTable(root, kSkins, node))
		{
			// Early out if no skins.
			return true;
		}

		// Old format - table of skins.
		if (node.IsTable())
		{
			auto const iBegin0 = r.TableBegin(node);
			auto const iEnd0 = r.TableEnd(node);
			for (auto i0 = iBegin0; iEnd0 != i0; ++i0) // these are skins
			{
				auto const iBegin1 = r.TableBegin(i0->Second);
				auto const iEnd1 = r.TableEnd(i0->Second);
				for (auto i1 = iBegin1; iEnd1 != i1; ++i1) // these are slots
				{
					auto const iBegin2 = r.TableBegin(i1->Second);
					auto const iEnd2 = r.TableEnd(i1->Second);
					for (auto i2 = iBegin2; iEnd2 != i2; ++i2) // these are attachments
					{
						if (!PostProcessAnimation2DAttachment(
							filePath,
							sDirectory,
							sImages,
							r,
							i2->First,
							i2->Second))
						{
							return false;
						}
					}
				}
			}
		}
		// New, array of skins.
		else
		{
			UInt32 uSkins = 0u;
			(void)r.GetArrayCount(node, uSkins);
			for (UInt32 i0 = 0u; i0 < uSkins; ++i0) // these are skins
			{
				DataNode skinNode;
				(void)r.GetValueFromArray(node, i0, skinNode);

				DataNode attachmentsNode;
				if (r.GetValueFromTable(skinNode, kAttachments, attachmentsNode))
				{
					auto const iBegin1 = r.TableBegin(attachmentsNode);
					auto const iEnd1 = r.TableEnd(attachmentsNode);
					for (auto i1 = iBegin1; iEnd1 != i1; ++i1) // these are slots
					{
						auto const iBegin2 = r.TableBegin(i1->Second);
						auto const iEnd2 = r.TableEnd(i1->Second);
						for (auto i2 = iBegin2; iEnd2 != i2; ++i2) // these are attachments
						{
							if (!PostProcessAnimation2DAttachment(
								filePath,
								sDirectory,
								sImages,
								r,
								i2->First,
								i2->Second))
							{
								return false;
							}
						}
					}
				}
			}
		}

		return true;
	}

	Bool PostProcessAnimation2DData(
		FilePath filePath,
		DataStore& r) const
	{
		auto const root = r.GetRootNode();
		DataNode metadata;
		if (!r.GetValueFromTable(root, kSkeleton, metadata))
		{
			SEOUL_LOG_COOKING("%s: failed getting skeleton metadata.\n", filePath.CStr());
			return false;
		}

		DataNode value;

		// Check version.
		{
			String sVersion;
			if (!r.GetValueFromTable(metadata, kSpine, value) ||
				!r.AsString(value, sVersion) ||
				sVersion != Animation2D::ksExpectedSpineVersion)
			{
				SEOUL_LOG_COOKING("%s: expected version '%s', got version '%s'.\n",
					filePath.CStr(),
					Animation2D::ksExpectedSpineVersion.CStr(),
					sVersion.CStr());
				return false;
			}
		}

		// Fixup skin references to include a FilePath.
		if (!PostProcessAnimation2DSkins(filePath, r))
		{
			return false;
		}

		// Prune everything except height and width from the metadata prior to return.
		(void)r.EraseValueFromTable(metadata, kHash);
		(void)r.EraseValueFromTable(metadata, kImages);
		(void)r.EraseValueFromTable(metadata, kSpine);

		return true;
	}

	virtual Bool InternalCook(ICookContext& rContext, FilePath filePath) SEOUL_OVERRIDE
	{
		String s;
		if (!FileManager::Get()->ReadAll(filePath.GetAbsoluteFilenameInSource(), s))
		{
			SEOUL_LOG_COOKING("%s: failed reading source data from disk.", filePath.CStr());
			return false;
		}

		DataStore dataStore;

		// TODO: Spine emits duplicate keys in tables. Appears to be an editor bug as
		// of version 3.5.46.
		if (!DataStoreParser::FromString(s, dataStore, DataStoreParserFlags::kLogParseErrors | DataStoreParserFlags::kAllowDuplicateTableKeys, filePath))
		{
			SEOUL_LOG_COOKING("%s: failed loading animation .json into a DataStore.", filePath.CStr());
			return false;
		}

		if (!PostProcessAnimation2DData(filePath, dataStore))
		{
			SEOUL_LOG_COOKING("%s: failed post processing animation data.", filePath.CStr());
			return false;
		}

		// Now that the dataStore has been post-filtered, serialization into an animation data
		// instance and save to a buffer.
		SharedPtr<Animation2D::DataDefinition> pData(SEOUL_NEW(MemoryBudgets::Rendering) Animation2D::DataDefinition(filePath));
		Reflection::Animation2DContext context(
			filePath,
			dataStore,
			dataStore.GetRootNode(),
			TypeId<Animation2D::DataDefinition>());
		if (!Reflection::DeserializeObject(context, dataStore, dataStore.GetRootNode(), pData.GetPtr()))
		{
			SEOUL_LOG_COOKING("%s: post-processed animation data failed serialization into DataDefinition.", filePath.CStr());
			return false;
		}

		StreamBuffer buffer;
		Animation2D::ReadWriteUtil util(buffer, rContext.GetPlatform());
		if (!pData->Save(util))
		{
			SEOUL_LOG_COOKING("%s: failed saving animation data to binary blob.", filePath.CStr());
			return false;
		}
		if (!util.EndWrite())
		{
			SEOUL_LOG_COOKING("%s: failed saving animation data to binary blob, write termination failed.", filePath.CStr());
			return false;
		}

		{
			void* p = nullptr;
			UInt32 u = 0u;
			auto const deferred(MakeDeferredAction([&]()
				{
					MemoryManager::Deallocate(p);
					p = nullptr;
					u = 0u;
				}));

			if (!ZSTDCompress(
				buffer.GetBuffer(),
				buffer.GetTotalDataSizeInBytes(),
				p,
				u))
			{
				SEOUL_LOG_COOKING("%s: failed compression output animation data.", filePath.CStr());
				return false;
			}

			// Obfuscate
			Animation2D::Obfuscate((Byte*)p, u, filePath);
			return AtomicWriteFinalOutput(rContext, p, u, filePath);
		}
	}
}; // class Animation2DCookTask

} // namespace Cooking

SEOUL_BEGIN_TYPE(Cooking::Animation2DCookTask, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Cooking::BaseCookTask)
SEOUL_END_TYPE()

} // namespace Seoul

#endif // /#if SEOUL_WITH_ANIMATION_2D
