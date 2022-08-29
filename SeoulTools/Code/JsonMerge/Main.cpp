/**
 * \file Main.cpp
 * \brief JsonMerge is a specialized merge implementation
 * for .json files, setup for Perforce (arguments are:
 *   <base> <theirs> <yours> <output>
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DataStore.h"
#include "DataStoreParser.h"
#include "Directory.h"
#include "DiskFileSystem.h"
#include "Path.h"
#include "Platform.h"
#include "ReflectionCommandLineArgs.h"
#include "ReflectionDefine.h"
#include "ReflectionScriptStub.h"
#include "ScopedAction.h"
#include "SeoulUtil.h"
#include "StringUtil.h"

#define SEOUL_ERR(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)

namespace Seoul
{

/**
 * Root level command-line arguments - handled by reflection, can be
 * configured via the literal command-line, environment variables, or
 * a configuration file.
 */
class JsonMergeCommandLineArgs SEOUL_SEALED
{
public:
	static String Base;
	static String Theirs;
	static String Yours;
	static String Output;

private:
	JsonMergeCommandLineArgs() = delete; // Static class.
	SEOUL_DISABLE_COPY(JsonMergeCommandLineArgs);
};

String JsonMergeCommandLineArgs::Base{};
String JsonMergeCommandLineArgs::Theirs{};
String JsonMergeCommandLineArgs::Yours{};
String JsonMergeCommandLineArgs::Output{};

SEOUL_BEGIN_TYPE(JsonMergeCommandLineArgs, TypeFlags::kDisableNew | TypeFlags::kDisableCopy)
	SEOUL_CMDLINE_PROPERTY(Base, 0, "base", true)
	SEOUL_CMDLINE_PROPERTY(Theirs, 1, "theirs", true)
	SEOUL_CMDLINE_PROPERTY(Yours, 2, "yours", true)
	SEOUL_CMDLINE_PROPERTY(Output, 3, "output", true)
SEOUL_END_TYPE()

// TODO: Move into Engine.
static String GetPwd()
{
#if SEOUL_PLATFORM_WINDOWS
	auto const uLength = ::GetCurrentDirectoryW(0u, nullptr);
	if (0u == uLength)
	{
		return String();
	}

	Vector<WCHAR> v(uLength);
	if (0u == ::GetCurrentDirectoryW(uLength, v.Data()))
	{
		return String();
	}

	return WCharTToUTF8(v.Data());
#else
#	error "Define for this platform."
#endif
}

// Use default core virtuals
const CoreVirtuals* g_pCoreVirtuals = &g_DefaultCoreVirtuals;

namespace
{

struct Args
{
	String m_sBase;
	String m_sTheirs;
	String m_sYours;
	String m_sOutput;
};

} // namespace anonymous

static Bool ResolveFilename(String& rs, Bool bInput)
{
	if (!Path::IsRooted(rs))
	{
		rs = Path::Combine(GetPwd(), rs);
	}

	rs = Path::GetExactPathName(rs);

	if (bInput)
	{
		if (!DiskSyncFile::FileExists(rs))
		{
			SEOUL_ERR("File '%s' does not exist.", rs.CStr());
			return false;
		}
	}

	return true;
}

static Bool GetCommandLineArgs(Int iArgC, Byte** ppArgV, Args& rArgs)
{
	if (!Reflection::CommandLineArgs::Parse(ppArgV + 1, ppArgV + iArgC))
	{
		return false;
	}

	rArgs.m_sBase = JsonMergeCommandLineArgs::Base;
	rArgs.m_sTheirs = JsonMergeCommandLineArgs::Theirs;
	rArgs.m_sYours = JsonMergeCommandLineArgs::Yours;
	rArgs.m_sOutput = JsonMergeCommandLineArgs::Output;

	// Resolve and check.
	if (!ResolveFilename(rArgs.m_sBase, true)) { return false; }
	if (!ResolveFilename(rArgs.m_sTheirs, true)) { return false; }
	if (!ResolveFilename(rArgs.m_sYours, true)) { return false; }
	if (!ResolveFilename(rArgs.m_sOutput, false)) { return false; }

	// Done success.
	return true;
}

