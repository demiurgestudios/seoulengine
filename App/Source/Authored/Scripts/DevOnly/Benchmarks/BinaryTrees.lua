-- The Computer Language Benchmarks Game
-- http://benchmarksgame.alioth.debian.org/
-- contributed by Mike Pall

local function BottomUpTree(item, depth)
  if depth > 0 then
    local i = item + item
    depth = depth - 1
    local left, right = BottomUpTree(i-1, depth), BottomUpTree(i, depth)
    return { item, left, right }
  else
    return { item }
  end
end

local function ItemCheck(tree)
  if tree[2] then
    return tree[1] + ItemCheck(tree[2]) - ItemCheck(tree[3])
  else
    return tree[1]
  end
end

local N = 8
local mindepth = 4
local maxdepth = mindepth + 2
if maxdepth < N then maxdepth = N end

function BinaryTrees(n)
	for _=1,n do
		for depth=mindepth,maxdepth,2 do
			local iterations = 2 ^ (maxdepth - depth + mindepth)
			local check = 0
			for _=1,iterations do
				check = check + ItemCheck(BottomUpTree(1, depth)) + ItemCheck(BottomUpTree(-1, depth))
			end
		end
	end
end