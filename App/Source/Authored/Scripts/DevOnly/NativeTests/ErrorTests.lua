-- Tests in this file explicitly
-- trigger errors to verify.

local t
t = {
	function()
		local a = nil
		local b = {}
		b[1] = 2
		a[2] = 1
		print(b[1])
	end,

	function()
		local a = nil
		local b = 1
		local c = "1"
		local d = b + c
		print(d)
		local e = a + b
		print(e)
	end,

	function()
		local a = nil
		local b = {}
		b[a] = 1
		print(b[a])
		b[1] = 2
		print(b[1])
	end,

	function()
		local a = nil
		local b = function() return 1 end
		print(b())
		print(a())
	end,

	function()
		local a = function() return 1 end
		local b = 1
		print(b())
		print(a())
	end,

	function()
		local a = function() return 1 end
		local b = "a"
		print(b())
		print(a())
	end,

	function()
		local a = function() return 1 end
		local b = true
		print(b())
		print(a())
	end,

	function()
		local a = nil
		local b = 2
		local c = 3
		print(b < c)
		print(a < b)
	end,

	function()
		local a = nil
		local b = 2
		local c = 3
		print(b > c)
		print(a > b)
	end,

	function()
		local a = function() return 2 end
		local b = t[1]

		a()
		b()
	end,

	function()
		local function getn() return nil end
		for i=1,getn() do
			print(i)
		end
	end,

	function()
		local function geti() return nil end
		for i=geti(),10 do
			print(i)
		end
	end,

	function()
		local function getstep() return nil end
		for i=1,10,getstep() do
			print(i)
		end
	end,

	function()
		local a = 1
		local b = 2
		local c = nil
		a = a + b
		b = b / a
		a = b * a
		b = a * b
		b = c * c
	end,

	function()
		local a = 1
		local b = nil
		local c = "a"
		print(a .. c)
		print(a .. b)
	end,

	function()
		local a = function() error('Error') end

		local t = {
			a = 1,
			b = 2,
			c = 3,
			d = a(),
			e = 5,
			f = 6,
		}
		print(t)
	end,
}

for _,v in ipairs(t) do
	local b, msg = pcall(v)
	assert(not b)
	print(msg)
end
