/**
 * \file PrimitiveType.h
 * \brief Enum of types that describe a primitive stream for submission
 * to graphics hardware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef PRIMITIVE_TYPE_H
#define PRIMITIVE_TYPE_H

namespace Seoul
{

enum class PrimitiveType : UInt32
{
	kNone = 0,
	kPointList,
	kLineList,
	kLineStrip,
	kTriangleList,
};

/**
 * Given a primitive type and index count, this
 * function will return the number of primitives that will
 * be drawn by the index buffer.
 */
inline UInt32 GetNumberOfPrimitives(
	PrimitiveType ePrimitiveType,
	UInt32 indexCount)
{
	if (indexCount == 0u)
	{
		return 0u;
	}
	else
	{
		switch (ePrimitiveType)
		{
		// One index per point.
		case PrimitiveType::kPointList:
			return indexCount;
		// Two indices per point.
		case PrimitiveType::kLineList:
			SEOUL_ASSERT(indexCount % 2u == 0u);
			return (indexCount / 2u);
		// One fewer lines than the number of indices,
		// see: http://msdn.microsoft.com/en-us/library/bb174701%28VS.85%29.aspx
		case PrimitiveType::kLineStrip:
			SEOUL_ASSERT(indexCount >= 2u);
			return (indexCount - 1u);
		// Three indices per triangle, exactly. Typically
		// the most used format, also the most flexible for
		// drawing triangles.
		case PrimitiveType::kTriangleList:
			SEOUL_ASSERT(indexCount % 3u == 0u);
			return (indexCount / 3u);
		default:
			return 0u;
		}
	}
}

/**
 * Given a primitive type and primitive count, this
 * function will return the number of indices that need
 * to be defined in an index buffer.
 */
inline UInt32 GetNumberOfIndices(
	PrimitiveType ePrimitiveType,
	UInt32 nPrimitiveCount)
{
	if (nPrimitiveCount == 0u)
	{
		return 0u;
	}
	else
	{
		switch (ePrimitiveType)
		{
		// One index per point.
		case PrimitiveType::kPointList:
			return nPrimitiveCount;
		// Two indices per point.
		case PrimitiveType::kLineList:
			return (nPrimitiveCount * 2u);
		// One more index than the number of lines,
		// see: http://msdn.microsoft.com/en-us/library/bb174701%28VS.85%29.aspx
		case PrimitiveType::kLineStrip:
			return (nPrimitiveCount + 1u);
		// Three indices per triangle.
		case PrimitiveType::kTriangleList:
			return (nPrimitiveCount * 3u);
		default:
			return 0u;
		}
	}
}

/**
 * Given a primitive type and primitive count, this
 * function will return the number of vertices that need
 * to be defined in a continuous vertex buffer that will
 * be used for drawing without an index buffer.
 */
inline UInt32 GetNumberOfVertices(
	PrimitiveType ePrimitiveType,
	UInt32 nPrimitiveCount)
{
	if (nPrimitiveCount == 0u)
	{
		return 0u;
	}
	else
	{
		switch (ePrimitiveType)
		{
		// One vertex per point.
		case PrimitiveType::kPointList:
			return nPrimitiveCount;
		// Two vertices per point.
		case PrimitiveType::kLineList:
			return (nPrimitiveCount * 2u);
		// One more vertex than the number of lines,
		// see: http://msdn.microsoft.com/en-us/library/bb174701%28VS.85%29.aspx
		case PrimitiveType::kLineStrip:
			return (nPrimitiveCount + 1u);
		// Three vertices per triangle.
		case PrimitiveType::kTriangleList:
			return (nPrimitiveCount * 3u);
		default:
			return 0u;
		}
	}
}

} // namespace Seoul

#endif // include guard
