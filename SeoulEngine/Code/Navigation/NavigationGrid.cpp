/**
 * \file NavigationGrid.cpp
 * \brief Core data type of the navigation project. Defines
 * a grid on which 2D navigation queries can be issued
 * (currently implemented with jump point search). All const
 * API of Grid are thread safe, to support threaded
 * navigation queries.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Compress.h"
#include "NavigationGrid.h"
#include "SeoulMath.h"

#if SEOUL_WITH_NAVIGATION

namespace Seoul::Navigation
{

static const UInt32 kuSignature = 0xF6906B9d;

static inline Bool ReadUInt32(UInt8 const*& rpData, UInt32& ruDataSizeInBytes, UInt32& ru)
{
	if (ruDataSizeInBytes < sizeof(ru))
	{
		return false;
	}

	memcpy(&ru, rpData, sizeof(ru));
	rpData += sizeof(ru);
	ruDataSizeInBytes -= sizeof(ru);

#if !SEOUL_LITTLE_ENDIAN
	ru = EndianSwap32(ru);
#endif // /#if !SEOUL_LITTLE_ENDIAN

	return true;
}

static inline void WriteUInt32(UInt8*& rpData, UInt32 u)
{
#if !SEOUL_LITTLE_ENDIAN
	u = EndianSwap32(u);
#endif // /#if !SEOUL_LITTLE_ENDIAN

	memcpy(rpData, &u, sizeof(u));
	rpData += sizeof(u);
}

struct ScopedDeallocate
{
	ScopedDeallocate(void* pData)
		: m_pData(pData)
	{
	}

	~ScopedDeallocate()
	{
		MemoryManager::Deallocate(m_pData);
		m_pData = nullptr;
	}

	void* m_pData;
};

Grid* Grid::Create(UInt32 uWidth, UInt32 uHeight)
{
	// Size check.
	if (uWidth > kuMaxDimension || uHeight > kuMaxDimension)
	{
		return nullptr;
	}

	// Special handling for an empty grid.
	if (0u == uWidth || 0u == uHeight)
	{
		Grid* pReturn = SEOUL_NEW(MemoryBudgets::Navigation) Grid(nullptr, uWidth, uHeight);
		return pReturn;
	}

	UInt32 const uGridSizeInBytes = (uWidth * uHeight * sizeof(UInt8));
	UInt8* pGrid = (UInt8*)MemoryManager::Allocate((size_t)uGridSizeInBytes, MemoryBudgets::Navigation);
	memset(pGrid, 0, (size_t)uGridSizeInBytes);

	Grid* pReturn = SEOUL_NEW(MemoryBudgets::Navigation) Grid(pGrid, uWidth, uHeight);
	return pReturn;
}

Grid* Grid::CreateFromFileInMemory(UInt8 const* pCompressedData, UInt32 uCompressedDataSizeInBytes)
{
	void* pRawData = nullptr;
	UInt32 uDataSizeInBytes = 0u;
	if (!LZ4Decompress(
		pCompressedData,
		uCompressedDataSizeInBytes,
		pRawData,
		uDataSizeInBytes,
		MemoryBudgets::Navigation))
	{
		return nullptr;
	}
	UInt8 const* pData = (UInt8 const*)pRawData;
	ScopedDeallocate scoped(pRawData);

	UInt32 uSignature = 0u;
	if (!ReadUInt32(pData, uDataSizeInBytes, uSignature)) { return nullptr; }
	if (uSignature != kuSignature) { return nullptr; }

	UInt32 uWidth = 0u;
	UInt32 uHeight = 0u;
	if (!ReadUInt32(pData, uDataSizeInBytes, uWidth)) { return nullptr; }
	if (!ReadUInt32(pData, uDataSizeInBytes, uHeight)) { return nullptr; }

	// Size check.
	if (uWidth > kuMaxDimension || uHeight > kuMaxDimension)
	{
		return nullptr;
	}

	// Special handling for an empty grid.
	if (0u == uWidth || 0u == uHeight)
	{
		Grid* pReturn = SEOUL_NEW(MemoryBudgets::Navigation) Grid(nullptr, uWidth, uHeight);
		return pReturn;
	}

	UInt32 const uGridSizeInBytes = (uWidth * uHeight * sizeof(UInt8));
	if (uDataSizeInBytes < uGridSizeInBytes)
	{
		return nullptr;
	}

	UInt8* pGrid = (UInt8*)MemoryManager::Allocate((size_t)uGridSizeInBytes, MemoryBudgets::Navigation);
	memcpy(pGrid, pData, (size_t)uGridSizeInBytes);

	Grid* pReturn = SEOUL_NEW(MemoryBudgets::Navigation) Grid(pGrid, uWidth, uHeight);
	return pReturn;
}

Grid* Grid::CreateFromGrid(UInt32 uWidth, UInt32 uHeight, const Grid& grid)
{
	// Size check.
	if (uWidth > kuMaxDimension || uHeight > kuMaxDimension)
	{
		return nullptr;
	}

	// Special handling for an empty grid.
	if (0u == uWidth || 0u == uHeight)
	{
		Grid* pReturn = SEOUL_NEW(MemoryBudgets::Navigation) Grid(nullptr, uWidth, uHeight);
		return pReturn;
	}

	UInt32 const uGridSizeInBytes = (uWidth * uHeight * sizeof(UInt8));
	UInt8* pGrid = (UInt8*)MemoryManager::Allocate((size_t)uGridSizeInBytes, MemoryBudgets::Navigation);
	memset(pGrid, 0, (size_t)uGridSizeInBytes);

	// Copy through existing grid.
	UInt32 const uMinWidth = Min(uWidth, grid.GetWidth());
	UInt32 const uMinHeight = Min(uHeight, grid.GetHeight());
	for (UInt32 uY = 0u; uY < uMinHeight; ++uY)
	{
		for (UInt32 uX = 0u; uX < uMinWidth; ++uX)
		{
			pGrid[uY * uWidth + uX] = grid.GetCell(uX, uY);
		}
	}

	Grid* pReturn = SEOUL_NEW(MemoryBudgets::Navigation) Grid(pGrid, uWidth, uHeight);
	return pReturn;
}

void Grid::Destroy(Grid*& rpGrid)
{
	Grid* p = rpGrid;
	rpGrid = nullptr;
	SEOUL_DELETE p;
}

Grid::Grid(UInt8* pGrid, UInt32 uWidth, UInt32 uHeight)
	: m_pGrid(pGrid)
	, m_uWidth(uWidth)
	, m_uHeight(uHeight)
{
	// Programmer sanity check - make sure Create() functions enforce this.
	SEOUL_ASSERT(uWidth <= kuMaxDimension && uHeight <= kuMaxDimension);
}

Grid::~Grid()
{
	MemoryManager::Deallocate(m_pGrid);
}

UInt8* Grid::Save(UInt32& ruDataSizeInBytes) const
{
	UInt32 const uGridSizeInBytes = (m_uWidth * m_uHeight * sizeof(UInt8));
	UInt32 const uUncompressedDataSizeInBytes =
		sizeof(UInt32) * 3u + // signature, width, height
		uGridSizeInBytes; // size of grid data
	UInt8* pUncompressedData = (UInt8*)MemoryManager::Allocate(
		uUncompressedDataSizeInBytes,
		MemoryBudgets::Navigation);
	ScopedDeallocate scopedUncompressed(pUncompressedData);

	// Write data.
	{
		UInt8* pWrite = pUncompressedData;
		WriteUInt32(pWrite, kuSignature);
		WriteUInt32(pWrite, m_uWidth);
		WriteUInt32(pWrite, m_uHeight);
		memcpy(pWrite, m_pGrid, uGridSizeInBytes);
	}

	// Compress the data.
	void* pCompressedData = nullptr;
	UInt32 uCompressedDataSizeInBytes = 0u;
	if (!LZ4Compress(
		pUncompressedData,
		uUncompressedDataSizeInBytes,
		pCompressedData,
		uCompressedDataSizeInBytes,
		LZ4CompressionLevel::kNormal, // Use normal compression - compromise between compression speed (matters in editors) and size
		MemoryBudgets::Compression))
	{
		return nullptr;
	}

	// Success.
	ruDataSizeInBytes = uCompressedDataSizeInBytes;
	return (UInt8*)pCompressedData;
}

} // namespace Seoul::Navigation

#else // #if !SEOUL_WITH_NAVIGATION

// Avoid warnings with the iOS linker.
namespace Seoul { namespace Navigation { Int32 g_iUnusedGlobal; } }

#endif // /#if !SEOUL_WITH_NAVIGATION
