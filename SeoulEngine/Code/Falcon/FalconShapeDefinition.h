/**
 * \file FalconShapeDefinition.h
 * \brief The immutable shared data of a Falcon::ShapeInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_SHAPE_DEFINITION_H
#define FALCON_SHAPE_DEFINITION_H

#include "FalconDefinition.h"
#include "FalconRenderFeature.h"
#include "FalconTriangleListDescription.h"
#include "FalconTypes.h"
#include "Vector.h"

namespace Seoul::Falcon
{

// forward declarations
class BitmapDefinition;
class ShapeDefinition;
class SwfReader;
class Tesselator;

struct ShapePath
{
	ShapePath()
		: m_iFillStyle0(-1)
		, m_iFillStyle1(-1)
		, m_iLineStyle(-1)
		, m_fStartX(0.0f)
		, m_fStartY(0.0f)
		, m_vEdges()
	{
	}

	typedef Vector<ShapeEdge, MemoryBudgets::Falcon> Edges;

	Int m_iFillStyle0;
	Int m_iFillStyle1;
	Int m_iLineStyle;
	Float m_fStartX;
	Float m_fStartY;
	Edges m_vEdges;

	void Swap(ShapePath& rv)
	{
		Seoul::Swap(m_iFillStyle0, rv.m_iFillStyle0);
		Seoul::Swap(m_iFillStyle1, rv.m_iFillStyle1);
		Seoul::Swap(m_iLineStyle, rv.m_iLineStyle);
		Seoul::Swap(m_fStartX, rv.m_fStartX);
		Seoul::Swap(m_fStartY, rv.m_fStartY);
		m_vEdges.Swap(rv.m_vEdges);
	}

	void Tesselate(const ShapeDefinition& definition, Tesselator& rTesselator);
};

struct ShapeFillDrawable
{
	typedef Vector<UInt16, MemoryBudgets::Falcon> Indices;
	typedef Vector<ShapeVertex, MemoryBudgets::Falcon> Vertices;

	ShapeFillDrawable()
		: m_mOcclusionTransform(Matrix2x3::Identity())
		, m_vIndices()
		, m_vVertices()
		, m_pBitmapDefinition()
		, m_Bounds(Rectangle::Create(0, 0, 0, 0))
		, m_eTriangleListDescription(TriangleListDescription::kNotSpecific)
		, m_eFeature(Render::Feature::kNone)
		, m_bMatchesBounds(false)
		, m_bCanOcclude(false)
	{
	}

	Matrix2x3 m_mOcclusionTransform;
	Indices m_vIndices;
	Vertices m_vVertices;
	SharedPtr<BitmapDefinition> m_pBitmapDefinition;
	Rectangle m_Bounds;
	TriangleListDescription m_eTriangleListDescription;
	Render::Feature::Enum m_eFeature;
	Bool m_bMatchesBounds;
	Bool m_bCanOcclude;
}; // struct ShapeFillDrawable

class ShapeDefinition SEOUL_SEALED : public Definition
{
public:
	ShapeDefinition(const Rectangle& rectangle, UInt16 uDefinitionID)
		: Definition(DefinitionType::kShape, uDefinitionID)
		, m_vFillStyles()
		, m_vLineStyles()
		, m_vPaths()
		, m_Rectangle(rectangle)
	{
	}

	Bool Read(FCNFile& rFile, SwfReader& rBuffer, Int32 iDefineShapeVersion);

	typedef Vector<ShapeFillDrawable, MemoryBudgets::Falcon> FillDrawables;
	typedef Vector<FillStyle, MemoryBudgets::Falcon> FillStyles;
	typedef Vector<LineStyle, MemoryBudgets::Falcon> LineStyles;
	typedef Vector<ShapePath, MemoryBudgets::Falcon> Paths;

	const FillDrawables& GetFillDrawables() const
	{
		return m_vFillDrawables;
	}

	const FillStyles& GetFillStyles() const
	{
		return m_vFillStyles;
	}

	const LineStyles& GetLineStyles() const
	{
		return m_vLineStyles;
	}

	const Rectangle& GetRectangle() const
	{
		return m_Rectangle;
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(ShapeDefinition);

	virtual void DoCreateInstance(SharedPtr<Instance>& rp) const SEOUL_OVERRIDE;

	void AbsorbPath(ShapePath& r, Int32 iCurrentX, Int32 iCurrentY, Bool bClearStyles = false);
	Bool ReadFillStyle(FCNFile& rFile, SwfReader& rBuffer, Int32 iDefineShapeVersion, FillStyle& rFillStyle);
	Bool ReadFillStyles(FCNFile& rFile, SwfReader& rBuffer, Int32 iDefineShapeVersion, FillStyles& rvFillStyles);
	Bool ReadFocalGradient(SwfReader& rBuffer, Int32 iDefineShapeVersion, Gradient& rGradient);
	Bool ReadGradient(SwfReader& rBuffer, Int32 iDefineShapeVersion, Gradient& rGradient);
	Bool ReadGradientRecord(SwfReader& rBuffer, Int32 iDefineShapeVersion, GradientRecord& rGradientRecord);
	Bool ReadLineStyle(FCNFile& rFile, SwfReader& rBuffer, Int32 iDefineShapeVersion, LineStyle& rLineStyle);
	Bool ReadLineStyles(FCNFile& rFile, SwfReader& rBuffer, Int32 iDefineShapeVersion, LineStyles& rvLineStyles);
	Bool Tesselate(FCNFile& rFile);

	FillDrawables m_vFillDrawables;
	FillStyles m_vFillStyles;
	LineStyles m_vLineStyles;
	Paths m_vPaths;
	Rectangle m_Rectangle;

	SEOUL_DISABLE_COPY(ShapeDefinition);
}; // class ShapeDefinition
template <> struct DefinitionTypeOf<ShapeDefinition> { static const DefinitionType Value = DefinitionType::kShape; };

} // namespace Seoul::Falcon

#endif // include guard
