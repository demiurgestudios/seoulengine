using static SlimCS;
public class Test
{
	public static int Main ()
	{
		string [] strings = new string [] { "Foo", "Bar", "Baz" };
		foreach (var item in strings)
		{
			print(item);
			if (item.GetType() != typeof (string))
				return 1;
		}

		int [] ints = new int [] { 2, 4, 8, 16, 42 };
		foreach (var item in ints)
		{
			print(item);
			if (item.GetType() != typeof (int))
				return 2;
		}

		return 0;
	}
}
