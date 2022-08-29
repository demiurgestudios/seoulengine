/**
 * \file ScriptDebuggerClient.h
 * Debug client for SlimCS, implements the protocol
 * for talking to SlimCS enabled hosts (debuggers).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_DEBUGGER_CLIENT_H
#define SCRIPT_DEBUGGER_CLIENT_H

#include "AtomicRingBuffer.h"
#include "Delegate.h"
#include "Mutex.h"
#include "Prereqs.h"
#include "SeoulSignal.h"
#include "SeoulSocket.h"
#include "Singleton.h"
#include "SocketStream.h"
#include "StreamBuffer.h"
#include "ThreadId.h"
extern "C" { struct lua_Debug; }
extern "C" { struct lua_State; }
namespace Seoul { class FileChangeNotifier; }
namespace Seoul { class Thread; }

namespace Seoul
{

// Seoul forward declarations
class Thread;

// Conditional class - only define the debugger client in
// limited builds.
#if SEOUL_ENABLE_DEBUGGER_CLIENT

namespace Script
{

/**
 * Tags for messages that are sent from the client (e.g. SeoulEngine)
 * to the debugger server (e.g. Visual Studio).
 */
enum class DebuggerClientTag : Int32
{
	kUnknown = -1,
	kAskBreakpoints,
	kBreakAt,
	kFrame,
	kGetChildren,
	kHeartbeat,
	kSetVariable,
	kSync,
	kVersion,
	kWatch,
};

/**
 * Tags for messages that are sent from the debugger server to the client.
 */
enum class DebuggerServerTag : Int32
{
	kUnknown = -1,
	kAddWatch,
	kBreak,
	kContinue,
	kGetFrame,
	kGetChildren,
	kRemoveWatch,
	kSetBreakpoints,
	kSetVariable,
	kStepInto,
	kStepOut,
	kStepOver,
};

enum class SuspendReason
{
	kUnknown = 0,

	/** Client hit a user defined breakpoint. */
	kBreakpoint = 1,

	/** Break for triggered watchpoint. */
	kWatch = 2,

	/** Client experienced an unrecoverable error. */
	kFault = 3,

	/**
	 * Client is asking the server to allow a stop - in our usage model, we always
	 * just stop if we need to stop.
	 */
	kStopRequest = 4,

	/**
	 * Once the client has stopped for a user defined breakpoint, the debugger can
	 * request various step actions passed the breakpoint. Once the client stops again
	 * after completion of the step action, its SuspendReason is kStep.
	 */
	kStep = 5,

	/** Debugger equivalent to the native assembly 'trap' or 'int' instruction. */
	kHaltOpcode = 6,

	/**
	 * When a VM is first encountered, the client transmits a script lookup table
	 * (to allow script file identifiers to be 16-bits) and it breaks to give the
	 * debugger a chance to attach.
	 */
	kScriptLoaded = 7,
};

/**
 * Used in server-to-client and client-to-server messages when a data
 * type is required (matches the Lua value types).
 */
enum class DebuggerVariableType
{
	kNil,
	kBoolean,
	kLightUserData,
	kNumber,
	kString,
	kTable,
	kFunction,
	kUserData,
	kThread,

	// Special value - used to indicate a table
	// that has no children, to avoid displaying a +
	// for table that will expand to nothing.
	kEmptyTable,
};

/**
 * Used to record the current execution state of the debugger client.
 */
enum class DebuggerExecuteState
{
	/** Regular execution, run until stop, user-defined breakpoint, falt, etc. */
	kRunning,

	/** Break - execution is paused until a continue or step is sent. */
	kBreak,

	/** Running until a step into - break will occur at the next instruction when the stack index is greater-equal than the stack index at start of the step into. */
	kStepInto,

	/** Running until a step out - break will occur at the next instruction when the stack index is less than the stack index at the start of the step out. */
	kStepOut,

	/** Running until a step over - break will occur at the next instruction when the stack index is equal to the stack index at the step over. */
	kStepOver,
};

/**
 * Utility used by the Script::DebuggerClient::Message class to unify
 * read/write calls of data types.
 */
template <typename T, Int SIZE> struct DebuggerReadWriteUtility;

/**
 * Read/write of a 1-byte POD type.
 */
