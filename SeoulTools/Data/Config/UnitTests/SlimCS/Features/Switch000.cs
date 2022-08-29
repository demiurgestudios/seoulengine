using static SlimCS;
public static class Test
{
	public static void Main()
	{
		for (int i = 0; i < 10; ++i)
		{
			switch (i)
			{
				case 0: print("one"); break;
				case 1: print("two"); break;
				case 2: // fall-through
				case 3: print("four"); break;
				case 4: print("five"); break;
				case 5: print("six"); goto case 7;
				case 6: print("seven"); break;
				case 7: print("eight"); goto default;
				case 8: print("nine"); break;
				default: print("ten"); break;
			}
		}
	}
}
