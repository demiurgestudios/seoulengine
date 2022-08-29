-- Utilities for working with Google Protocol Buffers
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



--- <summary>
--- A protobuf type schema, loaded by the lua pb library
--- </summary>
local PbMessageType = class 'PbMessageType'



function PbMessageType:Constructor(name)

	self.m_PbType = Protobuf.PbType(name)
end

function PbMessageType:Decode(message)

	return Protobuf.PbDecode(message, self.m_PbType)
end

function PbMessageType:Encode(data)

	return Protobuf.PbEncode(data, self.m_PbType)
end


local Protobuf = class_static 'Protobuf'













local s_ExtPbLibrary = nil

function Protobuf.cctor()

	s_ExtPbLibrary = require 'External/lua_protobuf/pb'
end

local function PbLoad(dataHandle)

	return s_ExtPbLibrary.load(dataHandle)
end

function Protobuf.PbType(name)

	return s_ExtPbLibrary.type(name)
end

function Protobuf.PbEncode(data, loadedType)

	return s_ExtPbLibrary.encode(data, loadedType)
end

function Protobuf.PbDecode(message, loadedType)

	return s_ExtPbLibrary.decode(message, loadedType)
end

function Protobuf.LoadSchema(path)

	local dataHandle = _G.SeoulLuaReadPb(path)
	return PbLoad(dataHandle)
end
return Protobuf