template <typename T>
struct DebuggerReadWriteUtility<T, 1>
{
	static Bool Read(StreamBuffer& r, T& rOut)
	{
		SEOUL_STATIC_ASSERT_MESSAGE(CanMemCpy<T>::Value, "You are trying to byte read a type that the compiler does not think is POD.");
		return r.Read(rOut);
	}

	static void Write(StreamBuffer& r, T in)
	{
		SEOUL_STATIC_ASSERT_MESSAGE(CanMemCpy<T>::Value, "You are trying to byte write a type that the compiler does not think is POD.");
		r.Write(in);
	}
};

/**
 * Read/write of a 2-byte POD type.
 */
template <typename T>
struct DebuggerReadWriteUtility<T, 2>
{
	static Bool Read(StreamBuffer& r, T& rOut)
	{
		SEOUL_STATIC_ASSERT_MESSAGE(CanMemCpy<T>::Value, "You are trying to byte read a type that the compiler does not think is POD.");
		return r.ReadLittleEndian16(rOut);
	}

	static void Write(StreamBuffer& r, T in)
	{
		SEOUL_STATIC_ASSERT_MESSAGE(CanMemCpy<T>::Value, "You are trying to byte write a type that the compiler does not think is POD.");
		r.WriteLittleEndian16(in);
	}
};

/**
 * Read/write of a 4-byte POD type.
 */
template <typename T>
struct DebuggerReadWriteUtility<T, 4>
{
	static Bool Read(StreamBuffer& r, T& rOut)
	{
		SEOUL_STATIC_ASSERT_MESSAGE(CanMemCpy<T>::Value, "You are trying to byte read a type that the compiler does not think is POD.");
		return r.ReadLittleEndian32(*((UInt32*)&rOut));
	}

	static void Write(StreamBuffer& r, T in)
	{
		SEOUL_STATIC_ASSERT_MESSAGE(CanMemCpy<T>::Value, "You are trying to byte write a type that the compiler does not think is POD.");
		r.WriteLittleEndian32((UInt32)in);
	}
};

/**
 * Read/write of an 8-byte POD type.
 */
template <typename T>
struct DebuggerReadWriteUtility<T, 8>
{
	static Bool Read(StreamBuffer& r, T& rOut)
	{
		SEOUL_STATIC_ASSERT_MESSAGE(CanMemCpy<T>::Value, "You are trying to byte read a type that the compiler does not think is POD.");
		return r.ReadLittleEndian64(*((UInt64*)&rOut));
	}

	static void Write(StreamBuffer& r, T in)
	{
		SEOUL_STATIC_ASSERT_MESSAGE(CanMemCpy<T>::Value, "You are trying to byte write a type that the compiler does not think is POD.");
		r.WriteLittleEndian64((UInt64)in);
	}
};

/**
 * Implements the protocol for talking to SlimCS enabled hosts (debuggers).
 */
class DebuggerClient SEOUL_SEALED : public Singleton<DebuggerClient>
{
public:
	/** Standard debugger port */
	static const Int32 kDebuggerPort = 25762;

	/** Sanity check to catch bad messages and avoid crashes due to OOM allocation attempts. */
	static const UInt32 kMaxMessageSize = (1 << 16);

	/** Protocol version. */
	static const UInt32 kuDebuggerVersion = 3u;

	/** Connection signature. */
	static const UInt32 kuConnectMagic = 0x75e7498f;

	SEOUL_DELEGATE_TARGET(DebuggerClient);

	DebuggerClient(FilePath appScriptProjectPath, const String& sServerHostname);
	~DebuggerClient();

	/**
	 * @return true if the debugger server is currently listening
	 */
	Bool IsDebuggerServerListening() const
	{
		return m_bDebuggerServerListening;
	}

	// Hard check to refresh listener status. Slow.
	void RefreshDebuggerServerListening();

	/**
	 * When enabled, log messages will be generated for nearly
	 * every debugger action (client or server). Useful for
	 * diagnosing bugs in the debugger itself.
	 *
	 * @return true if verbose logging is enabled, false otherwise.
	 */
	Bool GetVerboseLogging() const
	{
		return m_bVerboseLogging;
	}

	/** Set the logging level to "verbose". */
	void SetVerboseLogging(Bool bVerbose)
	{
		m_bVerboseLogging = bVerbose;
	}

