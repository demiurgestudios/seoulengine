/**
 * \file FalconShapeDefinition.cpp
 * \brief The immutable shared data of a Falcon::ShapeInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Algorithms.h"
#include "Color.h"
#include "FalconBitmapDefinition.h"
#include "FalconFCNFile.h"
#include "FalconRenderPoser.h"
#include "FalconShapeDefinition.h"
#include "FalconShapeInstance.h"
#include "FalconSwfReader.h"
#include "FalconTesselator.h"
#include "Logger.h"

namespace Seoul::Falcon
{

void ShapePath::Tesselate(const ShapeDefinition& definition, Tesselator& rTesselator)
{
	rTesselator.BeginPath(
		(m_iFillStyle0 >= 0 ? &definition.GetFillStyles()[m_iFillStyle0] : nullptr),
		(m_iFillStyle1 >= 0 ? &definition.GetFillStyles()[m_iFillStyle1] : nullptr),
		(m_iLineStyle >= 0 ? &definition.GetLineStyles()[m_iLineStyle] : nullptr),
		Vector2D(m_fStartX, m_fStartY));

	Edges::SizeType zSize = m_vEdges.GetSize();
	for (Edges::SizeType i = 0; i < zSize; ++i)
	{
		const ShapeEdge& edge = m_vEdges[i];
		rTesselator.AddEdge(edge);
	}
	rTesselator.EndPath();
}

Bool ShapeDefinition::Read(FCNFile& rFile, SwfReader& rBuffer, Int32 iDefineShapeVersion)
{
	FillStyles vFillStyles;
	LineStyles vLineStyles;

	if (!ReadFillStyles(rFile, rBuffer, iDefineShapeVersion, vFillStyles))
	{
		return false;
	}

	if (!ReadLineStyles(rFile, rBuffer, iDefineShapeVersion, vLineStyles))
	{
		return false;
	}

	UInt32 uFillBits = rBuffer.ReadUBits(4);
	UInt32 uLineBits = rBuffer.ReadUBits(4);
	Int32 iX = 0;
	Int32 iY = 0;
	ShapePath currentPath;
	while (true)
	{
		Bool bEdgeRecord = rBuffer.ReadBit();
		if (!bEdgeRecord)
		{
			ShapeRecordFlags eFlags = (ShapeRecordFlags)rBuffer.ReadUBits(5);

			// If all flags are 0, this is an end record and we're done processing.
			if (ShapeRecordFlags::kEndShape == eFlags)
			{
				AbsorbPath(currentPath, iX, iY);
				break;
			}
			// Otherwise, this is a StyleChangeRecord.
			else
			{
				// MoveTo defined
				if (ShapeRecordFlags::kStateMoveTo == (ShapeRecordFlags::kStateMoveTo & eFlags))
				{
					AbsorbPath(currentPath, iX, iY);

					Int32 nMoveBits = (Int32)rBuffer.ReadUBits(5);
					iX = rBuffer.ReadSBits(nMoveBits);
					iY = rBuffer.ReadSBits(nMoveBits);

					currentPath.m_fStartX = TwipsToPixels(iX);
					currentPath.m_fStartY = TwipsToPixels(iY);
				}

				if (ShapeRecordFlags::kStateFillStyle0 == (ShapeRecordFlags::kStateFillStyle0 & eFlags))
				{
					AbsorbPath(currentPath, iX, iY);

					int const iFillStyle = (int)rBuffer.ReadUBits(uFillBits);
					currentPath.m_iFillStyle0 = (iFillStyle > 0
						? (iFillStyle + (int)m_vFillStyles.GetSize() - 1)
						: -1);
				}

				if (ShapeRecordFlags::kStateFillStyle1 == (ShapeRecordFlags::kStateFillStyle1 & eFlags))
				{
					AbsorbPath(currentPath, iX, iY);

					int const iFillStyle = (int)rBuffer.ReadUBits(uFillBits);
					currentPath.m_iFillStyle1 = (iFillStyle > 0
						? (iFillStyle + (int)m_vFillStyles.GetSize() - 1)
						: -1);
				}

				if (ShapeRecordFlags::kStateLineStyle == (ShapeRecordFlags::kStateLineStyle & eFlags))
				{
					AbsorbPath(currentPath, iX, iY);

					int const iLineStyle = (int)rBuffer.ReadUBits(uLineBits);
					currentPath.m_iLineStyle = (iLineStyle > 0
						? (iLineStyle + (int)m_vLineStyles.GetSize() - 1)
						: -1);
				}

				if (ShapeRecordFlags::kStateNewStyles == (ShapeRecordFlags::kStateNewStyles & eFlags))
				{
					AbsorbPath(currentPath, iX, iY, true);

					if (!vFillStyles.IsEmpty())
					{
						m_vFillStyles.Append(vFillStyles);
					}
					if (!ReadFillStyles(rFile, rBuffer, iDefineShapeVersion, vFillStyles))
					{
						return false;
					}

					if (!vLineStyles.IsEmpty())
					{
						m_vLineStyles.Append(vLineStyles);
					}
					if (!ReadLineStyles(rFile, rBuffer, iDefineShapeVersion, vLineStyles))
					{
						return false;
					}

					uFillBits = rBuffer.ReadUBits(4);
					uLineBits = rBuffer.ReadUBits(4);
				}
			}
		}
		else
		{
			Bool bStraightEdge = rBuffer.ReadBit();
			UInt32 nNumBits = rBuffer.ReadUBits(4);

			ShapeEdge edge;
			Int32 iX1 = 0;
			Int32 iY1 = 0;

			// Straight edge
			if (bStraightEdge)
			{
				Bool bGeneralLine = rBuffer.ReadBit();
				Bool bVerticalLine = !bGeneralLine && rBuffer.ReadBit();

				int const iDX = ((bGeneralLine || !bVerticalLine) ? rBuffer.ReadSBits(nNumBits + 2) : 0);
				int const iDY = ((bGeneralLine || bVerticalLine) ? rBuffer.ReadSBits(nNumBits + 2) : 0);

				iX1 = (iDX + iX);
				iY1 = (iDY + iY);

				edge.m_fAnchorX = TwipsToPixels(iX1);
				edge.m_fAnchorY = TwipsToPixels(iY1);
				edge.m_fControlX = TwipsToPixels(iX1);
				edge.m_fControlY = TwipsToPixels(iY1);
			}
			// Curved edge
			else
			{
				int iControlX = iX + rBuffer.ReadSBits(nNumBits + 2);
				int iControlY = iY + rBuffer.ReadSBits(nNumBits + 2);
				int iAnchorX = iControlX + rBuffer.ReadSBits(nNumBits + 2);
				int iAnchorY = iControlY + rBuffer.ReadSBits(nNumBits + 2);

				edge.m_fAnchorX = TwipsToPixels(iAnchorX);
				edge.m_fAnchorY = TwipsToPixels(iAnchorY);
				edge.m_fControlX = TwipsToPixels(iControlX);
				edge.m_fControlY = TwipsToPixels(iControlY);

				iX1 = iAnchorX;
				iY1 = iAnchorY;
			}

			currentPath.m_vEdges.PushBack(edge);

			iX = iX1;
			iY = iY1;
		}
	}

	if (!vFillStyles.IsEmpty())
	{
		m_vFillStyles.Append(vFillStyles);
	}

	if (!vLineStyles.IsEmpty())
	{
		m_vLineStyles.Append(vLineStyles);
	}

	// Back to byte boundaries
	rBuffer.Align();

	// Tesselate.
	return Tesselate(rFile);
}

void ShapeDefinition::DoCreateInstance(SharedPtr<Instance>& rp) const
{
	rp.Reset(SEOUL_NEW(MemoryBudgets::Falcon) ShapeInstance(SharedPtr<ShapeDefinition const>(this)));
}

void ShapeDefinition::AbsorbPath(ShapePath& r, Int32 iCurrentX, Int32 iCurrentY, Bool bClearStyles /*= false*/)
{
	if (!r.m_vEdges.IsEmpty())
	{
		m_vPaths.PushBack(r);
		r.m_vEdges.Clear();

		if (bClearStyles)
		{
			// clear styles on new styles record.
			r.m_iFillStyle0 = -1;
			r.m_iFillStyle1 = -1;
			r.m_iLineStyle = -1;
		}
		else
		{
			r.m_fStartX = TwipsToPixels(iCurrentX);
			r.m_fStartY = TwipsToPixels(iCurrentY);
		}
	}
}

