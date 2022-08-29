// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Pipes;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Text;
using System.Threading;

namespace SlimCSLib.Compilation
{
	/// <summary>
	/// Defines the compilation service, used to
	/// server and connect to a compilation daemon.
	/// </summary>
	public static class Service
	{
		#region Private members
		/// <summary>
		/// Max time a client SlimCS will wait for an initial connection
		/// handshake to the SlimCS daemon.
		/// </summary>
		const int kClientConnectTimeoutMs = 500;

		#region Explicit CreateProcessW binding so we can breakway from job.
		[Flags]
		enum ProcessCreationFlags : uint
		{
			ZERO_FLAG = 0x00000000,
			CREATE_BREAKAWAY_FROM_JOB = 0x01000000,
			CREATE_DEFAULT_ERROR_MODE = 0x04000000,
			CREATE_NEW_CONSOLE = 0x00000010,
			CREATE_NEW_PROCESS_GROUP = 0x00000200,
			CREATE_NO_WINDOW = 0x08000000,
			CREATE_PROTECTED_PROCESS = 0x00040000,
			CREATE_PRESERVE_CODE_AUTHZ_LEVEL = 0x02000000,
			CREATE_SEPARATE_WOW_VDM = 0x00001000,
			CREATE_SHARED_WOW_VDM = 0x00001000,
			CREATE_SUSPENDED = 0x00000004,
			CREATE_UNICODE_ENVIRONMENT = 0x00000400,
			DEBUG_ONLY_THIS_PROCESS = 0x00000002,
			DEBUG_PROCESS = 0x00000001,
			DETACHED_PROCESS = 0x00000008,
			EXTENDED_STARTUPINFO_PRESENT = 0x00080000,
			INHERIT_PARENT_AFFINITY = 0x00010000
		}

		[StructLayout(LayoutKind.Sequential)]
		struct PROCESS_INFORMATION
		{
			public IntPtr hProcess;
			public IntPtr hThread;
			public int dwProcessId;
			public int dwThreadId;
		}

		[StructLayout(LayoutKind.Sequential)]
		struct SECURITY_ATTRIBUTES
		{
			public UInt32 nLength;
			public IntPtr lpSecurityDescriptor;
			public Int32 bInheritHandle;
		}

