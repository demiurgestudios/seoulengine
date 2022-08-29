/**
 * \file AndroidLimitedWriter.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

import java.io.IOException;
import java.io.Writer;

/**
 * Utility, wrapper around writer that limits the total
 * amount of data that can be written.
 */
public final class AndroidLimitedWriter extends Writer
{
	final int m_iMaxSize;
	final Writer m_Writer;
	int m_iSize;

	public AndroidLimitedWriter(final Writer writer, final int iMaxSize)
	{
		super(writer);

		m_iMaxSize = iMaxSize;
		m_Writer = writer;
	}

	@Override
	public void close() throws IOException
	{
		m_Writer.close();
	}

	@Override
	public void flush() throws IOException
	{
		m_Writer.flush();
	}

	@Override
	public void write(char[] aBuffer, int iOffset, int iSize) throws IOException
	{
		if (m_iSize + iSize < m_iMaxSize)
		{
			m_Writer.write(aBuffer, iOffset, iSize);
			m_iSize += iSize;
		}
		else
		{
			m_Writer.write(aBuffer, iOffset, m_iMaxSize - m_iSize);
			m_iSize = m_iMaxSize;
		}
	}
}
