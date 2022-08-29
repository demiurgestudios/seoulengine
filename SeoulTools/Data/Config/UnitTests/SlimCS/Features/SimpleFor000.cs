using static SlimCS;
public static class Test
{
	public static void Main()
	{
		// Variables that should be recognized as simple for loops,
		// convertible to:
		// for i=1,j[,k]

		// >= and <=
		{
			// ++ and --
			for (int i = 0; i <= 9; ++i) { print(i); }
			for (int i = 7; i <= 12; i++) { print(i); }
			for (int i = 39; i >= 12; --i) { print(i); }
			for (int i = 17; i >= 3; i--) { print(i); }

			// += and -= with 1 step.
			for (int i = 0; i <= 10; i += 1) { print(i); }
			for (int i = 7; i >= 1; i += -1) { print(i); }
			for (int i = 11; i >= 3; i -= 1) { print(i); }
			for (int i = 5; i <= 13; i -= -1) { print(i); }

			// += and -= with 2 step.
			for (int i = 0; i <= 10; i += 2) { print(i); }
			for (int i = 7; i >= 1; i += -2) { print(i); }
			for (int i = 11; i >= 3; i -= 2) { print(i); }
			for (int i = 5; i <= 13; i -= -2) { print(i); }

			// i = i + 1 and i = i - 1
			for (int i = 0; i <= 10; i = i + 1) { print(i); }
			for (int i = 7; i >= 1; i = i + -1) { print(i); }
			for (int i = 11; i >= 3; i = i - 1) { print(i); }
			for (int i = 5; i <= 13; i = i - -1) { print(i); }

			// += and -= with 2 step.
			for (int i = 0; i <= 10; i = i + 2) { print(i); }
			for (int i = 7; i >= 1; i = i + -2) { print(i); }
			for (int i = 11; i >= 3; i = i - 2) { print(i); }
			for (int i = 5; i <= 13; i = i - -2) { print(i); }
		}

		// > and <
		{
			// ++ and --
			for (int i = 0; i < 9; ++i) { print(i); }
			for (int i = 7; i < 12; i++) { print(i); }
			for (int i = 39; i > 12; --i) { print(i); }
			for (int i = 17; i > 3; i--) { print(i); }

			// += and -= with 1 step.
			for (int i = 0; i < 10; i += 1) { print(i); }
			for (int i = 7; i > 1; i += -1) { print(i); }
			for (int i = 11; i > 3; i -= 1) { print(i); }
			for (int i = 5; i < 13; i -= -1) { print(i); }

			// += and -= with 2 step.
			for (int i = 0; i < 10; i += 2) { print(i); }
			for (int i = 7; i > 1; i += -2) { print(i); }
			for (int i = 11; i > 3; i -= 2) { print(i); }
			for (int i = 5; i < 13; i -= -2) { print(i); }

			// i = i + 1 and i = i - 1
			for (int i = 0; i < 10; i = i + 1) { print(i); }
			for (int i = 7; i > 1; i = i + -1) { print(i); }
			for (int i = 11; i > 3; i = i - 1) { print(i); }
			for (int i = 5; i < 13; i = i - -1) { print(i); }

			// += and -= with 2 step.
			for (int i = 0; i < 10; i = i + 2) { print(i); }
			for (int i = 7; i > 1; i = i + -2) { print(i); }
			for (int i = 11; i > 3; i = i - 2) { print(i); }
			for (int i = 5; i < 13; i = i - -2) { print(i); }
		}
	}
}