Bool ShapeDefinition::ReadFillStyle(FCNFile& rFile, SwfReader& rBuffer, Int32 iDefineShapeVersion, FillStyle& rFillStyle)
{
	rFillStyle.m_eFillStyleType = (FillStyleType)rBuffer.ReadUInt8();
	if (FillStyleType::kSoldFill == rFillStyle.m_eFillStyleType)
	{
		if (iDefineShapeVersion >= 3)
		{
			rFillStyle.m_Color = rBuffer.ReadRGBA();
		}
		else
		{
			rFillStyle.m_Color = rBuffer.ReadRGB();
		}
		return true;
	}
	else if (
		FillStyleType::kLinearGradientFill == rFillStyle.m_eFillStyleType ||
		FillStyleType::kFocalRadialGradientFill == rFillStyle.m_eFillStyleType ||
		FillStyleType::kRadialGradientFill == rFillStyle.m_eFillStyleType)
	{
		rFillStyle.m_mGradientTransform = rBuffer.ReadMatrix();

		// Gradient transform, need to undo twips across the board.
		rFillStyle.m_mGradientTransform =
			Matrix2x3::CreateScale(kfTwipsToPixelsFactor, kfTwipsToPixelsFactor) *
			rFillStyle.m_mGradientTransform;

		rBuffer.Align();

		if (FillStyleType::kFocalRadialGradientFill == rFillStyle.m_eFillStyleType)
		{
			if (!ReadFocalGradient(rBuffer, iDefineShapeVersion, rFillStyle.m_Gradient))
			{
				return false;
			}
		}
		else
		{
			if (!ReadGradient(rBuffer, iDefineShapeVersion, rFillStyle.m_Gradient))
			{
				return false;
			}
		}
		return true;
	}
	else if (
		FillStyleType::kClippedBitmapFill == rFillStyle.m_eFillStyleType ||
		FillStyleType::kNonSmoothedClippedBitmapFill == rFillStyle.m_eFillStyleType ||
		FillStyleType::kNonSmoothedRepeatingBitmapFill == rFillStyle.m_eFillStyleType ||
		FillStyleType::kRepeatingBitmapFill == rFillStyle.m_eFillStyleType)
	{
		rFillStyle.m_uBitmapId = rBuffer.ReadUInt16();
		rFillStyle.m_mBitmapTransform = rBuffer.ReadMatrix();

		// Bitmap transform, need to undo twips across the board.
		rFillStyle.m_mBitmapTransform =
			Matrix2x3::CreateScale(kfTwipsToPixelsFactor, kfTwipsToPixelsFactor) *
			rFillStyle.m_mBitmapTransform;

		return true;
	}
	else
	{
		SEOUL_WARN("'%s' is unsupported or corrupted, invalid fill style "
			"type '%d'.",
			rFile.GetURL().CStr(),
			(int)rFillStyle.m_eFillStyleType);
		return false;
	}
}

