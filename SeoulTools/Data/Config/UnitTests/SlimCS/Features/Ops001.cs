using static SlimCS;
public static class Test
{
	public static void Main()
	{
		// Mixing math with all C# math operators on integers.
		var i = 100000; print(i);
		i++; print(i);
		++i; print(i);
		i--; print(i);
		--i; print(i);
		i = -i; print(i);
		i = +i; print(i);
		i = ~i; print(i);

		i *= i; print(i);
		i = i * i; print(i);

		i /= 5; print(i);
		i = i / 1; print(i);

		i %= 2; print(i);
		i = i % 3; print(i);

		i += i; print(i);
		i = i + i; print(i);

		i -= 5; print(i);
		i = i - 5; print(i);

		i <<= 5; print(i);
		i = i << 5; print(i);

		i >>= 5; print(i);
		i = i >> 5; print(i);

		i &= (7 << 1); print(i);
		i = i & (7 << 1); print(i);

		i ^= 15; print(i);
		i = i ^ 15; print(i);

		i |= i; print(i);
		i = i | i; print(i);
	}
}
