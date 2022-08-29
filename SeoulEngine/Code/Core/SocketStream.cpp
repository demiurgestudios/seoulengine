/**
 * \file SocketStream.cpp
 * \brief Wrapper class for streaming data over a socket
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "Path.h"
#include "SeoulSocket.h"
#include "SocketStream.h"
#include "Vector.h"

#include <stdint.h>

namespace Seoul
{

/**
 * Attaches this SocketStream to the given socket.  The socket need not be
 * connected to a peer.  The stream remains attached when the socket is closed,
 * however it's not of much use until after the socket is reconnected.  When
 * the socket is closed, SocketStream::Clear() should be called to clear out
 * any unwritten/unread buffered data.
 *
 * At most one SocketStream should ever be attached to a socket, otherwise
 * the behavior will be undefined.
 */
SocketStream::SocketStream(Socket& socket)
	: m_Socket(socket)
	, m_nSendBufferCurrentSize(0)
	, m_nReceiveBufferOffset(0)
	, m_nReceiveBufferCurrentSize(0)
{
}

/** Writes a binary blob of data into the socket stream */
Bool SocketStream::Write(const void *pData, SizeType zSize)
{
	SizeType uBytesLeft = kSendBufferSize - m_nSendBufferCurrentSize;
	if (zSize < uBytesLeft)
	{
		// Normal case, copy the data into the buffer
		memcpy(m_aSendBuffer + m_nSendBufferCurrentSize, pData, zSize);
		m_nSendBufferCurrentSize += zSize;

		return true;
	}
	else if (zSize > kSendBufferSize)
	{
		// If we're writing more than one buffer's worth, just send it all
		// and don't copy it
		return (Flush() && WriteImmediate(pData, zSize));
	}
	else
	{
		// Otherwise, we just filled up the buffer, so split this data
		memcpy(m_aSendBuffer + m_nSendBufferCurrentSize, pData, uBytesLeft);
		pData = (const char *)pData + uBytesLeft;
		zSize -= uBytesLeft;
		m_nSendBufferCurrentSize += uBytesLeft;

		if (Flush())
		{
			if (zSize > 0)
			{
				memcpy(m_aSendBuffer, pData, zSize);
				m_nSendBufferCurrentSize += zSize;
			}

			return true;
		}
		else
		{
			return false;
		}
	}
}

/** Writes a 1-bit boolean into the socket stream */
Bool SocketStream::Write1(Bool bBit)
{
	return Write8((UInt8)bBit);
}

/** Writes a signed 8-bit integer into the socket stream */
Bool SocketStream::Write8(Int8 iData)
{
	return Write8((UInt8)iData);
}

/** Writes an unsigned 8-bit integer into the socket stream */
Bool SocketStream::Write8(UInt8 uData)
{
	return Write(&uData, sizeof(uData));
}

/** Writes a signed 16-bit integer into the socket stream */
Bool SocketStream::Write16(Int16 iData)
{
	return Write16((UInt16)iData);
}

/** Writes an unsigned 16-bit integer into the socket stream */
Bool SocketStream::Write16(UInt16 uData)
{
	uData = htons(uData);
	return Write(&uData, sizeof(uData));
}

/** Writes a signed 32-bit integer into the socket stream */
Bool SocketStream::Write32(Int32 iData)
{
	return Write32((UInt32)iData);
}

/** Writes an unsigned 32-bit integer into the socket stream */
Bool SocketStream::Write32(UInt32 uData)
{
	uData = htonl(uData);
	return Write(&uData, sizeof(uData));
}

/** Writes a signed 64-bit integer into the socket stream */
Bool SocketStream::Write64(Int64 iData)
{
	return Write64((UInt64)iData);
}

/** Writes an unsigned 64-bit integer into the socket stream */
Bool SocketStream::Write64(UInt64 uData)
{
#if SEOUL_LITTLE_ENDIAN
	uData = EndianSwap64(uData);
#endif
	return Write(&uData, sizeof(uData));
}

/** Writes a String into the socket stream */
Bool SocketStream::Write(const String& sString)
{
	UInt32 uLength = sString.GetSize();

	return
		Write32(uLength) &&
		Write(sString.CStr(), uLength);
}

/** Writes an HString into the socket stream */
Bool SocketStream::Write(HString hString)
{
	UInt32 uLength = hString.GetSizeInBytes();

	return
		Write32(uLength) &&
		Write(hString.CStr(), uLength);
}