Bool ShapeDefinition::ReadFillStyles(FCNFile& rFile, SwfReader& rBuffer, Int32 iDefineShapeVersion, FillStyles& rvFillStyles)
{
	UInt32 u = (UInt32)rBuffer.ReadUInt8();
	UInt32 nCount = 0;
	if (0xFF == u)
	{
		nCount = (UInt32)rBuffer.ReadUInt16();
	}
	else
	{
		nCount = (UInt32)u;
	}

	rvFillStyles.Clear();
	rvFillStyles.Resize(nCount);

	for (UInt32 i = 0; i < nCount; ++i)
	{
		if (!ReadFillStyle(rFile, rBuffer, iDefineShapeVersion, rvFillStyles[i]))
		{
			return false;
		}
	}

	return true;
}

Bool ShapeDefinition::ReadFocalGradient(SwfReader& rBuffer, Int32 iDefineShapeVersion, Gradient& rGradient)
{
	if (!ReadGradient(rBuffer, iDefineShapeVersion, rGradient))
	{
		return false;
	}

	rGradient.m_FocalPoint = rBuffer.ReadFixed88();
	rGradient.m_bFocalGradient = true;
	return true;
}

Bool ShapeDefinition::ReadGradient(SwfReader& rBuffer, Int32 iDefineShapeVersion, Gradient& rGradient)
{
	typedef Vector<GradientRecord, MemoryBudgets::Falcon>::SizeType SizeType;

	rGradient.m_eSpreadMode = (GradientSpreadMode)rBuffer.ReadUBits(2);
	rGradient.m_eInterpolationMode = (GradientInterpolationMode)rBuffer.ReadUBits(2);

	SizeType const zSize = (SizeType)rBuffer.ReadUBits(4);
	rGradient.m_vGradientRecords.Clear();
	rGradient.m_vGradientRecords.Resize(zSize);
	for (SizeType i = 0; i < rGradient.m_vGradientRecords.GetSize(); ++i)
	{
		if (!ReadGradientRecord(rBuffer, iDefineShapeVersion, rGradient.m_vGradientRecords[i]))
		{
			return false;
		}
	}
	rGradient.m_bFocalGradient = false;

	return true;
}

