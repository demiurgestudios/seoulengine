using static SlimCS;
public static class Test
{
	// Make sure all these functions generate valid Lua code,
	// they use params the use reserved Lua keywords.
	static void Do1(double and) { print(and); }
	// Also C# reserved: static void Do2(double break) { print(break); }
	// Also C# reserved: static void Do3(double do) { print(do); }
	// Also C# reserved: static void Do4(double else) { print(else); }
	static void Do5(double elseif) { print(elseif); }
	static void Do6(double end) { print(end); }
	// Also C# reserved: static void Do7(double false) { print(false); }
	// Also C# reserved: static void Do8(double for) { print(for); }
	static void Do9(double function) { print(function); }
	// Also C# reserved: static void Do10(double if) { print(if); }
	// Also C# reserved: static void Do11(double in) { print(in); }
	static void Do12(double local) { print(local); }
	static void Do13(double nil) { print(nil); }
	static void Do14(double not) { print(not); }
	static void Do15(double or) { print(or); }
	static void Do16(double repeat) { print(repeat); }
	// Also C# reserved: static void Do17(double return) { print(return); }
	static void Do18(double then) { print(then); }
	static void Do19(double until) { print(until); }
	// Also C# reserved: static void Do20(double while) { print(while); }

	public static int Main ()
	{
		Do1(1.0);
		Do5(5.0);
		Do6(6.0);
		Do9(9.0);
		Do12(12.0);
		Do13(13.0);
		Do14(14.0);
		Do15(15.0);
		Do16(16.0);
		Do18(18.0);
		Do19(19.0);
		return 0;
	}
}
