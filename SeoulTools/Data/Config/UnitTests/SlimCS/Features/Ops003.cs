using static SlimCS;
public static class Test
{
	static bool True()
	{
		print("TRUE");
		return true;
	}

	static bool False()
	{
		print("FALSE");
		return false;
	}

	public static void Functions()
	{
		print(False() | False());
		print(False() | True());
		print(True() | False());
		print(True() | True());

		print(False() & False());
		print(False() & True());
		print(True() & False());
		print(True() & True());

		print(False() ^ False());
		print(False() ^ True());
		print(True() ^ False());
		print(True() ^ True());
	}

	public static void Literals()
	{
		print(false | false);
		print(false | true);
		print(true | false);
		print(true | true);

		print(false & false);
		print(false & true);
		print(true & false);
		print(true & true);

		print(false ^ false);
		print(false ^ true);
		print(true ^ false);
		print(true ^ true);
	}

	public static void Variables()
	{
		bool a;
		bool b;
		a = false; b = false; print(a | b);
		a = false; b = true; print(a | b);
		a = true; b = false; print(a | b);
		a = true; b = true; print(a | b);

		a = false; b = false; print(a & b);
		a = false; b = true; print(a & b);
		a = true; b = false; print(a & b);
		a = true; b = true; print(a & b);

		a = false; b = false; print(a ^ b);
		a = false; b = true; print(a ^ b);
		a = true; b = false; print(a ^ b);
		a = true; b = true; print(a ^ b);

		a = false; b = false; a |= b; print(a);
		a = false; b = true; a |= b; print(a);
		a = true; b = false; a |= b; print(a);
		a = true; b = true; a |= b; print(a);

		a = false; b = false; a &= b; print(a);
		a = false; b = true; a &= b; print(a);
		a = true; b = false; a &= b; print(a);
		a = true; b = true; a &= b; print(a);

		a = false; b = false; a ^= b; print(a);
		a = false; b = true; a ^= b; print(a);
		a = true; b = false; a ^= b; print(a);
		a = true; b = true; a ^= b; print(a);
	}

	public static void Main()
	{
		// Bit operations on booleans.
		Functions();
		Literals();
		Variables();
	}
}