	/**
	 * Marks the current debug exection break point (the UInt32
	 * is a combination of a 16-bit file mapping and a 16-bit
	 * line number). FilePath is typically empty - it is used
	 * for stack traces that haven't been given ids by
	 * the server the first time they are encountered.
	 */
	struct BreakInfo SEOUL_SEALED
	{
		BreakInfo()
			: m_uBreakpoint(0u)
			, m_FileName()
		{
		}

		UInt32 m_uBreakpoint;
		HString m_FileName;
	}; // struct BreakInfo

	/**
	 * Structure used to record information about a global or local variable.
	 */
	struct VariableInfo SEOUL_SEALED
	{
		/**
		 * Symbol that identifies the variable - required.
		 */
		String m_sName;

		/** Basic type of the variable. */
		DebuggerVariableType m_eType;

		/** Additional type info if the variable is of type table. */
		String m_sExtendedType;

		/** Value of a variable converted to a string. */
		String m_sValue;
	}; // struct VariableInfo

	/**
	 * Info about the current function frame. Exists per stack level in the current VM.
	 */
	struct FrameInfo SEOUL_SEALED
	{
		typedef Vector<VariableInfo, MemoryBudgets::Scripting> Variables;
		Variables m_vVariables;
	}; // struct FrameInfo

	/**
	 * A single level in the current stack of a VM. Tracks the name of the function
	 * at this stack level, file/line position (as "BreakInfo"), and frame information
	 * tracking registers and stack variables that are in the current stack level.
	 */
	struct StackInfo SEOUL_SEALED
	{
		/** Name of the function we're in at the current stack level. */
		HString m_FunctionName;

		/** File/line information, primarily used to detect user breakpoints. */
		BreakInfo m_BreakInfo;
	}; // struct StackInfo

	/** Used to track the call stack of a movie. */
	typedef Vector<StackInfo, MemoryBudgets::Scripting> Stack;

	/**
	 * All data associated with an individual VM that we're tracking for debugging purposes.
	 */
	struct VmEntry SEOUL_SEALED
	{
		VmEntry()
			: m_PendingGetChildren()
			, m_PendingSetVariable()
			, m_iPendingGetStackFrame(-1)
			, m_pVm(nullptr)
			, m_eExecuteState(DebuggerExecuteState::kRunning)
			, m_ePendingExecuteState(DebuggerExecuteState::kRunning)
			, m_iStepStackFrames(-1)
			, m_StepBreakInfo()
		{
		}

		/**
		 * When the debugger server sends a GetChildren message, the receiver thread can't handle it
		 * immediately (the receiver thread is never the execution thread of the VM). Because
		 * we only allow GetChildren handling for VMs at a InternalBreak(), this structure is populated
		 * with the GetChildren request and the InternalBreak() is temporarily suspended.
		 *
		 * The break logic checks this member and if defined, handles the GetChildren request and
		 * then resumes the break.
		 */
		struct PendingGetChildren SEOUL_SEALED
		{
			PendingGetChildren()
				: m_uStackDepth(0)
				, m_sPath()
			{
			}

			/** Starting frame context - offset in the stack. */
			UInt32 m_uStackDepth;

			/** Full path to the variable to lookup, dot separated. */
			String m_sPath;

			/** @return True if a GetChildren is pending, false otherwise. */
			Bool IsValid() const
			{
				return !m_sPath.IsEmpty();
			}

			/** Restore this to its default state of !IsValid(). */
			void Reset()
			{
				m_uStackDepth = 0u;
				m_sPath.Clear();
			}
		}; // struct PendingGetChildren

		/** When an InternalBreak() resumes, if this is defined, a GetChildren message will be fulfilled to the server and the logic will again InternalBreak(). */
		PendingGetChildren m_PendingGetChildren;

		/**
		 * When the debugger server sends a SetVariable message, the receiver thread can't handle it
		 * immediately (the receiver thread is never the execution thread of the VM). Because
		 * we only allow SetVariable handling for VMs at an InternalBreak(), this structure is populated
		 * with the SetVariable request and the InternalBreak() is temporarily suspended.
		 *
		 * The break logic checks this member and if defined, handles the SetVariable request and
		 * then resumes the break.
		 */
		struct PendingSetVariable SEOUL_SEALED
		{
			PendingSetVariable()
				: m_uStackDepth(0u)
				, m_sPath()
				, m_eType(DebuggerVariableType::kNil)
				, m_sValue()
			{
			}

