/**
 * \file FalconPackerTree2D.h
 * \brief Part of the Falcon render backend, supports management
 * of 2D squares in an open 2D space.
 *
 * Used by Falcon::TextureCache for managing dynamic texture atlasses.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_PACKER_TREE_2D_H
#define FALCON_PACKER_TREE_2D_H

#include "FalconTypes.h"
#include "Geometry.h"
#include "HashTable.h"
#include "SeoulHString.h"
#include "Vector.h"

namespace Seoul::Falcon
{

class PackerTree2D SEOUL_SEALED
{
public:
	typedef UInt16 NodeID;

	struct Node SEOUL_SEALED
	{
		static Node CreateLeaf()
		{
			Node ret;
			memset(&ret, 0, sizeof(ret));
			return ret;
		}

		Rectangle2DInt GetChildA(const Rectangle2DInt& parentRectangle) const
		{
			Rectangle2DInt ret = parentRectangle;
			if (0 != m_bYAxis)
			{
				ret.m_iBottom = m_iSplit;
			}
			else
			{
				ret.m_iRight = m_iSplit;
			}
			return ret;
		}

		Rectangle2DInt GetChildB(const Rectangle2DInt& parentRectangle) const
		{
			Rectangle2DInt ret = parentRectangle;
			if (0 != m_bYAxis)
			{
				ret.m_iTop = m_iSplit;
			}
			else
			{
				ret.m_iLeft = m_iSplit;
			}
			return ret;
		}

		Bool HasObject() const
		{
			return IsLeaf() && (0 != m_bHasObject);
		}

		Bool IsLeaf() const
		{
			return (0 == m_uChildA);
		}

		Bool operator==(const Node& b) const
		{
			return (0 == memcmp(this, &b, sizeof(b)));
		}

		Bool operator!=(const Node& b) const
		{
			return !(*this == b);
		}

		Int32 m_iSplit : 31;
		Int32 m_bYAxis : 1;
		NodeID m_uChildA;
		union
		{
			NodeID m_uChildB;
			UInt16 m_bHasObject;
		};
	}; // struct Node
	SEOUL_STATIC_ASSERT(sizeof(Node) == 8);

	typedef Vector<Node, MemoryBudgets::Falcon> Nodes;

	PackerTree2D(Int32 iWidth, Int32 iHeight)
		: m_vNodes()
		, m_iWidth(iWidth)
		, m_iHeight(iHeight)
	{
	}

	Int32 GetHeight() const
	{
		return m_iHeight;
	}

	Int32 GetWidth() const
	{
		return m_iWidth;
	}

	void Clear()
	{
		m_vNodes.Clear();
		m_vFreeNodes.Clear();
	}

	void CollectGarbage()
	{
		if (m_vNodes.IsEmpty())
		{
			return;
		}

		DoCollectGarbage(0);
	}

	Bool Pack(Int32 iWidth, Int32 iHeight, NodeID& rNodeID, Int32& riX, Int32& riY)
	{
		CheckRootNode();

		Rectangle2DInt rootRectangle(0, 0, m_iWidth, m_iHeight);
		return Pack(0, rootRectangle, iWidth, iHeight, rNodeID, riX, riY);
	}

	Bool UnPack(NodeID nodeID)
	{
		if ((Nodes::SizeType)nodeID >= m_vNodes.GetSize())
		{
			return false;
		}

		Node node = m_vNodes[nodeID];
		if (!node.IsLeaf())
		{
			return false;
		}

		m_vNodes[nodeID].m_bHasObject = 0;
		return true;
	}

private:
	typedef Vector<UInt16, MemoryBudgets::Falcon> FreeNodes;
	FreeNodes m_vFreeNodes;
	Nodes m_vNodes;
	Int32 m_iWidth;
	Int32 m_iHeight;

	Bool AcquireNode(NodeID& rNodeID)
	{
		if (m_vFreeNodes.IsEmpty())
		{
			if (m_vNodes.GetSize() >= (NodeID)-1)
			{
				return false;
			}

			rNodeID = (NodeID)m_vNodes.GetSize();
			m_vNodes.PushBack(Node::CreateLeaf());
			return true;
		}
		else
		{
			rNodeID = m_vFreeNodes.Back();
			m_vFreeNodes.PopBack();
			SEOUL_ASSERT(m_vNodes[rNodeID] == Node::CreateLeaf());
			return true;
		}
	}

	void CheckRootNode()
	{
		if (m_vNodes.IsEmpty())
		{
			m_vNodes.PushBack(Node::CreateLeaf());
		}
	}

	void DoCollectGarbage(NodeID nodeID)
	{
		Node node = m_vNodes[nodeID];
		if (node.IsLeaf())
		{
			return;
		}

		DoCollectGarbage(node.m_uChildA);
		if (0 != node.m_uChildB)
		{
			DoCollectGarbage(node.m_uChildB);
		}

		Node childA = m_vNodes[node.m_uChildA];
		if (childA.IsLeaf() && !childA.HasObject())
		{
			if (0 != node.m_uChildB)
			{
				Node childB = m_vNodes[node.m_uChildB];
				if (!childB.IsLeaf() || childB.HasObject())
				{
					return;
				}

				m_vNodes[node.m_uChildB] = Node::CreateLeaf();
				m_vFreeNodes.PushBack(node.m_uChildB);
			}
			m_vNodes[node.m_uChildA] = Node::CreateLeaf();
			m_vFreeNodes.PushBack(node.m_uChildA);
			m_vNodes[nodeID] = Node::CreateLeaf();
		}
	}

	Bool Pack(
		NodeID uNode,
		const Rectangle2DInt& nodeRectangle,
		Int32 iWidth,
		Int32 iHeight,
		NodeID& rNodeID,
		Int32& riX,
		Int32& riY)
	{
		Node node = m_vNodes[uNode];
		if (node.IsLeaf())
		{
			if (node.HasObject())
			{
				return false;
			}

			Int32 const iNodeWidth = nodeRectangle.GetWidth();
			if (iNodeWidth < iWidth)
			{
				return false;
			}

			Int32 const iNodeHeight = nodeRectangle.GetHeight();
			if (iNodeHeight < iHeight)
			{
				return false;
			}

			if (iNodeWidth == iWidth && iNodeHeight == iHeight)
			{
				node.m_bHasObject = 1;
				rNodeID = uNode;
				riX = nodeRectangle.m_iLeft;
				riY = nodeRectangle.m_iTop;
				m_vNodes[uNode] = node;
				return true;
			}

			if (!AcquireNode(node.m_uChildA))
			{
				return false;
			}

			Int32 const iWidthDifference = (iNodeWidth - iWidth);
			Int32 const iHeightDifference = (iNodeHeight - iHeight);

			if (iWidthDifference > iHeightDifference)
			{
				node.m_bYAxis = 0;
				node.m_iSplit = (nodeRectangle.m_iLeft + iWidth);
			}
			else
			{
				node.m_bYAxis = 1;
				node.m_iSplit = (nodeRectangle.m_iTop + iHeight);
			}

			m_vNodes[uNode] = node;
			return Pack(
				node.m_uChildA,
				node.GetChildA(nodeRectangle),
				iWidth,
				iHeight,
				rNodeID,
				riX,
				riY);
		}
		else
		{
			if (Pack(
				node.m_uChildA,
				node.GetChildA(nodeRectangle),
				iWidth,
				iHeight,
				rNodeID,
				riX,
				riY))
			{
				return true;
			}

			if (0 == node.m_uChildB)
			{
				if (!AcquireNode(node.m_uChildB))
				{
					return false;
				}

				m_vNodes[uNode] = node;
			}

			if (Pack(
				node.m_uChildB,
				node.GetChildB(nodeRectangle),
				iWidth,
				iHeight,
				rNodeID,
				riX,
				riY))
			{
				return true;
			}

			return false;
		}
	}

	SEOUL_DISABLE_COPY(PackerTree2D);
}; // class PackerTree2D

} // namespace Seoul::Falcon

#endif // include guard
