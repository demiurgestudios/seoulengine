local function fibr(n)
	if (n < 2) then return n end
	return (fibr(n-2) + fibr(n-1))
end

function FibR(n)
	local b = 0
	for _=1,n do
		b = fibr(4)
	end
	return b
end

local function fibi(n)
	local last = 0
	local cur = 1
	n = n - 1
	while (n > 0)
	do
		n = n - 1
		local tmp = cur
		cur = last + cur
		last = tmp
	end
	return cur
end

function FibI(n)
	local b = 0
	for _=1,n do
		b = fibi(28)
	end
	return b
end

local function isprime(n)
	for i = 2, (n - 1) do
		if (n % i == 0) then
			return false
		end
	end
	return true
end

local function primes(n)
	local count = 0

	for i = 2,n do
		if (isprime(i)) then
			count = count + 1
		end
	end
	return count
end

function Primes(n)
	local b = 0
	for _=1,n do
		b = primes(1000)
	end
	return b
end