			/** Starting frame context - offset in the stack. */
			UInt32 m_uStackDepth;

			/** Full path to the variable to set, dot separated. */
			String m_sPath;

			/** Type of the value that has been sent. */
			DebuggerVariableType m_eType;

			/** Value to apply. */
			String m_sValue;

			/** @return True if a SetVariable is pending, false otherwise. */
			Bool IsValid() const
			{
				return !m_sPath.IsEmpty();
			}

			/** Restore this to its default state of !IsValid(). */
			void Reset()
			{
				m_uStackDepth = 0u;
				m_sPath.Clear();
				m_eType = DebuggerVariableType::kNil;
				m_sValue.Clear();
			}
		}; // struct PendingSetVariable

		/** When an InternalBreak() resumes, if this is defined, a SetVariable message will be fulfilled to the server and the logic will again InternalBreak(). */
		PendingSetVariable m_PendingSetVariable;

		/** When an InternalBreak() resumes, if this is >= 0, a GetFrame message will be fulfilled to the server. */
		Int m_iPendingGetStackFrame;

		/**
		 * Raw pointer to the VM this data is associated to.
		 *
		 * \warning Do not use this pointer. Is it used as a key. In all cases, if the Lua
		 * state needs to be queried or mutated, a fresh lua_State pointer will be present
		 * on the stack.
		 */
		CheckedPtr<lua_State> m_pVm;

		/** Lookup from raw Lua source data to 16-bit identifier. */
		typedef HashTable<void const*, UInt16, MemoryBudgets::Scripting> Lookup;
		typedef HashTable<void const*, HString, MemoryBudgets::Scripting> FileLookup;
		Lookup m_tLookup;
		FileLookup m_tFileLookup;

		/** Current run mode of the VM. */
		DebuggerExecuteState m_eExecuteState;
		DebuggerExecuteState m_ePendingExecuteState;

		/** If the run mode is one of the kStep* variants, this is the stack index when the mode was first entered. */
		Int m_iStepStackFrames;
		/** If the run mode is one of the kStep* variants, this is the breakpoint when the mode was first entered. */
		BreakInfo m_StepBreakInfo;
	}; // struct VmEntry

	/**
	 * All communication between client and server shares a message format represented by this structure. On the wire,
	 * the format is:
	 * - 32-bits: size of data in bytes after the tag
	 * - 32-bits: tag
	 * - <remaining data size defined by the first 32-bit field>
	 *
	 * The tag is either one of Script::DebuggerClientTag, for messages sent from client-to-server, or
	 * Script::DebuggerServerTag, for messages sent from server-to-client.
	 *
	 * In general, the debugger protocol is send/response - most server-to-client messages have
	 * a corresponding echo response. In most cases, this is a simple message (just a tag) or a message
	 * with a body that matches the body sent with the server-to-client message. There are a few out-of-band
	 * messages or messages that are sent by the client without prompting from the server (kScript, for example).
	 */
	struct Message SEOUL_SEALED
	{
		Message()
			: m_uTag(0u)
			, m_Data()
		{
		}

		// TODO: Cache the message structures instead of SEOUL_NEW/SafeDelete for each one.

		// These methods return "simple" messages, with the eTag type and no data (the message size field is 0).
		static Message* Create(DebuggerClientTag eTag);
		static Message* Create(DebuggerServerTag eTag);

		// Used to generate a Message structure from incoming data.
		static Message* Create(SocketStream& rStream);

		// Concrete methods for generating client-to-server messages.
		static Message* CreateClientAskBreakpoints();
		static Message* CreateClientFrame(CheckedPtr<lua_State> pLuaVm, UInt32 uDepth);
		static Message* CreateClientGetChildren(CheckedPtr<lua_State> pLuaVm, UInt32 uStackDepth, const String& sPath);
		static Message* CreateClientSetVariable(CheckedPtr<lua_State> pLuaVm, UInt32 uStackDepth, const String& sPath, DebuggerVariableType eType, const String& sValue);
		static Message* CreateClientVersion();

