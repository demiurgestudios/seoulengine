/**
 * \file NullGraphicsVertexFormat.h
 * \brief Nop implementation of a VertexFormat for
 * contexts without graphics hardware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef NULL_GRAPHICS_VERTEX_FORMAT_H
#define NULL_GRAPHICS_VERTEX_FORMAT_H

#include "VertexFormat.h"

namespace Seoul
{

class NullGraphicsVertexFormat SEOUL_SEALED : public VertexFormat
{
public:
	virtual Bool OnCreate() SEOUL_OVERRIDE;

private:
	NullGraphicsVertexFormat(const VertexElements& vVertexElements);
	~NullGraphicsVertexFormat();

	SEOUL_REFERENCE_COUNTED_SUBCLASS(NullGraphicsVertexFormat);

	friend class NullGraphicsDevice;
	friend class NullGraphicsRenderCommandStreamBuilder;

	SEOUL_DISABLE_COPY(NullGraphicsVertexFormat);
}; // class NullGraphicsVertexFormat

} // namespace Seoul

#endif // include guard
