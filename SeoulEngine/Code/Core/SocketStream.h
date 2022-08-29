/**
 * \file SocketStream.h
 * \brief Wrapper class for streaming data over a socket.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SOCKET_STREAM_H
#define SOCKET_STREAM_H

#include "FilePath.h"
#include "Vector.h"
namespace Seoul { class Socket; }

namespace Seoul
{

/**
 * Wrapper class for streaming data over a socket.  Provides data buffering and
 * convenience methods for serializing various types of data.
 */
class SocketStream SEOUL_SEALED
{
public:
	/** Typed used for sized reads and writes. */
	typedef UInt32 SizeType;

	// Attaches the stream to the socket, which does not need to be connected
	SocketStream(Socket& socket);

	// Writes various kinds of data into the buffer in network byte order
	// (big-endian).  If the buffer fills up, it is sent synchronously.
	// Returns true if data was written successfully.
	Bool Write(const void *pData, SizeType zSize);
	Bool Write1(Bool bBit);
	Bool Write8(Int8 iData);
	Bool Write8(UInt8 uData);
	Bool Write16(Int16 iData);
	Bool Write16(UInt16 uData);
	Bool Write32(Int32 iData);
	Bool Write32(UInt32 uData);
	Bool Write64(Int64 iData);
	Bool Write64(UInt64 uData);
	Bool Write(const String& sString);
	Bool Write(HString hString);
	Bool Write(FilePath filePath);

	// Flushes the current buffer and immediately sends the data without
	// copying it
	Bool WriteImmediate(const void *pData, SizeType zSize);

	// Writes a length-prefixed vector of objects into the socket stream
	template <typename T>
	Bool Write(const Vector<T>& vData)
	{
		UInt32 uLength = vData.GetSize();
		if (!Write32(uLength))
		{
			return false;
		}

		for (UInt32 i = 0; i < uLength; i++)
		{
			if (!Write(vData[i]))
			{
				return false;
			}
		}

		return true;
	}

	// Sends any currently buffered data
	Bool Flush();

	// Reads various kinds of data out of the receive the buffer from network
	// byte order (big-endian).  If the buffer is empty, more data is
	// received synchronously.  Returns true if data was read successfully.
	Bool Read(void *pBuffer, SizeType zSize);
	Bool Read1(Bool& bOutBit);
	Bool Read8(Int8& iOutData);
	Bool Read8(UInt8& uOutData);
	Bool Read16(Int16& iOutData);
	Bool Read16(UInt16& uOutData);
	Bool Read32(Int32& iOutData);
	Bool Read32(UInt32& uOutData);
	Bool Read64(Int64& iOutData);
	Bool Read64(UInt64& uOutData);
	Bool Read(String& sOutString);
	Bool Read(HString& hOutString);
	Bool Read(FilePath& outFilePath);

	// Receives data directly into the buffer without copying, except buffered
	// data that's already been received is copied into the buffer.
	Bool ReadImmediate(void *pBuffer, SizeType zSize);

	// Reads a length-prefixed vector of objects from the socket stream
	template <typename T>
	Bool Read(Vector<T>& vOutData)
	{
		UInt32 uLength;
		if (!Read32(uLength))
		{
			return false;
		}

		vOutData.Resize(uLength);
		for (UInt32 i = 0; i < uLength; i++)
		{
			if (!Read(vOutData[i]))
			{
				return false;
			}
		}

		return true;
	}

	/** Clears out any buffered send/receive data */
	void Clear();

private:
	SEOUL_DISABLE_COPY(SocketStream);

	/** Socket to which we are attached */
	Socket& m_Socket;

	/** Total size of the send buffer */
	static const SizeType kSendBufferSize = 8192;

	/** Buffer used for buffering data to send */
	Byte m_aSendBuffer[kSendBufferSize];

	/** Amount of data currently in the send buffer */
	int m_nSendBufferCurrentSize;

	/** Size of the receive buffer */
	static const SizeType kReceiveBufferSize = 16384;

	/** Buffer used for buffering received data */
	Byte m_aReceiveBuffer[kReceiveBufferSize];

	/**
	 * Pointer to beginning of the received but unread data in the receive
	 * buffer
	 */
	int m_nReceiveBufferOffset;

	/** Size of received but unread data in the receive buffer */
	int m_nReceiveBufferCurrentSize;
}; // class SocketStream

} // namespace Seoul

#endif  // Include guard
