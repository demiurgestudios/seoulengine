// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.IO;
using System.Runtime.InteropServices;

namespace SlimCSLib
{
    /// <summary>
    /// System.Console wrapper.
    /// </summary>
    /// <remarks>
    /// Detects and disables output or error to the Console. Prevents bad behavior
    /// when in the SlimCS daemon (which runs as a detected process with no stdout or stderr).
    /// </remarks>
    public static class ConsoleWrapper
    {
        #region Non-public members
        static readonly bool s_bHasStdError;
        static readonly bool s_bHasStdOutput;

        #region kernel32.dll access to std handle info.
        static readonly IntPtr INVALID_HANDLE_VALUE = new IntPtr(-1);
        const int STD_ERROR_HANDLE = -12;
        const int STD_OUTPUT_HANDLE = -11;

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr GetStdHandle(int iStdHandle);
        #endregion

        static bool HasStdHandle(int iStdHandle)
        {
            var handle = GetStdHandle(iStdHandle);
            return (IntPtr.Zero != handle && INVALID_HANDLE_VALUE != handle);
        }

        static ConsoleWrapper()
        {
			// Track whether the process has stderr and stdout handles.
            s_bHasStdError = HasStdHandle(STD_ERROR_HANDLE);
            s_bHasStdOutput = HasStdHandle(STD_OUTPUT_HANDLE);

			// If either handle exists, this is assumed to not be a fully detached SlimCS daemon
			// process, so we enable UTF8 encoding on console outputs.
            if (s_bHasStdError || s_bHasStdOutput)
            {
                // Ensure Unicode outputs are preserved.
			    System.Console.OutputEncoding = System.Text.Encoding.UTF8;
            }
        }
        #endregion

        public static TextWriter Out
        {
            get
            {
				// Pass through Console.Out if available, otherwise return the Null writer.
                if (s_bHasStdOutput)
                {
                    return Console.Out;
                }
                else
                {
                    return TextWriter.Null;
                }
            }
        }

        public static TextWriter Error
        {
            get
            {
				// Pass through Console.Err if available, otherwise return the Null writer.
                if (s_bHasStdError)
                {
                    return Console.Error;
                }
                else
                {
                    return TextWriter.Null;
                }
            }
        }

        public static void Write(string value) { if (s_bHasStdOutput) { Console.Write(value); } }
        public static void Write(object value) { if (s_bHasStdOutput) { Console.Write(value); } }
        public static void Write(ulong value) { if (s_bHasStdOutput) { Console.Write(value); } }
        public static void Write(long value) { if (s_bHasStdOutput) { Console.Write(value); } }
        public static void Write(string format, object arg0, object arg1) { if (s_bHasStdOutput) { Console.Write(format, arg0, arg1); } }
        public static void Write(int value) { if (s_bHasStdOutput) { Console.Write(value); } }
        public static void Write(string format, object arg0) { if (s_bHasStdOutput) { Console.Write(format, arg0); } }
        public static void Write(uint value) { if (s_bHasStdOutput) { Console.Write(value); } }
        public static void Write(string format, params object[] arg) { if (s_bHasStdOutput) { Console.Write(format, arg); } }
        public static void Write(bool value) { if (s_bHasStdOutput) { Console.Write(value); } }
        public static void Write(char value) { if (s_bHasStdOutput) { Console.Write(value); } }
        public static void Write(char[] buffer) { if (s_bHasStdOutput) { Console.Write(buffer); } }
        public static void Write(char[] buffer, int index, int count) { if (s_bHasStdOutput) { Console.Write(buffer, index, count); } }
        public static void Write(string format, object arg0, object arg1, object arg2) { if (s_bHasStdOutput) { Console.Write(format, arg0, arg1, arg2); } }
        public static void Write(decimal value) { if (s_bHasStdOutput) { Console.Write(value); } }
        public static void Write(float value) { if (s_bHasStdOutput) { Console.Write(value); } }
        public static void Write(double value) { if (s_bHasStdOutput) { Console.Write(value); } }
        public static void WriteLine() { if (s_bHasStdOutput) { Console.WriteLine(); } }
        public static void WriteLine(float value) { if (s_bHasStdOutput) { Console.WriteLine(value); } }
        public static void WriteLine(int value) { if (s_bHasStdOutput) { Console.WriteLine(value); } }
        public static void WriteLine(uint value) { if (s_bHasStdOutput) { Console.WriteLine(value); } }
        public static void WriteLine(long value) { if (s_bHasStdOutput) { Console.WriteLine(value); } }
        public static void WriteLine(ulong value) { if (s_bHasStdOutput) { Console.WriteLine(value); } }
        public static void WriteLine(object value) { if (s_bHasStdOutput) { Console.WriteLine(value); } }
        public static void WriteLine(string value) { if (s_bHasStdOutput) { Console.WriteLine(value); } }
        public static void WriteLine(string format, object arg0) { if (s_bHasStdOutput) { Console.WriteLine(format, arg0); } }
        public static void WriteLine(string format, object arg0, object arg1, object arg2){ if (s_bHasStdOutput) { Console.WriteLine(format, arg0, arg1, arg2); } }
        public static void WriteLine(string format, params object[] arg){ if (s_bHasStdOutput) { Console.WriteLine(format, arg); } }
        public static void WriteLine(char[] buffer, int index, int count){ if (s_bHasStdOutput) { Console.WriteLine(buffer, index, count); } }
        public static void WriteLine(decimal value) { if (s_bHasStdOutput) { Console.WriteLine(value); } }
        public static void WriteLine(char[] buffer) { if (s_bHasStdOutput) { Console.WriteLine(buffer); } }
        public static void WriteLine(char value) { if (s_bHasStdOutput) { Console.WriteLine(value); } }
        public static void WriteLine(bool value) { if (s_bHasStdOutput) { Console.WriteLine(value); } }
        public static void WriteLine(string format, object arg0, object arg1){ if (s_bHasStdOutput) { Console.WriteLine(format, arg0, arg1); } }
        public static void WriteLine(double value) { if (s_bHasStdOutput) { Console.WriteLine(value); } }
    }
}