		template <typename T>
		Bool Read(T& r)
		{
			return DebuggerReadWriteUtility<T, sizeof(T)>::Read(m_Data, r);
		}

		Bool Read(Bool& rb)
		{
			UInt8 u;
			if (Read(u))
			{
				rb = (0u != u);
				return true;
			}
			return false;
		}

		template <typename T>
		Bool Read(T*& rp)
		{
			size_t z;
			if (Read(z))
			{
				rp = (T*)z;
				return true;
			}

			return false;
		}

		template <typename T>
		Bool Read(T const*& rp)
		{
			size_t z;
			if (Read(z))
			{
				rp = (T const*)z;
				return true;
			}

			return false;
		}

		template <typename T>
		void Write(T* p)
		{
			Write((size_t)p);
		}

		template <typename T>
		void Write(T const* p)
		{
			Write((size_t)p);
		}

		template <typename T>
		void Write(T v)
		{
			DebuggerReadWriteUtility<T, sizeof(T)>::Write(m_Data, v);
		}

		Bool Read(HString& rh);
		Bool Read(String& rs);
		void Write(Byte const* s, UInt32 uSize);
		void Write(Byte const* s)
		{
			Write(s, StrLen(s));
		}
		void Write(HString h)
		{
			Write(h.CStr(), h.GetSizeInBytes());
		}
		void Write(const String& s)
		{
			Write(s.CStr(), s.GetSize());
		}

		void Write(const VariableInfo& info);

		// Actually send this message down the wire into the stream defined by SocketStream r.
		Bool Send(SocketStream& r) const;

		/** @return true if the read stream has data remaining. */
		Bool HasData() const
		{
			return m_Data.HasMoreData();
		}

		/**
		 * Identifiers the message type, one of either Script::DebuggerClientTag for client-to-server
		 * messages, or Script::DebuggerServerTag for server-to-client messages.
		 */
		UInt32 m_uTag;

		/** Data blob of the message, can be empty for "simple" messages. */
		StreamBuffer m_Data;
	}; // struct Message.

	/** Atomic ring buffer of messages, used by the send and receive threads. */
	typedef AtomicRingBuffer<Message*> Buffer;

	/** The debugger client uses 2 worker threads, a send and receive thread, to handle communication with the debugger server. */
	struct WorkerThread SEOUL_SEALED
	{
		WorkerThread();
		~WorkerThread();

		ScopedPtr<Thread> m_pThread;
		ThreadId m_ThreadId;
		Signal m_Signal;
		Buffer m_Buffer;
		Atomic32Value<Bool> m_bShuttingDown;

		void Shutdown();
		void WaitForThread();
	}; // struct WorkerThread

private:
	void EnqueueSend(Message* pMessage);

	// This block of methods are the actual entry points through which an IggyUIMovie reports state
	// to the debugger client. They can only be accessed via an Script::DebuggerClientLock to synchronize
	// access to the debugger in chunks.
	friend class DebuggerClientLock;
	void OnStep(CheckedPtr<lua_State> pLuaVm, CheckedPtr<lua_Debug> pDebugInfo);
	void OnVmDestroy(CheckedPtr<lua_State> pLuaVm);

	/** State that must be synchronized - ONLY ACCESS USING StateLock(m_State). */
	struct State SEOUL_SEALED
	{
		State();
		~State();

		void InsideLockDestroy();

		/** Mutex locked by StateLock() to synchronize access to internal debugger client state. */
		Mutex m_Mutex;

		/** All user defined breakpoints. */
		typedef HashTable<UInt32, Bool, MemoryBudgets::Scripting> Breakpoints;
		Breakpoints m_tBreakpoints;

		/** List of known VMs. */
		typedef Vector<CheckedPtr<VmEntry>, MemoryBudgets::Scripting> Vms;
		Vms m_vVms;

		/** Utility to find a specific VM data. */
		CheckedPtr<VmEntry> FindVm(CheckedPtr<lua_State> pVm) const
		{
			for (auto const& p : m_vVms)
			{
				if (p->m_pVm == pVm)
				{
					return p;
				}
			}

			return nullptr;
		}

		// Create a BreakAt message for the server.
		Message* CreateClientBreakAt(
			DebuggerClient& r,
			VmEntry& rVmEntry,
			CheckedPtr<lua_State> pLuaVm,
			SuspendReason eSuspendReason);