/** Writes a FilePath into the socket stream */
Bool SocketStream::Write(FilePath filePath)
{
	SEOUL_STATIC_ASSERT((UInt32)GameDirectory::GAME_DIRECTORY_COUNT <= (UInt32)UINT8_MAX);

	// Normalize slashes - arbitrary choose the "\" as the "net" version.
	String sRelativeFilenameWithoutExtension(filePath.GetRelativeFilenameWithoutExtension().ToString());
	sRelativeFilenameWithoutExtension = sRelativeFilenameWithoutExtension.ReplaceAll("/", "\\");

	return
		Write8((UInt8)filePath.GetDirectory()) &&
		Write8((UInt8)filePath.GetType()) &&
		Write(sRelativeFilenameWithoutExtension);
}

/**
 * Flushes the current buffer and immediately sends the data without copying it
 */
Bool SocketStream::WriteImmediate(const void *pData, SizeType zSize)
{
	if (!Flush())
	{
		return false;
	}

	int nBytesSent = m_Socket.SendAll(pData, zSize);
	if (nBytesSent < (int)zSize)
	{
		SEOUL_LOG_NETWORK("SocketStream::WriteImmediate: error %d sending data\n", Socket::GetLastSocketError());
		return false;
	}

	return true;
}

/**
 * Sends any currently buffered write data and clears out the send buffer.
 *
 * @return True if the data was successfully sent, or false if an error
 *         occurred
 */
Bool SocketStream::Flush()
{
	// Anything to send?
	if (m_nSendBufferCurrentSize == 0)
	{
		return true;
	}

	if (m_Socket.SendAll(m_aSendBuffer, m_nSendBufferCurrentSize) < m_nSendBufferCurrentSize)
	{
		SEOUL_LOG_NETWORK("SocketStream::Flush: error %d sending data\n", Socket::GetLastSocketError());
		m_nSendBufferCurrentSize = 0;
		return false;
	}

	m_nSendBufferCurrentSize = 0;

	return true;
}

/** Reads a binary blob of data from this socket stream */
Bool SocketStream::Read(void *pBuffer, SizeType zSize)
{
	// If we have the data available in the buffer, copy it in (normal case)
	if ((SizeType)m_nReceiveBufferCurrentSize >= zSize)
	{
		memcpy(pBuffer, m_aReceiveBuffer + m_nReceiveBufferOffset, zSize);
		m_nReceiveBufferOffset += zSize;
		m_nReceiveBufferCurrentSize -= zSize;
		return true;
	}
	else if (zSize > kReceiveBufferSize)
	{
		// If we're trying to receive more than our total buffer size, just
		// read it directly into the destination buffer
		return ReadImmediate(pBuffer, zSize);
	}
	else
	{
		// Otherwise, do a partial copy into the buffer, then keep receiving
		// data until we get enough or an error occurs
		SizeType uFirstCopySize = m_nReceiveBufferCurrentSize;
		memcpy(pBuffer, m_aReceiveBuffer + m_nReceiveBufferOffset, uFirstCopySize);

		pBuffer = (char *)pBuffer + uFirstCopySize;
		zSize -= uFirstCopySize;
		m_nReceiveBufferOffset = 0;
		m_nReceiveBufferCurrentSize = 0;

		while (m_nReceiveBufferCurrentSize < (int)zSize)
		{
			// Try to read some more data to fill up our buffer
			int nBytesRead = m_Socket.Receive(m_aReceiveBuffer + m_nReceiveBufferCurrentSize, kReceiveBufferSize - m_nReceiveBufferCurrentSize);
			if (nBytesRead < 0)
			{
				return false;
			}

			m_nReceiveBufferCurrentSize += nBytesRead;
		}

		// Now consume the data
		memcpy(pBuffer, m_aReceiveBuffer, zSize);
		m_nReceiveBufferOffset += zSize;
		m_nReceiveBufferCurrentSize -= zSize;

		return true;
	}
}

/** Reads a 1-bit boolean from the socket stream */
Bool SocketStream::Read1(Bool& bOutBit)
{
	UInt8 uByte;
	if (Read(&uByte, sizeof(uByte)))
	{
		bOutBit = (uByte != 0);
		return true;
	}
	else
	{
		return false;
	}
}

/** Reads a signed 8-bit integer from the socket stream */
Bool SocketStream::Read8(Int8& iOutData)
{
	return Read8((UInt8&)iOutData);
}

/** Reads an unsigned 8-bit integer from the socket stream */
Bool SocketStream::Read8(UInt8& uOutData)
{
	return Read(&uOutData, sizeof(uOutData));
}

/** Reads a signed 16-bit integer from the socket stream */
Bool SocketStream::Read16(Int16& iOutData)
{
	return Read16((UInt16&)iOutData);
}

/** Reads an unsigned 16-bit integer from the socket stream */
Bool SocketStream::Read16(UInt16& uOutData)
{
	UInt16 uData;
	if (Read(&uData, sizeof(uData)))
	{
		uOutData = ntohs(uData);
		return true;
	}
	else
	{
		return false;
	}
}