Bool ShapeDefinition::ReadGradientRecord(SwfReader& rBuffer, Int32 iDefineShapeVersion, GradientRecord& rGradientRecord)
{
	rGradientRecord.m_uRatio = (UInt8)rBuffer.ReadUInt8();
	if (iDefineShapeVersion >= 3)
	{
		rGradientRecord.m_Color = rBuffer.ReadRGBA();
	}
	else
	{
		rGradientRecord.m_Color = rBuffer.ReadRGB();
	}
	return true;
}

Bool ShapeDefinition::ReadLineStyle(FCNFile& rFile, SwfReader& rBuffer, Int32 iDefineShapeVersion, LineStyle& rLineStyle)
{
	rLineStyle.m_uWidth = rBuffer.ReadUInt16();

	if (iDefineShapeVersion >= 4)
	{
		(void)rBuffer.ReadUBits(2); // uStartCapStyle
		UInt32 const uJoinStyle = rBuffer.ReadUBits(2); // uJoinStyle
		Bool const bHasFillFlag = rBuffer.ReadBit(); // bHasFillFlag
		(void)rBuffer.ReadBit(); // bNoHScaleFlag
		(void)rBuffer.ReadBit(); // bNoVScaleFlag

		{
			UInt32 const uReservedBits = rBuffer.ReadUBits(5);
			if (0 != uReservedBits) // Reserved, must be 0
			{
				SEOUL_WARN("'%s' is unsupported or corrupted, invalid line style "
					"reserved bits expected to be 0 have value '%u'.",
					rFile.GetURL().CStr(),
					uReservedBits);
				return false;
			}
		}

		(void)rBuffer.ReadBit(); // bNoClose
		(void)rBuffer.ReadUBits(2); // iEndCapStyle

		if (2 == uJoinStyle)
		{
			(void)rBuffer.ReadUInt16(); // uMiterLimitFactor
		}

		if (!bHasFillFlag)
		{
			rLineStyle.m_Color = rBuffer.ReadRGBA();
		}
		else
		{
			FillStyle fillStyle;
			if (!ReadFillStyle(rFile, rBuffer, iDefineShapeVersion, fillStyle))
			{
				return false;
			}
			(void)fillStyle;
		}
	}
	else
	{
		if (iDefineShapeVersion >= 3)
		{
			rLineStyle.m_Color = rBuffer.ReadRGBA();
		}
		else
		{
			rLineStyle.m_Color = rBuffer.ReadRGB();
		}
	}

	return true;
}

Bool ShapeDefinition::ReadLineStyles(FCNFile& rFile, SwfReader& rBuffer, Int32 iDefineShapeVersion, LineStyles& rvLineStyles)
{
	UInt32 u = (UInt32)rBuffer.ReadUInt8();
	UInt32 nCount = 0;
	if (0xFF == u)
	{
		nCount = (UInt32)rBuffer.ReadUInt16();
	}
	else
	{
		nCount = (UInt32)u;
	}

	rvLineStyles.Clear();
	rvLineStyles.Resize(nCount);

	for (UInt32 i = 0; i < nCount; ++i)
	{
		if (!ReadLineStyle(rFile, rBuffer, iDefineShapeVersion, rvLineStyles[i]))
		{
			return false;
		}
	}

	return true;
}