		// Return breakpoint token corresponding to the given context.
		void GetCurrentBreakInfo(
			DebuggerClient& r,
			VmEntry& rVmEntry,
			CheckedPtr<lua_State> pLuaVm,
			BreakInfo& rInfo);
		void ToBreakpointUnpopulated(
			DebuggerClient& r,
			VmEntry& rVmEntry,
			CheckedPtr<lua_State> pLuaVm,
			CheckedPtr<lua_Debug> pDebugInfo,
			BreakInfo& rInfo);
		void ToBreakpointPopulated(
			DebuggerClient& r,
			VmEntry& rVmEntry,
			CheckedPtr<lua_State> pLuaVm,
			CheckedPtr<lua_Debug> pDebugInfo,
			BreakInfo& rInfo);

		/** Universal table of script identifiers, maps HString filename to a universal 16-bit ID. */
		typedef HashTable<HString, UInt16, MemoryBudgets::Scripting> Scripts;
		Scripts m_tScripts;

		/** The most recently debugger VM - will always be non-nullptr after the initial handshake. */
		CheckedPtr<VmEntry> m_pActiveVm;

		/* False until the first VM connection to the debugger server. */
		Bool m_bConnectionHandshake;

		/** Set to true when a disconnect occurs, state is flushed as soon as execution leaves the debugger client. */
		Bool m_bPendingHandleDisconnect;
	}; // struct State

	/** Used to synchronize access to m_State. */
	class StateLock SEOUL_SEALED
	{
	public:
		StateLock(State& rState)
			: m_rState(rState)
		{
			m_rState.m_Mutex.Lock();
		}

		~StateLock()
		{
			m_rState.m_Mutex.Unlock();
		}

		/** Called by the receive thread whe we lose connection to the debugger server. */
		void OnDisconnect();

		// Does the work triggered by Script::DebuggerClient::OnStep().
		void OnStep(DebuggerClient& r, CheckedPtr<lua_State> pLuaVm, CheckedPtr<lua_Debug> pDebugInfo);

		// Does the work triggered by Script::DebuggerClient::OnVmDestroy().
		void OnVmDestroy(DebuggerClient& r, CheckedPtr<lua_State> pLuaVm);

		/**
		 * This method is called by the receive thread to tell the thread that is currently at a break
		 * to unbreak, send a GetFrame response message, and then restore the break.
		 */
		Bool RequestGetStackFrame(DebuggerClient& r, UInt32 uDepth);

		/**
		 * This method is called by the receive thread to tell the thread that is currently at a break
		 * to unbreak, send a GetChildren response message, and then restore the break.
		 */
		Bool RequestGetChildren(DebuggerClient& r, UInt32 uStackDepth, const String& sPath);

		/**
		 * This method is called by the receive thread to tell the thread that is currently at a break
		 * to unbreak, attempt to update a variable value, and then restore the break.
		 */
		Bool RequestSetVariable(DebuggerClient& r, UInt32 uStackDepth, const String& sPath, DebuggerVariableType eType, const String& sValue);

		/** Get or lazily create active VM data. */
		void SetActiveVm(DebuggerClient& r, CheckedPtr<lua_State> pLuaVm);

		/** Update the execute state of the currently active VM, if one is defined. Captures the current stack index also. */
		Bool SetActiveVmExecuteState(DebuggerExecuteState eExecuteState)
		{
			if (m_rState.m_pActiveVm)
			{
				m_rState.m_pActiveVm->m_ePendingExecuteState = eExecuteState;
				return true;
			}

			return false;
		}

		/** Set or refresh a user-defined breakpoint. */
		void SetBreakpoint(UInt32 uBreakpoint)
		{
			m_rState.m_tBreakpoints.Overwrite(uBreakpoint, true);
		}

		/** Update an entry in the script to script id lookup table. */
		void SetFileAssociation(HString filename, UInt16 uId)
		{
			m_rState.m_tScripts.Overwrite(filename, uId);
		}

		/** Clear the entire set of user-defined breakpoints. */
		void UnsetAllBreakpoints()
		{
			m_rState.m_tBreakpoints.Clear();
		}

