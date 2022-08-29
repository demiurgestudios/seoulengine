/**
 * \file NavigationGrid.h
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

#pragma once
#ifndef NAVIGATION_GRID_H
#define NAVIGATION_GRID_H

#include "Prereqs.h"

#if SEOUL_WITH_NAVIGATION

namespace Seoul::Navigation
{

class Grid SEOUL_SEALED
{
public:
	/**
	 * Max width or height of a navigation grid.
	 *
	 * Keep in-sync with NAVAPI_MAX_GRID_DIMENSION.
	 */
	static const UInt32 kuMaxDimension = 4096u;

	static Grid* Create(UInt32 uWidth, UInt32 uHeight);
	static Grid* CreateFromFileInMemory(UInt8 const* pData, UInt32 uDataSizeInBytes);
	static Grid* CreateFromGrid(UInt32 uWidth, UInt32 uHeight, const Grid& grid);
	static void Destroy(Grid*& rpGrid);

	UInt8 GetCell(UInt32 uX, UInt32 uY) const
	{
		SEOUL_ASSERT(uX < m_uWidth);
		SEOUL_ASSERT(uY < m_uHeight);

		return m_pGrid[uY * m_uWidth + uX];
	}

	UInt8 const* GetGrid() const
	{
		return m_pGrid;
	}

	UInt32 GetHeight() const
	{
		return m_uHeight;
	}

	UInt32 GetWidth() const
	{
		return m_uWidth;
	}

	UInt8* Save(UInt32& ruDataSizeInBytes) const;

	void SetCell(UInt32 uX, UInt32 uY, UInt8 uCellValue)
	{
		SEOUL_ASSERT(uX < m_uWidth);
		SEOUL_ASSERT(uY < m_uHeight);

		m_pGrid[uY * m_uWidth + uX] = uCellValue;
	}

private:
	Grid(UInt8* pGrid, UInt32 uWidth, UInt32 uHeight);
	~Grid();

	UInt8* const m_pGrid;
	UInt32 const m_uWidth;
	UInt32 const m_uHeight;

	SEOUL_DISABLE_COPY(Grid);
}; // class Grid

} // namespace Seoul::Navigation

#endif // /#if SEOUL_WITH_NAVIGATION

#endif // include guard
