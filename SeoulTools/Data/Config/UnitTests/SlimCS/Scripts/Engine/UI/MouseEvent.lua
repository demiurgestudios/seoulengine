-- Table of mouse event types, roughly corresponds
-- to ActionScript equivalents.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.

-- Events related to mouse input.
local MouseEvent = class_static 'MouseEvent'

-- Fired once on mouse/touch press.
MouseEvent.MOUSE_DOWN = 'mouseDown'

-- Fired on mouse/touch position changes.
MouseEvent.MOUSE_MOVE = 'mouseMove'

-- Fired once when a mouse cursor leaves the shape
-- (after having previously entered the shape).
--
-- Identical semantics to MOUSE_DOWN and MOUSE_UP
-- except driven by the mouse cursor's intersection
-- with the MovieClip's hit shape.
--
-- Only valid on platforms with a visible mouse cursor.
MouseEvent.MOUSE_OUT = 'mouseOut'

-- Fired once when a mouse cursor enters the shape
-- of the movie clip. Only valid on platforms with
-- a visible mouse cursor.
MouseEvent.MOUSE_OVER = 'mouseOver'

-- Fired once on mouse/touch released.
MouseEvent.MOUSE_UP = 'mouseUp'

-- Mouse down/up events delivered to all movies, until a movie completely
-- blocks input. This even is fired post any captured mouse input (the targets
-- of MOUSE_UP, MOUSE_DOWN, etc.). It is useful for post handling (e.g. dismiss
-- of a drop down list).
MouseEvent.MOVIE_MOUSE_DOWN = 'movieMouseDown'
MouseEvent.MOVIE_MOUSE_UP = 'movieMouseUp'

-- Fire whenever the mouse wheel changes position.
MouseEvent.MOUSE_WHEEL = 'mouseWheel'
return MouseEvent