		/** Erase a user-defined breakpoint. */
		void UnsetBreakpoint(UInt32 uBreakpoint)
		{
			(void)m_rState.m_tBreakpoints.Erase(uBreakpoint);
		}

	private:
		// TODO: Lots of unfinished error handling here - not possible to quit the game
		// in most cases once we hit a break, for example.

		/**
		 * Call to break the currently active VM - this suspends execution of the current thread until
		 * the debugger server sends some form of resume message (Continue, Step*, etc.)
		 */
		void InternalBreak(
			DebuggerClient& r,
			VmEntry& rVmEntry,
			CheckedPtr<lua_State> pLuaVm,
			SuspendReason eSuspendReason);

		/** Called from OnStep() when a pending disconnect is set, to perform actual cleanup. */
		void InternalDoDisconnectCleanup(DebuggerClient& r);

		/**
		 * Similar to break, but specifically to give the client a chance to synchronize with the server
		 * (client break's until server sends a continue message).
		 */
		void InternalSync(DebuggerClient& r);

		State& m_rState;
	}; // class StateLock

	/** Base path of the app script project driving debugging. */
	FilePath const m_AppScriptProjectPath;
	String const m_sScriptsPath;

	/** File watcher for the debugger listener file. */
	ScopedPtr<FileChangeNotifier> m_pNotifier;

	/** Server hostname or IP that is hosting the debugger server. */
	String m_sServerHostname;

	/** Mutex used by Script::DebuggerClientLock to synchronize access to the debugger from multiple VMs possibly running on multiple threads. */
	Mutex m_PublicMutex;

	/** Synchronized state - ONLY ACCESS VIA StateLock(m_State). */
	State m_State;

	/** Receiver thread. */
	WorkerThread m_Receive;

	/** Sender thread. */
	WorkerThread m_Send;

	/**
	 * When a break occurs, the thread running the script at the break waits on this signal. Various
	 * messages from the server (Continue, Step*, etc.) will activate this signal for resume.
	 */
	Signal m_BreakSignal;

	/** TCP socket used for communication with the debugger server. */
	Socket m_Socket;

	/** SocketStream wrapper around m_Socket. */
	SocketStream m_Stream;

	/** Gates processing of the sender queue, set to true by the receiver thread, which manages the initial TCP handshake. */
	Atomic32Value<Bool> m_bCanSend;

	/** State of the debugger listener. */
	Atomic32Value<Bool> m_bDebuggerServerListening;

	/** True if everything and the kitchen sink should be logged, false otherwise. */
	Atomic32Value<Bool> m_bVerboseLogging;

	void InternalSafeDeleteAllBufferContents();
	void InternalPollReceive();
	void InternalReceiveThreadResetConnectionState();
	Bool ThreadReceive();
	Bool ThreadSend(UInt32& ruSentCount);
	Int ReceiveThreadBody(const Thread& thread);
	Int SendThreadBody(const Thread& thread);

	SEOUL_DISABLE_COPY(DebuggerClient);
}; // class Script::DebuggerClient

/**
 * Synchronize access to Script::DebuggerClient - all access
 * to the debugger client must be accessed via this class to
 * keep interactions with the debugger server one-at-a-time.
 */
class DebuggerClientLock SEOUL_SEALED
{
public:
	DebuggerClientLock(CheckedPtr<DebuggerClient> p)
		: m_p(p)
	{
		if (m_p)
		{
			m_p->m_PublicMutex.Lock();
		}
	}

	~DebuggerClientLock()
	{
		if (m_p)
		{
			m_p->m_PublicMutex.Unlock();
		}
	}

	void OnStep(CheckedPtr<lua_State> pLuaVm, CheckedPtr<lua_Debug> pDebugInfo)
	{
		if (m_p)
		{
			m_p->OnStep(pLuaVm, pDebugInfo);
		}
	}

	void OnVmDestroy(CheckedPtr<lua_State> pLuaVm)
	{
		if (m_p)
		{
			m_p->OnVmDestroy(pLuaVm);
		}
	}

private:
	CheckedPtr<DebuggerClient> m_p;

	SEOUL_DISABLE_COPY(DebuggerClientLock);
}; // class Script::DebuggerClientLock

} // namespace Script

#endif // /#if SEOUL_ENABLE_DEBUGGER_CLIENT

} // namespace Seoul

#endif // include guard
