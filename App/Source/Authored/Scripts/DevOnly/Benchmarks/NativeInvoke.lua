local native = SeoulNativeNewNativeUserData('ScriptBenchmarkNativeTester')

function NativeAdd2N(n)
	local a = 0
	local b = 1
	for _=1,n do
		a = native:Add2N(a, b)
	end
	return a
end

function NativeAdd3N(n)
	local a = 0
	local b = 1
	local c = 2
	for _=1,n do
		a = native:Add3N(a, b, c)
	end
	return a
end

function NativeAdd4N(n)
	local a = 0
	local b = 1
	local c = 2
	local d = 3
	for _=1,n do
		a = native:Add4N(a, b, c, d)
	end
	return a
end

function NativeAdd5N(n)
	local a = 0
	local b = 1
	local c = 2
	local d = 3
	local e = 4
	for _=1,n do
		a = native:Add5N(a, b, c, d, e)
	end
	return a
end
