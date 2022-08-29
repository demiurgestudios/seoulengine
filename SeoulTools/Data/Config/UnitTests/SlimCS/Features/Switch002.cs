using static SlimCS;
public static class Test
{
	public static int Main()
	{
		int v = 10;
		int a;

		switch (v) {
		case 0:
			int i = v + 1;
			print(i);
			a = i;
			print(a, i);
			break;
		default:
			i = 5;
			a = i;
			print(a, i);
			break;
		}
		print(a);
		if (a != 5)
			return 1;
		print(a);


		v = 20;
		int r = 0;

		switch (v){
		case 20:
			print(v, r);
			r++;
			print(v, r);
			int j = 10;
			print(v, r, j);
			r += j;
			print(v, r, j);
			break;
		}
		print(v, r);
		if (r != 11)
			return 5;
		print(v, r);

		return 0;
	}
}