static Bool ReadFile(const String& s, DataStore& r)
{
	// Read the body.
	void* p = nullptr;
	UInt32 u = 0u;
	if (!DiskSyncFile::ReadAll(s, p, u, 0u, MemoryBudgets::Cooking))
	{
		SEOUL_ERR("failed reading '%s'", s.CStr());
		return false;
	}

	auto const scoped(MakeScopedAction([]() {}, [&]() { MemoryManager::Deallocate(p); p = nullptr; u = 0u; }));

	if (!DataStoreParser::FromString((Byte const*)p, u, r, DataStoreParserFlags::kLeaveFilePathAsString | DataStoreParserFlags::kLogParseErrors))
	{
		return false;
	}

	return true;
}

static Bool WriteFile(const String& s, const DataStore& ds, const String& sOut)
{
	// Read the body.
	void* p = nullptr;
	UInt32 u = 0u;
	if (!DiskSyncFile::ReadAll(s, p, u, 0u, MemoryBudgets::Cooking))
	{
		SEOUL_ERR("failed reading '%s'", s.CStr());
		return false;
	}

	auto const scoped(MakeScopedAction([]() {}, [&]() { MemoryManager::Deallocate(p); p = nullptr; u = 0u; }));

	// Derive hinting from existing file.
	SharedPtr<DataStoreHint> pHint;
	if (!DataStorePrinter::ParseHintsNoCopy((Byte const*)p, u, pHint))
	{
		SEOUL_ERR("failed parsing '%s' for hinting, cannot write.", s.CStr());
		return false;
	}

	// Pretty print with DataStorePrinter.
	String sBody;
	DataStorePrinter::PrintWithHints(ds, pHint, sBody);

	// Commit.
	if (!DiskSyncFile::WriteAll(sOut, sBody.CStr(), sBody.GetSize()))
	{
		SEOUL_ERR("failed writing output to '%s'.", sOut.CStr());
		return false;
	}

	// Done.
	return true;
}

typedef Delegate<Bool(const DataStore& ds, const DataNode& dn)> WriteOut;

static Bool IsAtomMerge(
	const DataStore& theirs, const DataNode& theirsNode,
	const DataStore& yours, const DataNode& yoursNode)
{
	// If theirs and yours type is a mismatch, then base and yours must be equal
	// (in which case, we just set theirs straightaway).
	if (theirsNode.GetType() != yoursNode.GetType() ||
		// Same rule if types are equal but the type is not an array or a table.
		(!theirsNode.IsArray() && !theirsNode.IsTable()))
	{
		return true;
	}

	// Container merge.
	return false;
}

// Atom will be called on simple values (not a table or array), or
// if the values are table/array but a mismatch (e.g.theirs is a table
// but yours is an array).
static Bool PerformMergeAtom(
	const DataStore& base, const DataNode& baseNode,
	const DataStore& theirs, const DataNode& theirsNode,
	const DataStore& yours, const DataNode& yoursNode,
	const WriteOut& writeOut)
{
	// If base is not equal to yours, then we keep yours.
	if (!DataStore::Equals(base, baseNode, yours, yoursNode, true))
	{
		return writeOut(yours, yoursNode);
	}

	// Base and yours are equal, so we can just copy theirs to output.
	return writeOut(theirs, theirsNode);
}

namespace
{

class WriteUtil SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(WriteUtil);

	WriteUtil(DataStore& out, const DataNode& outNode, HString key)
		: m_Out(out)
		, m_OutNode(outNode)
		, m_Key(key)
		, m_uIndex(0u)
	{
	}

	WriteUtil(DataStore& out, const DataNode& outNode, UInt32 uIndex)
		: m_Out(out)
		, m_OutNode(outNode)
		, m_Key()
		, m_uIndex(uIndex)
	{
	}

	Bool WriteArrayVal(const DataStore& in, const DataNode& inNode)
	{
		return m_Out.DeepCopyToArray(in, inNode, m_OutNode, m_uIndex);
	}

	Bool WriteTableVal(const DataStore& in, const DataNode& inNode)
	{
		return m_Out.DeepCopyToTable(in, inNode, m_OutNode, m_Key);
	}

private:
	SEOUL_DISABLE_COPY(WriteUtil);