// Sort functor, sorts a sequence of points counter clockwise
// around a center of mass.
struct ConvexSorter SEOUL_SEALED
{
	ConvexSorter(const Vector2D& vCenterOfMass)
		: m_vCenterOfMass(vCenterOfMass)
	{
	}

	Bool operator()(const ShapeVertex& a, const ShapeVertex& b) const
	{
		Float fAngleA = (a.m_vP - m_vCenterOfMass).GetAngle();
		Float fAngleB = (b.m_vP - m_vCenterOfMass).GetAngle();

		return fAngleA > fAngleB;
	}

	Vector2D m_vCenterOfMass;
}; // struct ConvexSorter

class ShapeTesselateUtility SEOUL_SEALED : public TesselationCallback
{
public:
	ShapeTesselateUtility(FCNFile& rFile, ShapeDefinition::FillDrawables& rvFillDrawables)
		: m_rFile(rFile)
		, m_rvFillDrawables(rvFillDrawables)
	{
	}

	virtual void BeginShape() SEOUL_OVERRIDE
	{
	}

	virtual void AcceptLineStrip(const LineStyle& lineStyle, const LineStrip& vLineStrip) SEOUL_OVERRIDE
	{
		// TODO: Implement line rendering and accepting of line strips.
	}

	virtual void AcceptTriangleList(const FillStyle& fillStyle, const Vertices& vVertices, const Indices& vIndices, Bool bConvex) SEOUL_OVERRIDE
	{
		ShapeFillDrawable drawable;

		// Before continuing, perform some actions that can fail against
		// the drawable and return without adding the drawable
		// if those actions fail.
		if (IsBitmap(fillStyle.m_eFillStyleType))
		{
			// Make sure that the bitmap can be resolved.
			if (!m_rFile.GetDefinition(fillStyle.m_uBitmapId, drawable.m_pBitmapDefinition))
			{
				SEOUL_WARN("'%s' contains an invalid fill style with invalid bitmap ID '%u', "
					"this likely indicates a Falcon bug.",
					m_rFile.GetURL().CStr(),
					(UInt32)fillStyle.m_uBitmapId);
				return;
			}

			// If the bitmap is not visible (all transparent pixels), don't insert
			// the fill drawable. This is not an error, just an optimization, so we
			// don't warn about it.
			if (!drawable.m_pBitmapDefinition->IsVisible())
			{
				return;
			}
		}

		// start with inverse max
		drawable.m_Bounds = Rectangle::Create(FloatMax, -FloatMax, FloatMax, -FloatMax);
		drawable.m_vIndices = vIndices;

		// Setup drawables in 3 passes:
		// - first pass, absorb all points to establish the bounds.
		drawable.m_vVertices.Resize(vVertices.GetSize());
		Vertices::SizeType const zSize = vVertices.GetSize();
		for (Vertices::SizeType i = 0; i < zSize; ++i)
		{
			const Vector2D& v = vVertices[i];
			Float32 const fX = v.X;
			Float32 const fY = v.Y;

			// absorb points to compute the total bounds.
			drawable.m_Bounds.AbsorbPoint(fX, fY);
		}

		// - second pass, determine if m_bMatchesBounds is true or false (m_bMatchesBounds
		//   is true if the shape matches its bounding rectangle, which is used to
		//   optimize masks and some input tests)
		drawable.m_bMatchesBounds = true;
		for (Vertices::SizeType i = 0; i < zSize; ++i)
		{
			const Vector2D& v = vVertices[i];
			Float32 const fX = v.X;
			if (!Seoul::Equals(drawable.m_Bounds.m_fLeft, fX, kfAboutEqualPosition) &&
				!Seoul::Equals(drawable.m_Bounds.m_fRight, fX, kfAboutEqualPosition))
			{
				drawable.m_bMatchesBounds = false;
				break;
			}

			Float32 const fY = v.Y;
			if (!Seoul::Equals(drawable.m_Bounds.m_fTop, fY, kfAboutEqualPosition) &&
				!Seoul::Equals(drawable.m_Bounds.m_fBottom, fY, kfAboutEqualPosition))
			{
				drawable.m_bMatchesBounds = false;
				break;
			}
		}

		// - third pass, insert the points as vertices.
		for (Vertices::SizeType i = 0; i < zSize; ++i)
		{
			const Vector2D& v = vVertices[i];

			ShapeVertex& rVertex = drawable.m_vVertices[i];
			rVertex = ShapeVertex::Create(v.X, v.Y);
		}

		// Initially, all shapes that match their bounds can occlude.
		// Sold fill shapes that apply per-vertex color with an alpha < 255
		// cannot.
		drawable.m_bCanOcclude = drawable.m_bMatchesBounds;
		if (IsBitmap(fillStyle.m_eFillStyleType))
		{
			Float const fTXFactor = (drawable.m_pBitmapDefinition.IsValid() ? (1.0f / drawable.m_pBitmapDefinition->GetWidth()) : 1.0f);
			Float const fTYFactor = (drawable.m_pBitmapDefinition.IsValid() ? (1.0f / drawable.m_pBitmapDefinition->GetHeight()) : 1.0f);

			// From gameswf, the transform is actually an inverse of what we need here.
			Matrix2x3 const m = fillStyle.m_mBitmapTransform.Inverse();
			for (Vertices::SizeType i = 0; i < zSize; ++i)
			{
				ShapeVertex& rVertex = drawable.m_vVertices[i];
				rVertex.m_vT = Vector4D(Matrix2x3::TransformPosition(m, rVertex.m_vP), Vector2D::Zero());
				rVertex.m_vT.X *= fTXFactor;
				rVertex.m_vT.Y *= fTYFactor;
			}

			// TODO: This adjustment breaks mask rendering with this
			// shape, since the mask would normally include the entire area of the shape,
			// even fully transparent pixels in the bitmap. If that becomes
			// a problem in practice, we could store "visible vertices" in
			// addition to the regular vertices, and draw those during
			// normal rendering only (this would also require updating
			// the rendering API so that shape draw methods know
			// they're part of a mask draw vs. a normal color draw).

			// If the visible part of the bitmap is smaller than the entire bitmap,
			// adjust the vertices and texture coordinates to only sample from
			// the visible part. This is a fill rate optimization.
			if (!drawable.m_pBitmapDefinition->IsVisibleToEdges())
			{
				// Compute and cache the min/max texture coordinates of the visible
				// area of the bitmap.
				Rectangle const visibleRectangle = drawable.m_pBitmapDefinition->GetVisibleRectangle();
				Vector2D const vMinT(Vector2D(
					visibleRectangle.m_fLeft * fTXFactor,
					visibleRectangle.m_fTop * fTYFactor));
				Vector2D const vMaxT(Vector2D(
					visibleRectangle.m_fRight * fTXFactor,
					visibleRectangle.m_fBottom * fTYFactor));

				// Now adjust all the shape vertices so they only sample from
				// the part of the bitmap that is not fully transparent.
				for (Vertices::SizeType i = 0; i < zSize; ++i)
				{
					ShapeVertex& rVertex = drawable.m_vVertices[i];
					Vector2D const vClampedT = Vector2D::Clamp(
						rVertex.m_vT.GetXY(),
						vMinT,
						vMaxT);

					// Adjustment of texture coordinates.
					Vector2D const vDeltaT = vClampedT - rVertex.m_vT.GetXY();

					// Shape adjustment is the texture coordinate adjustment
					// transformed into "shape space" by the bitmap transform.
					Vector2D const vDeltaP = Matrix2x3::TransformDirection(
						fillStyle.m_mBitmapTransform,
						Vector2D(vDeltaT.X / fTXFactor, vDeltaT.Y / fTYFactor));

					// Adjust vertex attributes.
					rVertex.m_vP += vDeltaP;
					rVertex.m_vT = Vector4D(rVertex.m_vT.GetXY() + vDeltaT, rVertex.m_vT.GetZW());
				}
			}
		}
		else if (IsGradientFill(fillStyle.m_eFillStyleType))
		{
			// From gameswf, the transform is actually an inverse of what we need here.
			Matrix2x3 const m = fillStyle.m_mGradientTransform.Inverse();
			for (Vertices::SizeType i = 0; i < zSize; ++i)
			{
				ShapeVertex& rVertex = drawable.m_vVertices[i];
				rVertex.m_vT = Vector4D(Matrix2x3::TransformPosition(m, rVertex.m_vP), Vector2D::Zero());
			}

			// TODO: Would prefer to handle this by adjusting the geometry
			// (this requires generating "fins" to account for the solid fill
			// outside the gradient) but no time for that right now.
			//
			// Gradient fills require sampling outside the [0, 1] range (gradients
			// can be applied such that geometry is clamped to the outside of the
			// defined gradient). We check for this and when true, mark the
			// gradient fill bitmap as not packable (it will always break render
			// batches and cannot be packed in a texture atlas).
			Bool bCanPack = true;
			if (FillStyleType::kLinearGradientFill == fillStyle.m_eFillStyleType)
			{
				for (Vertices::SizeType i = 0; i < zSize; ++i)
				{
					ShapeVertex& rVertex = drawable.m_vVertices[i];

					// gameswf applies these correction factors, it appears
					// that "gradient space" post-matrix multiplication is on
					// [-16384, 16384], which we remap to [0, 1] (along U)
					// for linear gradient sampling.
					rVertex.m_vT.X = (rVertex.m_vT.X / 32768.0f) + 0.5f;

					// Always V component of 0.0f for a linear gradient,
					// since the gradient texture. is a single pixel along V.
					rVertex.m_vT.Y = 0.0f;
					
					// Pack tracking - outside range means we can't pack the definition.
					if (rVertex.m_vT.X < 0.0f || rVertex.m_vT.X > 1.0f) { bCanPack = false; }
				}
			}
			else
			{
				for (Vertices::SizeType i = 0; i < zSize; ++i)
				{
					ShapeVertex& rVertex = drawable.m_vVertices[i];

					// gameswf applies these correction factors, it appears
					// that "gradient space" post-matrix multiplication is on
					// [-16384, 16384], which we remap to [0, 1] (along U and V)
					// for radial gradient sampling.
					rVertex.m_vT.X = (rVertex.m_vT.X / 32768.0f) + 0.5f;
					rVertex.m_vT.Y = (rVertex.m_vT.Y / 32768.0f) + 0.5f;
					
					// Pack tracking - outside range means we can't pack the definition.
					if (rVertex.m_vT.X < 0.0f || rVertex.m_vT.X > 1.0f) { bCanPack = false; }
					if (rVertex.m_vT.Y < 0.0f || rVertex.m_vT.Y > 1.0f) { bCanPack = false; }
				}
			}
			
			drawable.m_pBitmapDefinition.Reset(SEOUL_NEW(MemoryBudgets::Falcon) BitmapDefinition(
				fillStyle.m_eFillStyleType,
				fillStyle.m_Gradient,
				bCanPack));

			// Can occlude if the gradient is a full occluder.
			drawable.m_bCanOcclude = drawable.m_bCanOcclude && (drawable.m_pBitmapDefinition->IsFullOccluder());
		}
		else
		{
			// Can occlude if the fill color's alpha is
			// at or above the 8-bit occlusion threshold.
			drawable.m_bCanOcclude = drawable.m_bCanOcclude && (fillStyle.m_Color.m_A >= ku8bitColorOcclusionThreshold);

			if (fillStyle.m_Color != RGBA::White())
			{
				drawable.m_eFeature = Render::Feature::kColorMultiply;
			}

			for (Vertices::SizeType i = 0; i < zSize; ++i)
			{
				ShapeVertex& rVertex = drawable.m_vVertices[i];
				rVertex.m_ColorMultiply = fillStyle.m_Color;
			}
		}

		// Fill out the occlusion transform (maps texture/bitmap space
		// to the local positional space of the shape drawable).
		if (drawable.m_bCanOcclude)
		{
			// Occlusion transform is the bitmap transform with the inverse of the 
			// TX factors applied.
			if (IsBitmap(fillStyle.m_eFillStyleType))
			{
				Float const fSX = (drawable.m_pBitmapDefinition.IsValid() ? (Float)drawable.m_pBitmapDefinition->GetWidth() : 1.0f);
				Float const fSY = (drawable.m_pBitmapDefinition.IsValid() ? (Float)drawable.m_pBitmapDefinition->GetHeight() : 1.0f);

				drawable.m_mOcclusionTransform = fillStyle.m_mBitmapTransform * Matrix2x3::CreateScale(fSX, fSY);
			}
			// Occlusion transform is derived from the bounds for everything else (gradients and solid fill).
			else
			{
				drawable.m_mOcclusionTransform = Matrix2x3::CreateFrom(
					Matrix2D::CreateScale(drawable.m_Bounds.GetWidth(), drawable.m_Bounds.GetHeight()),
					Vector2D(drawable.m_Bounds.m_fLeft, drawable.m_Bounds.m_fTop));
			}
		}
		else
		{
			drawable.m_mOcclusionTransform = Matrix2x3::Identity();
		}

		// Clamp texture coordinates if they were generated, since our optimized dynamic
		// texture atlas generation (see FalconTexturePacker.h) requires all texture
		// coordinates to fall on [0, 1].
		//
		// See above comment when constructing gradient fills - they require texture
		// coordinates outside the [0, 1] range so are not clamped here (we currently
		// account for this by not allowing gradient fills to be packed into an atlas).
		if (IsBitmap(fillStyle.m_eFillStyleType))
		{
			for (Vertices::SizeType i = 0; i < zSize; ++i)
			{
				ShapeVertex& rVertex = drawable.m_vVertices[i];
				rVertex.m_vT = Vector4D(Vector2D::Clamp(rVertex.m_vT.GetXY(), Vector2D::Zero(), Vector2D::One()), Vector2D::Zero());
			}
		}

		// Attempt to make the drawable a normalized convex shape - if successful,
		// set kConvex as the description of the shape.
		if (MakeNormalizedConvex(drawable, bConvex))
		{
			drawable.m_eTriangleListDescription = TriangleListDescription::kConvex;
		}
		// Otherwise, we must treat the shape as an unknown bucket of triangles.
		else
		{
			drawable.m_eTriangleListDescription = TriangleListDescription::kNotSpecific;
		}

		// Done, insert the drawable.
		m_rvFillDrawables.PushBack(drawable);
	}

