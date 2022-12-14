
public class TestIntOps {

	public static double double_add (double a, double b) {
		return a+b;
	}

	public static int int_add (int a, int b) {
		return a+b;
	}

	public static int int_sub (int a, int b) {
		return a-b;
	}

	public static int int_mul (int a, int b) {
		return a*b;
	}

	public static int Main() {
		int num = 1;

		if (int_add (1, 1)   != 2)  return num;
		num++;

		if (int_add (31, -1) != 30) return num;
		num++;

		if (int_sub (31, -1) != 32) return num;
		num++;

		if (int_mul (12, 12) != 144) return num;
		num++;


		if (double_add (1.5, 1.5) != 3)  return num;
		num++;

		// add more meaningful tests

    	return 0;
	}
}
