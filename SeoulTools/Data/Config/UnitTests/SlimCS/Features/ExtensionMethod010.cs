using static SlimCS;
using System;

public class My
{
	bool ContentTransferEncoding {
		set { }
	}
}

public static class Test
{
	public static int Main()
	{
		var test = new My ();

		int result = test.ContentTransferEncoding ();
		print(result);
		if (result != 1)
			return 1;

		result = test.ContentTransferEncoding<int> ();
		print(result);
		if (result != 2)
			return 2;

		return 0;
	}

	public static int ContentTransferEncoding<T> (this My email)
	{
		print(2);
		return 2;
	}

	public static int ContentTransferEncoding (this My email)
	{
		print(1);
		return 1;
	}
}