		[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
		struct STARTUPINFO
		{
			public Int32 cb;
			public string lpReserved;
			public string lpDesktop;
			public string lpTitle;
			public Int32 dwX;
			public Int32 dwY;
			public Int32 dwXSize;
			public Int32 dwYSize;
			public Int32 dwXCountChars;
			public Int32 dwYCountChars;
			public Int32 dwFillAttribute;
			public Int32 dwFlags;
			public Int16 wShowWindow;
			public Int16 cbReserved2;
			public IntPtr lpReserved2;
			public IntPtr hStdInput;
			public IntPtr hStdOutput;
			public IntPtr hStdError;
		}

		[return: MarshalAs(UnmanagedType.Bool)]
		[DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
		static extern bool CreateProcessW(
			string lpApplicationName,
			string lpCommandLine,
			IntPtr lpProcessAttributes,
			IntPtr lpThreadAttributes,
			bool bInheritHandles,
			uint dwCreationFlags,
			IntPtr lpEnvironment,
			string lpCurrentDirectory,
			ref STARTUPINFO lpStartupInfo,
			out PROCESS_INFORMATION lpProcessInformation);

		[return: MarshalAs(UnmanagedType.Bool)]
		[DllImport("kernel32.dll", SetLastError = true)]
		private static extern bool CloseHandle(IntPtr hObject);
		#endregion

		[return: MarshalAs(UnmanagedType.Bool)]
		[DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
		static extern bool WaitNamedPipe(string name, int timeout);

		/// <summary>
		/// Global mutex name to prevent race between clients trying to start the daemon.
		/// </summary>
		static string MutexName
		{
			get
			{
				return "SlimCSDaemonMutex";
			}
		}

		/// <summary>
		/// Global pipe name for client <-> server daemon communication.
		/// </summary>
		static string PipeName
		{
			get
			{
				return "SlimCSDaemonCommunicationPipe";
			}
		}

		/// <summary>
		/// Check if the server daemon has a listener connection.
		/// </summary>
		/// <returns>true if the server daemon is listening, false otherwise.</returns>
		static bool IsServerListening()
		{
			try
			{
				// Path to the server pipe as expected by the Win32 API.
				var sNormalizedPath = Path.GetFullPath($@"\\.\pipe\{PipeName}");

				// Query very the Win32 API.
				var bExists = WaitNamedPipe(sNormalizedPath, 0);
				if (!bExists)
				{
					// Various failure cases.
					var iError = Marshal.GetLastWin32Error();
					if (0 == iError)
					{
						// Pipe does not exist.
						return false;
					}
					else if (2 == iError)
					{
						// File not found error, pipe does not exist.
						return false;
					}
				}

				// Unknown error or success, assume it exists.
				return true;
			}
			catch (Exception)
			{
				// All exceptions, assume the server does not exist.
				return false;
			}
		}

		/// <summary>
		/// Check if a server process is active.
		/// </summary>
		/// <returns>true if the server daemon is active, false otherwise.</returns>
		static bool IsServerRunning()
		{
			// Get client data to decide where the server is located.
			var assembly = Assembly.GetEntryAssembly();
			var sApp = assembly.Location;
			var sName = Path.GetFileNameWithoutExtension(sApp);

			// Kill all processes named "SlimCS".
			var aProcesses = Process.GetProcessesByName(sName);
			var thisId = Process.GetCurrentProcess().Id;
			foreach (var process in aProcesses)
			{
				// Skip self.
				if (process.Id == thisId)
				{
					continue;
				}

				// Found one.
				return true;
			}

			return false;
		}

		// List of files we need to clone and monitor.
		static readonly HashSet<string> compilerFiles = new HashSet<string>()
		{
			"Microsoft.CodeAnalysis.CSharp.dll",
			"Microsoft.CodeAnalysis.CSharp.xml",
			"Microsoft.CodeAnalysis.dll",
			"Microsoft.CodeAnalysis.xml",
			"SlimCS.exe",
			"SlimCS.exe.config",
			"SlimCSCorlib.dll",
			"SlimCSCorlib.xml",
			"SlimCSLib.dll",
			"SlimCSLib.dll.config",
			"System.Collections.Immutable.dll",
			"System.Collections.Immutable.xml",
			"System.Reflection.Metadata.dll",
			"System.Reflection.Metadata.xml",
		};

		/// <summary>
		/// Utility that copies the client files into a separate folder
		/// to serve as the server daemon.
		/// </summary>
		/// <param name="sApp">Full path to the client application .exe.</param>
		/// <returns>Path to the server .exe.</returns>
		static string SynchronizeServerDaemon(string sApp)
		{
			var sInDir = Path.GetDirectoryName(sApp);
			var sOutDir = Path.Combine(sInDir, "SlimCSDaemon");

			// Enumerate and copy files.
			Directory.CreateDirectory(sOutDir);
			foreach (var s in compilerFiles)
			{
				var sIn = Path.GetFullPath(Path.Combine(sInDir, s));
				var sOut = Path.GetFullPath(Path.Combine(sOutDir, s));
				if (File.Exists(sOut))
				{
					File.SetAttributes(sOut, File.GetAttributes(sOut) & ~FileAttributes.ReadOnly);
				}
				File.Copy(sIn, sOut, true);
				File.SetLastWriteTime(sOut, File.GetLastWriteTime(sIn));
			}

			// Done.
			return Path.Combine(sOutDir, "SlimCS.exe");
		}

		/// <summary>
		/// Valid messages from or to the SlimCS daemon.
		/// </summary>
		enum MessageType : byte
		{
			// Placeholder for unknown/undefined message type.
			kInvalid,

			// A command-line argument for a compile job (client-to-server).
			kArg,

			// Terminator that marks the end of command-line arguments (client-to-server).
			kArgEnd,

			// Terminator that marks the end of a compilation job (server-to-client).
			kEnd,

			// Message from compilation, redirect to stderr (server-to-client).
			kError,

			// Message from compilation, redirect to stdout (server-to-client).
			kOutput,

			// Response with the server's process identifier (server-to-client).
			kPid
		}

		/// <summary>
		/// Utility for sending and receiving messages to-from the SlimCS daemon
		/// over a named interprocess pipe.
		/// </summary>
		sealed class Messenger : IDisposable
		{
			/// <summary>
			/// Sanity threshold, if a received string reports a length
			/// larger than this, we assume the message is corrupt.
			/// </summary>
			public int kMaxLength = (1 << 24);

			#region Non-public members
			readonly PipeStream m_Pipe;

			/// <summary>
			/// Encapsulates "write complete", should end all Write() bodies.
			/// </summary>
			void FlushImpl()
			{
				m_Pipe.Flush();
				m_Pipe.WaitForPipeDrain();
			}

			/// <summary>
			/// Read a MessageType value from the pipe.
			/// </summary>
			/// <returns>
			/// true on successful read, false otherwise.
			/// </returns>
			bool RawRead(out MessageType type)
			{
				// Read 1 byte of data.
				int data = m_Pipe.ReadByte();

				// -1 indicates read failure (EOF).
				if (data < 0)
				{
					// Default value and fail.
					type = MessageType.kInvalid;
					return false;
				}

				// Cast and return.
				type = (MessageType)((byte)data);
				return true;
			}

			/// <summary>
			/// Read a single UTF8 encoded string from the pipe.
			/// </summary>
			/// <returns>
			/// true on successful read, false otherwise.
			/// </returns>
			bool RawRead(out string val)
			{
				// Read the string length - on failure, default and fail.
				uint length;
				if (!RawRead(out length))
				{
					val = null;
					return false;
				}

				// Sanitize the value to our max expected string length - treat
				// very large values as a corrupted read.
				if (length > kMaxLength)
				{
					val = null;
					return false;
				}

				// Now consume sufficient bytes from the stream to
				// read the string body.
				var bytes = new byte[length];
				int read = m_Pipe.Read(bytes, 0, (int)length);
				while (read < (int)length)
				{
					// A return size of 0 always indicates EOF and a read failure.
					if (0 == read)
					{
						val = null;
						return false;
					}

					// Consume more bytes until we hit our target length.
					read = m_Pipe.Read(bytes, read, (int)length - read);
				}

				// Decode into a string and return success.
				val = Encoding.UTF8.GetString(bytes);
				return true;
			}

			/// <summary>
			/// Read a single UInt32 value from the pipe.
			/// </summary>
			/// <returns>
			/// true on successful read, false otherwise.
			/// </returns>
			bool RawRead(out uint val)
			{
				// Read 4 bytes (was sent little endian).
				int a = m_Pipe.ReadByte();
				if (a < 0) { goto fail; }
				int b = m_Pipe.ReadByte();
				if (b < 0) { goto fail; }
				int c = m_Pipe.ReadByte();
				if (c < 0) { goto fail; }
				int d = m_Pipe.ReadByte();
				if (d < 0) { goto fail; }

				// Assemble the full UInt32 value and return success.
				val = (
					(((uint)((byte)a)) << 0) |
					(((uint)((byte)b)) << 8) |
					(((uint)((byte)c)) << 16) |
					(((uint)((byte)d)) << 24));
				return true;

				// All failure cases - 0 default and return failure.
fail:
				val = 0;
				return false;
			}

			/// <summary>
			/// Write a single MessageType code to the pipe.
			/// </summary>
			void RawWrite(MessageType type)
			{
				m_Pipe.WriteByte((byte)type);
			}

			/// <summary>
			/// Write a length prefixed UTF8 string to the pipe.
			/// </summary>
			void RawWrite(string val)
			{
				// Get the UTF8 bytes of the string.
				var bytes = Encoding.UTF8.GetBytes(val);

				// Write the length of the bytes.
				RawWrite((uint)bytes.Length);

				// Write the full bytes.
				m_Pipe.Write(bytes, 0, bytes.Length);
			}

			/// <summary>
			/// Write a single UInt32 value to the pipe.
			/// </summary>
			void RawWrite(uint val)
			{
				m_Pipe.WriteByte((byte)(val >> 0));
				m_Pipe.WriteByte((byte)(val >> 8));
				m_Pipe.WriteByte((byte)(val >> 16));
				m_Pipe.WriteByte((byte)(val >> 24));
			}

			/// <summary>
			/// Write a full message - type prefix + lengthed string.
			/// </summary>
			void WriteImpl(MessageType type, string body)
			{
				RawWrite(type);
				RawWrite(body);
			}
			#endregion

			public Messenger(PipeStream pipe)
			{
				m_Pipe = pipe;
			}

			/// <summary>
			/// Read a single message from the pipe.
			/// </summary>
			/// <returns>
			/// true on successful read of a message, false otherwise.
			/// </returns>
			public bool Read(out MessageType type, out string body)
			{
				// Read the type.
				if (!RawRead(out type))
				{
					body = null;
					return false;
				}

				// Read the message body (a string).
				if (!RawRead(out body))
				{
					return false;
				}

				return true;
			}

			public void Dispose()
			{
				m_Pipe.Close();
			}

			/// <summary>
			/// Write command-line args to the pipe (client-to-server).
			/// </summary>
			public void WriteArgs(params string[] args)
			{
				// Each arg is a kArg message.
				foreach (var arg in args)
				{
					WriteImpl(MessageType.kArg, arg);
				}

				// Finish with a kArgEnd message.
				WriteImpl(MessageType.kArgEnd, string.Empty);

				// Done.
				FlushImpl();
			}

			/// <summary>
			/// Write a compilation complete message to the pipe (server-to-client).
			/// </summary>
			public void WriteEnd()
			{
				WriteImpl(MessageType.kEnd, string.Empty);
				FlushImpl();
			}

			/// <summary>
			/// Write a stderr message to the pipe (server-to-client).
			/// </summary>
			public void WriteError(string sError)
			{
				WriteImpl(MessageType.kError, sError);
				FlushImpl();
			}

			/// <summary>
			/// Write a stdout message to the pipe (server-to-client).
			/// </summary>
			public void WriteOutput(string sError)
			{
				WriteImpl(MessageType.kOutput, sError);
				FlushImpl();
			}

			/// <summary>
			/// Write the daemon process id to the pipe (server-to-client).
			/// </summary>
			public void WritePid(int iPid)
			{
				WriteImpl(MessageType.kPid, iPid.ToString());
				FlushImpl();
			}
		}

		/// <summary>
		/// Attempt to kill a server daemon willfully.
		/// </summary>
		/// <returns>true if kill was complete, false otherwise.</returns>
		/// <remarks>
		/// Sends a message to the server daemon pipe, asking the server daemon
		/// to exit. This is a graceful exit - code paths will fall back to
		/// a forced exit (kill process).
		/// </remarks>
		static bool GracefulKillServer()
		{
			try
			{
				using (var pipe = new NamedPipeClientStream(".", PipeName, PipeDirection.InOut, PipeOptions.None))
				{
					// Connect if not already connected.
					if (!pipe.IsConnected)
					{
						pipe.Connect();
					}

					using (var messenger = new Messenger(pipe))
					{
						// Send the kill argument to terminate the daemon.
						messenger.WriteArgs("--kill");

						// Get the pid.
						MessageType type;
						string sPid;
						if (messenger.Read(out type, out sPid))
						{
							// Parse the pid of the server daemon.
							int iPid = 0;
							if (int.TryParse(sPid, out iPid))
							{
								// Acquire the daemon process.
								var process = Process.GetProcessById(iPid);
								if (null != process)
								{
									// Wait for it to exit - when this suceeds, graceful
									// shutdown also succeeds.
									process.WaitForExit(kiGracefulKillTimeoutInMilliseconds);
									ConsoleWrapper.Out.WriteLine("Graceful server kill succeeded.");
									return true;
								}
							}
						}
					}
				}
			}
			catch (Exception e)
			{
				// Fall-through, report but eat the error.
				ConsoleWrapper.Error.WriteLine(e);
			}

			// If we get here, failure.
			return false;
		}

		/// <summary>
		/// Force the server to exit with a process kill.
		/// </summary>
		/// <remarks>
		/// We don't perform this operation by default, since
		/// it cannot distinguish between a host daemon and
		/// another client instance, so it make mistakenly
		/// kill a client instance.
		/// </remarks>
		static void ForceKillServer()
		{
			// Get client data to decide where the server is located.
			var assembly = Assembly.GetEntryAssembly();
			var sApp = assembly.Location;
			var sName = Path.GetFileNameWithoutExtension(sApp);

			// Kill all processes named "SlimCS".
			var aProcesses = Process.GetProcessesByName(sName);
			var thisId = Process.GetCurrentProcess().Id;
			foreach (var process in aProcesses)
			{
				// Skip self.
				if (process.Id == thisId)
				{
					continue;
				}

				// Kill it.
				process.Kill();
			}
		}

		/// <summary>
		/// Kill a running daemon if it exists.
		/// </summary>
		static void KillServer()
		{
			// Early out if not running.
			if (!IsServerRunning())
			{
				return;
			}

			try
			{
				// First try a graceful kill.
				if (!GracefulKillServer() || IsServerRunning())
				{ 
					// If still running, force kill the process.
					ForceKillServer();
				}
			}
			catch (Exception e)
			{
				// Fall-through, report but eat the error.
				ConsoleWrapper.Error.WriteLine(e);
			}
		}

		/// <summary>
		/// Kill a running daemon if it is not up-to-date.
		/// </summary>
		static void CheckServerUpToDate()
		{
			// Global lock, so multiple clients are not attempting to
			// kill the server simultaneously.
			using (var mutex = new Mutex(true, MutexName))
			{
				// Get client data to decide where the server is located.
				var assembly = Assembly.GetEntryAssembly();
				var sApp = assembly.Location;
				var sLocation = Path.GetDirectoryName(sApp);

				// Enumerate and check.
				bool bUpToDate = true;
				var sInDir = Path.GetDirectoryName(sApp);
				var sOutDir = Path.Combine(sInDir, "SlimCSDaemon");
				foreach (var s in compilerFiles)
				{
					try
					{
						var sIn = Path.GetFullPath(Path.Combine(sInDir, s));
						var sOut = Path.GetFullPath(Path.Combine(sOutDir, s));
						if (File.GetLastWriteTime(sIn) != File.GetLastWriteTime(sOut))
						{
							// Timestamp mismatch, out of date. Break immediately.
							bUpToDate = false;
							break;
						}
					}
					catch (Exception)
					{
						bUpToDate = false;
					}
				}

				// If not up to date, kill the server daemon.
				if (!bUpToDate)
				{
					ConsoleWrapper.Out.WriteLine("Killing SlimCS daemon, client files have changed and are out-of-sync.");

					// Kill the current daemon.
					KillServer();

					// Also synchronize before releasing the global mutex.
					sApp = SynchronizeServerDaemon(sApp);
				}
			}
		}

		/// <summary>
		/// Entry point to kick off a new SlimCS daemon server.
		/// </summary>
		/// <returns>true if started successfully, false otherwise.</returns>
		static bool StartServer()
		{
			// Global lock, so multiple clients are not attempting to
			// start the server simultaneously.
			using (var mutex = new Mutex(true, MutexName))
			{
				try
				{
					// Get client data to decide where to put the server.
					var assembly = Assembly.GetEntryAssembly();
					var sApp = assembly.Location;
					var sLocation = Path.GetDirectoryName(sApp);

					// Make sure the server daemon .exe is in sync with
					// the client .exe
					sApp = SynchronizeServerDaemon(sApp);

					// Launch the server process - if successful, leave it running.
					// We use an explicit p/invoke here instead of the diagnostics
					// Process wrapper because we need to breakaway or our daemon
					// may be killed with any parent process (e.g. a SeoulEngine
					// application).
					{
						var startupInfo = new STARTUPINFO
						{
							cb = Marshal.SizeOf(typeof(STARTUPINFO)),
						};
						var processInfo = new PROCESS_INFORMATION();
						if (!CreateProcessW(
							sApp,
							Utilities.FormatArgumentsForCommandLine(sApp, "--daemon", sLocation),
							IntPtr.Zero,
							IntPtr.Zero,
							false,
							(uint)(ProcessCreationFlags.CREATE_NO_WINDOW |
							ProcessCreationFlags.CREATE_BREAKAWAY_FROM_JOB |
							ProcessCreationFlags.DETACHED_PROCESS),
							IntPtr.Zero,
							null,
							ref startupInfo,
							out processInfo))
						{
							return false;
						}

						// Close our binding and return success.
						CloseHandle(processInfo.hProcess);
						return true;
					}
				}
				catch (Exception e)
				{
					ConsoleWrapper.Error.WriteLine(e);
					return false;
				}
			}
		}
		#endregion

		/// <summary>
		/// How long we give a service daemon to terminate gracefully.
		/// </summary>
		const int kiGracefulKillTimeoutInMilliseconds = 10 * 1000;

		delegate void OnFileEvent(string sName);

		/// <summary>
		/// Main function of a SlimCS.exe process running as a server daemon.
		/// </summary>
		/// <param name="daemonArgs">Args passed in to control the daemon.</param>
		public static void ServerDaemonMain(CommandArgs daemonArgs)
		{
			bool bRunning = true;

			// Loop indefinitely until we're killed.
			while (bRunning)
			{
				try
				{
					using (var pipe = new NamedPipeServerStream(PipeName, PipeDirection.InOut, 1))
					{
						try
						{
							using (var messenger = new Messenger(pipe))
							{
								// Stdout/stderr routing for server daemon builds.
								Pipeline.WriteLineHandler stdErr = (sLine) =>
								{
									messenger.WriteError(sLine);
								};
								Pipeline.WriteLineHandler stdOut = (sLine) =>
								{
									messenger.WriteOutput(sLine);
								};

								// Inner loop until explicit kill.
								while (bRunning)
								{
									// Blocking wait on a client connection.
									pipe.WaitForConnectionAsync().Wait();

									try
									{
										// Success, read input arguments to invoke.
										CommandArgs args;
										{
											var rawArgs = new List<string>();

											// Consume all args - expect a kArgEnd message to terminate, though
											// we don't check for that explicitly (the presence of one simple
											// avoids the server prematurely consuming a different message).
											MessageType type;
											string sBody;
											while (messenger.Read(out type, out sBody) && MessageType.kArg == type)
											{
												rawArgs.Add(sBody);
											}

											args = new CommandArgs(rawArgs.ToArray());
										}

										// If requested, stop runnning.
										if (args.m_bKill)
										{
											messenger.WritePid(Process.GetCurrentProcess().Id);
											bRunning = false;
											continue;
										}

										try
										{
											// Perform the invocation.
											Pipeline.Build(
												stdOut,
												stdErr,
												args,
												false,
												false);
										}
										catch (Exception e) when
											(e is SlimCSCompilationException ||
											e is CSharpCompilationException ||
											e is TestRunException ||
											e is UnsupportedNodeException)
										{
											stdErr(e.Message);
										}
										catch (Exception e)
										{
											stdErr("Unexpected exception, likely, a compiler bug.");
											stdErr(e.ToString());
										}

										// Terminate.
										messenger.WriteEnd();
									}
									finally
									{
										try
										{
											pipe.Disconnect();
										}
										catch
										{
										}
									}
								}
							}
						}
						finally
						{
							try
							{
								pipe.Close();
							}
							catch
							{
							}
						}
					}
				}
				catch (Exception)
				{
					// Ignore.
				}

				// Force the GC to run so we're not idle with high memory usage.
				GC.Collect();
			}
		}

		/// <summary>
		/// Build entry point that runs a build through a server daemon.
		/// </summary>
		/// <param name="args">Arguments to pass to the build.</param>
		/// <returns>true on build success, false on failure.</returns>
		/// <remarks>
		/// A daemon is used to eliminate the time for JIT compiling the SlimCS
		/// compiler itself, which can be significant.
		/// </remarks>
		public static bool DaemonBuild(CommandArgs args)
		{
			// Check if a running daemon is up-to-date. This will kill
			// the daemon if it is out-of-date with the client process.
			CheckServerUpToDate();

			// Start the server daemon if it is not running already.
			if (!IsServerListening())
			{
				// If the server is running but not listening, it means
				// there is another active build, so abandon the daemon build.
				if (IsServerRunning())
				{
					return false;
				}

				// If server failed to start, just build immediately.
				if (!StartServer())
				{
					// Fallback handling on error.
					Pipeline.Build(ConsoleWrapper.Out.WriteLine, ConsoleWrapper.Error.WriteLine, args, false, false);
					return true;
				}
			}

			try
			{
				using (var pipe = new NamedPipeClientStream(".", PipeName, PipeDirection.InOut, PipeOptions.None))
				{
					try
					{
						// Connect if not already connected.
						if (!pipe.IsConnected)
						{
							// Connect with a timeout will raise a TimeoutException,
							// this will be handled by our blanket outer process (catch and report
							// the exception, then fallback to an in-process compilation).
							pipe.Connect(kClientConnectTimeoutMs);
						}

						using (var messenger = new Messenger(pipe))
						{
							// Send arguments.
							messenger.WriteArgs(args.OriginalArguments);

							// Now get results.
							bool bSuccess = true;
							while (true)
							{
								// Consume a message - we expect to terminate gracefully
								// by receiving a kEnd message. Premature pipe close indicates
								// something bad may have happened to the daemon, so we
								// assume that and fallback to our outer handling (report
								// the exception and try an in-process build).
								MessageType type;
								string sBody;
								if (!messenger.Read(out type, out sBody))
								{
									throw new Exception("SlimCS daemon terminated unexpectedly.");
								}

								// Process the message.
								switch (type)
								{
										// Output for stderr. Also indicates a compilation failure.
									case MessageType.kError:
										ConsoleWrapper.Error.WriteLine(sBody);
										bSuccess = false;
										break;

										// Output for stdout.
									case MessageType.kOutput:
										ConsoleWrapper.Out.WriteLine(sBody);
										break;
										
										// Compilation has finished gracefully.
									case MessageType.kEnd:
										return bSuccess;

										// Any other message is unexpected server-to-client, so
										// we raise an exception and fallback to our outer handling.
									default:
										throw new Exception($"Unexpected output from daemon: ({type}, '{sBody}')");
								}
							}
						}
					}
					finally
					{
						try
						{
							pipe.Close();
						}
						catch
						{
						}
					}
				}
			}
			catch (Exception e)
			{
				// Fall-through, perform manual build.
				ConsoleWrapper.Error.WriteLine(e);
			}

			// Fallback handling if error talking to server daemon.
			Pipeline.Build(ConsoleWrapper.Out.WriteLine, ConsoleWrapper.Error.WriteLine, args, false, false);
			return true;
		}
	}
}
