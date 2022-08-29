local __i32mod__ = math.i32mod
local __i32mul__ = math.i32mul
local __i32narrow__ = _G.bit.tobit
local __i32truncate__ = math.i32truncate

function AddNV(n)
	local count = 0
	for _=1,n do
		count = 2 + count
	end
	return count
end

function AddVN(n)
	local count = 0
	for _=1,n do
		count = count + 2
	end
	return count
end

function AddVV(n)
	local a = 0
	local b = 1
	for _=1,n do
		a = a + b
	end
	return a
end

function DivNV(n)
	local count = 0
	for _=1,n do
		count = 2 / count
	end
	return count
end

function DivVN(n)
	local count = 0
	for _=1,n do
		count = count / 2
	end
	return count
end

function DivVV(n)
	local a = 0
	local b = 1
	for _=1,n do
		a = a / b
	end
	return a
end

function ModNV(n)
	local count = 0
	for _=1,n do
		count = 2 % count
	end
	return count
end

function ModVN(n)
	local count = 0
	for _=1,n do
		count = count % 2
	end
	return count
end

function ModVV(n)
	local a = 0
	local b = 1
	for _=1,n do
		a = a % b
	end
	return a
end

function MulNV(n)
	local count = 0
	for _=1,n do
		count = 2 * count
	end
	return count
end

function MulVN(n)
	local count = 0
	for _=1,n do
		count = count * 2
	end
	return count
end

function MulVV(n)
	local a = 0
	local b = 1
	for _=1,n do
		a = a * b
	end
	return a
end

function SubNV(n)
	local count = 0
	for _=1,n do
		count = 2 - count
	end
	return count
end

function SubVN(n)
	local count = 0
	for _=1,n do
		count = count - 2
	end
	return count
end

function SubVV(n)
	local a = 0
	local b = 1
	for _=1,n do
		a = a - b
	end
	return a
end

function I32AddNV(n)
	local count = 0
	for _=1,n do
		count = __i32narrow__(2 + count)
	end
	return count
end

function I32AddVN(n)
	local count = 0
	for _=1,n do
		count = __i32narrow__(count + 2)
	end
	return count
end

function I32AddVV(n)
	local a = 0
	local b = 1
	for _=1,n do
		a = __i32narrow__(a + b)
	end
	return a
end

function I32DivNV(n)
	local count = 0
	for _=1,n do
		count = __i32truncate__(2 / count)
	end
	return count
end

function I32DivVN(n)
	local count = 0
	for _=1,n do
		count = __i32truncate__(count / 2)
	end
	return count
end

function I32DivVV(n)
	local a = 0
	local b = 1
	for _=1,n do
		a = __i32truncate__(a / b)
	end
	return a
end

function I32ModNV(n)
	local count = 0
	for _=1,n do
		count = __i32mod__(2, count)
	end
	return count
end

function I32ModVN(n)
	local count = 0
	for _=1,n do
		count = __i32mod__(count, 2)
	end
	return count
end

function I32ModVV(n)
	local a = 0
	local b = 1
	for _=1,n do
		a = __i32mod__(a, b)
	end
	return a
end

function I32MulNV(n)
	local count = 0
	for _=1,n do
		count = __i32mul__(2, count)
	end
	return count
end

function I32MulVN(n)
	local count = 0
	for _=1,n do
		count = __i32mul__(count, 2)
	end
	return count
end

function I32MulVV(n)
	local a = 0
	local b = 1
	for _=1,n do
		a = __i32mul__(a, b)
	end
	return a
end

function I32SubNV(n)
	local count = 0
	for _=1,n do
		count = __i32narrow__(2 - count)
	end
	return count
end

function I32SubVN(n)
	local count = 0
	for _=1,n do
		count = __i32narrow__(count - 2)
	end
	return count
end

function I32SubVV(n)
	local a = 0
	local b = 1
	for _=1,n do
		a = __i32narrow__(a - b)
	end
	return a
end

function I32Truncate(n)
	local a = 3.5
	local b = 0
	for _=1,n do
		b = __i32truncate__(a)
	end
	return b
end