/** Reads a signed 32-bit integer from the socket stream */
Bool SocketStream::Read32(Int32& iOutData)
{
	return Read32((UInt32&)iOutData);
}

/** Reads an unsigned 32-bit integer from the socket stream */
Bool SocketStream::Read32(UInt32& uOutData)
{
	UInt32 uData;
	if (Read(&uData, sizeof(uData)))
	{
		uOutData = ntohl(uData);
		return true;
	}
	else
	{
		return false;
	}
}

/** Reads a signed 64-bit integer from the socket stream */
Bool SocketStream::Read64(Int64& iOutData)
{
	return Read64((UInt64&)iOutData);
}

/** Reads an unsigned 64-bit integer from the socket stream */
Bool SocketStream::Read64(UInt64& uOutData)
{
	UInt64 uData;
	if (Read(&uData, sizeof(uData)))
	{
		uOutData = uData;
#if SEOUL_LITTLE_ENDIAN
		uOutData = EndianSwap64(uOutData);
#endif
		return true;
	}
	else
	{
		return false;
	}
}

/** Reads a String from the socket stream */
Bool SocketStream::Read(String& sOutString)
{
	UInt32 uLength;
	if (Read32(uLength))
	{
		SEOUL_ASSERT(uLength < 0x01000000);  // Sanity check

		// If an empty string, clear the output string and return success.
		if (uLength == 0)
		{
			sOutString.Clear();
			return true;
		}

		// FIXME: Optimize this to only do 1 allocation instead of 2
		Vector<Byte> vString(uLength);
		if (Read(vString.Get(0), uLength))
		{
			sOutString.Assign(vString.Get(0), uLength);
			return true;
		}
	}

	return false;
}

/** Reads an HString from the socket stream */
Bool SocketStream::Read(HString& hOutString)
{
	UInt32 uLength;
	if (Read32(uLength))
	{
		SEOUL_ASSERT(uLength <= UINT16_MAX);  // Sanity check

		// If an empty string, clear the output string and return success.
		if (uLength == 0)
		{
			hOutString = HString();
			return true;
		}

		Vector<Byte> vString(uLength);
		if (Read(vString.Get(0), uLength))
		{
			hOutString = HString(vString.Get(0), uLength, false);
			return true;
		}
	}

	return false;
}

/** Reads a FilePath from the socket stream */
Bool SocketStream::Read(FilePath& outFilePath)
{
	UInt8 uDirectory, uType;
	String sRelativePathWithoutExtension;
	if (Read8(uDirectory) &&
		Read8(uType) &&
		Read(sRelativePathWithoutExtension))
	{
		// Normalize slashes - arbitrary choose the "\" as the "net" version.
		sRelativePathWithoutExtension =
			sRelativePathWithoutExtension.ReplaceAll("\\", Path::DirectorySeparatorChar());

		outFilePath.SetDirectory((GameDirectory)uDirectory);
		outFilePath.SetType((FileType)uType);
		outFilePath.SetRelativeFilenameWithoutExtension(FilePathRelativeFilename(sRelativePathWithoutExtension));
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Receives data directly into the buffer without copying, except buffered data
 * that's already been received is copied into the buffer.
 */
Bool SocketStream::ReadImmediate(void *pBuffer, SizeType zSize)
{
	// Copy in any current buffered data
	if (m_nReceiveBufferCurrentSize > 0)
	{
		// Is all the data we need buffered already?
		if (zSize <= (SizeType)m_nReceiveBufferCurrentSize)
		{
			memcpy(pBuffer, m_aReceiveBuffer + m_nReceiveBufferOffset, zSize);
			m_nReceiveBufferOffset += zSize;
			m_nReceiveBufferCurrentSize -= zSize;
			return true;
		}
		else
		{
			// Do a partial copy
			memcpy(pBuffer, m_aReceiveBuffer + m_nReceiveBufferOffset, m_nReceiveBufferCurrentSize);
			pBuffer = (char *)pBuffer + m_nReceiveBufferCurrentSize;
			zSize -= m_nReceiveBufferCurrentSize;
			m_nReceiveBufferOffset = 0;
			m_nReceiveBufferCurrentSize = 0;
		}
	}

	// Read the data directly into the destination buffer
	return (m_Socket.ReceiveAll(pBuffer, zSize) == (int)zSize);
}

/** Clears out any buffered send/receive data */
void SocketStream::Clear()
{
	m_nSendBufferCurrentSize = 0;
	m_nReceiveBufferOffset = 0;
	m_nReceiveBufferCurrentSize = 0;
}

} // namespace Seoul
