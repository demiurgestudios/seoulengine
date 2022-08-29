using static SlimCS;
using System;

class Test
{
	public static void Main ()
	{
		print(M (1));
		try {
			print(M (null));
		} catch (Exception) {
			print("thrown");
		}
	}

	static string M (object data)
	{
		return data?.ToString () ?? throw null;
	}
}