	DataStore& m_Out;
	const DataNode m_OutNode;
	HString const m_Key;
	UInt32 const m_uIndex;
};

} // namespace anonymous

static Bool PerformMergeContainers(
	UInt32& ruConflicting,
	const DataStore& base, const DataNode& baseNode,
	const DataStore& theirs, const DataNode& theirsNode,
	const DataStore& yours, const DataNode& yoursNode,
	DataStore& out, const DataNode& outNode)
{
	// If we get here, we know that theirs and yours is the same type, and it
	// is either an array or a table.
	if (theirsNode.IsTable())
	{
		// Tables are straight-forward:
		// - value exists in theirs but not yours - if it also did not exist in base,
		//   then it's an add, otherwise it is skipped.
		// - value does *not* exist in theirs but exists in yours - if it also existed
		//   in base, then it is a remove, otherwise it is skipped.
		// - value exists in both, recurse.

		// first case and last cases - enumerate theirs, find values that exist in theirs
		// (but not yours) or exist in both.
		for (auto i = theirs.TableBegin(theirsNode), iEnd = theirs.TableEnd(theirsNode); iEnd != i; ++i)
		{
			// Does not exist in yours.
			DataNode tester;
			if (!yours.GetValueFromTable(yoursNode, i->First, tester))
			{
				// Also does not exist in base, add.
				if (!base.GetValueFromTable(baseNode, i->First, tester))
				{
					SEOUL_VERIFY(out.DeepCopyToTable(theirs, i->Second, outNode, i->First, false));
				}
				// Otherwise, just ignore.
			}
			// Exists in both, recurse.
			else
			{
				// Acquire.
				DataNode correspondingBaseNode;
				(void)base.GetValueFromTable(baseNode, i->First, correspondingBaseNode); // Might fail.

				// Atom merge, perform.
				if (IsAtomMerge(theirs, i->Second, yours, tester))
				{
					WriteUtil util(out, outNode, i->First);
					auto const writeOut = SEOUL_BIND_DELEGATE(&WriteUtil::WriteTableVal, &util);

					if (!PerformMergeAtom(
						base, correspondingBaseNode,
						theirs, i->Second,
						yours, tester,
						writeOut))
					{
						return false;
					}
				}
				// Container mege.
				else
				{
					// Make the necessary container output type.
					if (i->Second.IsTable())
					{
						SEOUL_VERIFY(out.SetTableToTable(outNode, i->First));
					}
					else
					{
						SEOUL_VERIFY(out.SetArrayToTable(outNode, i->First));
					}

					// Acquire.
					DataNode correspondingOutNode;
					SEOUL_VERIFY(out.GetValueFromTable(outNode, i->First, correspondingOutNode));

					if (!PerformMergeContainers(
						ruConflicting,
						base, correspondingBaseNode,
						theirs, i->Second,
						yours, tester,
						out, correspondingOutNode))
					{
						return false;
					}
				}
			}
		}

		// second case - exists in yours but not theirs.
		for (auto i = yours.TableBegin(yoursNode), iEnd = yours.TableEnd(yoursNode); iEnd != i; ++i)
		{
			// Does not exist in theirs.
			DataNode tester;
			if (!theirs.GetValueFromTable(theirsNode, i->First, tester))
			{
				// If the value *did* exist in base, then it's a remove,
				// so we do nothing (don't copy to output, so we delete it).
				//
				// If it does not, then we're keeping the yours value,
				// so we must copy it through.
				if (!base.GetValueFromTable(baseNode, i->First, tester))
				{
					SEOUL_VERIFY(out.DeepCopyToTable(yours, i->Second, outNode, i->First, false));
				}
			}
		}
	}
	// Array case.
	else
	{
		UInt32 uBaseCount = 0u;
		(void)base.GetArrayCount(baseNode, uBaseCount); // May fail.
		UInt32 uYoursCount = 0u;
		SEOUL_VERIFY(yours.GetArrayCount(yoursNode, uYoursCount));
		UInt32 uTheirsCount = 0u;
		SEOUL_VERIFY(theirs.GetArrayCount(theirsNode, uTheirsCount));

		// Arrays are more complicated - we have to use heuristics to both determine
		// if a difference between base and theirs is an add, remove, or change,
		// and then also determine the same between theirs and yours to determine
		// the appropriate action for out.
		UInt32 uBase = 0u, uYours = 0u, uTheirs = 0u, uOut = 0u;
		while (
			uBase < uBaseCount ||
			uTheirs < uTheirsCount ||
			uYours < uYoursCount)
		{
			// Determine if this is an add or a remove.
			DataNode testBase, testTheirs, testYours;
			Bool bBase = false, bTheirs = false, bYours = false;
			bBase = base.GetValueFromArray(baseNode, uBase, testBase);
			bTheirs = theirs.GetValueFromArray(theirsNode, uTheirs, testTheirs);
			bYours = yours.GetValueFromArray(yoursNode, uYours, testYours);

			// Check for equality.
			auto const bTheirsEqual = DataStore::Equals(base, testBase, theirs, testTheirs, true);
			auto const bYoursEqual = DataStore::Equals(base, testBase, yours, testYours, true);

			// Simple case, all equal, copy through and advance.
			if (bTheirsEqual && bYoursEqual)
			{
				SEOUL_VERIFY(out.DeepCopyToArray(yours, testYours, outNode, uOut));
				++uBase;
				++uTheirs;
				++uYours;
				++uOut;
				continue;
			}

			// Complicate cases, need to decide how we treat the discrepancy.

			// If yours are equal (base is equal to current state of target),
			// then we treat as either as change or a remove.
			if (bYoursEqual)
			{
				// Treat as a remove if the next entry of base is equal to
				// the current entry of theirs, of if we have no theirs.
				if (uTheirs >= uTheirsCount)
				{
					// Advance uBase and uYours to remove the entry (no copy to output),
					// then continue.
					++uBase;
					++uYours;
					continue;
				}
				else if (uBase + 1 < uBaseCount)
				{
					DataNode nextBase;
					if (base.GetValueFromArray(baseNode, uBase + 1, nextBase) &&
						// A remove if either the next base is equal to current theirs,
						// *or* the current base is not a container and the next base
						// is a matching container.
						(DataStore::Equals(base, nextBase, theirs, testTheirs, true) ||
							(!testBase.IsArray() && !testBase.IsTable() && (nextBase.IsArray() || nextBase.IsTable()) &&
								(nextBase.GetType() == testTheirs.GetType()))))
					{
						// Advance uBase and uYours to remove the entry (no copy to output),
						// then continue.
						++uBase;
						++uYours;
						continue;
					}
				}

				// Fall-through to general change handling.
			}
			// Otherwise, if theirs are equal, we keep the values of yours but
			// we need to decide how we update uBase and uTheirs.
			else if (bTheirsEqual)
			{
				// Always copy through yours if it is valid.
				if (uYours < uYoursCount)
				{
					SEOUL_VERIFY(out.DeepCopyToArray(yours, testYours, outNode, uOut));
					++uYours;
					++uOut;

					// If the next yours is valid and equal to base, we leave uBase
					// and uTheirs alone (we assume that uYours is an add relative
					// to uBase).
					if (uYours < uYoursCount)
					{
						SEOUL_VERIFY(yours.GetValueFromArray(yoursNode, uYours, testYours));
						if (DataStore::Equals(base, testBase, yours, testYours, true))
						{
							// Just continue, don't advance uBase or uTheirs.
							continue;
						}
					}
				}

				// If we get here for any reason, skip uBase and uTheirs.
				++uBase;
				++uTheirs;
				continue;
			}
			// Finally, if none are equal, then we apply conflict handling
			// unless the types are the same type of container, in which
			// case we fall through to treat as a change.
			else
			{
				// Same type of container.
				if (testBase.GetType() == testTheirs.GetType() &&
					testBase.GetType() == testYours.GetType() &&
					(testBase.IsArray() || testBase.IsTable()))
				{
					// Fall-through to treat as a change.
				}
				else
				{
					// Append to the end case:
					// If we have consumed all of base, assume extra items in theirs can be appended
					if (uBase >= uBaseCount)
					{
						// Add and advance theirs, leave base/yours at current indices.
						SEOUL_VERIFY(out.DeepCopyToArray(theirs, testTheirs, outNode, uOut));
						++uOut;
						++uTheirs;
						continue;
					}

					// Insertion case:
					// Treat as an add if there are more entries in theirs, and the next entry
					// is equal to the current base.
					if (uTheirs + 1 < uTheirsCount)
					{
						DataNode nextTheirs;
						if (theirs.GetValueFromArray(theirsNode, uTheirs + 1u, nextTheirs) &&
							DataStore::Equals(base, testBase, theirs, nextTheirs, true))
						{
							// Add and advance theirs, leave base/yours at current indices.
							SEOUL_VERIFY(out.DeepCopyToArray(theirs, testTheirs, outNode, uOut));
							++uOut;
							++uTheirs;
							continue;
						}
					}

					// Conflict.
					++ruConflicting;
					return false;
				}
			}

			// If we get here for any reason, treat as a change - if the same type of container,
			// recurse, otherwise accept theirs.
			if (testBase.GetType() == testTheirs.GetType() &&
				testBase.GetType() == testYours.GetType() &&
				(testBase.IsArray() || testBase.IsTable()))
			{
				// Sanity, must have valid theirs.
				SEOUL_ASSERT(uTheirs < uTheirsCount);

				// Establish out.
				if (testBase.IsArray())
				{
					SEOUL_VERIFY(out.SetArrayToArray(outNode, uOut));
				}
				else
				{
					SEOUL_VERIFY(out.SetTableToArray(outNode, uOut));
				}

				// Acquire.
				DataNode correspondingOut;
				SEOUL_VERIFY(out.GetValueFromArray(outNode, uOut, correspondingOut));

				// Recurse.
				if (!PerformMergeContainers(
					ruConflicting,
					base, testBase,
					theirs, testTheirs,
					yours, testYours,
					out, correspondingOut))
				{
					return false;
				}
			}
			else
			{
				// Otherwise, just accept theirs.
				// Sanity, must have valid theirs.
				SEOUL_ASSERT(uTheirs < uTheirsCount);
				SEOUL_VERIFY(out.DeepCopyToArray(theirs, testTheirs, outNode, uOut));
			}

			// In all above cases, we've treated as a change, so advance all equally.
			++uBase;
			++uTheirs;
			++uYours;
			++uOut;
			continue;
		}
	}

	return true;
}

