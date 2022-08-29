using static SlimCS;
public class Test
{
	public static int Main ()
	{
		for (var i = 0; i < 1; ++i)
		{
			print(i);
			if (i.GetType() != typeof (int))
			{
				return 1;
			}
		}

		return 0;
	}
}