	virtual void EndShape() SEOUL_OVERRIDE
	{
	}

private:
	FCNFile& m_rFile;
	ShapeDefinition::FillDrawables& m_rvFillDrawables;

	// Utility - detects if rDrawable is a convex shape and if so,
	// normalizes it so that the indices and vertices are formed
	// as a triangle fan.
	Bool MakeNormalizedConvex(ShapeFillDrawable& rDrawable, Bool bConvex)
	{
		// Resort vertices and regenerate indices.
		if (!bConvex)
		{
			return false;
		}

		if (rDrawable.m_vVertices.IsEmpty())
		{
			return false;
		}

		// Find the center of mass.
		UInt32 const uVertices = rDrawable.m_vVertices.GetSize();
		Vector2D vCenterOfMass = Vector2D::Zero();
		for (UInt32 i = 0; i < uVertices; ++i)
		{
			vCenterOfMass += rDrawable.m_vVertices[i].m_vP;
		}
		vCenterOfMass /= (Float)uVertices;

		// Now sort the vertices based on angle from the center.
		ConvexSorter sorter(vCenterOfMass);
		QuickSort(rDrawable.m_vVertices.Begin(), rDrawable.m_vVertices.End(), sorter);

		// Now generate triangle fan style indices.
		rDrawable.m_vIndices.Resize(3 * (uVertices - 2));
		Indices::SizeType uIndex = 0;
		for (UInt32 i = 2; i < uVertices; ++i)
		{
			rDrawable.m_vIndices[uIndex++] = (Indices::ValueType)0;
			rDrawable.m_vIndices[uIndex++] = (Indices::ValueType)(i - 1);
			rDrawable.m_vIndices[uIndex++] = (Indices::ValueType)(i - 0);
		}

		return true;
	}

	SEOUL_DISABLE_COPY(ShapeTesselateUtility);
}; // struct ShapeTesselateUtility

Bool ShapeDefinition::Tesselate(FCNFile& rFile)
{
	ShapeTesselateUtility utility(rFile, m_vFillDrawables);
	Tesselator tesselator(utility);

	tesselator.BeginShape();
	Paths::SizeType const zSize = m_vPaths.GetSize();
	for (Paths::SizeType i = 0; i < zSize; ++i)
	{
		m_vPaths[i].Tesselate(*this, tesselator);
	}
	tesselator.EndShape();

	return true;
}

} // namespace Seoul::Falcon