static Bool PerformMerge(const DataStore& base, const DataStore& theirs, const DataStore& yours, DataStore& out)
{
	if (theirs.GetRootNode().GetType() != yours.GetRootNode().GetType())
	{
		SEOUL_ERR("1 conflicting");
		return false;
	}

	if (theirs.GetRootNode().IsArray())
	{
		out.MakeArray();
	}
	else
	{
		out.MakeTable();
	}

	UInt32 uConflicting = 0u;
	if (!PerformMergeContainers(
		uConflicting,
		base, base.GetRootNode(),
		theirs, theirs.GetRootNode(),
		yours, yours.GetRootNode(),
		out, out.GetRootNode()))
	{
		SEOUL_ERR("%u conflicting", uConflicting);
		return false;
	}

	return true;
}

static Bool Merge(const Args& args)
{
	// Read the three inputs.
	DataStore base, theirs, yours;
	if (!ReadFile(args.m_sBase, base)) { return false; }
	if (!ReadFile(args.m_sTheirs, theirs)) { return false; }
	if (!ReadFile(args.m_sYours, yours)) { return false; }

	DataStore output;
	if (!PerformMerge(base, theirs, yours, output))
	{
		return false;
	}

	// Commit the output.
	return WriteFile(args.m_sYours, output, args.m_sOutput);
}

} // namespace Seoul

int main(int iArgC, char** ppArgV)
{
	using namespace Seoul;

	Args args;
	if (!GetCommandLineArgs(iArgC, ppArgV, args))
	{
		return 1;
	}

	if (Merge(args))
	{
		return 0;
	}
	else
	{
		return 1;
	}
}
