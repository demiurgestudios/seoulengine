using static SlimCS;
public static class Test
{
	public static void Main()
	{
		int i = 0;
		int j = 0;
		int t = 0;
		int k = 0;

		for (i = 0; i < 10; i++) {
			print(i, j, t, k);
			if (i == 5)
				break;
		}

		print(i, j, t, k);
		if (i != 5)
			return;
		print(i, j, t, k);

		t = 0;
		k = 0;
		for (i = 0; i < 10; i++){
			print(i, j, t, k);
			for (j = 0; j < 10; j++){
				print(i, j, t, k);
				if (j > 3)
					break;
				t++;

				print(i, j, t, k);

				if (j >= 1)
					continue;

				print(i, j, t, k);

				k++;
			}
		}

		print(i, j, t, k);
		if (t != 40)
			return;
		print(i, j, t, k);
		if (k != 10)
			return;
		print(i, j, t, k);

		t = 0;
		do {
			print(i, j, t, k);
			t++;
			print(i, j, t, k);
			if (t == 5)
				break;
			print(i, j, t, k);
			k++;
			print(i, j, t, k);
		} while (k < 11);

		print(i, j, t, k);
		if (t != 5)
			return;

		print(i, j, t, k);
	}
}
